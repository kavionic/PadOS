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

struct USB_ControlRequest;
struct USB_DescEndpoint;

enum class USB_Speed : uint8_t;

namespace kernel
{

enum class USB_TransferResult : uint8_t
{
    Success,
    Failed,
    Stalled,
    Timeout
};

class USBDriver
{
public:
    virtual void EndpointStall(uint8_t endpointAddr) = 0;
    virtual void EndpointClearStall(uint8_t endpointAddr) = 0;
    virtual bool EndpointOpen(const USB_DescEndpoint& endpointDescriptor) = 0;
    virtual void EndpointClose(uint8_t endpointAddr) = 0;
    virtual void EndpointCloseAll() = 0;
    virtual bool EndpointTransfer(uint8_t endpointAddr, uint8_t* buffer, size_t totalLength) = 0;
    virtual bool SetAddress(uint8_t deviceAddr) = 0;  // Return 'true' if a response needs to be sent.

    std::function<void()>                                                           IRQSuspend;
    std::function<void()>                                                           IRQResume;
    std::function<void()>                                                           IRQSessionEnded;
    std::function<void()>                                                           IRQDeviceDisconnected;
    std::function<void()>                                                           IRQStartOfFrame;
    std::function<void(USB_Speed)>                                                  IRQBusReset;
    std::function<void(const USB_ControlRequest&)>                                  IRQControlRequestReceived;
    std::function<void(uint8_t endpointAddr, uint32_t length , USB_TransferResult)> IRQTransferComplete;
    std::function<void()>                                                           IRQIncompleteIsochronousINTransfer;
};


} // namespace kernel
