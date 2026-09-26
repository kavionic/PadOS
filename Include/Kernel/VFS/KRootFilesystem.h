// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    Ptr<KVirtualFSVolume>       m_Volume;
    Ptr<KVirtualFSBaseInode>    m_DevRoot;
};

} // namespace kernel
