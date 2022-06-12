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
// Created: 08.06.2022 22:00


#include <Kernel/USB/ClassDrivers/USBCDCChannel.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBClassDriver.h>
#include <Kernel/USB/USBDevice.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBCDCChannel::USBCDCChannel(USBDevice* deviceHandler, uint8_t endpointNotification, uint8_t endpointOut, uint8_t endpointIn, uint16_t endpointOutMaxSize, uint16_t endpointInMaxSize)
    : KNamedObject("usbcdc", KNamedObjectType::Generic)
    , m_DeviceHandler(deviceHandler)
    , m_ReceiveCondition("usbcdc_receive")
    , m_TransmitCondition("usbcdc_transmit")
    , m_EndpointNotifications(endpointNotification)
    , m_EndpointOut(endpointOut)
    , m_EndpointIn(endpointIn)
{
    m_LineCoding.dwDTERate   = 115200;
    m_LineCoding.bCharFormat = USB_CDC_LineCodingStopBits::StopBits1;
    m_LineCoding.bParityType = USB_CDC_LineCodingParity::None;
    m_LineCoding.bDataBits   = 8;

    m_OutEndpointBuffer.resize(endpointOutMaxSize);
    m_InEndpointBuffer.resize(endpointInMaxSize);

    StartOutTransaction();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBCDCChannel::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    if (m_DeviceHandler != nullptr)
    {
        kassert(!m_DeviceHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

        switch (mode)
        {
            case ObjectWaitMode::Read:
                if (m_ReceiveFIFO.GetLength() == 0) {
                    return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read);
                } else {
                    return false; // Will not block.
                }
            case ObjectWaitMode::Write:
                if (m_TransmitFIFO.GetRemainingSpace() == 0) {
                    return m_TransmitCondition.AddListener(waitNode, ObjectWaitMode::Read);
                } else {
                    return false; // Will not block.
                }
            case ObjectWaitMode::ReadWrite:
                if (m_ReceiveFIFO.GetLength() == 0 && m_TransmitFIFO.GetRemainingSpace() == 0) {
                    return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read) && m_TransmitCondition.AddListener(waitNode, ObjectWaitMode::Read);
                } else {
                    return false; // Will not block.
                }
            default:
                return false;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBCDCChannel::Close()
{
    m_DeviceHandler = nullptr;
    m_ReceiveCondition.WakeupAll();
    m_TransmitCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBCDCChannel::GetReadBytesAvailable() const
{
    if (m_DeviceHandler != nullptr)
    {
        kassert(!m_DeviceHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
        return m_ReceiveFIFO.GetLength();
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBCDCChannel::Read(void* buffer, size_t length)
{
    if (m_DeviceHandler != nullptr)
    {
        kassert(!m_DeviceHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

        while (m_ReceiveFIFO.GetLength() == 0) {
            m_ReceiveCondition.Wait(m_DeviceHandler->GetMutex());
        }
        const ssize_t result = m_ReceiveFIFO.Read(buffer, length);
        if (result != 0) {
            StartOutTransaction();
        }
        return result;
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBCDCChannel::Write(void const* buffer, size_t length)
{
    if (m_DeviceHandler != nullptr)
    {
        kassert(!m_DeviceHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

        while (m_TransmitFIFO.GetRemainingSpace() == 0)
        {
            if (!m_TransmitCondition.Wait(m_DeviceHandler->GetMutex()) && get_last_error() != EAGAIN) {
                return -1;
            }
        }
        const size_t result = m_TransmitFIFO.Write(buffer, length);

        if (m_TransmitFIFO.GetLength() >= m_InEndpointBuffer.size()) {
            FlushInternal();
        }
        return result;
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBCDCChannel::Flush()
{
    if (m_DeviceHandler != nullptr)
    {
        kassert(!m_DeviceHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
        return FlushInternal();
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBCDCChannel::GetWriteBytesAvailable() const
{
    if (m_DeviceHandler != nullptr) {
        kassert(!m_DeviceHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
        return m_TransmitFIFO.GetRemainingSpace();
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBCDCChannel::HandleControlTransfer(USB_ControlStage stage, const USB_ControlRequest& request)
{
    switch (USB_CDC_ManagementRequest(request.bRequest))
    {
        case USB_CDC_ManagementRequest::SET_LINE_CODING:
            if (stage == USB_ControlStage::SETUP)
            {
                kernel_log(LogCategoryUSB, KLogSeverity::INFO_LOW_VOL, "USB: CDC set line coding.\n");
                return m_DeviceHandler->GetControlEndpointHandler().SendControlDataReply(request, &m_LineCoding, sizeof(m_LineCoding));
            }
            else if (stage == USB_ControlStage::ACK)
            {
                SignalLineCodingChanged(m_LineCoding);
            }
            break;

        case USB_CDC_ManagementRequest::GET_LINE_CODING:
            if (stage == USB_ControlStage::SETUP)
            {
                kernel_log(LogCategoryUSB, KLogSeverity::INFO_HIGH_VOL, "USB: CDC get line coding.\n");
                return m_DeviceHandler->GetControlEndpointHandler().SendControlDataReply(request, &m_LineCoding, sizeof(m_LineCoding));
            }
            break;

        case USB_CDC_ManagementRequest::SET_CONTROL_LINE_STATE:
            if (stage == USB_ControlStage::SETUP)
            {
                return m_DeviceHandler->GetControlEndpointHandler().SendControlStatusReply(request);
            }
            else if (stage == USB_ControlStage::ACK)
            {
                m_DTR = (request.wValue & USB_DTE_LINE_CONTROL_STATE_DTE_PRESENT) != 0;
                m_RTS = (request.wValue & USB_DTE_LINE_CONTROL_STATE_CARRIER_ACTIVE) != 0;

                kernel_log(LogCategoryUSB, KLogSeverity::INFO_HIGH_VOL, "USB: CDC Set control line state: DTR = %d, RTS = %d.\n", m_DTR, m_RTS);

                SignalControlLineStateChanged(m_DTR, m_RTS);
            }
            break;
        case USB_CDC_ManagementRequest::SEND_BREAK:
            if (stage == USB_ControlStage::SETUP)
            {
                return m_DeviceHandler->GetControlEndpointHandler().SendControlStatusReply(request);
            }
            else if (stage == USB_ControlStage::ACK)
            {
                kernel_log(LogCategoryUSB, KLogSeverity::INFO_LOW_VOL, "USB: CDC Send break.\n");
                if (request.wValue != 0xffff) {
                    SignalBreak(TimeValMicros::FromMilliseconds(request.wValue));
                }
                else {
                    SignalBreak(TimeValMicros::infinit);
                }
            }
            break;

        default:
            return false; // Stall unsupported request.
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBCDCChannel::HandleDataTransfer(uint8_t endpointAddr, USB_TransferResult result, uint32_t length)
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    if (endpointAddr == m_EndpointOut)
    {
        m_ReceiveFIFO.Write(m_OutEndpointBuffer.data(), length);
        m_ReceiveCondition.WakeupAll();
        StartOutTransaction();
    }
    else if (endpointAddr == m_EndpointIn)
    {
        if (FlushInternal() == 0)
        {
            // If the last block of data from the FIFO exactly filled the endpoint, send an
            // empty packet to tell the host that there is no more data coming. 
            if (length != 0 && m_TransmitFIFO.GetLength() == 0 && (length & (m_InEndpointBuffer.size() - 1)) == 0)
            {
                if (m_DeviceHandler->ClaimEndpoint(m_EndpointIn))
                {
                    m_DeviceHandler->EndpointTransfer(m_EndpointIn, nullptr, 0);
                }
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t USBCDCChannel::FlushInternal()
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    if (!m_DeviceHandler->IsReady() || m_TransmitFIFO.GetLength() == 0) {
        return 0;
    }
    if (!m_DeviceHandler->ClaimEndpoint(m_EndpointIn)) {
        return 0;
    }
    const size_t result = m_TransmitFIFO.Read(m_InEndpointBuffer.data(), m_InEndpointBuffer.size());

    if (result != 0)
    {
        m_TransmitCondition.WakeupAll();
        if (m_DeviceHandler->EndpointTransfer(m_EndpointIn, m_InEndpointBuffer.data(), result)) {
            return result;
        }
        return 0;
    }
    else
    {
        m_DeviceHandler->ReleaseEndpoint(m_EndpointIn);
        return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBCDCChannel::StartOutTransaction()
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    const size_t available = m_ReceiveFIFO.GetRemainingSpace();

    if (available < m_OutEndpointBuffer.size()) {
        return false;
    }
    if (!m_DeviceHandler->ClaimEndpoint(m_EndpointOut)) {
        return false;
    }
    if (available >= m_OutEndpointBuffer.size())
    {
        return m_DeviceHandler->EndpointTransfer(m_EndpointOut, m_OutEndpointBuffer.data(), m_OutEndpointBuffer.size());
    }
    else
    {
        m_DeviceHandler->ReleaseEndpoint(m_EndpointOut);
        return false;
    }
}

} // namespace kernel