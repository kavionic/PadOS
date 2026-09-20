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

#include <sys/stat.h>

#include <PadOS/Time.h>
#include <Utils/String.h>
#include <Kernel/KTime.h>
#include <Kernel/USB/ClassDrivers/USBClientCDCChannel.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBDriver.h>
#include <Kernel/USB/USBClassDriverDevice.h>
#include <Kernel/USB/USBDevice.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KFileHandle.h>
#include <System/ExceptionHandling.h>
#include <DeviceControl/USART.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBClientCDCChannel::USBClientCDCChannel(USBDevice* deviceHandler, uint8_t endpointNotification, uint8_t endpointOut, uint8_t endpointIn, uint16_t endpointOutMaxSize, uint16_t endpointInMaxSize)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
    , m_DeviceHandler(deviceHandler)
    , m_ReceiveCondition("usbdcdc_receive")
    , m_TransmitCondition("usbdcdc_transmit")
    , m_EndpointNotifications(endpointNotification)
    , m_EndpointOut(endpointOut)
    , m_EndpointIn(endpointIn)
    , m_ReceivePacketSize(endpointOutMaxSize)
    , m_TransmitPacketSize(endpointInMaxSize)
    , m_Buffers(endpointOutMaxSize, endpointInMaxSize)
{
    m_ATime = m_MTime = m_CTime = kget_real_time();

    m_LineCoding.dwDTERate   = 115200;
    m_LineCoding.bCharFormat = USB_CDC_LineCodingStopBits::StopBits1;
    m_LineCoding.bParityType = USB_CDC_LineCodingParity::None;
    m_LineCoding.bDataBits   = 8;

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientCDCChannel::Start_pl(int channelIndex)
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());
    m_DevNodeHandle = kregister_device_root_trw(PString::format_string("com/udp{}", channelIndex).c_str(), ptr_tmp_cast(this));

    StartOutTransaction_pl();
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
                if (!m_ReceiveError && m_Buffers.GetReceiveQueue().GetLength() == 0) {
                    return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read);
                } else {
                    return false; // Will not block.
                }
            case ObjectWaitMode::Write:
                if (!m_TransmitError && m_Buffers.GetTransmitQueue().GetWriteSpace() == 0) {
                    return m_TransmitCondition.AddListener(waitNode, ObjectWaitMode::Read);
                } else {
                    return false; // Will not block.
                }
            case ObjectWaitMode::ReadWrite:
                if (!m_ReceiveError && !m_TransmitError && m_Buffers.GetReceiveQueue().GetLength() == 0 && m_Buffers.GetTransmitQueue().GetWriteSpace() == 0) {
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

int USBClientCDCChannel::Close()
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    const int devNodeHandle = m_DevNodeHandle;

    if (m_IsActive)
    {
        // Closing is synchronous: storage stays owned until endpoint DMA has stopped or the core is held in reset.
        m_DeviceHandler->CloseEndpoint(m_EndpointIn);
        m_DeviceHandler->CloseEndpoint(m_EndpointOut);
        if (m_EndpointNotifications != 0) {
            m_DeviceHandler->CloseEndpoint(m_EndpointNotifications);
        }
    }
    m_DevNodeHandle = -1;
    m_IsActive = false;
    m_ReceiveCondition.WakeupAll();
    m_TransmitCondition.WakeupAll();

    return devNodeHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> USBClientCDCChannel::OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

    if (!m_IsActive) {
        PERROR_THROW_CODE(PErrorCode::NODEV);
    }
    return KFilesystemFileOps::OpenFile(volume, inode, openFlags);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientCDCChannel::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    m_ReceiveCondition.WakeupAll();
    m_TransmitCondition.WakeupAll();
    KFilesystemFileOps::CloseFile(volume, file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBClientCDCChannel::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

    if (!file->HasReadAccess())
    {
        PERROR_THROW_CODE(PErrorCode::ACCES);
    }
    if (m_IsActive)
    {
        if (m_ReceiveError && m_Buffers.GetReceiveQueue().GetLength() == 0) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        if (m_Buffers.GetReceiveQueue().GetLength() == 0)
        {
            if ((file->GetOpenFlags() & O_NONBLOCK) == 0)
            {
                TimeValNanos deadline = m_ReadTimeout.IsInfinit() ? TimeValNanos::infinit : (kget_monotonic_time() + m_ReadTimeout);

                while (m_Buffers.GetReceiveQueue().GetLength() == 0)
                {
                    const PErrorCode result = m_ReceiveCondition.WaitDeadline(m_DeviceHandler->GetMutex(), deadline);
                    if (result != PErrorCode::Success && result != PErrorCode::INTR)
                    {
                        PERROR_THROW_CODE(result);
                    }
                    if (!m_IsActive)
                    {
                        PERROR_THROW_CODE(PErrorCode::PIPE);
                    }
                    if (m_ReceiveError && m_Buffers.GetReceiveQueue().GetLength() == 0) {
                        PERROR_THROW_CODE(PErrorCode::IO);
                    }
                    if (!file->HasReadAccess())
                    {
                        PERROR_THROW_CODE(PErrorCode::ACCES);
                    }
                }
            }
            else
            {
                return 0;
            }
        }
        const size_t result = m_Buffers.GetReceiveQueue().Read(buffer, length);
        if (result != 0) {
            StartOutTransaction_pl();
        }
        return result;
    }
    PERROR_THROW_CODE(PErrorCode::PIPE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBClientCDCChannel::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());

    if (!file->HasWriteAccess())
    {
        PERROR_THROW_CODE(PErrorCode::ACCES);
    }
    if (m_IsActive)
    {
        if (m_TransmitError) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        if (m_Buffers.GetTransmitQueue().GetWriteSpace() == 0)
        {
            if ((file->GetOpenFlags() & O_NONBLOCK) == 0)
            {
                TimeValNanos deadline = m_WriteTimeout.IsInfinit() ? TimeValNanos::infinit : (kget_monotonic_time() + m_WriteTimeout);

                while (m_Buffers.GetTransmitQueue().GetWriteSpace() == 0)
                {
                    FlushInternal_pl();
                    if (m_TransmitError) {
                        PERROR_THROW_CODE(PErrorCode::IO);
                    }
                    const PErrorCode result = m_TransmitCondition.WaitDeadline(m_DeviceHandler->GetMutex(), deadline);
                    if (result != PErrorCode::Success && result != PErrorCode::INTR)
                    {
                        PERROR_THROW_CODE(result);
                    }
                    if (!m_IsActive)
                    {
                        PERROR_THROW_CODE(PErrorCode::PIPE);
                    }
                    if (m_TransmitError) {
                        PERROR_THROW_CODE(PErrorCode::IO);
                    }
                    if (!file->HasWriteAccess())
                    {
                        PERROR_THROW_CODE(PErrorCode::ACCES);
                    }
                }
            }
            else
            {
                return 0;
            }
        }
        const size_t result = std::min(m_Buffers.GetTransmitQueue().GetWriteSpace(), length);
        m_Buffers.GetTransmitQueue().Write(buffer, result);

        if ((file->GetOpenFlags() & (O_SYNC | O_DIRECT))
            || m_Buffers.GetTransmitQueue().GetLength() - m_TransmitLength >= m_TransmitPacketSize) {
            FlushInternal_pl();
        }
        return result;
    }
    PERROR_THROW_CODE(PErrorCode::PIPE);
}

void USBClientCDCChannel::Sync(Ptr<KFileNode> file)
{
    kassert(!m_DeviceHandler->GetMutex().IsLocked());
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
    if (m_IsActive)
    {
        FlushInternal_pl();
        if (m_TransmitError) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        return;
    }
    PERROR_THROW_CODE(PErrorCode::PIPE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientCDCChannel::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientCDCChannel::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    switch (request)
    {
        case USARTIOCTL_SET_READ_TIMEOUT:
            if (inDataLength == sizeof(bigtime_t))
            {
                bigtime_t nanos = *((const bigtime_t*)inData);
                m_ReadTimeout = TimeValNanos::FromNanoseconds(nanos);
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_GET_READ_TIMEOUT:
            if (outDataLength == sizeof(bigtime_t))
            {
                bigtime_t* nanos = (bigtime_t*)outData;
                *nanos = m_ReadTimeout.AsNanoseconds();
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_SET_WRITE_TIMEOUT:
            if (inDataLength == sizeof(bigtime_t))
            {
                bigtime_t nanos = *((const bigtime_t*)inData);
                m_WriteTimeout = TimeValNanos::FromNanoseconds(nanos);
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_GET_WRITE_TIMEOUT:
            if (outDataLength == sizeof(bigtime_t))
            {
                bigtime_t* nanos = (bigtime_t*)outData;
                *nanos = m_WriteTimeout.AsNanoseconds();
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
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
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "CDC set line coding.");
                return m_DeviceHandler->GetControlEndpointHandler().ReceiveControlData(request, &m_LineCoding, sizeof(m_LineCoding));
            }
            else if (stage == USB_ControlStage::ACK)
            {
                SignalLineCodingChanged(m_LineCoding);
            }
            break;

        case USB_CDC_ManagementRequest::GET_LINE_CODING:
            if (stage == USB_ControlStage::SETUP)
            {
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "CDC get line coding.");
                return m_DeviceHandler->GetControlEndpointHandler().SendControlDataReply(request, &m_LineCoding, sizeof(m_LineCoding));
            }
            break;

        case USB_CDC_ManagementRequest::SET_CONTROL_LINE_STATE:
            if (stage == USB_ControlStage::SETUP)
            {
                m_DTR = (request.wValue & USB_DTE_LINE_CONTROL_STATE_DTE_PRESENT) != 0;
                m_RTS = (request.wValue & USB_DTE_LINE_CONTROL_STATE_CARRIER_ACTIVE) != 0;

                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCategoryUSBDevice, "CDC Set control line state: DTR = {}, RTS = {}.", m_DTR, m_RTS);

                return m_DeviceHandler->GetControlEndpointHandler().SendControlStatusReply(request);
            }
            else if (stage == USB_ControlStage::ACK)
            {
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
                kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "CDC Send break.");
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

    if (!m_IsActive) {
        return false;
    }
    if (endpointAddr == m_EndpointOut && m_ReceiveActive)
    {
        m_ReceiveActive = false;
        m_ReceiveHaltError = result == USB_TransferResult::Stalled;
        m_ReceiveError = result != USB_TransferResult::Success || length > m_ReceivePacketSize;
        m_Buffers.GetReceiveQueue().CompleteReceive(m_ReceiveError ? 0 : length);
        m_ReceiveCondition.WakeupAll();
        if (!m_ReceiveError) {
            StartOutTransaction_pl();
        }
        return !m_ReceiveError;
    }
    else if (endpointAddr == m_EndpointIn && m_TransmitActive)
    {
        m_TransmitActive = false;
        m_TransmitHaltError = result == USB_TransferResult::Stalled;
        m_TransmitError = result != USB_TransferResult::Success || length != m_TransmitLength;
        if (m_TransmitLength != 0) {
            // Cancellation does not report reliable progress. Drop only this block; never replay its possible prefix.
            m_Buffers.GetTransmitQueue().CompleteTransmit();
        }
        m_TransmitLength = 0;
        m_TransmitFlushPending = m_Buffers.GetTransmitQueue().GetLength() != 0;
        m_TransmitCondition.WakeupAll();
        // A canceled ZLP stays pending; it has no payload to replay and owns no queue block.
        if (!m_TransmitError)
        {
            m_TransmitZLPPending = length != 0 && length % m_TransmitPacketSize == 0;
            FlushInternal_pl();
        }
        return !m_TransmitError;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientCDCChannel::HandleEndpointHaltCleared(uint8_t endpointAddr)
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    if (m_IsActive) {
        if (endpointAddr == m_EndpointOut)
        {
            if (m_ReceiveHaltError)
            {
                m_ReceiveHaltError = false;
                m_ReceiveError = false;
            }
            StartOutTransaction_pl();
            m_ReceiveCondition.WakeupAll();
        }
        else if (endpointAddr == m_EndpointIn)
        {
            if (m_TransmitHaltError)
            {
                m_TransmitHaltError = false;
                m_TransmitError = false;
            }
            // Leave an idle short write buffered unless a flush was requested before or during the halt.
            if (m_TransmitFlushPending || m_TransmitZLPPending) {
                FlushInternal_pl();
            }
            m_TransmitCondition.WakeupAll();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t USBClientCDCChannel::FlushInternal_pl()
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    if (!m_IsActive || m_TransmitError) {
        return 0;
    }
    if (m_Buffers.GetTransmitQueue().GetLength() != 0) {
        m_TransmitFlushPending = true;
    }
    if (m_TransmitActive || !m_DeviceHandler->IsReady() || (!m_TransmitFlushPending && !m_TransmitZLPPending)) {
        return 0;
    }
    if (!m_DeviceHandler->ClaimEndpoint(m_EndpointIn)) {
        return 0;
    }
    // A pending ZLP owns no queue block. Later payload can provide its own short-packet/ZLP termination.
    uint8_t* storage = nullptr;
    if (m_TransmitFlushPending)
    {
        storage = m_Buffers.GetTransmitQueue().BeginTransmit(m_TransmitLength);
        kassert(storage != nullptr);
    }
    m_TransmitActive = m_DeviceHandler->EndpointTransfer(m_EndpointIn, storage, m_TransmitLength);
    if (!m_TransmitActive)
    {
        if (storage != nullptr) {
            m_Buffers.GetTransmitQueue().CancelTransmit();
        }
        m_TransmitLength = 0;
        m_TransmitError = true;
        m_TransmitCondition.WakeupAll();
    }
    else if (storage != nullptr)
    {
        m_TransmitZLPPending = false;
    }
    return m_TransmitLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientCDCChannel::StartOutTransaction_pl()
{
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    if (!m_IsActive || m_ReceiveError || m_ReceiveActive) {
        return false;
    }
    uint8_t* storage = m_Buffers.GetReceiveQueue().BeginReceive();
    if (storage == nullptr) {
        return false;
    }
    if (!m_DeviceHandler->ClaimEndpoint(m_EndpointOut))
    {
        m_Buffers.GetReceiveQueue().CompleteReceive(0);
        return false;
    }
    m_ReceiveActive = m_DeviceHandler->EndpointTransfer(
        m_EndpointOut, storage, m_ReceivePacketSize, m_Buffers.GetReceiveQueue().GetBlockSize());
    if (!m_ReceiveActive)
    {
        m_Buffers.GetReceiveQueue().CompleteReceive(0);
        m_ReceiveError = true;
        m_ReceiveCondition.WakeupAll();
    }
    return m_ReceiveActive;
}

} // namespace kernel
