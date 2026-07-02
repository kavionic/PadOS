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

#pragma once

#include <stdint.h>

#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KInode.h>

namespace kernel
{
class USBHost;

class USBDeviceControlInode : public KInode, public KFilesystemFileOps
{
public:
    USBDeviceControlInode(USBHost* host, uint8_t deviceAddress);

    virtual void ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

private:
    USBHost* m_Host = nullptr;
    uint8_t  m_DeviceAddress = 0;
};

} // namespace kernel
