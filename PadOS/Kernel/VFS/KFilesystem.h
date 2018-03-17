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
// Created: 23.02.2018 01:53:33

#pragma once


#include <sys/types.h>
#include <errno.h>
#include <stddef.h>

#include "System/Ptr/PtrTarget.h"
#include "System/Ptr/Ptr.h"
#include "Kernel/Kernel.h"

namespace kernel
{

class KFSVolume;
class KFileHandle;
class KINode;

class KFilesystem : public PtrTarget
{
public:
    virtual Ptr<KFSVolume> Mount(Ptr<KINode> mountPoint, const char* devicePath, int devicePathLength);
    virtual Ptr<KINode> LocateInode(Ptr<KINode> parent, const char* path, int pathLength);
    virtual void ReleaseInode(Ptr<KINode> inode);
    virtual Ptr<KFileHandle> OpenFile(Ptr<KINode> node, int flags);
    virtual Ptr<KFileHandle> CreateFile(Ptr<KINode> parent, const char* name, int nameLength, int flags, int permission);
    virtual int CloseFile(Ptr<KFileHandle> file);

    virtual Ptr<KFileHandle> OpenDirectory(Ptr<KINode> node);
    virtual Ptr<KFileHandle> CreateDirectory(Ptr<KINode> parent, const char* name, int nameLength, int permission);

    virtual ssize_t Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length);
    virtual ssize_t Write(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length);
    virtual int     DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

    virtual int ReadAsync(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback);
    virtual int WriteAsync(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback);
    virtual int CancelAsyncRequest(Ptr<KFileHandle> file, int handle);

};

} // namespace
