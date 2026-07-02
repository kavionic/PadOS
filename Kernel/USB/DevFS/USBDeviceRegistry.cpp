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

#include <algorithm>

#include <System/Endian.h>
#include <System/ExceptionHandling.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/USB/DevFS/USBDeviceRegistry.h>

#include "USBDeviceControlInode.h"
#include "USBDeviceInterfaceInode.h"

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDeviceRegistry::USBDeviceRegistry(USBHost* host, uint8_t busIndex)
    : m_Host(host)
    , m_BusIndex(busIndex)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::RegisterDevice(const USBDeviceNode& device)
{
    if (device.m_Address == 0) {
        return;
    }

    RemoveDevice(device.m_Address);

    DeviceEntry entry;
    entry.DeviceAddress = device.m_Address;
    entry.DevicePath = MakeDevicePath(device.m_Address);

    try
    {
        RegisterControlNode(entry);
        RegisterInterfaceNodes(entry, device);
        CreateTopologyLink(entry, device);
        m_Devices.push_back(std::move(entry));
    }
    catch (...)
    {
        RemoveEntry(entry);
        throw;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::RemoveDevice(uint8_t deviceAddress)
{
    for (auto iterator = m_Devices.begin(); iterator != m_Devices.end(); ++iterator)
    {
        if (iterator->DeviceAddress == deviceAddress)
        {
            DeviceEntry entry = std::move(*iterator);
            m_Devices.erase(iterator);
            RemoveEntry(entry);
            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::Clear()
{
    while (!m_Devices.empty())
    {
        DeviceEntry entry = std::move(m_Devices.back());
        m_Devices.pop_back();
        RemoveEntry(entry);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::RegisterControlNode(DeviceEntry& entry)
{
    RegisterNode(entry, entry.DevicePath + "/control", ptr_new<USBDeviceControlInode>(m_Host, m_BusIndex, entry.DeviceAddress));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::RegisterInterfaceNodes(DeviceEntry& entry, const USBDeviceNode& device)
{
    if (device.m_ConfigurationDescriptor.size() < sizeof(USB_DescConfiguration)) {
        return;
    }

    const uint8_t* descriptorData = device.m_ConfigurationDescriptor.data();
    const USB_DescConfiguration* configDesc = reinterpret_cast<const USB_DescConfiguration*>(descriptorData);
    const size_t totalLength = std::min<size_t>(PLittleEndianToHost(configDesc->wTotalLength), device.m_ConfigurationDescriptor.size());

    const uint8_t* endDescriptor = descriptorData + totalLength;

    if (!configDesc->ValidateLength(endDescriptor) || configDesc->bLength < sizeof(USB_DescConfiguration)) {
        return;
    }

    std::vector<uint8_t> registeredInterfaces;

    for (const USB_DescriptorHeader* header = configDesc->GetNext(); header->ValidateLength(endDescriptor); header = header->GetNext())
    {
        if (header->bDescriptorType == USB_DescriptorType::INTERFACE && header->bLength >= sizeof(USB_DescInterface))
        {
            const USB_DescInterface* interfaceDesc = reinterpret_cast<const USB_DescInterface*>(header);
            const auto registeredIterator = std::find(registeredInterfaces.begin(), registeredInterfaces.end(), interfaceDesc->bInterfaceNumber);

            if (interfaceDesc->bAlternateSetting == 0 && registeredIterator == registeredInterfaces.end())
            {
                const PString path = entry.DevicePath + PString::format_string("/interface{}", interfaceDesc->bInterfaceNumber);
                RegisterNode(entry, path, ptr_new<USBDeviceInterfaceInode>(m_Host, m_BusIndex, entry.DeviceAddress, interfaceDesc->bInterfaceNumber));
                registeredInterfaces.push_back(interfaceDesc->bInterfaceNumber);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::CreateTopologyLink(DeviceEntry& entry, const USBDeviceNode& device)
{
    const PString topologyPath = BuildTopologyPath(device);
    const PString linkPath = topologyPath + "/device";
    const PString targetPath = "/dev/" + entry.DevicePath;

    EnsureDirectory(entry, topologyPath);

    try
    {
        ksymlink_trw(KLocateFlag::None, targetPath.c_str(), linkPath.c_str());
    }
    PERROR_CATCH([&](PErrorCode error)
    {
        if (error != PErrorCode::EXIST) {
            PERROR_THROW_CODE(error);
        }
        kunlink_trw(KLocateFlag::None, linkPath.c_str());
        ksymlink_trw(KLocateFlag::None, targetPath.c_str(), linkPath.c_str());
    });

    entry.SymlinkPaths.push_back(linkPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::RegisterNode(DeviceEntry& entry, const PString& path, Ptr<KInode> inode)
{
    const int handle = kregister_device_root_trw(path.c_str(), inode);
    entry.DeviceNodeHandles.push_back(handle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::EnsureDirectory(DeviceEntry& entry, const PString& path)
{
    size_t componentEnd = path.find('/', 1);
    while (componentEnd != PString::npos)
    {
        CreateDirectory(entry, PString(path.data(), componentEnd));
        componentEnd = path.find('/', componentEnd + 1);
    }
    CreateDirectory(entry, path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::CreateDirectory(DeviceEntry& entry, const PString& path)
{
    try
    {
        kcreate_directory_trw(KLocateFlag::None, path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
        entry.DirectoryPaths.push_back(path);
    }
    PERROR_CATCH([&](PErrorCode error)
    {
        if (error != PErrorCode::EXIST) {
            PERROR_THROW_CODE(error);
        }
    });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString USBDeviceRegistry::MakeDevicePath(uint8_t deviceAddress) const
{
    return PString::format_string("usb/bus{}/dev{}", m_BusIndex, deviceAddress);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString USBDeviceRegistry::BuildTopologyPath(const USBDeviceNode& device) const
{
    if (device.m_ParentHubAddress == 0)
    {
        return PString::format_string("/dev/usb/topology/bus{}", m_BusIndex);
    }

    USBDeviceNode* parentHub = m_Host->GetDevice(device.m_ParentHubAddress);
    if (parentHub == nullptr) {
        return PString::format_string("/dev/usb/topology/bus{}/unknown/dev{}", m_BusIndex, device.m_Address);
    }
    return BuildTopologyPath(*parentHub) + PString::format_string("/port{}", device.m_ParentHubPort);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceRegistry::RemoveEntry(DeviceEntry& entry)
{
    for (auto iterator = entry.SymlinkPaths.rbegin(); iterator != entry.SymlinkPaths.rend(); ++iterator)
    {
        try
        {
            kunlink_trw(KLocateFlag::None, iterator->c_str());
        }
        PERROR_CATCH([](PErrorCode) {});
    }
    entry.SymlinkPaths.clear();

    for (int handle : entry.DeviceNodeHandles)
    {
        try
        {
            kremove_device_root_trw(handle);
        }
        PERROR_CATCH([](PErrorCode) {});
    }
    entry.DeviceNodeHandles.clear();

    for (auto iterator = entry.DirectoryPaths.rbegin(); iterator != entry.DirectoryPaths.rend(); ++iterator)
    {
        try
        {
            kremove_directory_trw(KLocateFlag::None, iterator->c_str());
        }
        PERROR_CATCH([](PErrorCode) {});
    }
    entry.DirectoryPaths.clear();
}

} // namespace kernel
