// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 23.02.2018 01:44:32

#pragma once

#include <atomic>
#include <sys/stat.h>
#include <sys/types.h>

#include <System/Sections.h>
#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include <Utils/IntrusiveList.h>
#include <Kernel/KWaitableObject.h>


namespace kernel
{

class KFilesystem;
class KFilesystemFileOps;
class KFSVolume;
class KInode;

class KInode : public PtrTarget, public KWaitableObject, public PIntrusiveListNode<KInode>
{
public:
    KInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KFilesystemFileOps* fileOps, mode_t fileMode);
    virtual ~KInode();
    
    virtual bool LastReferenceGone() override;

    bool IsActive() const noexcept { return m_Filesystem != nullptr && m_Volume != nullptr && m_FileOps != nullptr; }
    void Detach() noexcept;

    void MarkDirty() noexcept;
    void DiscardDirty() noexcept;
    bool IsDirty() const noexcept { return m_IsDirty.load(std::memory_order_relaxed); }
    bool IsDirtyPending() const noexcept { return m_IsDirtyPending.load(std::memory_order_relaxed); }
    bool IsWritebackInProgress() const noexcept { return m_IsWritebackInProgress; }

    bool SetDirtyFlag() noexcept;
    bool ClearDirtyFlag() noexcept;
    void BeginWriteback() noexcept;
    void FinishWriteback() noexcept;
    
    inline void SetDeletedFlag(bool isDeleted)  noexcept { m_IsDeleted = isDeleted; }
    inline bool IsDeleted() const  noexcept { return m_IsDeleted; }
    inline bool IsDirectory() const noexcept { return S_ISDIR(m_FileMode); }
    inline void SetDontCache(bool dontCache) noexcept { m_DontCache = dontCache; }
    inline bool GetDontCache() const noexcept { return m_DontCache; }
        
    Ptr<KFilesystem>    m_Filesystem;
    Ptr<KFSVolume>      m_Volume; // The volume this i-node came from.
    KFilesystemFileOps* m_FileOps;
    Ptr<KInode>         m_MountRoot; // Root node of filesystem mounted on this inode if any.
    ino_t               m_InodeID = 0;
    mode_t              m_FileMode = 0;
    
    TimeValNanos        m_CTime;
    TimeValNanos        m_MTime;
    TimeValNanos        m_ATime;

    static_assert(sizeof(ino_t) == 8);

    bool m_DontCache = false;
    bool m_IsDeleted = false;

private:
    std::atomic_bool m_IsDirty = false;
    std::atomic_bool m_IsDirtyPending = false;
    bool             m_IsWritebackInProgress = false;
};

} // namespace
