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

#include <DeviceControl/DeviceControlInvoker.h>
#include <Kernel/USB/USBProtocol.h>

static constexpr size_t PUSB_MAX_ENDPOINTS_PER_INTERFACE = 16;

static constexpr int PUSBDeviceControlRequest_GetDeviceAddress = 0;
static constexpr int PUSBDeviceControlRequest_GetDeviceInfo = 1;
static constexpr int PUSBDeviceControlRequest_GetDeviceDescriptorSize = 2;
static constexpr int PUSBDeviceControlRequest_GetConfigurationDescriptorSize = 3;
static constexpr int PUSBDeviceControlRequest_ReadDeviceDescriptor = 4;
static constexpr int PUSBDeviceControlRequest_ReadConfigurationDescriptor = 5;
static constexpr int PUSBDeviceControlRequest_GetStringLength = 6;
static constexpr int PUSBDeviceControlRequest_ReadString = 7;

static constexpr int PUSBDeviceInterfaceRequest_GetDeviceAddress = 0;
static constexpr int PUSBDeviceInterfaceRequest_GetInterfaceNumber = 1;
static constexpr int PUSBDeviceInterfaceRequest_GetInterfaceInfo = 2;
static constexpr int PUSBDeviceInterfaceRequest_GetDescriptorSize = 3;
static constexpr int PUSBDeviceInterfaceRequest_ReadDescriptor = 4;
static constexpr int PUSBDeviceInterfaceRequest_GetHostPipeDebugEntryCount = 5;
static constexpr int PUSBDeviceInterfaceRequest_GetHostPipeDebugEntryLabelLength = 6;
static constexpr int PUSBDeviceInterfaceRequest_ReadHostPipeDebugEntryLabel = 7;
static constexpr int PUSBDeviceInterfaceRequest_GetHostPipeDebugEntryValueLength = 8;
static constexpr int PUSBDeviceInterfaceRequest_ReadHostPipeDebugEntryValue = 9;

enum class PUSBDeviceStringID : uint8_t
{
    Manufacturer,
    Product,
    SerialNumber
};

struct PUSBHostPipeInfo
{
    int32_t PipeIndex = -1;
    uint8_t DeviceAddress = 0;
    uint8_t EndpointAddress = 0;
    USB_Speed Speed = USB_Speed::FULL;
    USB_RequestDirection Direction = USB_RequestDirection::HOST_TO_DEVICE;
    USB_TransferType EndpointType = USB_TransferType::CONTROL;
    USB_URBState URBState = USB_URBState::Idle;
    USB_URBState PendingIRQURBState = USB_URBState::Idle;
    size_t MaxPacketSize = 0;
    size_t PendingIRQTransferLength = 0;
    bool IsValid = false;
    bool HasTransactionCallback = false;
    bool HasPendingIRQURBState = false;
};

struct PUSBDeviceInfo
{
    uint8_t BusIndex = 0;
    uint8_t DeviceAddress = 0;
    uint8_t ParentHubAddress = 0;
    uint8_t ParentHubPort = 0;
    uint8_t SelectedConfiguration = 0;
    USB_Speed Speed = USB_Speed::FULL;

    bool SupportsRemoteWakeup = false;
    bool SelfPowered = false;
    bool IsConnected = false;
    bool IsConfigured = false;
    bool IsHub = false;

    uint8_t HubPortCount = 0;
    uint16_t HubPowerOnDelayMS = 0;

    USB_DescDevice DeviceDescriptor;
    bool HasConfigurationDescriptor = false;
    size_t ConfigurationDescriptorSize = 0;
    USB_DescConfiguration ConfigurationDescriptor = USB_DescConfiguration(0, 0, 0, 0, 0, 0);

    size_t ManufacturerStringLength = 0;
    size_t ProductStringLength = 0;
    size_t SerialNumberStringLength = 0;
};

struct PUSBDeviceInterfaceInfo
{
    uint8_t BusIndex = 0;
    uint8_t DeviceAddress = 0;
    uint8_t InterfaceNumber = 0;
    uint8_t AlternateSettingCount = 0;
    uint8_t EndpointCount = 0;

    size_t DescriptorOffset = 0;
    size_t DescriptorSize = 0;

