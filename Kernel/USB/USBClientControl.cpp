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

#include <string.h>
#include <algorithm>
#include <Kernel/USB/USBClientControl.h>
#include <Kernel/USB/USBClassDriverDevice.h>
#include <Kernel/USB/USBDevice.h>
#include <Kernel/USB/USBDriver.h>
#include <Kernel/USB/USBCommon.h>

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBClientControl::~USBClientControl()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientControl::Setup(USBDevice* deviceHandler, USBDriver* driver, uint32_t controlEndpointSize)
{
    m_DeviceHandler = deviceHandler;
    m_Driver        = driver;

    m_ControlEndpointBuffer.resize(controlEndpointSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientControl::Reset()
{
    m_TransferBuffer        = nullptr;
    m_TransferLength        = 0;
    m_TransferBytesSent     = 0;
    m_TransferHandlerType   = ControlTransferHandler::None;
    m_TransferHandlerDriver = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientControl::SetRequest(const USB_ControlRequest& request, void* buffer, uint32_t length)
{
    m_Request           = request;
    m_TransferBuffer    = reinterpret_cast<uint8_t*>(buffer);
    m_TransferBytesSent = 0;
    m_TransferLength    = std::min(length, uint32_t(LittleEndianToHost(request.wLength)));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientControl::SetControlTransferHandler(ControlTransferHandler handlerType, Ptr<USBClassDriverDevice> classDriver)
{
    m_TransferHandlerType   = handlerType;
    m_TransferHandlerDriver = classDriver;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientControl::ControlTransferComplete(uint8_t endpointAddr, USB_TransferResult result, uint32_t length)
{
    // If request and endpoint direction is opposite this Status stage is complete.
    const bool requestDirIn = (m_Request.bmRequestType & USB_ControlRequest::REQUESTTYPE_DIR_IN) != 0;
    const bool endpointDirIn = (endpointAddr & USB_ControlRequest::REQUESTTYPE_DIR_IN) != 0;
    if (endpointDirIn != requestDirIn)
    {
        if (length != 0) {
            return InvokeControlTransferHandler(USB_ControlStage::ACK);
        } else {
            return false;
        }
    }
    if (!requestDirIn)
    {
        if (m_TransferBuffer == nullptr) {
            return false;
        }
        if (length > m_ControlEndpointBuffer.size())
        {
            kernel_log(LogCategoryUSBDevice, KLogSeverity::ERROR, "USBD: USBClientControl::ControlTransferComplete(%02x) invalid transfer length: &lu.\n", endpointAddr, length);
            return false;
        }
        memcpy(m_TransferBuffer, m_ControlEndpointBuffer.data(), length);
    }
    m_TransferBytesSent += length;
    m_TransferBuffer    += length;

    if (m_TransferBytesSent == m_Request.wLength || length < m_ControlEndpointBuffer.size())
    {
        // Data stage completed.
        if (InvokeControlTransferHandler(USB_ControlStage::DATA))
        {
            return StartControlStatusTransfer(m_Request);
        }
        else
        {
            m_Driver->EndpointStall(USB_MK_OUT_ADDRESS(0));
            m_Driver->EndpointStall(USB_MK_IN_ADDRESS(0));
        }
    }
    else
    {
        // More data to transfer.
        return StartControlDataTransfer();
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientControl::InvokeControlTransferHandler(USB_ControlStage stage)
{
    switch (m_TransferHandlerType)
    {
        case ControlTransferHandler::None:
            return true;
        case ControlTransferHandler::ClassDriver:
            if (m_TransferHandlerDriver != nullptr) {
                return m_TransferHandlerDriver->HandleControlTransfer(stage, m_Request);
            }
            return false;
        case ControlTransferHandler::VendorSpecific:
            if (m_DeviceHandler != nullptr && !m_DeviceHandler->SignalHandleVendorControlTransfer.Empty()) {
                return m_DeviceHandler->SignalHandleVendorControlTransfer(stage, m_Request);
            }
            return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientControl::SendControlStatusReply(const USB_ControlRequest& request)
{
    SetRequest(request, nullptr, 0);
    return StartControlStatusTransfer(request);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
/// Transmit data to the control endpoint. If USB_ControlRequest::wLength
/// is 0, a status packet is sent.
///////////////////////////////////////////////////////////////////////////////

bool USBClientControl::SendControlDataReply(const USB_ControlRequest& request, void* buffer, uint32_t length)
{
    SetRequest(request, buffer, length);

    if (request.wLength > 0)
    {
        if (m_TransferLength > 0 && buffer == nullptr) {
            return false;
        }
        return StartControlDataTransfer();
    }
    else
    {
        return StartControlStatusTransfer(request);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///
/// Start zero length status transfer.
///////////////////////////////////////////////////////////////////////////////

bool USBClientControl::StartControlStatusTransfer(const USB_ControlRequest& request)
{
    // Opposite to endpoint in data phase.
    const uint8_t endpointAddr = (request.bmRequestType & USB_ControlRequest::REQUESTTYPE_DIR_IN) ? USB_MK_OUT_ADDRESS(0) : USB_MK_IN_ADDRESS(0);
    return m_DeviceHandler->EndpointTransfer(endpointAddr, nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientControl::StartControlDataTransfer()
{
    const size_t  length = std::min(m_TransferLength - m_TransferBytesSent, uint32_t(m_ControlEndpointBuffer.size()));
    const uint8_t endpointAddr = (m_Request.bmRequestType & USB_ControlRequest::REQUESTTYPE_DIR_IN) ? USB_MK_IN_ADDRESS(0) : USB_MK_OUT_ADDRESS(0);

    if (m_Request.bmRequestType & USB_ControlRequest::REQUESTTYPE_DIR_IN) {
        memcpy(m_ControlEndpointBuffer.data(), m_TransferBuffer, length);
    }
    return m_DeviceHandler->EndpointTransfer(endpointAddr, (length != 0) ? m_ControlEndpointBuffer.data() : nullptr, length);
}

} // namespace kernel
