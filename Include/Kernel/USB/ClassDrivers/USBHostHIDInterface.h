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

#include <array>
#include <stdint.h>
#include <vector>

#include <Ptr/PtrTarget.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBProtocolHID.h>

namespace kernel
{

class USBHost;
enum class USB_URBState : uint8_t;

class USBHostHIDInterface : public PtrTarget
{
public:
    USBHostHIDInterface(USBHost* hostHandler);

    const USB_DescriptorHeader* Open(uint8_t deviceAddress, uint32_t interfaceIndex, const USB_DescInterface* interfaceDescriptor, const void* endDescriptor);
    void Close();
    void Startup();

    uint8_t GetDeviceAddress() const { return m_DeviceAddress; }
    bool    IsActive() const { return m_IsActive; }

private:
    const char* GetProtocolName() const;
    bool        IsBootKeyboard() const;
    bool        IsBootMouse() const;
    bool        UsesBootProtocol() const;

    void ReqSetIdle();
    void ReqSetBootProtocol();
    void StartReceive();

    void HandleSetIdleResult(bool result, uint8_t deviceAddress);
    void HandleSetProtocolResult(bool result, uint8_t deviceAddress);
    void ReceiveTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);

    void LogReport(size_t length);
    void LogKeyboardReport(size_t length);
    void LogKeyboardKey(bool pressed, uint8_t usageCode, uint8_t modifiers);
    void LogMouseReport(size_t length);
    void LogRawReport(size_t length);

    static bool KeyboardReportContains(const uint8_t* report, uint8_t usageCode);

    USBHost*                m_HostHandler = nullptr;
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
    std::array<uint8_t, 8>  m_PreviousKeyboardReport = {};
    uint8_t                 m_PreviousMouseButtons = 0;
};

} // namespace kernel
