// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 29.01.2026 23:00

#pragma once

#include <map>

#include <Utils/String.h>
#include <Kernel/KMutex.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/FSDrivers/VirtualFSBase.h>

namespace kernel
{

class KBinFilesystem : public KVirtualFilesystemBase
{
public:
    virtual Ptr<KFSVolume>  Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;

private:
};


} // namespace kernel
