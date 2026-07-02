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

#include <stddef.h>
#include <stdint.h>

#include <DeviceControl/USB.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KInode.h>
#include <RPC/RPCDispatcher.h>

class PString;

namespace kernel
{
class USBDeviceNode;
class USBHost;

class USBDeviceControlInode : public KInode, public KFilesystemFileOps
{
public:
    USBDeviceControlInode(USBHost* host, uint8_t busIndex, uint8_t deviceAddress);

    virtual void ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;
    virtual void DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    uint8_t              GetDeviceAddress() const;
    void                 GetDeviceInfo(PUSBDeviceInfo* outInfo) const;
    size_t               GetDeviceDescriptorSize() const;
    size_t               GetConfigurationDescriptorSize() const;
    size_t               ReadDeviceDescriptor(size_t offset, void* buffer, size_t bufferSize) const;
    size_t               ReadConfigurationDescriptor(size_t offset, void* buffer, size_t bufferSize) const;
    size_t               GetStringLength(PUSBDeviceStringID stringID) const;
    size_t               ReadString(PUSBDeviceStringID stringID, char* buffer, size_t bufferSize) const;
    USBHost&             GetHost() const;
    const USBDeviceNode& GetDevice(USBHost& host) const;
    const PString&       GetDeviceString(const USBDeviceNode& device, PUSBDeviceStringID stringID) const;
    static size_t        CopyDescriptorBytes(const uint8_t* descriptor, size_t descriptorSize, size_t offset, void* buffer, size_t bufferSize);

    USBHost*       m_Host = nullptr;
    uint8_t        m_BusIndex = 0;
    uint8_t        m_DeviceAddress = 0;
    PRPCDispatcher m_DeviceControlDispatcher;
};

} // namespace kernel
