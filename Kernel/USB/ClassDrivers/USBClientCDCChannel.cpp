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


#include <Utils/String.h>
#include <Kernel/USB/ClassDrivers/USBClientCDCChannel.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBClassDriverDevice.h>
#include <Kernel/USB/USBDevice.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include "../../../Include/DeviceControl/USART.h"

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBClientCDCChannel::USBClientCDCChannel(USBDevice* deviceHandler, int channelIndex, uint8_t endpointNotification, uint8_t endpointOut, uint8_t endpointIn, uint16_t endpointOutMaxSize, uint16_t endpointInMaxSize)
    : KINode(nullptr, nullptr, this, false)
    , m_DeviceHandler(deviceHandler)
    , m_ReceiveCondition("usbdcdc_receive")
    , m_TransmitCondition("usbdcdc_transmit")
    , m_EndpointNotifications(endpointNotification)
    , m_EndpointOut(endpointOut)
    , m_EndpointIn(endpointIn)
{
    m_CreateTime = get_real_time();

    m_LineCoding.dwDTERate   = 115200;
    m_LineCoding.bCharFormat = USB_CDC_LineCodingStopBits::StopBits1;
    m_LineCoding.bParityType = USB_CDC_LineCodingParity::None;
    m_LineCoding.bDataBits   = 8;

    m_OutEndpointBuffer.resize(endpointOutMaxSize);
    m_InEndpointBuffer.resize(endpointInMaxSize);

    m_DevNodeHandle = Kernel::RegisterDevice(String::format_string("com/udp%u", channelIndex).c_str(), ptr_tmp_cast(this));

    StartOutTransaction();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientCDCChannel::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
    if (m_IsActive)
    {
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

void USBClientCDCChannel::Close()
{
    if (m_DevNodeHandle != -1)
    {
        Kernel::RemoveDevice(m_DevNodeHandle);
        m_DevNodeHandle = -1;
    }
    m_IsActive = false;
    m_ReceiveCondition.WakeupAll();
    m_TransmitCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBClientCDCChannel::GetReadBytesAvailable() const
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
    if (m_IsActive)
    {
        return m_ReceiveFIFO.GetLength();
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBClientCDCChannel::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    KFilesystemFileOps::CloseFile(volume, file);
    file->SetOpenFlags(0);
    m_ReceiveCondition.WakeupAll();
    m_TransmitCondition.WakeupAll();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode USBClientCDCChannel::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

    if (!file->HasReadAccess())
    {
        return PErrorCode::NoAccess;
    }
    if (m_IsActive)
    {
        if (m_ReceiveFIFO.GetLength() == 0)
        {
            if ((file->GetOpenFlags() & O_NONBLOCK) == 0)
            {
                TimeValNanos deadline = m_ReadTimeout.IsInfinit() ? TimeValNanos::infinit : (kget_system_time() + m_ReadTimeout);

                while (m_ReceiveFIFO.GetLength() == 0)
                {
                    const PErrorCode result = m_ReceiveCondition.WaitDeadline(m_DeviceHandler->GetMutex(), deadline);
                    if (result != PErrorCode::Success && result != PErrorCode::Interrupted)
                    {
                        return result;
                    }
                    if (!m_IsActive)
                    {
                        return PErrorCode::BrokenPipe;
                    }
                    if (!file->HasReadAccess())
                    {
                        return PErrorCode::NoAccess;
                    }
                }
            }
            else
            {
                outLength = 0;
                return PErrorCode::Success;
            }
        }
        const ssize_t result = m_ReceiveFIFO.Read(buffer, length);
        if (result != 0) {
            StartOutTransaction();
        }
        outLength = result;
        return PErrorCode::Success;
    }
    return PErrorCode::BrokenPipe;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode USBClientCDCChannel::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

    if (!file->HasWriteAccess())
    {
        return PErrorCode::NoAccess;
    }
    if (m_IsActive)
    {
        if (m_TransmitFIFO.GetRemainingSpace() == 0)
        {
            if ((file->GetOpenFlags() & O_NONBLOCK) == 0)
            {
                TimeValNanos deadline = m_WriteTimeout.IsInfinit() ? TimeValNanos::infinit : (kget_system_time() + m_WriteTimeout);

                while (m_TransmitFIFO.GetRemainingSpace() == 0)
                {
                    FlushInternal();
                    const PErrorCode result = m_TransmitCondition.WaitDeadline(m_DeviceHandler->GetMutex(), deadline);
                    if (result != PErrorCode::Success && result != PErrorCode::Interrupted)
                    {
                        return result;
                    }
                    if (!m_IsActive)
                    {
                        return PErrorCode::BrokenPipe;
                    }
                    if (!file->HasWriteAccess())
                    {
                        return PErrorCode::NoAccess;
                    }
                }
            }
            else
            {
                outLength = 0;
                return PErrorCode::Success;
            }
        }
        const size_t result = std::min(m_TransmitFIFO.GetRemainingSpace(), length);
        m_TransmitFIFO.Write(buffer, result);

        if ((file->GetOpenFlags() & (O_SYNC | O_DIRECT)) || m_TransmitFIFO.GetLength() >= m_InEndpointBuffer.size()) {
            FlushInternal();
        }
        outLength = result;
        return PErrorCode::Success;
    }
    return PErrorCode::BrokenPipe;
}

int USBClientCDCChannel::Sync(Ptr<KFileNode> file)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
    if (m_IsActive)
    {
        FlushInternal();
        return 0;
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBClientCDCChannel::ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* outStats)
{
    outStats->st_dev = dev_t(volume->m_VolumeID);
    outStats->st_ino = node->m_INodeID;
    outStats->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    outStats->st_mode |= S_IFREG;

    if (volume->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
        outStats->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    }
    outStats->st_nlink = 1;
    outStats->st_uid = 0;
    outStats->st_gid = 0;
    outStats->st_size = 0;
    outStats->st_blksize = 1;
    outStats->st_atim = outStats->st_mtim = outStats->st_ctim = m_CreateTime.AsTimespec();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBClientCDCChannel::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    switch (request)
    {
        case USARTIOCTL_SET_READ_TIMEOUT:
            if (inDataLength == sizeof(bigtime_t))
            {
                bigtime_t nanos = *((const bigtime_t*)inData);
                m_ReadTimeout = TimeValNanos::FromNanoseconds(nanos);
                return 0;
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_GET_READ_TIMEOUT:
            if (outDataLength == sizeof(bigtime_t))
            {
                bigtime_t* nanos = (bigtime_t*)outData;
                *nanos = m_ReadTimeout.AsNanoseconds();
                return 0;
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_SET_WRITE_TIMEOUT:
            if (inDataLength == sizeof(bigtime_t))
            {
                bigtime_t nanos = *((const bigtime_t*)inData);
                m_WriteTimeout = TimeValNanos::FromNanoseconds(nanos);
                return 0;
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_GET_WRITE_TIMEOUT:
            if (outDataLength == sizeof(bigtime_t))
            {
                bigtime_t* nanos = (bigtime_t*)outData;
                *nanos = m_WriteTimeout.AsNanoseconds();
                return 0;
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        default:
            set_last_error(EINVAL);
            return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBClientCDCChannel::GetWriteBytesAvailable() const
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
    if (m_IsActive) {
        return m_TransmitFIFO.GetRemainingSpace();
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientCDCChannel::HandleControlTransfer(USB_ControlStage stage, const USB_ControlRequest& request)
{
    switch (USB_CDC_ManagementRequest(request.bRequest))
    {
        case USB_CDC_ManagementRequest::SET_LINE_CODING:
            if (stage == USB_ControlStage::SETUP)
            {
                kernel_log(LogCategoryUSBDevice, KLogSeverity::INFO_LOW_VOL, "USBD: CDC set line coding.\n");
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
                kernel_log(LogCategoryUSBDevice, KLogSeverity::INFO_HIGH_VOL, "USBD: CDC get line coding.\n");
                return m_DeviceHandler->GetControlEndpointHandler().SendControlDataReply(request, &m_LineCoding, sizeof(m_LineCoding));
            }
            break;

        case USB_CDC_ManagementRequest::SET_CONTROL_LINE_STATE:
            if (stage == USB_ControlStage::SETUP)
            {
                m_DTR = (request.wValue & USB_DTE_LINE_CONTROL_STATE_DTE_PRESENT) != 0;
                m_RTS = (request.wValue & USB_DTE_LINE_CONTROL_STATE_CARRIER_ACTIVE) != 0;

                kernel_log(LogCategoryUSBDevice, KLogSeverity::INFO_HIGH_VOL, "USBD: CDC Set control line state: DTR = %d, RTS = %d.\n", m_DTR, m_RTS);

                SignalControlLineStateChanged(m_DTR, m_RTS);
                return m_DeviceHandler->GetControlEndpointHandler().SendControlStatusReply(request);
            }
            break;
        case USB_CDC_ManagementRequest::SEND_BREAK:
            if (stage == USB_ControlStage::SETUP)
            {
                return m_DeviceHandler->GetControlEndpointHandler().SendControlStatusReply(request);
            }
            else if (stage == USB_ControlStage::ACK)
            {
                kernel_log(LogCategoryUSBDevice, KLogSeverity::INFO_LOW_VOL, "USBD: CDC Send break.\n");
                if (request.wValue != 0xffff) {
                    SignalBreak(TimeValNanos::FromMilliseconds(request.wValue));
                }
                else {
                    SignalBreak(TimeValNanos::infinit);
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

bool USBClientCDCChannel::HandleDataTransfer(uint8_t endpointAddr, USB_TransferResult result, uint32_t length)
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

uint32_t USBClientCDCChannel::FlushInternal()
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

bool USBClientCDCChannel::StartOutTransaction()
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
