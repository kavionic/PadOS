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

#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

bool KFileNode::LastReferenceGone()
{
    try
    {
        Ptr<KInode> inode = GetInode();
        if (inode != nullptr && inode->IsActive()) {
            inode->m_FileOps->CloseFile(inode->m_Volume, this);
        }
    }
    catch (const std::exception&) {}
    return KFileTableNode::LastReferenceGone();
}

bool KDirectoryNode::LastReferenceGone()
{
    try
    {
        Ptr<KInode> inode = GetInode();
        if (inode != nullptr && inode->IsActive()) {
            inode->m_FileOps->CloseDirectory(inode->m_Volume, ptr_tmp_cast(this));
        }
    }
    catch (const std::exception&) {}
    return KFileTableNode::LastReferenceGone();
}

size_t KDirectoryNode::ReadDirectory(void* buffer, size_t bufferSize)
{
    Ptr<KInode> inode = GetInode();
    if (inode == nullptr) {
        PERROR_THROW_CODE(PErrorCode::BADF);
    }
    if (!inode->IsActive()) {
        PERROR_THROW_CODE(PErrorCode(ENODEV));
    }
    return inode->m_FileOps->ReadDirectory(inode->m_Volume, ptr_tmp_cast(this), buffer, bufferSize);
}

void KDirectoryNode::RewindDirectory()
{
    Ptr<KInode> inode = GetInode();
    if (inode == nullptr) {
        PERROR_THROW_CODE(PErrorCode::BADF);
    }
    if (!inode->IsActive()) {
        PERROR_THROW_CODE(PErrorCode(ENODEV));
    }
    inode->m_FileOps->RewindDirectory(inode->m_Volume, ptr_tmp_cast(this));
}

} // namespace kernel
