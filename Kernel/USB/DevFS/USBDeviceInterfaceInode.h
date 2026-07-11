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

class USBDeviceInterfaceInode : public KInode, public KFilesystemFileOps
{
public:
    USBDeviceInterfaceInode(USBHost* host, uint8_t busIndex, uint8_t deviceAddress, uint8_t interfaceNumber);

    virtual void ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;
    virtual void DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    uint8_t                  GetDeviceAddress() const;
    uint8_t                  GetInterfaceNumber() const;
    void                     GetInterfaceInfo(PUSBDeviceInterfaceInfo* outInfo) const;
    size_t                   GetDescriptorSize() const;
    size_t                   ReadDescriptor(size_t offset, void* buffer, size_t bufferSize) const;
    size_t                   GetHostPipeDebugEntryCount(uint8_t endpointAddr) const;
    size_t                   GetHostPipeDebugEntryLabelLength(uint8_t endpointAddr, size_t entryIndex) const;
    size_t                   ReadHostPipeDebugEntryLabel(uint8_t endpointAddr, size_t entryIndex, char* buffer, size_t bufferSize) const;
    size_t                   GetHostPipeDebugEntryValueLength(uint8_t endpointAddr, size_t entryIndex) const;
    size_t                   ReadHostPipeDebugEntryValue(uint8_t endpointAddr, size_t entryIndex, char* buffer, size_t bufferSize) const;
    USBHost&                 GetHost() const;
    const USBDeviceNode&     GetDevice(USBHost& host) const;
    const USB_DescInterface& FindInterfaceDescriptor(const USBDeviceNode& device, const uint8_t** outDescriptorData, size_t* outDescriptorOffset, size_t* outDescriptorSize) const;
    static size_t            GetInterfaceDescriptorBlockSize(const USB_DescriptorHeader* descriptor, const uint8_t* endDescriptor, uint8_t interfaceNumber);
    static size_t            CopyDescriptorBytes(const uint8_t* descriptor, size_t descriptorSize, size_t offset, void* buffer, size_t bufferSize);
    static size_t            CopyStringBytes(const PString& string, char* buffer, size_t bufferSize);
    static void              InitializeEndpointDescriptors(PUSBDeviceInterfaceInfo& info);

    USBHost*       m_Host = nullptr;
    uint8_t        m_BusIndex = 0;
    uint8_t        m_DeviceAddress = 0;
    uint8_t        m_InterfaceNumber = 0;
    PRPCDispatcher m_DeviceControlDispatcher;
};

} // namespace kernel
