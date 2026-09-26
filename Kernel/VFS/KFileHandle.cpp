// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
