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
#include <stddef.h>

#include "System/Ptr/PtrTarget.h"
#include "System/Ptr/Ptr.h"
#include "KINode.h"

namespace kernel
{


class KFileTableNode : public PtrTarget
{
public:
    KFileTableNode(bool isDirectory) : m_IsDirectory(isDirectory) {}

    void        SetINode(Ptr<KINode> inode) { m_INode = inode; }        
    Ptr<KINode> GetINode()          { return m_INode; }
    bool        IsDirectory() const { return m_IsDirectory; }

private:
    Ptr<KINode> m_INode;
    bool        m_IsDirectory;
};

class KFileNode : public KFileTableNode
{
public:
    KFileNode() : KFileTableNode(false) {}
    int         m_FileMode = 0;
    off64_t     m_Position = 0;
};

class KDirectoryNode : public KFileTableNode
{
public:
    KDirectoryNode() : KFileTableNode(true) {}
};

/*
class KDirectoryHandle : public KFileHandle
{
public:
//    KDirectoryHandle(Ptr<KINode> inode);
};*/

} // namespace
