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
// Created: 23.02.2018 01:44:32

#pragma once

#include <sys/types.h>

#include <System/Sections.h>
#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include <Utils/IntrusiveList.h>
#include <Kernel/KWaitableObject.h>

namespace os
{
class String;
}

namespace kernel
{

class KFilesystem;
class KFilesystemFileOps;
class KFSVolume;
class KINode;

class KINode : public PtrTarget, public KWaitableObject, public IntrusiveListNode<KINode>
{
public:
    IFLASHC KINode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KFilesystemFileOps* fileOps, bool isDirectory);
    IFLASHC virtual ~KINode();
    
    IFLASHC virtual bool LastReferenceGone() override;
    
    inline void SetDeletedFlag(bool isDeleted) { m_IsDeleted = isDeleted; }
    inline bool IsDeleted() { return m_IsDeleted; }
    inline bool IsDirectory() const { return m_IsDirectory; }
    
    IFLASHC int     GetDirectoryPath(os::String* path);
    
    Ptr<KFilesystem>    m_Filesystem;
    Ptr<KFSVolume>      m_Volume; // The volume this i-node came from.
    KFilesystemFileOps* m_FileOps;
    Ptr<KINode>         m_MountRoot; // Root node of filesystem mounted on this inode if any.
    ino_t               m_INodeID = 0;
    time_t              m_LastUseTime = 0; // If the reference count is 0, this record the time (in seconds) when it reach 0.
    static_assert(sizeof(ino_t) == 8);

    bool m_IsDirectory;
    bool m_IsDeleted = false;
};

} // namespace
