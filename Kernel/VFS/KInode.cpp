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
// Created: 23.02.2018 01:44:31

#include "System/Platform.h"

#include <Kernel/KTime.h>
#include "Kernel/VFS/KInode.h"
#include "Kernel/VFS/KFilesystem.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KVFSManager.h"


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInode::KInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KFilesystemFileOps* fileOps, mode_t fileMode)
    : m_Filesystem(filesystem)
    , m_Volume(volume)
    , m_FileOps(fileOps)
    , m_FileMode(fileMode)
{
    m_ATime = m_MTime = m_CTime = kget_real_time();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KInode::~KInode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KInode::LastReferenceGone()
{
    if (!IsActive())
    {
        delete this;
        return true;
    }

    return KVFSManager::InodeReleased(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInode::Detach() noexcept
{
    m_FileOps = nullptr;
    m_Filesystem = nullptr;
    m_Volume = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInode::MarkDirty() noexcept
{
    KVFSManager::MarkInodeDirty(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInode::DiscardDirty() noexcept
{
    KVFSManager::DiscardInodeDirtyState(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KInode::SetDirtyFlag() noexcept
{
    if (IsDirty())
    {
        m_IsDirtyPending.store(true, std::memory_order_relaxed);
        return false;
    }

    m_IsDirty.store(true, std::memory_order_relaxed);
    m_IsDirtyPending.store(m_IsWritebackInProgress, std::memory_order_relaxed);
    m_Volume->InodeBecameDirty();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KInode::ClearDirtyFlag() noexcept
{
    if (!IsDirty()) {
        return false;
    }

    m_IsDirty.store(false, std::memory_order_relaxed);
    m_IsDirtyPending.store(false, std::memory_order_relaxed);
    m_Volume->InodeBecameClean();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInode::BeginWriteback() noexcept
{
    kassert(IsDirty());
    kassert(!m_IsWritebackInProgress);

    m_IsWritebackInProgress = true;
    m_IsDirtyPending.store(false, std::memory_order_relaxed);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KInode::FinishWriteback() noexcept
{
    kassert(m_IsWritebackInProgress);
    m_IsWritebackInProgress = false;
}

} // namespace kernel
