// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.02.2018 01:49:14

#pragma once

#include <map>
#include "KINode.h"
#include "KFilesystem.h"
#include "Utils/String.h"
#include "Kernel/KMutex.h"
#include "KFileHandle.h"

namespace kernel
{


class KDeviceNode;

class KRootFSINode : public KINode
{
public:
    IFLASHC KRootFSINode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KRootFSINode* parent, KFilesystemFileOps* fileOps, bool isDirectory);

    Ptr<KDeviceNode> m_DeviceNode;

    KRootFSINode* m_Parent = nullptr;

    std::map<os::String, Ptr<KINode>> m_Children;
    TimeValNanos                      m_CreateTime;
};

struct KRootFSDirectoryNode : public KDirectoryNode
{
    inline KRootFSDirectoryNode(int openFlags) : KDirectoryNode(openFlags) {}

    size_t    m_CurrentIndex = 0;
};

class KRootFilesystem : public KFilesystem, public KFilesystemFileOps
{
public:
    inline KRootFilesystem() : m_Mutex("root_fs_mutex", PEMutexRecursionMode_RaiseError) {}
    IFLASHC virtual Ptr<KFSVolume>      Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;
    IFLASHC virtual Ptr<KINode>         LocateInode(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength) override;
//    virtual Ptr<KFileHandle> OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int flags) override;

    IFLASHC virtual Ptr<KDirectoryNode> OpenDirectory(Ptr<KFSVolume> volume, Ptr<KINode> node) override;
    IFLASHC virtual void                CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory) override;

    IFLASHC virtual size_t              ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize) override;
    IFLASHC virtual void                RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> dirNode) override;

    IFLASHC virtual Ptr<KFileNode>      CreateFile(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int flags, int permission) override;

    IFLASHC virtual Ptr<KINode>         LoadInode(Ptr<KFSVolume> volume, ino_t inode) override;
    IFLASHC virtual void                CreateDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int permission) override;

    IFLASHC virtual void                ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> inode, struct stat* outStats) override;
    IFLASHC virtual void                WriteStat(Ptr<KFSVolume> volume, Ptr<KINode> inode, const struct stat* stats, uint32_t mask) override;

//    virtual ssize_t Read(Ptr<KFileHandle> file, off64_t position, void* buffer, size_t length) override;
//    virtual ssize_t Write(Ptr<KFileHandle> file, off64_t position, const void* buffer, size_t length) override;
//    virtual int     DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

//    virtual int ReadAsync(Ptr<KFileHandle> file, off64_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) override;
//    virtual int WriteAsync(Ptr<KFileHandle> file, off64_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) override;
//    virtual int CancelAsyncRequest(Ptr<KFileHandle> file, int handle) override;

    IFLASHC int  RegisterDevice(const char* path, Ptr<KINode> deviceNode);
    IFLASHC void RenameDevice(int handle, const char* newPath);
    IFLASHC void RemoveDevice(int handle);

    static IFLASHC int AllocINodeNumber();
private:
    IFLASHC Ptr<KINode>       FindINode(Ptr<KRootFSINode> parent, ino_t inodeNum, bool remove, Ptr<KRootFSINode>* parentNode);
    IFLASHC Ptr<KRootFSINode> LocateParentInode(Ptr<KRootFSINode> parent, const char* path, int pathLength, bool createParents, int* nameStart);

    KMutex            m_Mutex;
    Ptr<KFSVolume>    m_Volume;
    Ptr<KRootFSINode> m_DevRoot;
};

} // namespace