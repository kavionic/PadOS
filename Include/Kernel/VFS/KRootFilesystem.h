// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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

#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Utils/String.h>
#include <Kernel/KMutex.h>
#include <Kernel/FSDrivers/VirtualFSBase.h>
#include <Kernel/VFS/KFileHandle.h>

namespace kernel
{

class KRootFilesystem : public KVirtualFilesystemBase
{
public:
    virtual Ptr<KFSVolume>      Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;

    int  RegisterDevice(const char* path, Ptr<KInode> deviceNode);
    void RenameDevice(int handle, const char* newPath);
    void RemoveDevice(int handle);

private:
    Ptr<KVirtualFSBaseInode> m_DevRoot;
};

} // namespace kernel
