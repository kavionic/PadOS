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

#pragma once

#include <array>
#include <atomic>
#include <map>
#include <string>
#include <vector>

#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "Utils/IntrusiveList.h"
#include "Utils/String.h"
#include "Kernel/KMutex.h"
#include "KFilesystem.h"

namespace kernel
{

class KFilesystem;
class KInode;

enum
{
    VOLID_ROOT = 1,
    VOLID_FIRST_NORMAL = 100 // Reserve the first IDs for special mounts like root, dev, pty, etc etc
};

class KFSVolume : public PtrTarget
{
public:
    static constexpr size_t MAX_FLUSH_INODE_COUNT = 128;

    KFSVolume(fs_id volumeID, const PString& devicePath);
    
    inline void     SetFlags(uint32_t flags) { m_Flags = flags; }
    inline uint32_t GetFlags() const         { return m_Flags; }
    inline bool     HasFlag(FSVolumeFlags flag) const { return m_Flags & uint32_t(flag); }
    size_t          GetDirtyInodeCount() const noexcept { return m_DirtyInodeCount.load(std::memory_order_relaxed); }

    KUniqueLock LockInodeFlush();

    size_t          GetDeletedInodeCount() const noexcept;
    KInode*         TakeFirstDeletedInode() noexcept;
    bool            IsDeletedInodeQueued(const KInode* inode) const noexcept;
    bool            RemoveDeletedInodeIfQueued(KInode* inode) noexcept;
    void            QueueDirtyInode(KInode* inode) noexcept;
    void            QueueDeletedInode(KInode* inode) noexcept;

    void                     DiscardInodeDirtyState(KInode* inode) noexcept;
    void                     DiscardDirtyInodes(std::vector<Ptr<KInode>>& inodeReferences);
    std::vector<Ptr<KInode>> PrepareInodeWritebackBatch();
    void                     FinishInodeWritebackBatch(
        const std::vector<Ptr<KInode>>& inodeBatch,
        const std::array<bool, MAX_FLUSH_INODE_COUNT>& inodeWritesFailed,
        bool writebackReadOnly);

    void InodeBecameDirty() noexcept;
    void InodeBecameClean() noexcept;
    
    fs_id            m_VolumeID;
    uint32_t         m_Flags;
    Ptr<KFilesystem> m_Filesystem;
    Ptr<KInode>      m_MountPoint;
    PString          m_DevicePath;
    Ptr<KInode>      m_RootNode;

    std::map<ino_t, KInode*>    m_InodeMap;

private:
    KMutex                 m_InodeFlushMutex;
    std::atomic_size_t     m_DirtyInodeCount = 0;
    PIntrusiveList<KInode> m_DirtyInodes;
    PIntrusiveList<KInode> m_DeletedInodes;
};

} // namespace
