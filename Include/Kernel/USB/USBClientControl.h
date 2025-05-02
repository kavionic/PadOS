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
// Created: 04.06.2022 23:00

#pragma once

#include <vector>

#include <Ptr/Ptr.h>
#include <Kernel/USB/USBProtocol.h>

static constexpr uint32_t CFG_TUD_ENDPOINT0_SIZE = 64;

namespace kernel
{
class USBDevice;
class USBDriver;
class USBClassDriverDevice;
enum class USB_ControlStage : int;
enum class USB_TransferResult : uint8_t;

enum class ControlTransferHandler : int
{
    None,
    ClassDriver,
    VendorSpecific
};

class USBClientControl
{
public:
    ~USBClientControl();
    void Setup(USBDevice* deviceHandler, USBDriver* driver, uint32_t controlEndpointSize);
    void Reset();
    void SetRequest(const USB_ControlRequest& request, void* buffer, uint32_t length);
    void SetControlTransferHandler(ControlTransferHandler handlerType, Ptr<USBClassDriverDevice> classDriver = nullptr);
    bool ControlTransferComplete(uint8_t endpointAddr, USB_TransferResult result, uint32_t length);
    bool InvokeControlTransferHandler(USB_ControlStage stage);
    bool SendControlStatusReply(const USB_ControlRequest& request);
    bool SendControlDataReply(const USB_ControlRequest& request, void* buffer, uint32_t length);

private:
    bool StartControlStatusTransfer(const USB_ControlRequest& request);
    bool StartControlDataTransfer();

    USBDriver*              m_Driver = nullptr;
    USBDevice*              m_DeviceHandler = nullptr;
    USB_ControlRequest      m_Request;

    uint8_t*                m_TransferBuffer = nullptr;
    uint32_t                m_TransferLength = 0;
    uint32_t                m_TransferBytesSent = 0;

    ControlTransferHandler  m_TransferHandlerType = ControlTransferHandler::None;
    Ptr<USBClassDriverDevice>     m_TransferHandlerDriver;

    std::vector<uint8_t>    m_ControlEndpointBuffer;
};


} // namespace kernel
