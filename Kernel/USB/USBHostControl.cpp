// This file is part of PadOS.
//
// Copyright (C) 2022-2026 Kurt Skauen <http://kavionic.com/>
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

#include <utility>

#include <Utils/Utils.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/USB/USBHostControl.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/USBLanguages.h>


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
    FreePipes();

    m_PipeSize             = 64;
    m_ErrorCount           = 0;
    m_CurrentDeviceAddress = 0;
    m_Length               = 0;
    m_Buffer               = nullptr;
    m_RequestCallback      = nullptr;
    m_RequestActive        = false;
    m_RequestQueue.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::AllocPipes(uint8_t deviceAddr, USB_Speed speed, size_t pipeSize)
{
    FreePipes();

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
    if (m_HostHandler != nullptr)
    {
        m_HostHandler->FreePipe(m_PipeIn);
        m_HostHandler->FreePipe(m_PipeOut);
    }

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
    return QueueControlRequest(deviceAddr, deviceAddr, request, buffer, std::move(callback));
}

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

bool USBHostControl::ReqGetStringDescriptor(uint8_t deviceAddr, uint8_t stringIndex, PString& outString, USBHostControlRequestCallback&& callback)
{
    return ReqGetDescriptor(deviceAddr, USB_RequestRecipient::DEVICE, USB_RequestType::STANDARD, USB_DescriptorType::STRING, stringIndex, uint16_t(USB_LanguageID::ENGLISH_UNITED_STATES), m_CtrlDataBuffer, sizeof(m_CtrlDataBuffer),
        [this, &outString, callback = std::move(callback)](bool result, uint8_t deviceAddr)
        {
            if (result)
            {
                const USB_DescString* stringDesc = reinterpret_cast<const USB_DescString*>(m_CtrlDataBuffer);
                outString = ParseStringDescriptor(stringDesc);
            }
            if (callback) {
                callback(result, deviceAddr);
            }
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
    return QueueControlRequest(0, deviceAddr, request, nullptr, std::move(callback));
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

bool USBHostControl::ReqGetHubDescriptor(uint8_t deviceAddr, void* buffer, size_t length, USBHostControlRequestCallback&& callback)
{
    return ReqGetDescriptor(deviceAddr, USB_RequestRecipient::DEVICE, USB_RequestType::CLASS, USB_DescriptorType::HUB, 0, 0, buffer, length, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::ReqGetHubPortStatus(uint8_t deviceAddr, uint8_t portIndex, USB_HubPortStatus* status, USBHostControlRequestCallback&& callback)
{
    USB_ControlRequest request(
        USB_RequestRecipient::OTHER,
        USB_RequestType::CLASS,
        USB_RequestDirection::DEVICE_TO_HOST,
        uint8_t(USB_RequestCode::GET_STATUS),
        0,
        portIndex,
        sizeof(USB_HubPortStatus)
    );
    return SendControlRequest(deviceAddr, request, status, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::ReqSetHubPortFeature(uint8_t deviceAddr, uint8_t portIndex, USB_HubFeatureSelector feature, USBHostControlRequestCallback&& callback)
{
    USB_ControlRequest request(
        USB_RequestRecipient::OTHER,
        USB_RequestType::CLASS,
        USB_RequestDirection::HOST_TO_DEVICE,
        uint8_t(USB_RequestCode::SET_FEATURE),
        std::to_underlying(feature),
        portIndex,
        0
    );
    return SendControlRequest(deviceAddr, request, nullptr, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::ReqClearHubPortFeature(uint8_t deviceAddr, uint8_t portIndex, USB_HubFeatureSelector feature, USBHostControlRequestCallback&& callback)
{
    USB_ControlRequest request(
        USB_RequestRecipient::OTHER,
        USB_RequestType::CLASS,
        USB_RequestDirection::HOST_TO_DEVICE,
        uint8_t(USB_RequestCode::CLEAR_FEATURE),
        std::to_underlying(feature),
        portIndex,
        0
    );
    return SendControlRequest(deviceAddr, request, nullptr, std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::QueueControlRequest(uint8_t deviceAddr, uint8_t callbackDeviceAddr, const USB_ControlRequest& request, void* buffer, USBHostControlRequestCallback&& callback)
{
    ControlRequest controlRequest;
    controlRequest.DeviceAddress = deviceAddr;
    controlRequest.CallbackDeviceAddress = callbackDeviceAddr;
    controlRequest.Request = request;
    controlRequest.Buffer = buffer;
    controlRequest.Callback = std::move(callback);

    if (m_RequestActive)
    {
        m_RequestQueue.push_back(std::move(controlRequest));
        return true;
    }
    return StartControlRequest(std::move(controlRequest));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostControl::StartControlRequest(ControlRequest&& request)
{
    m_Setup                 = request.Request;
    m_Buffer                = static_cast<uint8_t*>(request.Buffer);
    m_Length                = PLittleEndianToHost(request.Request.wLength);
    m_CurrentDeviceAddress  = request.CallbackDeviceAddress;
    m_ErrorCount            = 0;
    m_RequestCallback       = std::move(request.Callback);
    m_RequestActive         = true;

    USBDeviceNode* device = m_HostHandler->GetDevice(request.DeviceAddress);
    if (device == nullptr)
    {
        HandleRequestCompletion(false);
        return false;
    }
    const size_t pipeSize = (device->m_DeviceDesc.bMaxPacketSize0 != 0) ? device->m_DeviceDesc.bMaxPacketSize0 : m_PipeSize;
    UpdatePipes(request.DeviceAddress, device->m_Speed, pipeSize);

    TimeValNanos deadline = kget_monotonic_time() + TimeValNanos::FromSeconds(2.0);
    while (m_HostHandler->GetURBState(m_PipeOut) != USB_URBState::Idle && kget_monotonic_time() < deadline) ksnooze_ms(1); // FIXME: Implement proper blocking for state-change waiting.
    if (m_HostHandler->GetURBState(m_PipeOut) != USB_URBState::Idle)
    {
        m_ErrorCount = 0;
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Control request error. Pipe not idle.");
        HandleRequestCompletion(false);
        return false;
    }
    if (!m_HostHandler->ControlSendSetup(m_PipeOut, &m_Setup, p_bind_method(this, &USBHostControl::ControlSentCallback)))
    {
        HandleRequestCompletion(false);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::StartNextQueuedRequest()
{
    if (!m_RequestActive && !m_RequestQueue.empty())
    {
        ControlRequest request = std::move(m_RequestQueue.front());
        m_RequestQueue.pop_front();
        StartControlRequest(std::move(request));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::HandleRequestError()
{
    if (m_ErrorCount++ == 0)
    {
        // Restart the request from SETUP.
        m_HostHandler->ControlSendSetup(m_PipeOut, &m_Setup, p_bind_method(this, &USBHostControl::ControlSentCallback));
    }
    else
    {
        m_ErrorCount = 0;
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Control request error. Device not responding.");
        HandleRequestCompletion(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostControl::HandleRequestCompletion(bool status)
{
    m_RequestActive = false;
    if (m_RequestCallback)
    {
        USBHostControlRequestCallback callback = std::move(m_RequestCallback);
        m_RequestCallback = nullptr;
        callback(status, m_CurrentDeviceAddress);
    }
    if (!m_RequestActive) {
        StartNextQueuedRequest();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString USBHostControl::ParseStringDescriptor(const USB_DescString* stringDesc)
{
    if (stringDesc->bDescriptorType != USB_DescriptorType::STRING) {
        return PString::zero;
    }
    const size_t     strlength = (stringDesc->bLength - sizeof(USB_DescString)) / sizeof(wchar16_t);
    const wchar16_t* utf16Str = reinterpret_cast<const wchar16_t*>(stringDesc + 1);

    PString result;
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
                m_HostHandler->ControlReceiveData(m_PipeIn, m_Buffer, m_Length, p_bind_method(this, &USBHostControl::ControlDataReceivedCallback));
            } else {
                m_HostHandler->ControlSendData(m_PipeOut, m_Buffer, m_Length, true, p_bind_method(this, &USBHostControl::ControlDataSentCallback));
            }
        }
        else
        {
            if (direction == USB_RequestDirection::DEVICE_TO_HOST) {
                m_HostHandler->ControlSendData(m_PipeOut, nullptr, 0, true, p_bind_method(this, &USBHostControl::ControlStatusSentCallback));
            } else {
                m_HostHandler->ControlReceiveData(m_PipeIn, nullptr, 0, p_bind_method(this, &USBHostControl::ControlStatusReceivedCallback));
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
        m_HostHandler->ControlSendData(m_PipeOut, nullptr, 0, true, p_bind_method(this, &USBHostControl::ControlStatusSentCallback));
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
        m_HostHandler->ControlReceiveData(m_PipeIn, nullptr, 0, p_bind_method(this, &USBHostControl::ControlStatusReceivedCallback));
    } else if (urbState == USB_URBState::Stall) {
        HandleRequestCompletion(false);
    } else if (urbState == USB_URBState::NotReady) { // Received NAK from device.
        m_HostHandler->ControlSendData(m_PipeOut, m_Buffer, m_Length, true, p_bind_method(this, &USBHostControl::ControlDataSentCallback));
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
        m_HostHandler->ControlSendData(m_PipeOut, nullptr, 0, true, p_bind_method(this, &USBHostControl::ControlStatusSentCallback));
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
