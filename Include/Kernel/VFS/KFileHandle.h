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

#pragma once


#include <sys/types.h>
#include <sys/dirent.h>
#include <stddef.h>
#include <fcntl.h>

#include <Kernel/Kernel.h>
#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include "KINode.h"

namespace kernel
{

class KFileTableNode : public PtrTarget
{
public:
    KFileTableNode(bool isDirectory, int openFlags) : m_IsDirectory(isDirectory), m_OpenFlags(openFlags & ~O_CREAT) {}

    inline void         SetINode(Ptr<KINode> inode) noexcept    { m_INode = inode; }
    inline Ptr<KINode>  GetINode() noexcept                     { return m_INode; }
    inline bool         IsDirectory() const noexcept            { return m_IsDirectory; }
    inline void         SetOpenFlags(int flags) noexcept        { m_OpenFlags = flags; }
    inline int          GetOpenFlags() const noexcept           { return m_OpenFlags; }
    inline bool         HasReadAccess() const noexcept          { return (m_OpenFlags & O_ACCMODE) == O_RDWR || (m_OpenFlags & O_ACCMODE) == O_RDONLY; }
    inline bool         HasWriteAccess() const noexcept         { return (m_OpenFlags & O_ACCMODE) == O_RDWR || (m_OpenFlags & O_ACCMODE) == O_WRONLY; }
private:
    Ptr<KINode> m_INode;
    bool        m_IsDirectory;
    int         m_OpenFlags = 0;
};

class KFileNode : public KFileTableNode
{
public:
    inline KFileNode(int openFlags) : KFileTableNode(false, openFlags) {}
        
    virtual bool LastReferenceGone() override;

    off64_t     m_Position = 0;
};

class KDirectoryNode : public KFileTableNode
{
public:
    inline KDirectoryNode(int openFlags) : KFileTableNode(true, openFlags) {}

    virtual bool LastReferenceGone() override;
    
    size_t ReadDirectory(dirent_t* entry, size_t bufSize);
    void RewindDirectory();
};


} // namespace
