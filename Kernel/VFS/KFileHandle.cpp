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
// Created: 23.02.2018 01:47:38

#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KINode.h"
#include "Kernel/VFS/KFilesystem.h"

bool kernel::KFileNode::LastReferenceGone()
{
    Ptr<KINode> inode = GetINode();
    inode->m_FileOps->CloseFile(inode->m_Volume, this);
    return KFileTableNode::LastReferenceGone();
}

bool kernel::KDirectoryNode::LastReferenceGone()
{
    Ptr<KINode> inode = GetINode();
    if (inode->m_FileOps != nullptr) {
        inode->m_FileOps->CloseDirectory(inode->m_Volume, ptr_tmp_cast(this));
    }
    return KFileTableNode::LastReferenceGone();
}

int kernel::KDirectoryNode::ReadDirectory(dir_entry* entry, size_t bufSize)
{
    Ptr<KINode> inode = GetINode();
    if (inode != nullptr && inode->m_FileOps != nullptr) {
        return inode->m_FileOps->ReadDirectory(inode->m_Volume, ptr_tmp_cast(this), entry, bufSize);
    }
    set_last_error(EINVAL);
    return -1;
}

int kernel::KDirectoryNode::RewindDirectory()
{
    Ptr<KINode> inode = GetINode();
    if (inode != nullptr && inode->m_FileOps != nullptr) {
        return inode->m_FileOps->RewindDirectory(inode->m_Volume, ptr_tmp_cast(this));
    }
    set_last_error(EINVAL);
    return -1;
}
