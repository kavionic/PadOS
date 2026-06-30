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

#include <utility>

#include <System/Endian.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>
#include <Kernel/KLogging.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/ClassDrivers/USBHostHIDInterface.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostHIDInterface::USBHostHIDInterface(USBHost* hostHandler)
    : m_HostHandler(hostHandler)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescriptorHeader* USBHostHIDInterface::Open(uint8_t deviceAddress, uint32_t interfaceIndex, const USB_DescInterface* interfaceDescriptor, const void* endDescriptor)
{
    if (interfaceDescriptor->bInterfaceClass != USB_ClassCode::HID) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    USBDeviceNode* device = m_HostHandler->GetDevice(deviceAddress);
    if (device == nullptr) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    m_DeviceAddress = deviceAddress;
    m_InterfaceIndex = interfaceIndex;
    m_InterfaceNumber = interfaceDescriptor->bInterfaceNumber;
    m_Subclass = USB_HID_SubclassCode(interfaceDescriptor->bInterfaceSubClass);
    m_Protocol = USB_HID_ProtocolCode(interfaceDescriptor->bInterfaceProtocol);
    m_ReportDescriptorLength = 0;
    m_ReportPipeIn = USB_INVALID_PIPE;
    m_ReportEndpointIn = USB_INVALID_ENDPOINT;
    m_ReportEndpointInSize = 0;
    m_PreviousKeyboardReport = {};
    m_PreviousMouseButtons = 0;

    const USB_DescriptorHeader* descriptor = interfaceDescriptor->GetNext();

    for (; descriptor < endDescriptor; descriptor = descriptor->GetNext())
    {
        if (descriptor->bDescriptorType == USB_DescriptorType::INTERFACE || descriptor->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION) {
            break;
        }
        if (std::to_underlying(descriptor->bDescriptorType) == std::to_underlying(USB_HID_DescriptorType::HID))
        {
            if (descriptor->bLength >= sizeof(USB_HID_DescHID) + sizeof(USB_HID_DescriptorInfo))
            {
                const USB_HID_DescHID* hidDescriptor = static_cast<const USB_HID_DescHID*>(descriptor);
                if (hidDescriptor->DescriptorCount != 0) {
                    m_ReportDescriptorLength = PLittleEndianToHost(hidDescriptor->Descriptors[0].DescriptorLength);
                }
            }
            continue;
        }
        if (descriptor->bDescriptorType != USB_DescriptorType::ENDPOINT) {
            continue;
        }

        const USB_DescEndpoint* endpointDescriptor = static_cast<const USB_DescEndpoint*>(descriptor);
        if (endpointDescriptor->GetTransferType() != USB_TransferType::INTERRUPT) {
            continue;
        }
        if ((endpointDescriptor->bEndpointAddress & USB_ADDRESS_DIR_IN) == 0) {
            continue;
        }
        if (!endpointDescriptor->Validate(device->m_Speed))
        {
            kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID interface {} has invalid interrupt endpoint {:02x}.", int(m_InterfaceNumber), int(endpointDescriptor->bEndpointAddress));
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        m_ReportEndpointIn = endpointDescriptor->bEndpointAddress;
        m_ReportEndpointInSize = endpointDescriptor->GetMaxPacketSize();
        break;
    }

    if (m_ReportEndpointIn == USB_INVALID_ENDPOINT || m_ReportEndpointInSize == 0)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID interface {} has no interrupt IN endpoint.", int(m_InterfaceNumber));
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    m_ReportBuffer.resize(m_ReportEndpointInSize);

    m_ReportPipeIn = m_HostHandler->AllocPipe(m_ReportEndpointIn);
    if (m_ReportPipeIn == USB_INVALID_PIPE)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID interface {} failed to allocate pipe.", int(m_InterfaceNumber));
        PERROR_THROW_CODE(PErrorCode::NOMEM);
    }

    if (!m_HostHandler->OpenPipe(m_ReportPipeIn, m_ReportEndpointIn, device->m_Address, device->m_Speed, USB_TransferType::INTERRUPT, m_ReportEndpointInSize))
    {
        m_HostHandler->FreePipe(m_ReportPipeIn);
        m_ReportPipeIn = USB_INVALID_PIPE;
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    m_HostHandler->SetDataToggle(m_ReportPipeIn, false);

    kernel_log<PLogSeverity::INFO_LOW_VOL>(
        LogCategoryUSBHost,
        "HID {} #{} opened: interface {}, endpoint {:02x}, report size {}, report descriptor {} bytes.",
        GetProtocolName(),
        m_InterfaceIndex,
        int(m_InterfaceNumber),
        int(m_ReportEndpointIn),
        m_ReportEndpointInSize,
        m_ReportDescriptorLength
    );

    return descriptor;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::Close()
{
    if (m_ReportPipeIn != USB_INVALID_PIPE)
    {
        m_HostHandler->ClosePipe(m_ReportPipeIn);
        m_HostHandler->FreePipe(m_ReportPipeIn);
        m_ReportPipeIn = USB_INVALID_PIPE;
    }
    m_ReportEndpointIn = USB_INVALID_ENDPOINT;
    m_ReportEndpointInSize = 0;
    m_ReportDescriptorLength = 0;
    m_ReportBuffer.clear();
    m_IsActive = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::Startup()
{
    if (!m_IsActive && m_ReportPipeIn != USB_INVALID_PIPE)
    {
        m_IsActive = true;

        if (IsBootKeyboard()) {
            ReqSetIdle();
        }
        if (UsesBootProtocol()) {
            ReqSetBootProtocol();
        }
        StartReceive();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const char* USBHostHIDInterface::GetProtocolName() const
{
    switch (m_Protocol)
    {
        case USB_HID_ProtocolCode::KEYBOARD:   return "keyboard";
        case USB_HID_ProtocolCode::MOUSE:      return "mouse";
        default:                               return "generic";
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostHIDInterface::IsBootKeyboard() const
{
    return m_Subclass == USB_HID_SubclassCode::BOOT_INTERFACE && m_Protocol == USB_HID_ProtocolCode::KEYBOARD;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostHIDInterface::IsBootMouse() const
{
    return m_Subclass == USB_HID_SubclassCode::BOOT_INTERFACE && m_Protocol == USB_HID_ProtocolCode::MOUSE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostHIDInterface::UsesBootProtocol() const
{
    return IsBootKeyboard() || IsBootMouse();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::ReqSetIdle()
{
    USB_ControlRequest request(
        USB_RequestRecipient::INTERFACE,
        USB_RequestType::CLASS,
        USB_RequestDirection::HOST_TO_DEVICE,
        std::to_underlying(USB_HID_Request::SET_IDLE),
        0,
        m_InterfaceNumber,
        0
    );
    m_HostHandler->GetControlHandler().SendControlRequest(m_DeviceAddress, request, nullptr, p_bind_method(this, &USBHostHIDInterface::HandleSetIdleResult));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::ReqSetBootProtocol()
{
    USB_ControlRequest request(
        USB_RequestRecipient::INTERFACE,
        USB_RequestType::CLASS,
        USB_RequestDirection::HOST_TO_DEVICE,
        std::to_underlying(USB_HID_Request::SET_PROTOCOL),
        std::to_underlying(USB_HID_Protocol::BOOT),
        m_InterfaceNumber,
        0
    );
    m_HostHandler->GetControlHandler().SendControlRequest(m_DeviceAddress, request, nullptr, p_bind_method(this, &USBHostHIDInterface::HandleSetProtocolResult));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::StartReceive()
{
    if (m_IsActive && m_ReportPipeIn != USB_INVALID_PIPE && !m_ReportBuffer.empty())
    {
        const bool receiveStarted = m_HostHandler->InterruptReceiveData(
            m_ReportPipeIn,
            m_ReportBuffer.data(),
            m_ReportBuffer.size(),
            p_bind_method(this, &USBHostHIDInterface::ReceiveTransactionCallback)
        );
        if (!receiveStarted) {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID {} failed to start interrupt receive.", GetProtocolName());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::HandleSetIdleResult(bool result, uint8_t deviceAddress)
{
    if (!result) {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID keyboard set idle failed on device {}.", int(deviceAddress));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::HandleSetProtocolResult(bool result, uint8_t deviceAddress)
{
    if (!result) {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID {} set boot protocol failed on device {}.", GetProtocolName(), int(deviceAddress));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::ReceiveTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (!m_IsActive) {
        return;
    }

    if (urbState == USB_URBState::Done)
    {
        size_t reportLength = transactionLength;
        if (reportLength > m_ReportBuffer.size()) {
            reportLength = m_ReportBuffer.size();
        }
        if (reportLength > 0) {
            LogReport(reportLength);
        }
        StartReceive();
    }
    else if (urbState == USB_URBState::NotReady)
    {
        StartReceive();
    }
    else if (urbState == USB_URBState::Stall)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID {} interrupt endpoint stalled.", GetProtocolName());
    }
    else if (urbState == USB_URBState::Error)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID {} interrupt receive failed.", GetProtocolName());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::LogReport(size_t length)
{
    if (IsBootKeyboard()) {
        LogKeyboardReport(length);
    } else if (IsBootMouse()) {
        LogMouseReport(length);
    } else {
        LogRawReport(length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::LogKeyboardReport(size_t length)
{
    if (length < m_PreviousKeyboardReport.size())
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID keyboard short report: {} bytes.", length);
        LogRawReport(length);
        return;
    }

    const uint8_t modifiers = m_ReportBuffer[0];
    const uint8_t previousModifiers = m_PreviousKeyboardReport[0];

    if (modifiers != previousModifiers) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "HID keyboard modifiers: old={:02x} new={:02x}.", int(previousModifiers), int(modifiers));
    }

    for (size_t keyIndex = 2; keyIndex < m_PreviousKeyboardReport.size(); ++keyIndex)
    {
        const uint8_t usageCode = m_PreviousKeyboardReport[keyIndex];
        if (usageCode != 0 && !KeyboardReportContains(m_ReportBuffer.data(), usageCode)) {
            LogKeyboardKey(false, usageCode, modifiers);
        }
    }

    for (size_t keyIndex = 2; keyIndex < m_PreviousKeyboardReport.size(); ++keyIndex)
    {
        const uint8_t usageCode = m_ReportBuffer[keyIndex];
        if (usageCode != 0 && !KeyboardReportContains(m_PreviousKeyboardReport.data(), usageCode)) {
            LogKeyboardKey(true, usageCode, modifiers);
        }
    }

    for (size_t reportIndex = 0; reportIndex < m_PreviousKeyboardReport.size(); ++reportIndex) {
        m_PreviousKeyboardReport[reportIndex] = m_ReportBuffer[reportIndex];
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::LogKeyboardKey(bool pressed, uint8_t usageCode, uint8_t modifiers)
{
    const char* state = pressed ? "down" : "up";
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "HID keyboard key {}: usage={:02x} modifiers={:02x}.", state, int(usageCode), int(modifiers));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::LogMouseReport(size_t length)
{
    if (length < 3)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID mouse short report: {} bytes.", length);
        LogRawReport(length);
        return;
    }

    const uint8_t buttons = m_ReportBuffer[0];
    const uint8_t changedButtons = buttons ^ m_PreviousMouseButtons;
    const int deltaX = int(int8_t(m_ReportBuffer[1]));
    const int deltaY = int(int8_t(m_ReportBuffer[2]));
    int wheel = 0;

    if (length >= 4) {
        wheel = int(int8_t(m_ReportBuffer[3]));
    }
    if (changedButtons != 0) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "HID mouse buttons: state={:02x} changed={:02x}.", int(buttons), int(changedButtons));
    }
    if (deltaX != 0 || deltaY != 0 || wheel != 0) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "HID mouse move: dx={} dy={} wheel={} buttons={:02x}.", deltaX, deltaY, wheel, int(buttons));
    }

    m_PreviousMouseButtons = buttons;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHIDInterface::LogRawReport(size_t length)
{
    PString reportText;

    for (size_t reportIndex = 0; reportIndex < length; ++reportIndex)
    {
        if (!reportText.empty()) {
            reportText += " ";
        }
        reportText += PString::format_string("{:02x}", int(m_ReportBuffer[reportIndex]));
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "HID {} report ({} bytes): {}", GetProtocolName(), length, reportText.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostHIDInterface::KeyboardReportContains(const uint8_t* report, uint8_t usageCode)
{
    for (size_t keyIndex = 2; keyIndex < 8; ++keyIndex) {
        if (report[keyIndex] == usageCode) {
            return true;
        }
    }
    return false;
}

} // namespace kernel
