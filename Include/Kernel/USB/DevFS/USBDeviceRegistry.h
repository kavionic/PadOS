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
#include <vector>

#include <Ptr/Ptr.h>
#include <Utils/String.h>

namespace kernel
{
class KInode;
class USBDeviceNode;
class USBHost;

class USBDeviceRegistry
{
public:
    USBDeviceRegistry(USBHost* host, uint8_t busIndex);

    void RegisterDevice(const USBDeviceNode& device);
    void RemoveDevice(uint8_t deviceAddress);
    void Clear();

private:
    struct DeviceEntry
    {
        uint8_t              DeviceAddress = 0;
        PString              DevicePath;
        std::vector<int>     DeviceNodeHandles;
        std::vector<PString> SymlinkPaths;
        std::vector<PString> DirectoryPaths;
    };

    void RegisterControlNode(DeviceEntry& entry);
    void RegisterInterfaceNodes(DeviceEntry& entry, const USBDeviceNode& device);
    void CreateTopologyLink(DeviceEntry& entry, const USBDeviceNode& device);
    void RegisterNode(DeviceEntry& entry, const PString& path, Ptr<KInode> inode);

    void EnsureDirectory(DeviceEntry& entry, const PString& path);
    void CreateDirectory(DeviceEntry& entry, const PString& path);

    PString MakeDevicePath(uint8_t deviceAddress) const;
    PString BuildTopologyPath(const USBDeviceNode& device) const;

    void RemoveEntry(DeviceEntry& entry);

    USBHost*                 m_Host = nullptr;
    uint8_t                  m_BusIndex = 0;
    std::vector<DeviceEntry> m_Devices;
};

} // namespace kernel
