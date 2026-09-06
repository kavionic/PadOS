// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.02.2018 01:46:15

 #include "System/Platform.h"

#include <algorithm>
#include <utility>

#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KInode.h"
#include "Kernel/VFS/KFilesystem.h"
#include "Utils/String.h"


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KFSVolume::KFSVolume(fs_id volumeID, const PString& devicePath )
    : m_VolumeID(volumeID)
    , m_DevicePath(devicePath)
    , m_InodeFlushMutex("inode_flush_mutex", PEMutexRecursionMode_RaiseError)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KUniqueLock KFSVolume::LockInodeFlush()
{
    return KUniqueLock(m_InodeFlushMutex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KFSVolume::GetDeletedInodeCount() const noexcept
{
    return m_DeletedInodes.GetCount();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInode* KFSVolume::TakeFirstDeletedInode() noexcept
{
    KInode* inode = m_DeletedInodes.GetFirst();
    if (inode != nullptr) {
        m_DeletedInodes.Remove(inode);
    }
    return inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KFSVolume::IsDeletedInodeQueued(const KInode* inode) const noexcept
{
    return inode->IsListMember(&m_DeletedInodes);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KFSVolume::RemoveDeletedInodeIfQueued(KInode* inode) noexcept
{
    if (!IsDeletedInodeQueued(inode)) {
        return false;
    }

    m_DeletedInodes.Remove(inode);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KFSVolume::QueueDirtyInode(KInode* inode) noexcept
{
    kassert(inode != nullptr);
    kassert(inode->m_Volume == this);
    kassert(inode->IsDirty());
    kassert(!inode->IsWritebackInProgress());
    kassert(!inode->IsListMember());
    m_DirtyInodes.Append(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KFSVolume::QueueDeletedInode(KInode* inode) noexcept
{
    kassert(inode != nullptr);
    kassert(inode->m_Volume == this);
    kassert(inode->IsDeleted());
    kassert(!inode->IsDirty());
    kassert(!inode->IsWritebackInProgress());
    kassert(!inode->IsListMember());
    m_DeletedInodes.Append(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KFSVolume::DiscardInodeDirtyState(KInode* inode) noexcept
{
    kassert(inode != nullptr);
    kassert(inode->m_Volume == this);

    if (!inode->IsDirty()) {
        return;
    }

    if (inode->IsListMember(&m_DirtyInodes)) {
        m_DirtyInodes.Remove(inode);
    }
    const bool wasDirty = inode->ClearDirtyFlag();
    kassert(wasDirty);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KFSVolume::DiscardDirtyInodes(std::vector<Ptr<KInode>>& inodeReferences)
{
    inodeReferences.reserve(GetDirtyInodeCount());

    for (;;)
    {
        KInode* inode = m_DirtyInodes.GetFirst();
        if (inode == nullptr) {
            break;
        }

        Ptr<KInode> inodeReference = (inode->GetPtrCount() == 0) ? ptr_tmp_cast(inode) : ptr_lock_cast(inode);
        kassert(inodeReference != nullptr);
        DiscardInodeDirtyState(inode);
        inodeReferences.push_back(std::move(inodeReference));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<Ptr<KInode>> KFSVolume::PrepareInodeWritebackBatch()
{
    kassert(m_InodeFlushMutex.IsLocked());

    std::vector<Ptr<KInode>> inodeBatch;
    const size_t batchSize = std::min(MAX_FLUSH_INODE_COUNT, m_DirtyInodes.GetCount());
    inodeBatch.reserve(batchSize);

    for (size_t inodeIndex = 0; inodeIndex < batchSize; ++inodeIndex)
    {
        KInode* inode = m_DirtyInodes.GetFirst();
        kassert(inode != nullptr);

        m_DirtyInodes.Remove(inode);
        inode->BeginWriteback();

        Ptr<KInode> inodeReference = (inode->GetPtrCount() == 0) ? ptr_tmp_cast(inode) : ptr_lock_cast(inode);
        kassert(inodeReference != nullptr);
        inodeBatch.push_back(std::move(inodeReference));
    }
    return inodeBatch;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KFSVolume::FinishInodeWritebackBatch(
    const std::vector<Ptr<KInode>>& inodeBatch,
    const std::array<bool, MAX_FLUSH_INODE_COUNT>& inodeWritesFailed,
    bool writebackReadOnly)
{
    kassert(m_InodeFlushMutex.IsLocked());
    kassert(inodeBatch.size() <= inodeWritesFailed.size());

    for (size_t inodeIndex = 0; inodeIndex < inodeBatch.size(); ++inodeIndex)
    {
        KInode* inode = ptr_raw_pointer_cast(inodeBatch[inodeIndex]);
        inode->FinishWriteback();

        if (!inode->IsDirty()) {
            continue;
        }

        const bool shouldDiscardDirtyState =
            inode->IsDeleted() ||
            writebackReadOnly ||
            (!inodeWritesFailed[inodeIndex] && !inode->IsDirtyPending());
        if (shouldDiscardDirtyState)
        {
            DiscardInodeDirtyState(inode);
        }
        else
        {
            QueueDirtyInode(inode);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KFSVolume::InodeBecameDirty() noexcept
{
    m_DirtyInodeCount.fetch_add(1, std::memory_order_relaxed);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KFSVolume::InodeBecameClean() noexcept
{
    const size_t previousCount = m_DirtyInodeCount.fetch_sub(1, std::memory_order_relaxed);
    kassert(previousCount != 0);
}

} // namespace kernel
