// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 19.02.2018 21:39:19

 #include "System/Platform.h"

#include <string.h>
#include <assert.h>
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

KMutex                                     KVFSManager::s_InodeMapMutex("inode_map_mutex", PEMutexRecursionMode_RaiseError);
std::map<std::pair<fs_id, ino_t>, KInode*> KVFSManager::s_InodeMap;
PIntrusiveList<KInode>                      KVFSManager::s_InodeMRUList;
KConditionVariable                         KVFSManager::s_InodeMapConditionVar("inode_map_condition");

std::map<fs_id, Ptr<KFSVolume>> KVFSManager::s_VolumeMap;

struct PartitionRecord
{
    uint8_t  m_Status;
    uint8_t  m_FirstHead;
    uint16_t m_FirstCyl;
    uint8_t  m_Type;
    uint8_t  m_LastHead;
    uint16_t m_LastCyl;
    uint32_t m_StartLBA;
    uint32_t m_Size;
} __attribute__((packed));

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
///* Decode a hard-disk partition table.
/// \ingroup DriverAPI
/// \par Description:
///	DecodeDiskPartitions() can be called by block-device drivers to
///	decode a disk's partition table. It will return both primary
///	partitions and logical partitions within the extended partition if
///	one exists. The extended partition itself will not be returned.
///
///	The caller must provide the device-geometry and a callback that
///	will be called to read the primary partition table and any existing
///	nested extended partition tables.
///
///	The partition table is validated and the function will fail if
///	it is found invalid. Checks performed includes overlapping partitions,
///	partitions ending outside the disk and the primary table containing
///	more than one extended partition.
///
/// \par Note:
///	Primary partitions are numbered 0-3 based on their position inside
///	the primary table. Logical partition are numbered from 4 and up.
///	This might leave "holes" in the returned array of partitions.
///	The returned count only indicate the highest used partition number
///	and the caller must check each returned partition and filter out
///	partitions where the type-field is '0'.
/// \param blockBuffer  Caller-provided buffer used for reading disk blocks.
/// \param bufferSize   Size of \p blockBuffer in bytes.
/// \param diskGeom     Structure describing the disk geometry (sector count and bytes per sector).
/// \param readCallback Callback invoked to read a block at a given offset into \p blockBuffer.
/// \param userData     Opaque pointer passed through to each \p readCallback invocation.
///
/// \return A vector of partition descriptors for all discovered primary and logical partitions.
/// \sa create_device_node(), delete_device_node()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<disk_partition_desc> KVFSManager::DecodeDiskPartitions_trw(void* blockBuffer, size_t bufferSize, const device_geometry& diskGeom, disk_read_op* readCallback, void* userData)
{
    uint8_t* buffer = reinterpret_cast<uint8_t*>(blockBuffer);
    PartitionRecord* recordTable = reinterpret_cast<PartitionRecord*>(&buffer[0x1be]);
    off64_t diskSize = diskGeom.sector_count * diskGeom.bytes_per_sector;
    off64_t tablePos = 0;
    off64_t extStart = 0;
    off64_t firstExtended = 0;
    int	    numExtended;
    int	    numActive;

    static const size_t MAX_PARTITIONS = 64; // Just a sanity check in case there is some kind of circular loop with the extended partition

    std::vector<disk_partition_desc> partitions;

    while (partitions.size() < MAX_PARTITIONS)
    {
        readCallback(userData, tablePos, buffer, bufferSize);
        if (*reinterpret_cast<uint16_t*>(&buffer[0x1fe]) != 0xaa55)
        {
            kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "KVFSManager::DecodeDiskPartitions() Invalid partition table signature {:04x}", *reinterpret_cast<uint16_t*>(&buffer[0x1fe]));
            PERROR_THROW_CODE(PErrorCode::FTYPE);
        }

        numActive = 0;
        numExtended = 0;

        for (int i = 0; i < 4; ++i)
        {
            if (recordTable[i].m_Status & 0x80) {
                numActive++;
            }
            if (recordTable[i].m_Type == 0x05 || recordTable[i].m_Type == 0x0f || recordTable[i].m_Type == 0x85) {
                numExtended++;
            }
            if (numActive > 1) {
                kernel_log<PLogSeverity::WARNING>(LogCatKernel_VFS, "KVFSManager::DecodeDiskPartitions() more than one active partitions.");
            }
            if (numExtended > 1)
            {
                kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "KVFSManager::DecodeDiskPartitions() more than one extended partitions.");
                PERROR_THROW_CODE(PErrorCode::FTYPE);
            }
        }
        for (int i = 0; i < 4 && partitions.size() < MAX_PARTITIONS; ++i)
        {
            if (recordTable[i].m_Type == 0) {
                continue;
            }
            disk_partition_desc partitionDesc;
            memset(&partitionDesc, 0, sizeof(partitionDesc));

            if (recordTable[i].m_Type == 0x05 || recordTable[i].m_Type == 0x0f || recordTable[i].m_Type == 0x85)
            {
                extStart = uint64_t(recordTable[i].m_StartLBA) * uint64_t(diskGeom.bytes_per_sector); // + nTablePos;
                if (firstExtended == 0) {
                    partitions.push_back(partitionDesc);
                }
                continue;
            }
            partitionDesc.p_type = recordTable[i].m_Type;
            partitionDesc.p_status = recordTable[i].m_Status;
            partitionDesc.p_start = uint64_t(recordTable[i].m_StartLBA) * uint64_t(diskGeom.bytes_per_sector) + tablePos;
            partitionDesc.p_size = uint64_t(recordTable[i].m_Size) * uint64_t(diskGeom.bytes_per_sector);

            if (partitionDesc.p_start + partitionDesc.p_size > diskSize)
            {
                kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "Partition {} extend outside the disk/extended partition.", partitions.size());
                PERROR_THROW_CODE(PErrorCode::FTYPE);
            }

            for (size_t j = 0; j < partitions.size(); ++j)
            {
                const disk_partition_desc& curPartition = partitions[j];
                if (partitionDesc.p_type == 0) {
                    continue;
                }
                if (curPartition.p_start + curPartition.p_size > partitionDesc.p_start &&
                    curPartition.p_start < partitionDesc.p_start + partitionDesc.p_size)
                {
                    kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "KVFSManager::DecodeDiskPartitions() partition {} overlap partition {}", j, partitions.size());
                    PERROR_THROW_CODE(PErrorCode::FTYPE);
                }
                if ((partitionDesc.p_status & 0x80) != 0 && (curPartition.p_status & 0x80) != 0)
                {
                    kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "KVFSManager::DecodeDiskPartitions() more than one active partitions.");
                    PERROR_THROW_CODE(PErrorCode::FTYPE);
                }
                if (partitionDesc.p_type == 0x05 && curPartition.p_type == 0x05)
                {
                    kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "KVFSManager::DecodeDiskPartitions() more than one extended partitions.");
                    PERROR_THROW_CODE(PErrorCode::FTYPE);
                }
            }
            partitions.push_back(partitionDesc);
        }
        if (extStart != 0)
        {
            tablePos = firstExtended + extStart;
            if (firstExtended == 0) {
                firstExtended = extStart;
            }
            extStart = 0;
        }
        else
        {
            break;
        }
    }
    return partitions;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::RegisterVolume_trw(Ptr<KFSVolume> volume)
{
    if (volume == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    KScopedLock inodeMapLock(s_InodeMapMutex);

    if (volume->m_Filesystem == nullptr ||
        volume->m_RootNode == nullptr ||
        !volume->m_RootNode->IsActive() ||
        volume->m_RootNode->m_Filesystem != volume->m_Filesystem ||
        volume->m_RootNode->m_Volume != volume)
    {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    if (s_VolumeMap.find(volume->m_VolumeID) != s_VolumeMap.end()) {
        kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "KVFSManager::RegisterVolume() failed to register volume {:#x}", intptr_t(ptr_raw_pointer_cast(volume)));
        PERROR_THROW_CODE(PErrorCode::EXIST);
    }

    if (volume->m_MountPoint != nullptr)
    {
        if (!volume->m_MountPoint->IsActive()) {
            PERROR_THROW_CODE(PErrorCode(ENODEV));
        }
        if (volume->m_MountPoint->m_MountRoot != nullptr) {
            PERROR_THROW_CODE(PErrorCode::BUSY);
        }
    }

    s_VolumeMap[volume->m_VolumeID] = volume;

    if (volume->m_MountPoint != nullptr) {
        volume->m_MountPoint->m_MountRoot = volume->m_RootNode;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DetachVolume_trw(Ptr<KFSVolume> volume)
{
    if (volume == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    std::vector<Ptr<KInode>> volumeInodes;
    const Ptr<KInode> rootNode = volume->m_RootNode;
    {
        KScopedLock inodeMapLock(s_InodeMapMutex);
        volumeInodes.reserve(s_InodeMap.size() + 1);

        for (;;)
        {
            bool shouldWaitForInode = false;

            for (const auto& inodeEntry : s_InodeMap)
            {
                if (inodeEntry.first.first != volume->m_VolumeID) {
                    continue;
                }
                if (inodeEntry.second == PENDING_INODE)
                {
                    shouldWaitForInode = true;
                    break;
                }

                KInode* inode = inodeEntry.second;
                if (inode->m_Volume != volume) {
                    continue;
                }
                if (inode->m_MountRoot != nullptr) {
                    PERROR_THROW_CODE(PErrorCode::BUSY);
                }

                bool inodeAlreadyReferenced = false;
                for (const Ptr<KInode>& referencedInode : volumeInodes)
                {
                    if (referencedInode == inode)
                    {
                        inodeAlreadyReferenced = true;
                        break;
                    }
                }
                if (inodeAlreadyReferenced) {
                    continue;
                }

                Ptr<KInode> inodeReference = TryAcquireInodeReference(inode);
                if (inodeReference == nullptr)
                {
                    shouldWaitForInode = true;
                    break;
                }
                volumeInodes.push_back(inodeReference);
            }

            if (shouldWaitForInode)
            {
                s_InodeMapConditionVar.Wait(s_InodeMapMutex);
                continue;
            }

            if (rootNode != nullptr && rootNode->m_MountRoot != nullptr) {
                PERROR_THROW_CODE(PErrorCode::BUSY);
            }
            break;
        }

        auto registeredVolume = s_VolumeMap.find(volume->m_VolumeID);
        if (registeredVolume != s_VolumeMap.end() && registeredVolume->second == volume) {
            s_VolumeMap.erase(registeredVolume);
        }

        for (auto inodeIterator = s_InodeMap.begin(); inodeIterator != s_InodeMap.end();)
        {
            KInode* inode = inodeIterator->second;
            if (inodeIterator->first.first != volume->m_VolumeID || inode == PENDING_INODE || inode->m_Volume != volume)
            {
                ++inodeIterator;
                continue;
            }

            inodeIterator = s_InodeMap.erase(inodeIterator);
        }

        if (rootNode != nullptr)
        {
            bool rootNodeIncluded = false;
            for (const Ptr<KInode>& inode : volumeInodes)
            {
                if (inode == rootNode)
                {
                    rootNodeIncluded = true;
                    break;
                }
            }
            if (!rootNodeIncluded) {
                volumeInodes.push_back(rootNode);
            }
        }

        if (volume->m_MountPoint != nullptr && volume->m_MountPoint->m_MountRoot == rootNode) {
            volume->m_MountPoint->m_MountRoot = nullptr;
        }
        volume->m_MountPoint = nullptr;
        volume->m_RootNode = nullptr;
    }

    KDirectoryCache::RemoveVolume(volume->m_VolumeID);

    for (Ptr<KInode>& inode : volumeInodes)
    {
        try
        {
            if (inode->IsActive()) {
                inode->m_Filesystem->ReleaseInode(ptr_raw_pointer_cast(inode));
            }
        }
        catch (const std::exception& exception)
        {
            kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "KVFSManager::DetachVolume_trw(): failed to release inode {:x}: {}", inode->m_InodeID, exception.what());
        }
        inode->Detach();
    }
    KDirectoryCache::RemoveVolume(volume->m_VolumeID);
    volume->m_Filesystem = nullptr;
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

Ptr<KInode> KVFSManager::GetInode_trw(fs_id volumeID, ino_t inodeID, bool crossMount)
{
    const auto key = std::make_pair(volumeID, inodeID);

    for (;;)
    {
        Ptr<KFSVolume> volume;
        {
            KScopedLock inodeMapLock(s_InodeMapMutex);

            auto i = s_InodeMap.find(key);
            if (i != s_InodeMap.end())
            {
                if (i->second == PENDING_INODE)
                {
                    s_InodeMapConditionVar.Wait(s_InodeMapMutex);
                    continue;
                }
                Ptr<KInode> inode = TryAcquireInodeReference(i->second);
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

            auto volumeIterator = s_VolumeMap.find(volumeID);
            if (volumeIterator == s_VolumeMap.end() || volumeIterator->second->m_Filesystem == nullptr) {
                PERROR_THROW_CODE(PErrorCode(ENODEV));
            }
            volume = volumeIterator->second;

            s_InodeMap[key] = PENDING_INODE;
        }

        Ptr<KInode> inode;
        {
            PScopeExit completeInodeLoad(
                [&key, &inode]()
                {
                    KScopedLock inodeMapLock(s_InodeMapMutex);

                    auto pendingInode = s_InodeMap.find(key);
                    kassert(pendingInode != s_InodeMap.end());
                    kassert(pendingInode->second == PENDING_INODE);

                    if (pendingInode != s_InodeMap.end() && pendingInode->second == PENDING_INODE)
                    {
                        if (inode != nullptr) {
                            pendingInode->second = ptr_raw_pointer_cast(inode);
                        } else {
                            s_InodeMap.erase(pendingInode);
                        }
                    }
                    s_InodeMapConditionVar.WakeupAll();
                });

            inode = volume->m_Filesystem->LoadInode(volume, inodeID);
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

    const auto key = std::make_pair(inode->m_Volume->m_VolumeID, inode->m_InodeID);
    auto inodeIterator = s_InodeMap.find(key);
    if (inodeIterator == s_InodeMap.end())
    {
        DeleteInode(inode);
        return true;
    }
    kassert(inodeIterator->second == inode);

    if (inode->GetDontCache())
    {
        if (inode->IsListMember(&s_InodeMRUList)) {
            s_InodeMRUList.Remove(inode);
        }
        DeleteInode(inode);
        return true;
    }

    if (inode->IsListMember(&s_InodeMRUList)) {
        s_InodeMRUList.Remove(inode);
    }
    s_InodeMRUList.Append(inode);
    if (s_InodeMRUList.GetCount() > MAX_INODE_CACHE_COUNT)
    {
        KInode* unusedInode = FindFirstUnusedInode();
        if (unusedInode != nullptr) {
            DiscardInode(unusedInode);
        }
    }

    s_InodeMapConditionVar.WakeupAll();
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::FlushInodes()
{
    KScopedLock inodeMapLock(s_InodeMapMutex);
    const TimeValNanos currentTime = kget_monotonic_time();

    for (;;)
    {
        KInode* inode = nullptr;
        if (s_InodeMRUList.GetCount() > MAX_INODE_CACHE_COUNT) {
            inode = FindFirstUnusedInode();
        }
        if (inode == nullptr) {
            inode = FindFirstExpiredUnusedInode(currentTime);
        }
        if (inode == nullptr) {
            break;
        }
        DiscardInode(inode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVFSManager::TryAcquireInodeReference(KInode* inode)
{
    kassert(s_InodeMapMutex.IsLocked());

    if (inode->IsListMember(&s_InodeMRUList))
    {
        s_InodeMRUList.Remove(inode);
        return ptr_tmp_cast(inode);
    }

    return ptr_lock_cast(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInode* KVFSManager::FindFirstUnusedInode()
{
    kassert(s_InodeMapMutex.IsLocked());

    for (KInode* inode : s_InodeMRUList)
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

KInode* KVFSManager::FindFirstExpiredUnusedInode(TimeValNanos currentTime)
{
    kassert(s_InodeMapMutex.IsLocked());

    for (KInode* inode : s_InodeMRUList)
    {
        if (inode->GetPtrCount() == 0 && currentTime > inode->m_LastUseTime + INODE_CACHE_EXPIRATION_TIME) {
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
    s_InodeMRUList.Remove(inode);
    DeleteInode(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DeleteInode(KInode* inode)
{
    kassert(s_InodeMapMutex.IsLocked());
    kassert(!inode->IsListMember(&s_InodeMRUList));
    
    auto key = std::make_pair(inode->m_Volume->m_VolumeID, inode->m_InodeID);
    auto i = s_InodeMap.find(key);
    const bool inodeIsRegistered = i != s_InodeMap.end();
    if (inodeIsRegistered)
    {
        kassert(i->second == inode);
        i->second = PENDING_INODE;
    }

    s_InodeMapMutex.Unlock();
    if (inode->IsDeleted() && inode->IsDirectory()) {
        KDirectoryCache::RemoveDirectory(key.first, key.second);
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
        i = s_InodeMap.find(key);
        kassert(i != s_InodeMap.end());
        kassert(i->second == PENDING_INODE);
        s_InodeMap.erase(i);
        s_InodeMapConditionVar.Wakeup(0);
    }
}

} // namespace kernel
