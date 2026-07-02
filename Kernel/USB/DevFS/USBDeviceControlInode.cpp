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
#include <string.h>

#include <DeviceControl/USB.h>
#include <Kernel/KAddressValidation.h>
#include <Kernel/USB/USBHost.h>

#include "USBDeviceControlInode.h"

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDeviceControlInode::USBDeviceControlInode(USBHost* host, uint8_t busIndex, uint8_t deviceAddress)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
    , m_Host(host)
    , m_BusIndex(busIndex)
    , m_DeviceAddress(deviceAddress)
{
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::GetDeviceAddress, this, &USBDeviceControlInode::GetDeviceAddress);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::GetDeviceInfo, this, &USBDeviceControlInode::GetDeviceInfo);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::GetDeviceDescriptorSize, this, &USBDeviceControlInode::GetDeviceDescriptorSize);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::GetConfigurationDescriptorSize, this, &USBDeviceControlInode::GetConfigurationDescriptorSize);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::ReadDeviceDescriptor, this, &USBDeviceControlInode::ReadDeviceDescriptor);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::ReadConfigurationDescriptor, this, &USBDeviceControlInode::ReadConfigurationDescriptor);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::GetStringLength, this, &USBDeviceControlInode::GetStringLength);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceControl::ReadString, this, &USBDeviceControlInode::ReadString);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceControlInode::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceControlInode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    m_DeviceControlDispatcher.Dispatch(request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t USBDeviceControlInode::GetDeviceAddress() const
{
    return m_DeviceAddress;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceControlInode::GetDeviceInfo(PUSBDeviceInfo* outInfo) const
{
    validate_user_write_pointer_trw(outInfo);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    const USBDeviceNode& device = GetDevice(host);
    PUSBDeviceInfo info;

    info.BusIndex = m_BusIndex;
    info.DeviceAddress = device.m_Address;
    info.ParentHubAddress = device.m_ParentHubAddress;
    info.ParentHubPort = device.m_ParentHubPort;
    info.SelectedConfiguration = device.m_SelectedConfiguration;
    info.Speed = device.m_Speed;
    info.SupportsRemoteWakeup = device.m_SupportRemoteWakeup;
    info.SelfPowered = device.m_SelfPowered;
    info.IsConnected = device.m_IsConnected;
    info.IsConfigured = device.m_IsConfigured;
    info.IsHub = device.m_IsHub;
    info.HubPortCount = device.m_HubPortCount;
    info.HubPowerOnDelayMS = device.m_HubPowerOnDelayMS;
    info.DeviceDescriptor = device.m_DeviceDesc;
    info.ConfigurationDescriptorSize = device.m_ConfigurationDescriptor.size();
    info.ManufacturerStringLength = device.m_ManufacturerString.size();
    info.ProductStringLength = device.m_ProductString.size();
    info.SerialNumberStringLength = device.m_SerialNumberString.size();

    if (device.m_ConfigurationDescriptor.size() >= sizeof(USB_DescConfiguration))
    {
        info.HasConfigurationDescriptor = true;
        info.ConfigurationDescriptor = *reinterpret_cast<const USB_DescConfiguration*>(device.m_ConfigurationDescriptor.data());
    }

    *outInfo = info;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceControlInode::GetDeviceDescriptorSize() const
{
    return sizeof(USB_DescDevice);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceControlInode::GetConfigurationDescriptorSize() const
{
    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    return GetDevice(host).m_ConfigurationDescriptor.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceControlInode::ReadDeviceDescriptor(size_t offset, void* buffer, size_t bufferSize) const
{
    validate_user_write_pointer_trw(buffer, bufferSize);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    const USBDeviceNode& device = GetDevice(host);
    return CopyDescriptorBytes(reinterpret_cast<const uint8_t*>(&device.m_DeviceDesc), sizeof(device.m_DeviceDesc), offset, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceControlInode::ReadConfigurationDescriptor(size_t offset, void* buffer, size_t bufferSize) const
{
    validate_user_write_pointer_trw(buffer, bufferSize);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    const USBDeviceNode& device = GetDevice(host);
    return CopyDescriptorBytes(device.m_ConfigurationDescriptor.data(), device.m_ConfigurationDescriptor.size(), offset, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceControlInode::GetStringLength(PUSBDeviceStringID stringID) const
{
    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    return GetDeviceString(GetDevice(host), stringID).size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceControlInode::ReadString(PUSBDeviceStringID stringID, char* buffer, size_t bufferSize) const
{
    validate_user_write_pointer_trw(buffer, bufferSize);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    const PString& string = GetDeviceString(GetDevice(host), stringID);
    size_t copyLength = 0;

    if (bufferSize != 0)
    {
        copyLength = std::min(string.size(), bufferSize - 1);
        memcpy(buffer, string.c_str(), copyLength);
        buffer[copyLength] = 0;
    }
    return copyLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHost& USBDeviceControlInode::GetHost() const
{
    if (m_Host == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }
    return *m_Host;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USBDeviceNode& USBDeviceControlInode::GetDevice(USBHost& host) const
{
    USBDeviceNode* device = host.GetDevice(m_DeviceAddress);
    if (device == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }
    return *device;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& USBDeviceControlInode::GetDeviceString(const USBDeviceNode& device, PUSBDeviceStringID stringID) const
{
    switch (stringID)
    {
        case PUSBDeviceStringID::Manufacturer:
            return device.m_ManufacturerString;
        case PUSBDeviceStringID::Product:
            return device.m_ProductString;
        case PUSBDeviceStringID::SerialNumber:
            return device.m_SerialNumberString;
    }
    PERROR_THROW_CODE(PErrorCode::INVAL);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceControlInode::CopyDescriptorBytes(const uint8_t* descriptor, size_t descriptorSize, size_t offset, void* buffer, size_t bufferSize)
{
    size_t copyLength = 0;

    if (bufferSize != 0 && offset < descriptorSize)
    {
        copyLength = std::min(descriptorSize - offset, bufferSize);
        memcpy(buffer, descriptor + offset, copyLength);
    }
    return copyLength;
}

} // namespace kernel
