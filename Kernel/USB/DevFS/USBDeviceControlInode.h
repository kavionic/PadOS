// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
