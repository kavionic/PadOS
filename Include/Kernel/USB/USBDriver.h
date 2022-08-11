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
// Created: 26.05.2022 13:00

#pragma once

#include <stdint.h>
#include <functional>
#include <System/Platform.h>
#include <Signals/Signal.h>
#include <Kernel/USB/USBCommon.h>

struct USB_ControlRequest;
struct USB_DescEndpoint;

enum class USB_Speed : uint8_t;
enum class USB_TransferType : uint8_t;
enum class USB_RequestDirection : uint8_t;


namespace kernel
{
enum class USBH_InitialTransactionPID : uint8_t;
enum class USB_URBState : uint8_t;

enum class USB_TransferResult : uint8_t
{
    Invalid,
    Success,
    Failed,
    Stalled,
    Timeout
};

class USBDriver
{
public:
    virtual USB_Speed   HostGetSpeed() const = 0;

    // Device interface:
    virtual USB_Speed   DeviceGetSpeed() const = 0;
    virtual void        EndpointStall(uint8_t endpointAddr) = 0;
    virtual void        EndpointClearStall(uint8_t endpointAddr) = 0;
    virtual bool        EndpointOpen(const USB_DescEndpoint& endpointDescriptor) = 0;
    virtual void        EndpointClose(uint8_t endpointAddr) = 0;
    virtual void        EndpointCloseAll() = 0;
    virtual bool        EndpointTransfer(uint8_t endpointAddr, void* buffer, size_t totalLength) = 0;
    virtual bool        SetAddress(uint8_t deviceAddr) = 0;  // Return 'true' if a response needs to be sent.

    // Host interface:
    virtual uint32_t    GetMaxPipeCount() const = 0;
    virtual bool        StartHost() = 0;
    virtual bool        StopHost() = 0;
    virtual bool        ResetPort() = 0;
    virtual uint32_t    GetCurrentHostFrame() = 0;
    virtual bool        SetupPipe(USB_PipeIndex pipeIndex, uint8_t endpointAddr, uint8_t deviceAddr, USB_Speed speed, USB_TransferType endpointType, size_t maxPacketSize) = 0;
    virtual bool        HaltChannel(USB_PipeIndex pipeIndex) = 0;
    virtual bool        HostSubmitRequest(USB_PipeIndex pipeIndex, USB_RequestDirection direction, USB_TransferType endpointType, USBH_InitialTransactionPID initialPID, void* buffer, size_t length, bool doPing) = 0;
    virtual bool        SetDataToggle(USB_PipeIndex pipeIndex, bool toggle) = 0;
    virtual bool        GetDataToggle(USB_PipeIndex pipeIndex) const = 0;


    virtual void        EnableIRQ(bool enable) = 0;

    std::function<void()>                                                           IRQSuspend;
    std::function<void()>                                                           IRQResume;
    std::function<void()>                                                           IRQDebounceDone;
    std::function<void()>                                                           IRQSessionEnded;
    std::function<void()>                                                           IRQDeviceConnected;     // Host mode only.
    std::function<void()>                                                           IRQDeviceDisconnected;  // Host & device mode.
    std::function<void()>                                                           IRQStartOfFrame;        // Host & device mode.
    std::function<void(USB_Speed)>                                                  IRQBusReset;
    std::function<void(const USB_ControlRequest&)>                                  IRQControlRequestReceived;
    std::function<void(uint8_t endpointAddr, uint32_t length , USB_TransferResult)> IRQTransferComplete;
    std::function<void()>                                                           IRQIncompleteIsochronousINTransfer;

    std::function<void(bool isEnabled)>                                                 IRQPortEnableChange;
    std::function<void(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t length)>  IRQPipeURBStateChanged;
};


} // namespace kernel
