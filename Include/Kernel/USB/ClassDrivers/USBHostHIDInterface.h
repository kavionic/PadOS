// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <stdint.h>
#include <vector>

#include <Ptr/Ptr.h>
#include <Ptr/PtrTarget.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBProtocolHID.h>
#include <Kernel/USB/ClassDrivers/USBHIDDriver.h>
#include <Kernel/USB/ClassDrivers/USBHIDReportParser.h>

namespace kernel
{

class USBHost;
class USBHIDDriver;
class USBHostClassHID;

class USBHostHIDInterface : public PtrTarget
{
public:
    USBHostHIDInterface(USBHost* hostHandler, USBHostClassHID* classDriver);

    const USB_DescriptorHeader* Open(uint8_t deviceAddress, uint32_t interfaceIndex, const USB_DescInterface* interfaceDescriptor, const void* endDescriptor);
    void Close();
    void Startup();

    USBHIDInterfaceInfo GetInterfaceInfo() const;
    const USBHIDReportDescriptor& GetReportDescriptor() const { return m_ReportDescriptor; }
    uint8_t             GetDeviceAddress() const { return m_DeviceAddress; }
    bool                IsActive() const { return m_IsActive; }

private:
    const char* GetProtocolName() const;
    bool        IsBootKeyboard() const;
    bool        IsBootMouse() const;
    bool        ShouldUseBootProtocol() const;

    void ReqSetIdle();
    void ReqSetBootProtocol();
    void ReqGetReportDescriptor();
    void StartInputDriverAndReceive();
    void StartReceive();

    void HandleSetIdleResult(bool result, uint8_t deviceAddress);
    void HandleSetProtocolResult(bool result, uint8_t deviceAddress);
    void HandleGetReportDescriptorResult(bool result, uint8_t deviceAddress);
    void ReceiveTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);

    void LogReport(size_t length);
    void LogKeyboardReport(size_t length);
    void LogKeyboardKey(bool pressed, uint8_t usageCode, uint8_t modifiers);
    void LogMouseReport(size_t length);
    void LogRawReport(size_t length);

    static bool KeyboardReportContains(const uint8_t* report, uint8_t usageCode);

    USBHost*                m_HostHandler = nullptr;
    USBHostClassHID*        m_ClassDriver = nullptr;
    uint32_t                m_InterfaceIndex = 0;
    uint8_t                 m_DeviceAddress = 0;
    uint8_t                 m_InterfaceNumber = 0;
    USB_HID_SubclassCode    m_Subclass = USB_HID_SubclassCode::NONE;
    USB_HID_ProtocolCode    m_Protocol = USB_HID_ProtocolCode::NONE;
    uint16_t                m_ReportDescriptorLength = 0;
    volatile bool           m_IsActive = false;

    USB_PipeIndex           m_ReportPipeIn = USB_INVALID_PIPE;
    uint8_t                 m_ReportEndpointIn = USB_INVALID_ENDPOINT;
    size_t                  m_ReportEndpointInSize = 0;

    std::vector<uint8_t>    m_ReportBuffer;
    std::vector<uint8_t>    m_ReportDescriptorBuffer;
    USBHIDReportDescriptor  m_ReportDescriptor;
    std::array<uint8_t, 8>  m_PreviousKeyboardReport = {};
    uint8_t                 m_PreviousMouseButtons = 0;
    Ptr<USBHIDDriver>       m_InputDriver;
};

} // namespace kernel
