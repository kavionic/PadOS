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
#include <System/Endian.h>
#include <Kernel/KAddressValidation.h>
#include <Kernel/USB/USBHost.h>
#include <Utils/String.h>

#include "USBDeviceInterfaceInode.h"

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBDeviceInterfaceInode::USBDeviceInterfaceInode(USBHost* host, uint8_t busIndex, uint8_t deviceAddress, uint8_t interfaceNumber)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
    , m_Host(host)
    , m_BusIndex(busIndex)
    , m_DeviceAddress(deviceAddress)
    , m_InterfaceNumber(interfaceNumber)
{
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::GetDeviceAddress, this, &USBDeviceInterfaceInode::GetDeviceAddress);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::GetInterfaceNumber, this, &USBDeviceInterfaceInode::GetInterfaceNumber);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::GetInterfaceInfo, this, &USBDeviceInterfaceInode::GetInterfaceInfo);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::GetDescriptorSize, this, &USBDeviceInterfaceInode::GetDescriptorSize);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::ReadDescriptor, this, &USBDeviceInterfaceInode::ReadDescriptor);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::GetHostPipeDebugEntryCount, this, &USBDeviceInterfaceInode::GetHostPipeDebugEntryCount);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::GetHostPipeDebugEntryLabelLength, this, &USBDeviceInterfaceInode::GetHostPipeDebugEntryLabelLength);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::ReadHostPipeDebugEntryLabel, this, &USBDeviceInterfaceInode::ReadHostPipeDebugEntryLabel);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::GetHostPipeDebugEntryValueLength, this, &USBDeviceInterfaceInode::GetHostPipeDebugEntryValueLength);
    m_DeviceControlDispatcher.AddHandler(&PUSBDeviceInterface::ReadHostPipeDebugEntryValue, this, &USBDeviceInterfaceInode::ReadHostPipeDebugEntryValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceInterfaceInode::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceInterfaceInode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    m_DeviceControlDispatcher.Dispatch(request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t USBDeviceInterfaceInode::GetDeviceAddress() const
{
    return m_DeviceAddress;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t USBDeviceInterfaceInode::GetInterfaceNumber() const
{
    return m_InterfaceNumber;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceInterfaceInode::GetInterfaceInfo(PUSBDeviceInterfaceInfo* outInfo) const
{
    validate_user_write_pointer_trw(outInfo);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    const USBDeviceNode& device = GetDevice(host);
    const uint8_t* descriptorData = nullptr;
    size_t descriptorOffset = 0;
    size_t descriptorSize = 0;
    const USB_DescInterface& interfaceDesc = FindInterfaceDescriptor(device, &descriptorData, &descriptorOffset, &descriptorSize);

    PUSBDeviceInterfaceInfo info;
    InitializeEndpointDescriptors(info);

    info.BusIndex = m_BusIndex;
    info.DeviceAddress = m_DeviceAddress;
    info.InterfaceNumber = m_InterfaceNumber;
    info.DescriptorOffset = descriptorOffset;
    info.DescriptorSize = descriptorSize;
    info.InterfaceDescriptor = interfaceDesc;

    const uint8_t* endDescriptor = descriptorData + descriptorSize;
    bool includeEndpoints = true;

    for (const USB_DescriptorHeader* header = reinterpret_cast<const USB_DescriptorHeader*>(descriptorData); header->ValidateLength(endDescriptor); header = header->GetNext())
    {
        if (header->bDescriptorType == USB_DescriptorType::INTERFACE && header->bLength >= sizeof(USB_DescInterface))
        {
            const USB_DescInterface* currentInterfaceDesc = reinterpret_cast<const USB_DescInterface*>(header);
            if (currentInterfaceDesc->bInterfaceNumber == m_InterfaceNumber)
            {
                ++info.AlternateSettingCount;
                includeEndpoints = currentInterfaceDesc->bAlternateSetting == interfaceDesc.bAlternateSetting;
            }
        }
        else if (header->bDescriptorType == USB_DescriptorType::ENDPOINT && header->bLength >= sizeof(USB_DescEndpoint) && includeEndpoints)
        {
            if (info.EndpointCount < PUSB_MAX_ENDPOINTS_PER_INTERFACE) {
                info.Endpoints[info.EndpointCount++] = *reinterpret_cast<const USB_DescEndpoint*>(header);
            }
        }
    }

    for (size_t endpointIndex = 0; endpointIndex < info.EndpointCount; ++endpointIndex)
    {
        host.GetPipeInfo(m_DeviceAddress, info.Endpoints[endpointIndex].bEndpointAddress, &info.HostPipes[endpointIndex]);
    }
    *outInfo = info;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::GetDescriptorSize() const
{
    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    const USBDeviceNode& device = GetDevice(host);
    const uint8_t* descriptorData = nullptr;
    size_t descriptorOffset = 0;
    size_t descriptorSize = 0;
    FindInterfaceDescriptor(device, &descriptorData, &descriptorOffset, &descriptorSize);
    return descriptorSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::ReadDescriptor(size_t offset, void* buffer, size_t bufferSize) const
{
    validate_user_write_pointer_trw(buffer, bufferSize);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    const USBDeviceNode& device = GetDevice(host);
    const uint8_t* descriptorData = nullptr;
    size_t descriptorOffset = 0;
    size_t descriptorSize = 0;
    FindInterfaceDescriptor(device, &descriptorData, &descriptorOffset, &descriptorSize);
    return CopyDescriptorBytes(descriptorData, descriptorSize, offset, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::GetHostPipeDebugEntryCount(uint8_t endpointAddr) const
{
    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    return host.GetPipeDebugEntryCount(m_DeviceAddress, endpointAddr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::GetHostPipeDebugEntryLabelLength(uint8_t endpointAddr, size_t entryIndex) const
{
    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    PString label;
    if (!host.GetPipeDebugEntryLabel(m_DeviceAddress, endpointAddr, entryIndex, &label)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return label.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::ReadHostPipeDebugEntryLabel(uint8_t endpointAddr, size_t entryIndex, char* buffer, size_t bufferSize) const
{
    validate_user_write_pointer_trw(buffer, bufferSize);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    PString label;
    if (!host.GetPipeDebugEntryLabel(m_DeviceAddress, endpointAddr, entryIndex, &label)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return CopyStringBytes(label, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::GetHostPipeDebugEntryValueLength(uint8_t endpointAddr, size_t entryIndex) const
{
    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    PString value;
    if (!host.GetPipeDebugEntryValue(m_DeviceAddress, endpointAddr, entryIndex, &value)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return value.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::ReadHostPipeDebugEntryValue(uint8_t endpointAddr, size_t entryIndex, char* buffer, size_t bufferSize) const
{
    validate_user_write_pointer_trw(buffer, bufferSize);

    USBHost& host = GetHost();
    CRITICAL_SCOPE(host.GetMutex());

    PString value;
    if (!host.GetPipeDebugEntryValue(m_DeviceAddress, endpointAddr, entryIndex, &value)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return CopyStringBytes(value, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHost& USBDeviceInterfaceInode::GetHost() const
{
    if (m_Host == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }
    return *m_Host;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USBDeviceNode& USBDeviceInterfaceInode::GetDevice(USBHost& host) const
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

const USB_DescInterface& USBDeviceInterfaceInode::FindInterfaceDescriptor(const USBDeviceNode& device, const uint8_t** outDescriptorData, size_t* outDescriptorOffset, size_t* outDescriptorSize) const
{
    if (device.m_ConfigurationDescriptor.size() < sizeof(USB_DescConfiguration)) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }

    const uint8_t* descriptorData = device.m_ConfigurationDescriptor.data();
    const USB_DescConfiguration* configDesc = reinterpret_cast<const USB_DescConfiguration*>(descriptorData);
    const size_t totalLength = std::min<size_t>(PLittleEndianToHost(configDesc->wTotalLength), device.m_ConfigurationDescriptor.size());

    if (configDesc->bLength < sizeof(USB_DescConfiguration) || configDesc->bLength > totalLength) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    const uint8_t* endDescriptor = descriptorData + totalLength;

    for (const USB_DescriptorHeader* descriptor = configDesc->GetNext(); descriptor->ValidateLength(endDescriptor); descriptor = descriptor->GetNext())
    {
        if (descriptor->bDescriptorType == USB_DescriptorType::INTERFACE && descriptor->bLength >= sizeof(USB_DescInterface))
        {
            const USB_DescInterface* interfaceDesc = reinterpret_cast<const USB_DescInterface*>(descriptor);
            if (interfaceDesc->bInterfaceNumber == m_InterfaceNumber && interfaceDesc->bAlternateSetting == 0)
            {
                const uint8_t* descriptorBytes = reinterpret_cast<const uint8_t*>(descriptor);
                *outDescriptorData = descriptorBytes;
                *outDescriptorOffset = size_t(descriptorBytes - descriptorData);
                *outDescriptorSize = GetInterfaceDescriptorBlockSize(descriptor, endDescriptor, m_InterfaceNumber);
                return *interfaceDesc;
            }
        }
    }
    PERROR_THROW_CODE(PErrorCode::NOENT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::GetInterfaceDescriptorBlockSize(const USB_DescriptorHeader* descriptor, const uint8_t* endDescriptor, uint8_t interfaceNumber)
{
    const USB_DescriptorHeader* blockEnd = descriptor;

    for (; blockEnd->ValidateLength(endDescriptor); blockEnd = blockEnd->GetNext())
    {
        if (blockEnd != descriptor)
        {
            if (blockEnd->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION) {
                break;
            }
            if (blockEnd->bDescriptorType == USB_DescriptorType::INTERFACE && blockEnd->bLength >= sizeof(USB_DescInterface))
            {
                const USB_DescInterface* interfaceDesc = reinterpret_cast<const USB_DescInterface*>(blockEnd);
                if (interfaceDesc->bInterfaceNumber != interfaceNumber) {
                    break;
                }
            }
        }
    }
    return size_t(reinterpret_cast<const uint8_t*>(blockEnd) - reinterpret_cast<const uint8_t*>(descriptor));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::CopyDescriptorBytes(const uint8_t* descriptor, size_t descriptorSize, size_t offset, void* buffer, size_t bufferSize)
{
    size_t copyLength = 0;

    if (bufferSize != 0 && offset < descriptorSize)
    {
        copyLength = std::min(descriptorSize - offset, bufferSize);
        memcpy(buffer, descriptor + offset, copyLength);
    }
    return copyLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBDeviceInterfaceInode::CopyStringBytes(const PString& string, char* buffer, size_t bufferSize)
{
    const size_t copyLength = std::min(string.size(), bufferSize);

    if (copyLength != 0) {
        memcpy(buffer, string.data(), copyLength);
    }
    return copyLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBDeviceInterfaceInode::InitializeEndpointDescriptors(PUSBDeviceInterfaceInfo& info)
{
    for (size_t endpointIndex = 0; endpointIndex < PUSB_MAX_ENDPOINTS_PER_INTERFACE; ++endpointIndex)
    {
        info.Endpoints[endpointIndex] = USB_DescEndpoint(0, USB_TransferType::CONTROL, USB_IsoEndpointSyncType::NONE, USB_EndpointUsageType::DATA, 0, 0);
        info.HostPipes[endpointIndex] = PUSBHostPipeInfo();
    }
}

} // namespace kernel
