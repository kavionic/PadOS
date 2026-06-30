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

#pragma once

#include <deque>
#include <functional>
#include <stdint.h>

#include <System/Sections.h>
#include <System/TimeValue.h>
#include <Utils/String.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/USB/USBProtocolHub.h>
#include <Kernel/USB/USBCommon.h>

namespace kernel
{
class USBHost;
enum class USB_URBState : uint8_t;

using USBHostControlRequestCallback = std::function<void(bool status, uint8_t deviceAddr)>;

class USBHostControl
{
public:
    void Setup(USBHost* host);

    void Reset();

    bool AllocPipes(uint8_t deviceAddr, USB_Speed speed, size_t pipeSize);
    void FreePipes();

    bool UpdatePipes(uint8_t deviceAddr, USB_Speed speed, size_t pipeSize);

    bool SendControlRequest(uint8_t deviceAddr, const USB_ControlRequest& request, void* buff, USBHostControlRequestCallback&& callback);
    TimeValNanos GetRequestDeadline() const { return m_RequestDeadline; }
    bool HandleRequestTimeout(TimeValNanos currentTime);

    bool ReqGetDescriptor(uint8_t deviceAddr, USB_RequestRecipient recipient, USB_RequestType type, USB_DescriptorType descType, uint16_t descIndex, uint16_t index, void* buffer, size_t length, USBHostControlRequestCallback&& callback);
    bool ReqGetStringDescriptor(uint8_t deviceAddr, uint8_t stringIndex, PString& outString, USBHostControlRequestCallback&& callback);
    bool ReqSetAddress(uint8_t deviceAddr, USBHostControlRequestCallback&& callback);
    bool ReqSetConfiguration(uint8_t deviceAddr, uint16_t configIndex, USBHostControlRequestCallback&& callback);
    bool ReqGetHubDescriptor(uint8_t deviceAddr, void* buffer, size_t length, USBHostControlRequestCallback&& callback);
    bool ReqGetHubPortStatus(uint8_t deviceAddr, uint8_t portIndex, USB_HubPortStatus* status, USBHostControlRequestCallback&& callback);
    bool ReqSetHubPortFeature(uint8_t deviceAddr, uint8_t portIndex, USB_HubFeatureSelector feature, USBHostControlRequestCallback&& callback);
    bool ReqClearHubPortFeature(uint8_t deviceAddr, uint8_t portIndex, USB_HubFeatureSelector feature, USBHostControlRequestCallback&& callback);

    uint8_t* GetCtrlDataBuffer() { return m_CtrlDataBuffer; }

private:
    static constexpr TimeValNanos REQUEST_TIMEOUT = TimeValNanos::FromSeconds(2.0);

    struct ControlRequest
    {
        uint8_t                         DeviceAddress = 0;
        uint8_t                         CallbackDeviceAddress = 0;
        USB_ControlRequest              Request;
        void*                           Buffer = nullptr;
        USBHostControlRequestCallback   Callback;
    };

    bool QueueControlRequest(uint8_t deviceAddr, uint8_t callbackDeviceAddr, const USB_ControlRequest& request, void* buffer, USBHostControlRequestCallback&& callback);
    bool StartControlRequest(ControlRequest&& request);
    void StartNextQueuedRequest();
    void CancelCurrentTransfer();
    void LogRequestError(const char* stage);
    void HandleRequestError();
    void HandleRequestCompletion(bool status);

    PString ParseStringDescriptor(const USB_DescString* stringDesc);

    void ControlSentCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    void ControlDataReceivedCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    void ControlDataSentCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    void ControlStatusSentCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    void ControlStatusReceivedCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);

    USBHost* m_HostHandler = nullptr;

    USB_PipeIndex       m_PipeIn                = USB_INVALID_PIPE;
    USB_PipeIndex       m_PipeOut               = USB_INVALID_PIPE;
    size_t              m_PipeSize              = 0;
    uint8_t             m_ErrorCount            = 0;
    uint8_t             m_CurrentDeviceAddress  = 0;
    size_t              m_Length                = 0;
    uint8_t*            m_Buffer                = nullptr;
    USB_ControlRequest  m_Setup;
    uint8_t             m_CtrlDataBuffer[1024];
    bool                m_RequestActive = false;
    TimeValNanos        m_RequestDeadline = TimeValNanos::infinit;
    std::deque<ControlRequest> m_RequestQueue;
    
    USBHostControlRequestCallback   m_RequestCallback;
};


} // namespace kernel
