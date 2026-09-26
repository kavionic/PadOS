// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 23.02.2018 01:47:38

#pragma once


#include <sys/types.h>
#include <sys/dirent.h>
#include <stddef.h>
#include <fcntl.h>

#include <Kernel/Kernel.h>
#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include "KInode.h"

namespace kernel
{

class KFileTableNode : public PtrTarget
{
public:
    KFileTableNode(int openFlags) noexcept : m_OpenFlags(openFlags & ~O_CREAT) {}

    inline void         SetInode(Ptr<KInode> inode) noexcept    { m_Inode = inode; }
    inline Ptr<KInode>  GetInode() noexcept                     { return m_Inode; }
    inline bool         IsDirectory() const noexcept            { return m_Inode != nullptr && S_ISDIR(m_Inode->m_FileMode); }
    inline bool         IsSymlink() const noexcept              { return m_Inode != nullptr && S_ISLNK(m_Inode->m_FileMode); }
    inline int          GetOpenFlags() const noexcept           { return m_OpenFlags; }
    inline void         SetOpenFlags(int flags) noexcept        { m_OpenFlags = flags; }
    inline bool         IsPathObject() const noexcept           { return (m_OpenFlags & O_PATH) != 0; }
    inline bool         HasReadAccess() const noexcept          { return (m_OpenFlags & O_ACCMODE) == O_RDWR || (m_OpenFlags & O_ACCMODE) == O_RDONLY; }
    inline bool         HasWriteAccess() const noexcept         { return (m_OpenFlags & O_ACCMODE) == O_RDWR || (m_OpenFlags & O_ACCMODE) == O_WRONLY; }

private:
    Ptr<KInode> m_Inode;
    int         m_OpenFlags = 0;
};

class KFileNode : public KFileTableNode
{
public:
    inline KFileNode(int openFlags) noexcept : KFileTableNode(openFlags) {}
        
    virtual bool LastReferenceGone() override;

    off64_t     m_Position = 0;
};

class KDirectoryNode : public KFileTableNode
{
public:
    inline KDirectoryNode(int openFlags) noexcept : KFileTableNode(openFlags) {}

    virtual bool LastReferenceGone() override;
    
    size_t ReadDirectory(void* buffer, size_t bufferSize);
    void RewindDirectory();
};


} // namespace
