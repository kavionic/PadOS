// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 29.01.2026 23:00

#pragma once

#include <map>

#include <Utils/String.h>
#include <Kernel/KMutex.h>
#include <Kernel/VFS/KINode.h>
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
