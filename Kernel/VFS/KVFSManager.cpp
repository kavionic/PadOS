// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 19.02.2018 21:39:19

 #include "System/Platform.h"

#include <algorithm>
#include <array>
#include <string.h>
#include <assert.h>
#include <utility>
#include <vector>

#include <PadOS/DeviceControl.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/VFS/KDirectoryCache.h>
#include <Kernel/VFS/KVFSManager.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KInode.h>

namespace kernel
{

KMutex                 KVFSManager::s_InodeMapMutex("inode_map_mutex", PEMutexRecursionMode_RaiseError);
PIntrusiveList<KInode> KVFSManager::s_InodeLRUList;
KConditionVariable     KVFSManager::s_InodeMapConditionVar("inode_map_condition");

std::map<fs_id, Ptr<KFSVolume>> KVFSManager::s_VolumeMap;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KVFSManager::KVFSManager()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KVFSManager::~KVFSManager()
{
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KVFSManager::GetVolume(fs_id volumeID)
{
    KScopedLock inodeMapLock(s_InodeMapMutex);

    auto i = s_VolumeMap.find(volumeID);
    if (i != s_VolumeMap.end()) {
        return i->second;
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVFSManager::GetInode_trw(KFSVolume& volume, ino_t inodeID, bool crossMount)
{
    for (;;)
    {
        Ptr<KFSVolume> volumeReference;
        {
            KScopedLock inodeMapLock(s_InodeMapMutex);

            auto inodeIterator = volume.m_InodeMap.find(inodeID);
            if (inodeIterator != volume.m_InodeMap.end())
            {
                if (inodeIterator->second == PENDING_INODE)
                {
                    s_InodeMapConditionVar.Wait(s_InodeMapMutex);
                    continue;
                }
                Ptr<KInode> inode = TryAcquireInodeReference(inodeIterator->second);
                if (inode == nullptr)
                {
                    s_InodeMapConditionVar.Wait(s_InodeMapMutex);
                    continue;
                }

                if (crossMount && inode->m_MountRoot != nullptr) {
                    inode = inode->m_MountRoot;
                }
                return inode;
            }

            // Verify that the volume is still mounted after waiting for s_InodeMapConditionVar.
            auto volumeIterator = s_VolumeMap.find(volume.m_VolumeID);
            if (volumeIterator == s_VolumeMap.end() ||
                volumeIterator->second != &volume ||
                volume.m_Filesystem == nullptr)
            {
                PERROR_THROW_CODE(PErrorCode(ENODEV));
            }
            volumeReference = volumeIterator->second;

            volume.m_InodeMap[inodeID] = PENDING_INODE;
        }

        Ptr<KInode> inode;
        {
            PScopeExit completeInodeLoad(
                [&volume, inodeID, &inode]()
                {
                    KScopedLock inodeMapLock(s_InodeMapMutex);

                    auto pendingInode = volume.m_InodeMap.find(inodeID);
                    kassert(pendingInode != volume.m_InodeMap.end());
                    kassert(pendingInode->second == PENDING_INODE);

                    if (pendingInode != volume.m_InodeMap.end() && pendingInode->second == PENDING_INODE)
                    {
                        if (inode != nullptr) {
                            pendingInode->second = ptr_raw_pointer_cast(inode);
                        } else {
                            volume.m_InodeMap.erase(pendingInode);
                        }
                    }
                    s_InodeMapConditionVar.WakeupAll();
                });

            inode = volume.m_Filesystem->LoadInode(std::move(volumeReference), inodeID);
        }

        if (crossMount && inode != nullptr && inode->m_MountRoot != nullptr) {
            inode = inode->m_MountRoot;
        }
        return inode;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KVFSManager::InodeReleased(KInode* inode)
{
    KScopedLock inodeMapLock(s_InodeMapMutex);
    if (inode->GetPtrCount() != 1)
    {
        s_InodeMapConditionVar.WakeupAll();
        return false;
    }

    if (inode->IsDeleted())
    {
        inode->m_Volume->DiscardInodeDirtyState(inode);
        if (inode->IsListMember(&s_InodeLRUList)) {
            s_InodeLRUList.Remove(inode);
        }
        if (!inode->m_Volume->IsDeletedInodeQueued(inode))
        {
            kassert(!inode->IsListMember());
            inode->m_Volume->QueueDeletedInode(inode);
        }
        s_InodeMapConditionVar.WakeupAll();
        return false;
    }

    if (inode->IsDirty())
    {
        if (!inode->IsWritebackInProgress() && !inode->IsListMember())
        {
            inode->m_Volume->QueueDirtyInode(inode);
        }
        s_InodeMapConditionVar.WakeupAll();
        return false;
    }

    auto inodeIterator = inode->m_Volume->m_InodeMap.find(inode->m_InodeID);
    if (inodeIterator == inode->m_Volume->m_InodeMap.end())
    {
        DeleteInode(inode);
        return true;
    }
    kassert(inodeIterator->second == inode);

    if (inode->GetDontCache())
    {
        if (inode->IsListMember(&s_InodeLRUList)) {
            s_InodeLRUList.Remove(inode);
        }
        DeleteInode(inode);
        return true;
    }

    if (inode->IsListMember(&s_InodeLRUList)) {
        s_InodeLRUList.Remove(inode);
    }
    s_InodeLRUList.Append(inode);
    while (s_InodeLRUList.GetCount() > MAX_INODE_CACHE_COUNT)
    {
        KInode* unusedInode = FindFirstUnusedInode();
        if (unusedInode == nullptr) {
            break;
        }
        DiscardInode(unusedInode);
    }

    s_InodeMapConditionVar.WakeupAll();
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::FlushInodes()
{
    std::vector<Ptr<KFSVolume>> volumes;
    {
        KScopedLock inodeMapLock(s_InodeMapMutex);
        volumes.reserve(s_VolumeMap.size());
        for (const auto& volumeEntry : s_VolumeMap) {
            volumes.push_back(volumeEntry.second);
        }
    }

    for (const Ptr<KFSVolume>& volume : volumes)
    {
        const PErrorCode flushError = FlushInodesInternal(ptr_raw_pointer_cast(volume));
        if (flushError != PErrorCode::Success)
        {
            kernel_log<PLogSeverity::ERROR>(
                LogCatKernel_VFS,
                "KVFSManager::FlushInodes(): failed to write inodes for volume {} ({}): {}.",
                volume->m_VolumeID,
                volume->m_DevicePath.c_str(),
                p_strerror(flushError));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::FlushInodes(KFSVolume* volume)
{
    if (volume == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    const PErrorCode flushError = FlushInodesInternal(volume);
    if (flushError != PErrorCode::Success) {
        PERROR_THROW_CODE(flushError);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DiscardDirtyInodes(KFSVolume* volume) noexcept
{
    if (volume == nullptr) {
        return;
    }

    std::vector<Ptr<KInode>> inodeReferences;
    {
        KScopedLock inodeMapLock(s_InodeMapMutex);
        volume->DiscardDirtyInodes(inodeReferences);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::MarkInodeDirty(KInode* inode) noexcept
{
    kassert(inode != nullptr);
    kassert(inode->IsActive());

    KScopedLock inodeMapLock(s_InodeMapMutex);

    if (inode->IsDeleted()) {
        return;
    }

    if (inode->SetDirtyFlag())
    {
        if (!inode->IsWritebackInProgress())
        {
            if (inode->IsListMember(&s_InodeLRUList)) {
                s_InodeLRUList.Remove(inode);
            }
            kassert(!inode->IsListMember());
            inode->m_Volume->QueueDirtyInode(inode);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DiscardInodeDirtyState(KInode* inode) noexcept
{
    kassert(inode != nullptr);

    KScopedLock inodeMapLock(s_InodeMapMutex);
    inode->m_Volume->DiscardInodeDirtyState(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVFSManager::TryAcquireInodeReference(KInode* inode)
{
    kassert(s_InodeMapMutex.IsLocked());

    if (inode->m_Volume->IsDeletedInodeQueued(inode)) {
        return nullptr;
    }
    if (inode->IsListMember(&s_InodeLRUList)) {
        s_InodeLRUList.Remove(inode);
    }
    if (inode->GetPtrCount() == 0) {
        return ptr_tmp_cast(inode);
    }
    return ptr_lock_cast(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KVFSManager::FlushInodesInternal(KFSVolume* volume)
{
    kassert(volume != nullptr);

    KUniqueLock inodeFlushLock = volume->LockInodeFlush();
    if (volume->m_Filesystem == nullptr) {
        return PErrorCode::Success;
    }
    return FlushInodes_pl(volume);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KVFSManager::FlushInodes_pl(KFSVolume* volume)
{
    DeleteReleasedInodes(volume);
    return WriteDirtyInodes(volume);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DeleteReleasedInodes(KFSVolume* volume)
{
    KScopedLock inodeMapLock(s_InodeMapMutex);

    const size_t releasedInodeCount = volume->GetDeletedInodeCount();
    for (size_t inodeIndex = 0; inodeIndex < releasedInodeCount; ++inodeIndex)
    {
        KInode* inode = volume->TakeFirstDeletedInode();
        kassert(inode != nullptr);

        if (inode->GetPtrCount() != 0)
        {
            volume->QueueDeletedInode(inode);
            continue;
        }
        DeleteInode(inode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KVFSManager::WriteDirtyInodes(KFSVolume* volume)
{
    const size_t dirtyInodeCount = volume->GetDirtyInodeCount();
    const size_t batchCount =
        (dirtyInodeCount + KFSVolume::MAX_FLUSH_INODE_COUNT - 1) / KFSVolume::MAX_FLUSH_INODE_COUNT;

    PErrorCode firstError = PErrorCode::Success;
    for (size_t batchIndex = 0; batchIndex < batchCount; ++batchIndex)
    {
        std::vector<Ptr<KInode>> inodeBatch;
        {
            KScopedLock inodeMapLock(s_InodeMapMutex);
            inodeBatch = volume->PrepareInodeWritebackBatch();
        }
        if (inodeBatch.empty()) {
            break;
        }

        std::array<bool, KFSVolume::MAX_FLUSH_INODE_COUNT> inodeWritesFailed;
        std::fill_n(inodeWritesFailed.begin(), inodeBatch.size(), false);
        bool writebackReadOnly = false;
        for (size_t inodeIndex = 0; inodeIndex < inodeBatch.size(); ++inodeIndex)
        {
            try
            {
                inodeBatch[inodeIndex]->m_Filesystem->WriteInode(ptr_raw_pointer_cast(inodeBatch[inodeIndex]));
            }
            PERROR_CATCH(([&firstError, &inodeWritesFailed, &writebackReadOnly, inodeIndex](PErrorCode error)
            {
                inodeWritesFailed[inodeIndex] = true;
                if (firstError == PErrorCode::Success) {
                    firstError = error;
                }
                if (error == PErrorCode::ROFS) {
                    writebackReadOnly = true;
                }
            }));

            if (writebackReadOnly) {
                break;
            }
        }

        {
            KScopedLock inodeMapLock(s_InodeMapMutex);
            volume->FinishInodeWritebackBatch(inodeBatch, inodeWritesFailed, writebackReadOnly);
        }

        inodeBatch.clear();
        if (writebackReadOnly)
        {
            DiscardDirtyInodes(volume);
            break;
        }
    }
    return firstError;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInode* KVFSManager::FindFirstUnusedInode()
{
    kassert(s_InodeMapMutex.IsLocked());

    for (KInode* inode : s_InodeLRUList)
    {
        if (inode->GetPtrCount() == 0) {
            return inode;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DiscardInode(KInode* inode)
{
    kassert(s_InodeMapMutex.IsLocked());
    kassert(inode->GetPtrCount() == 0);
    s_InodeLRUList.Remove(inode);
    DeleteInode(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DeleteInode(KInode* inode)
{
    kassert(s_InodeMapMutex.IsLocked());
    kassert(!inode->IsListMember());
    kassert(!inode->IsDirty());
    kassert(!inode->IsWritebackInProgress());
    
    Ptr<KFSVolume> volume = inode->m_Volume;
    const ino_t inodeID = inode->m_InodeID;
    auto inodeIterator = volume->m_InodeMap.find(inodeID);
    const bool inodeIsRegistered = inodeIterator != volume->m_InodeMap.end();
    if (inodeIsRegistered)
    {
        kassert(inodeIterator->second == inode);
        inodeIterator->second = PENDING_INODE;
    }

    s_InodeMapMutex.Unlock();
    if (inode->IsDeleted() && inode->IsDirectory()) {
        KDirectoryCache::RemoveDirectory(volume->m_VolumeID, inodeID);
    }
    try
    {
        inode->m_Filesystem->ReleaseInode(inode);
    }
    PERROR_CATCH([](PErrorCode error) { kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "ERROR: Failed to release inode."); });

    delete inode;
    s_InodeMapMutex.Lock();
    
    if (inodeIsRegistered)
    {
        inodeIterator = volume->m_InodeMap.find(inodeID);
        kassert(inodeIterator != volume->m_InodeMap.end());
        kassert(inodeIterator->second == PENDING_INODE);
        volume->m_InodeMap.erase(inodeIterator);
        s_InodeMapConditionVar.Wakeup(0);
    }
}

} // namespace kernel
