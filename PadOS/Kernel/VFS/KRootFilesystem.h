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
// Created: 23.02.2018 01:49:14

#pragma once

#include <map>
#include "KINode.h"
#include "KFilesystem.h"
#include "System/String.h"

namespace kernel
{


class KDeviceNode;

class KRootFSINode : public KINode
{
    public:
    KRootFSINode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume) : KINode(filesystem, volume) {}

    Ptr<KDeviceNode> m_DeviceNode;

    std::map<os::String, Ptr<KRootFSINode>> m_Children;
};

class KRootFilesystem : public KFilesystem
{
public:
    virtual Ptr<KFSVolume>   Mount(Ptr<KINode> mountPoint, const char* devicePath, int devicePathLength) override;
    virtual Ptr<KINode>      LocateInode(Ptr<KINode> parent, const char* path, int pathLength) override;
    virtual Ptr<KFileHandle> OpenFile(Ptr<KINode> node, int flags) override;
    virtual Ptr<KFileHandle> CreateFile(Ptr<KINode> parent, const char* name, int nameLength, int flags, int permission) override;

    virtual ssize_t Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length) override;
    virtual int     DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

    virtual int ReadAsync(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) override;
    virtual int WriteAsync(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) override;
    virtual int CancelAsyncRequest(Ptr<KFileHandle> file, int handle) override;

    int RegisterDevice(const char* path, Ptr<KDeviceNode> device);

private:
    Ptr<KRootFSINode> LocateParentInode(Ptr<KRootFSINode> parent, const char* path, int pathLength, bool createParents, int* nameStart);

    Ptr<KFSVolume>    m_Volume;
    Ptr<KRootFSINode> m_DevRoot;
};

} // namespace