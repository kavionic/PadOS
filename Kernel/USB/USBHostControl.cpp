// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.07.2022 20:00

#include <Utils/Utils.h>
#include <Kernel/KTime.h>
#include <Kernel/USB/USBHostControl.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/USBLanguages.h>

using namespace os;

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::Setup(USBHost* host)
{
    m_HostHandler = host;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::Reset()
{
    HandleRequestCompletion(false);
    m_PipeSize   = 64;
    m_ErrorCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::AllocPipes(uint8_t deviceAddr, USB_Speed speed, size_t pipeSize)
{
    m_PipeOut = m_HostHandler->AllocPipe(USB_MK_OUT_ADDRESS(0));
    m_PipeIn  = m_HostHandler->AllocPipe(USB_MK_IN_ADDRESS(0));
    if (pipeSize != 0) m_PipeSize = pipeSize;

    return UpdatePipes(deviceAddr, speed, m_PipeSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::FreePipes()
{
    m_HostHandler->FreePipe(m_PipeIn);
    m_HostHandler->FreePipe(m_PipeOut);

    m_PipeIn  = USB_INVALID_PIPE;
    m_PipeOut = USB_INVALID_PIPE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::UpdatePipes(uint8_t deviceAddr, USB_Speed speed, size_t pipeSize)
{
    if (pipeSize != 0) m_PipeSize = pipeSize;
    m_HostHandler->OpenPipe(m_PipeIn,  USB_MK_IN_ADDRESS(0),  deviceAddr, speed, USB_TransferType::CONTROL, m_PipeSize);
    m_HostHandler->OpenPipe(m_PipeOut, USB_MK_OUT_ADDRESS(0), deviceAddr, speed, USB_TransferType::CONTROL, m_PipeSize);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::SendControlRequest(uint8_t deviceAddr, const USB_ControlRequest& request, void* buffer, USBHostControlRequestCallback&& callback)
{
    m_Setup                 = request;
    m_Buffer                = static_cast<uint8_t*>(buffer);
    m_Length                = LittleEndianToHost(request.wLength);
    m_CurrentDeviceAddress  = deviceAddr;
    m_ErrorCount            = 0;
    m_RequestCallback       = std::move(callback);

    TimeValNanos deadline = kget_monotonic_time() + TimeValNanos::FromSeconds(2.0);
    while (m_HostHandler->GetURBState(m_PipeOut) != USB_URBState::Idle && kget_monotonic_time() < deadline) ksnooze_ms(1); // FIXME: Implement proper blocking for state-change waiting.
    if (m_HostHandler->GetURBState(m_PipeOut) != USB_URBState::Idle)
    {
        m_ErrorCount = 0;
        kernel_log(LogCategoryUSBHost, KLogSeverity::ERROR, "USBH: Control request error. Pipe not idle.\n");
        FreePipes();
        m_HostHandler->RestartDeviceInitialization();
        HandleRequestCompletion(false);
        return false;
    }
    return m_HostHandler->ControlSendSetup(m_PipeOut, &m_Setup, bind_method(this, &USBHostControl::ControlSentCallback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::ReqGetDescriptor(uint8_t deviceAddr, USB_RequestRecipient recipient, USB_RequestType type, USB_DescriptorType descType, uint16_t descIndex, uint16_t index, void* buffer, size_t length, USBHostControlRequestCallback&& callback)
{
    USB_ControlRequest request(
        recipient,
        type,
        USB_RequestDirection::DEVICE_TO_HOST,
        uint8_t(USB_RequestCode::GET_DESCRIPTOR),
        uint16_t((uint16_t(descType) << 8) | descIndex),
        index,
        uint16_t(length)
    );
    return SendControlRequest(deviceAddr, request, buffer, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::ReqGetStringDescriptor(uint8_t deviceAddr, uint8_t stringIndex, String& outString, USBHostControlRequestCallback&& callback)
{
    return ReqGetDescriptor(deviceAddr, USB_RequestRecipient::DEVICE, USB_RequestType::STANDARD, USB_DescriptorType::STRING, stringIndex, uint16_t(USB_LanguageID::ENGLISH_UNITED_STATES), m_CtrlDataBuffer, sizeof(m_CtrlDataBuffer),
        [this, &outString, callback](bool result, uint8_t deviceAddr)
        {
            if (result)
            {
                const USB_DescString* stringDesc = reinterpret_cast<const USB_DescString*>(m_CtrlDataBuffer);
                outString = ParseStringDescriptor(stringDesc);
            }
            callback(result, deviceAddr);
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::ReqSetAddress(uint8_t deviceAddr, USBHostControlRequestCallback&& callback)
{
    USB_ControlRequest request(
        USB_RequestRecipient::DEVICE,
        USB_RequestType::STANDARD,
        USB_RequestDirection::HOST_TO_DEVICE,
        uint8_t(USB_RequestCode::SET_ADDRESS),
        deviceAddr,
        0,
        0
    );
    if (SendControlRequest(0, request, nullptr, std::move(callback)))
    {
        m_CurrentDeviceAddress = deviceAddr;
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::ReqSetConfiguration(uint8_t deviceAddr, uint16_t configIndex, USBHostControlRequestCallback&& callback)
{
    USB_ControlRequest request(
        USB_RequestRecipient::DEVICE,
        USB_RequestType::STANDARD,
        USB_RequestDirection::HOST_TO_DEVICE,
        uint8_t(USB_RequestCode::SET_CONFIGURATION),
        configIndex,
        0,
        0
    );
    return SendControlRequest(deviceAddr, request, nullptr, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::HandleRequestError()
{
    if (m_ErrorCount++ == 0)
    {
        // Restart the request from SETUP.
        m_HostHandler->ControlSendSetup(m_PipeOut, &m_Setup, bind_method(this, &USBHostControl::ControlSentCallback));
    }
    else
    {
        m_ErrorCount = 0;
        kernel_log(LogCategoryUSBHost, KLogSeverity::ERROR, "USBH: Control request error. Device not responding.\n");
        FreePipes();
        m_HostHandler->RestartDeviceInitialization();
        HandleRequestCompletion(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::HandleRequestCompletion(bool status)
{
    if (m_RequestCallback)
    {
        USBHostControlRequestCallback callback = std::move(m_RequestCallback);
        m_RequestCallback = nullptr;
        callback(status, m_CurrentDeviceAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String USBHostControl::ParseStringDescriptor(const USB_DescString* stringDesc)
{
    if (stringDesc->bDescriptorType != USB_DescriptorType::STRING) {
        return String::zero;
    }
    const size_t     strlength = (stringDesc->bLength - sizeof(USB_DescString)) / sizeof(wchar16_t);
    const wchar16_t* utf16Str = reinterpret_cast<const wchar16_t*>(stringDesc + 1);

    String result;
    result.assign_utf16(utf16Str, strlength);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::ControlSentCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (urbState == USB_URBState::Done)
    {
        const USB_RequestDirection direction = USB_RequestDirection((m_Setup.bmRequestType & USB_ControlRequest::REQUESTTYPE_DIR) >> USB_ControlRequest::REQUESTTYPE_DIR_Pos);

        if (m_Length != 0)
        {
            if (direction == USB_RequestDirection::DEVICE_TO_HOST) {
                m_HostHandler->ControlReceiveData(m_PipeIn, m_Buffer, m_Length, bind_method(this, &USBHostControl::ControlDataReceivedCallback));
            } else {
                m_HostHandler->ControlSendData(m_PipeOut, m_Buffer, m_Length, true, bind_method(this, &USBHostControl::ControlDataSentCallback));
            }
        }
        else
        {
            if (direction == USB_RequestDirection::DEVICE_TO_HOST) {
                m_HostHandler->ControlSendData(m_PipeOut, nullptr, 0, true, bind_method(this, &USBHostControl::ControlStatusSentCallback));
            } else {
                m_HostHandler->ControlReceiveData(m_PipeIn, nullptr, 0, bind_method(this, &USBHostControl::ControlStatusReceivedCallback));
            }
        }
    }
    else
    {
        if (urbState == USB_URBState::Error || urbState == USB_URBState::NotReady) {
            HandleRequestError();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::ControlDataReceivedCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (urbState == USB_URBState::Done) {
        m_HostHandler->ControlSendData(m_PipeOut, nullptr, 0, true, bind_method(this, &USBHostControl::ControlStatusSentCallback));
    } else if (urbState == USB_URBState::Stall) {
        HandleRequestCompletion(false);
    } else if (urbState == USB_URBState::Error) {
        HandleRequestError();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::ControlDataSentCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (urbState == USB_URBState::Done) {
        m_HostHandler->ControlReceiveData(m_PipeIn, nullptr, 0, bind_method(this, &USBHostControl::ControlStatusReceivedCallback));
    } else if (urbState == USB_URBState::Stall) {
        HandleRequestCompletion(false);
    } else if (urbState == USB_URBState::NotReady) { // Received NAK from device.
        m_HostHandler->ControlSendData(m_PipeOut, m_Buffer, m_Length, true, bind_method(this, &USBHostControl::ControlDataSentCallback));
    } else if (urbState == USB_URBState::Error) {
        HandleRequestError();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::ControlStatusSentCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (urbState == USB_URBState::Done) {
        HandleRequestCompletion(true);
    } else if (urbState == USB_URBState::NotReady) {
        m_HostHandler->ControlSendData(m_PipeOut, nullptr, 0, true, bind_method(this, &USBHostControl::ControlStatusSentCallback));
    } else if (urbState == USB_URBState::Error) {
        HandleRequestError();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::ControlStatusReceivedCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (urbState == USB_URBState::Done) {
        HandleRequestCompletion(true);
    } else if (urbState == USB_URBState::Error) {
        HandleRequestError();
    } else if (urbState == USB_URBState::Stall) {
        HandleRequestCompletion(false);
    }
}


} // namespace kernel