    USB_DescInterface InterfaceDescriptor = USB_DescInterface(0, 0, 0, USB_ClassCode::UNSPECIFIED, 0, 0, 0);
    USB_DescEndpoint Endpoints[PUSB_MAX_ENDPOINTS_PER_INTERFACE];
    PUSBHostPipeInfo HostPipes[PUSB_MAX_ENDPOINTS_PER_INTERFACE];
};

class PUSBDeviceControl : public PDeviceControlInterface
{
public:
    PUSBDeviceControl()
        : GetDeviceAddress(*this)
        , GetDeviceInfo(*this)
        , GetDeviceDescriptorSize(*this)
        , GetConfigurationDescriptorSize(*this)
        , ReadDeviceDescriptor(*this)
        , ReadConfigurationDescriptor(*this)
        , GetStringLength(*this)
        , ReadString(*this)
    {
    }
    explicit PUSBDeviceControl(int fileHandle) : PUSBDeviceControl() { SetDeviceFD(fileHandle); }

    PDeviceControlInvoker<PUSBDeviceControlRequest_GetDeviceAddress, uint8_t() const> GetDeviceAddress;
    PDeviceControlInvoker<PUSBDeviceControlRequest_GetDeviceInfo, void(PUSBDeviceInfo* outInfo) const> GetDeviceInfo;
    PDeviceControlInvoker<PUSBDeviceControlRequest_GetDeviceDescriptorSize, size_t() const> GetDeviceDescriptorSize;
    PDeviceControlInvoker<PUSBDeviceControlRequest_GetConfigurationDescriptorSize, size_t() const> GetConfigurationDescriptorSize;
    PDeviceControlInvoker<PUSBDeviceControlRequest_ReadDeviceDescriptor, size_t(size_t offset, void* buffer, size_t bufferSize) const> ReadDeviceDescriptor;
    PDeviceControlInvoker<PUSBDeviceControlRequest_ReadConfigurationDescriptor, size_t(size_t offset, void* buffer, size_t bufferSize) const> ReadConfigurationDescriptor;
    PDeviceControlInvoker<PUSBDeviceControlRequest_GetStringLength, size_t(PUSBDeviceStringID stringID) const> GetStringLength;
    PDeviceControlInvoker<PUSBDeviceControlRequest_ReadString, size_t(PUSBDeviceStringID stringID, char* buffer, size_t bufferSize) const> ReadString;
};


class PUSBDeviceInterface : public PDeviceControlInterface
{
public:
    PUSBDeviceInterface()
        : GetDeviceAddress(*this)
        , GetInterfaceNumber(*this)
        , GetInterfaceInfo(*this)
        , GetDescriptorSize(*this)
        , ReadDescriptor(*this)
        , GetHostPipeDebugEntryCount(*this)
        , GetHostPipeDebugEntryLabelLength(*this)
        , ReadHostPipeDebugEntryLabel(*this)
        , GetHostPipeDebugEntryValueLength(*this)
        , ReadHostPipeDebugEntryValue(*this)
    {
    }
    explicit PUSBDeviceInterface(int fileHandle) : PUSBDeviceInterface() { SetDeviceFD(fileHandle); }

    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_GetDeviceAddress, uint8_t() const> GetDeviceAddress;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_GetInterfaceNumber, uint8_t() const> GetInterfaceNumber;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_GetInterfaceInfo, void(PUSBDeviceInterfaceInfo* outInfo) const> GetInterfaceInfo;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_GetDescriptorSize, size_t() const> GetDescriptorSize;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_ReadDescriptor, size_t(size_t offset, void* buffer, size_t bufferSize) const> ReadDescriptor;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_GetHostPipeDebugEntryCount, size_t(uint8_t endpointAddr) const> GetHostPipeDebugEntryCount;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_GetHostPipeDebugEntryLabelLength, size_t(uint8_t endpointAddr, size_t entryIndex) const> GetHostPipeDebugEntryLabelLength;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_ReadHostPipeDebugEntryLabel, size_t(uint8_t endpointAddr, size_t entryIndex, char* buffer, size_t bufferSize) const> ReadHostPipeDebugEntryLabel;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_GetHostPipeDebugEntryValueLength, size_t(uint8_t endpointAddr, size_t entryIndex) const> GetHostPipeDebugEntryValueLength;
    PDeviceControlInvoker<PUSBDeviceInterfaceRequest_ReadHostPipeDebugEntryValue, size_t(uint8_t endpointAddr, size_t entryIndex, char* buffer, size_t bufferSize) const> ReadHostPipeDebugEntryValue;
};
