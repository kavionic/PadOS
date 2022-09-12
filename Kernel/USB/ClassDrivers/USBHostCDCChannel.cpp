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
// Created: 10.08.2022 19:30


#include <DeviceControl/USART.h>
#include <Utils/Utils.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/ClassDrivers/USBHostCDCChannel.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>

using namespace os;

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostCDCChannel::USBHostCDCChannel(USBHost* hostHandler, USBHostClassCDC* classDriver)
    : KINode(nullptr, nullptr, this, false)
    , m_HostHandler(hostHandler)
    , m_ClassDriver(classDriver)
    , m_ReceiveCondition("usbhcdc_receive")
    , m_TransmitCondition("usbhcdc_transmit")
{
    m_CreateTime = get_real_time();

    m_LineCoding.dwDTERate   = 9600;
    m_LineCoding.bCharFormat = USB_CDC_LineCodingStopBits::StopBits1;
    m_LineCoding.bParityType = USB_CDC_LineCodingParity::None;
    m_LineCoding.bDataBits   = 8;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostCDCChannel::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    if (m_IsActive)
    {
        kassert(!m_HostHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_HostHandler->GetMutex());

        switch (mode)
        {
            case ObjectWaitMode::Read:
                if (m_ReceiveFIFO.GetLength() == 0) {
                    return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read);
                }
                else {
                    return false; // Will not block.
                }
            case ObjectWaitMode::Write:
                if (m_TransmitFIFO.GetRemainingSpace() == 0) {
                    return m_TransmitCondition.AddListener(waitNode, ObjectWaitMode::Read);
                }
                else {
                    return false; // Will not block.
                }
            case ObjectWaitMode::ReadWrite:
                if (m_ReceiveFIFO.GetLength() == 0 && m_TransmitFIFO.GetRemainingSpace() == 0) {
                    return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read) && m_TransmitCondition.AddListener(waitNode, ObjectWaitMode::Read);
                }
                else {
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

const USB_DescriptorHeader* USBHostCDCChannel::Open(uint8_t deviceAddr, int channelIndex, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc)
{
    m_NotificationPipe          = USB_INVALID_PIPE;
    m_NotificationEndpoint      = USB_INVALID_ENDPOINT;
    m_NotificationEndpointSize  = 0;

    m_DataPipeIn            = USB_INVALID_PIPE;
    m_DataPipeOut           = USB_INVALID_PIPE;
    m_DataEndpointOut       = USB_INVALID_ENDPOINT;
    m_DataEndpointIn        = USB_INVALID_ENDPOINT;
    m_DataEndpointOutSize   = 0;
    m_DataEndpointInSize    = 0;

    m_CurrentTxTransactionLength = 0;

    m_DeviceAddress = deviceAddr;

    if (interfaceDesc->bInterfaceClass != USB_ClassCode::CDC || USB_CDC_CommSubclassType(interfaceDesc->bInterfaceSubClass) != USB_CDC_CommSubclassType::ABSTRACT_CONTROL_MODEL) {
        return nullptr;
    }

    USBDeviceNode* device = m_HostHandler->GetDevice(deviceAddr);
    if (device == nullptr) {
        return nullptr;
    }

    const USB_DescriptorHeader* desc = interfaceDesc->GetNext();

    m_NotificationEndpoint  = USB_INVALID_ENDPOINT;
    m_DataEndpointIn        = USB_INVALID_ENDPOINT;
    m_DataEndpointOut       = USB_INVALID_ENDPOINT;

    for (; desc < endDesc; desc = desc->GetNext())
    {
        if (desc->bDescriptorType == USB_DescriptorType::INTERFACE || desc->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION) {
            break;
        }
        else if (desc->bDescriptorType != USB_DescriptorType::ENDPOINT) {
            continue;
        }
        const USB_DescEndpoint* endpointDesc = static_cast<const USB_DescEndpoint*>(desc);
        if (!endpointDesc->Validate(device->m_Speed)) {
            return nullptr;
        }
        if (endpointDesc->bEndpointAddress & USB_ADDRESS_DIR_IN)
        {
            m_NotificationEndpoint = endpointDesc->bEndpointAddress;
            m_NotificationEndpointSize = endpointDesc->wMaxPacketSize;
            break;
        }
    }
    if (m_NotificationEndpoint == USB_INVALID_ENDPOINT)
    {
        return nullptr;
    }

    for (; desc < endDesc; desc = desc->GetNext())
    {
        if (desc->bDescriptorType == USB_DescriptorType::INTERFACE) {
            break;
        }
    }

    interfaceDesc = static_cast<const USB_DescInterface*>(desc);
    if (desc >= endDesc || interfaceDesc->bInterfaceClass != USB_ClassCode::CDC_DATA) {
        kernel_log(LogCategoryUSBHost, KLogSeverity::WARNING, "USBH: CDC cannot find the interface for data interface.\n");
        return nullptr; // No data interface;
    }

    for (desc = desc->GetNext(); desc < endDesc; desc = desc->GetNext())
    {
        if (desc->bDescriptorType == USB_DescriptorType::INTERFACE || desc->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION) {
            break;
        } else if (desc->bDescriptorType != USB_DescriptorType::ENDPOINT) {
            continue;
        }
        const USB_DescEndpoint* endpointDesc = static_cast<const USB_DescEndpoint*>(desc);
        if (!endpointDesc->Validate(device->m_Speed)) {
            return nullptr;
        }
        if (endpointDesc->bEndpointAddress & USB_ADDRESS_DIR_IN)
        {
            m_DataEndpointIn = endpointDesc->bEndpointAddress;
            m_DataEndpointInSize = endpointDesc->wMaxPacketSize;
        }
        else
        {
            m_DataEndpointOut = endpointDesc->bEndpointAddress;
            m_DataEndpointOutSize = endpointDesc->wMaxPacketSize;
        }
        if (m_DataEndpointIn != USB_INVALID_ENDPOINT && m_DataEndpointOut != USB_INVALID_ENDPOINT) {
            break;
        }
    }
    if (m_DataEndpointIn == USB_INVALID_ENDPOINT || m_DataEndpointOut == USB_INVALID_ENDPOINT)
    {
        kernel_log(LogCategoryUSBHost, KLogSeverity::WARNING, "USBH: CDC cannot find the endpoints for data interface.\n");
        return nullptr;
    }

    m_NotificationPipe  = m_HostHandler->AllocPipe(m_NotificationEndpoint);
    m_DataPipeOut       = m_HostHandler->AllocPipe(m_DataEndpointOut);
    m_DataPipeIn        = m_HostHandler->AllocPipe(m_DataEndpointIn);

    m_HostHandler->OpenPipe(m_NotificationPipe, m_NotificationEndpoint, device->m_Address, device->m_Speed, USB_TransferType::INTERRUPT, m_NotificationEndpointSize);
    m_HostHandler->OpenPipe(m_DataPipeOut,      m_DataEndpointOut,      device->m_Address, device->m_Speed, USB_TransferType::BULK, m_DataEndpointOutSize);
    m_HostHandler->OpenPipe(m_DataPipeIn,       m_DataEndpointIn,       device->m_Address, device->m_Speed, USB_TransferType::BULK, m_DataEndpointInSize);

    m_HostHandler->SetDataToggle(m_NotificationPipe, false);
    m_HostHandler->SetDataToggle(m_DataPipeOut, false);
    m_HostHandler->SetDataToggle(m_DataPipeIn, false);

    m_OutEndpointBuffer.resize(m_DataEndpointOutSize);
    m_InEndpointBuffer.resize(m_DataEndpointInSize);

    if (m_DevNodeHandle != -1 ) {
        Kernel::RemoveDevice(m_DevNodeHandle);
    }
    m_DevNodeHandle = Kernel::RegisterDevice(String::format_string("com/uhp%u", channelIndex).c_str(), ptr_tmp_cast(this));

    return desc;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::Close()
{
    if (m_NotificationPipe != USB_INVALID_PIPE)
    {
        m_HostHandler->ClosePipe(m_NotificationPipe);
        m_HostHandler->FreePipe(m_NotificationPipe);
        m_NotificationPipe = USB_INVALID_PIPE;
    }

    if (m_DataPipeIn != USB_INVALID_PIPE)
    {
        m_HostHandler->ClosePipe(m_DataPipeIn);
        m_HostHandler->FreePipe(m_DataPipeIn);
        m_DataPipeIn = USB_INVALID_PIPE;
    }

    if (m_DataPipeOut != USB_INVALID_PIPE)
    {
        m_HostHandler->ClosePipe(m_DataPipeOut);
        m_HostHandler->FreePipe(m_DataPipeOut);
        m_DataPipeOut = USB_INVALID_PIPE;
    }
    m_OutEndpointBuffer.clear();
    m_InEndpointBuffer.clear();

    m_IsActive = false;

    if (m_DevNodeHandle != -1) {
        Kernel::RemoveDevice(m_DevNodeHandle);
        m_DevNodeHandle = -1;
    }

    m_ReceiveCondition.WakeupAll();
    m_TransmitCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::Startup()
{
    m_IsActive = true;
    const size_t maxLength = std::min(m_ReceiveFIFO.GetRemainingSpace(), m_InEndpointBuffer.size());
    m_HostHandler->BulkReceiveData(m_DataPipeIn, m_InEndpointBuffer.data(), maxLength, bind_method(this, &USBHostCDCChannel::ReceiveTransactionCallback));

    ReqSetLineCoding(&m_LineCoding);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBHostCDCChannel::GetReadBytesAvailable() const
{
    if (m_IsActive)
    {
        kassert(!m_HostHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_HostHandler->GetMutex());
        return m_ReceiveFIFO.GetLength();
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USBHostCDCChannel::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    if (m_IsActive)
    {
        kassert(!m_HostHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_HostHandler->GetMutex());

        if (m_ReceiveFIFO.GetLength() == 0)
        {
            if ((file->GetOpenFlags() & O_NONBLOCK) == 0)
            {
                while (m_ReceiveFIFO.GetLength() == 0) {
                    m_ReceiveCondition.Wait(m_HostHandler->GetMutex());
                }
            }
            else
            {
                return 0;
            }
        }
        const ssize_t result = m_ReceiveFIFO.Read(buffer, length);
        if (result != 0)
        {
            if (m_HostHandler->GetURBState(m_DataPipeIn) == USB_URBState::Idle)
            {
                const size_t maxLength = std::min(m_ReceiveFIFO.GetRemainingSpace(), m_InEndpointBuffer.size());
                if (maxLength > 0) {
                    m_HostHandler->BulkReceiveData(m_DataPipeIn, m_InEndpointBuffer.data(), maxLength, bind_method(this, &USBHostCDCChannel::ReceiveTransactionCallback));
                }
            }
        }
        return result;
    }
    set_last_error(EPIPE);
    return -1;
}

/////////////////////////////////////////////////////////////////////////////////
///// \author Kurt Skauen
/////////////////////////////////////////////////////////////////////////////////

ssize_t USBHostCDCChannel::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    if (m_IsActive)
    {
        kassert(!m_HostHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_HostHandler->GetMutex());

        if (m_TransmitFIFO.GetRemainingSpace() == 0)
        {
            if ((file->GetOpenFlags() & O_NONBLOCK) == 0)
            {
                while (m_TransmitFIFO.GetRemainingSpace() == 0)
                {
                    if (!m_TransmitCondition.Wait(m_HostHandler->GetMutex()) && get_last_error() != EINTR) {
                        return -1;
                    }
                    if (!m_IsActive)
                    {
                        set_last_error(EPIPE);
                        return -1;
                    }
                }
            }
            else
            {
                return 0;
            }
        }
        const size_t result = m_TransmitFIFO.Write(buffer, std::min(m_TransmitFIFO.GetRemainingSpace(), length));

        if ((file->GetOpenFlags() & (O_SYNC | O_DIRECT)) || m_TransmitFIFO.GetLength() >= m_InEndpointBuffer.size()) {
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

int USBHostCDCChannel::ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* outStats)
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
    outStats->st_atime = outStats->st_mtime = outStats->st_ctime = m_CreateTime.AsSecondsI();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHostCDCChannel::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    switch (request)
    {
        case USARTIOCTL_SET_BAUDRATE:
            if (inDataLength == sizeof(int))
            {
                const int baudrate = *((const int*)inData);
                USB_CDC_LineCoding linecoding = m_LineCoding;
                linecoding.dwDTERate = baudrate;
                SetLineCoding(linecoding);
                return 0;
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_GET_BAUDRATE:
            if (outDataLength == sizeof(int))
            {
                int* baudrate = (int*)outData;
                *baudrate = m_LineCoding.dwDTERate;
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

ssize_t USBHostCDCChannel::Sync(Ptr<KFileNode> file)
{
    if (m_IsActive)
    {
        kassert(!m_HostHandler->GetMutex().IsLocked());
        CRITICAL_SCOPE(m_HostHandler->GetMutex());
        return FlushInternal();
    }
    set_last_error(EPIPE);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::ReqGetLineCoding(USB_CDC_LineCoding* linecoding)
{
    USB_ControlRequest request(
        USB_RequestRecipient::INTERFACE,
        USB_RequestType::CLASS,
        USB_RequestDirection::DEVICE_TO_HOST,
        uint8_t(USB_CDC_ManagementRequest::GET_LINE_CODING),
        0,
        0,
        sizeof(USB_CDC_LineCoding)
    );
    m_HostHandler->GetControlHandler().SendControlRequest(m_DeviceAddress, request, linecoding, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::ReqSetLineCoding(USB_CDC_LineCoding* linecoding)
{
    USB_ControlRequest request(
        USB_RequestRecipient::INTERFACE,
        USB_RequestType::CLASS,
        USB_RequestDirection::HOST_TO_DEVICE,
        uint8_t(USB_CDC_ManagementRequest::SET_LINE_CODING),
        0,
        0,
        sizeof(USB_CDC_LineCoding)
    );
    m_HostHandler->GetControlHandler().SendControlRequest(m_DeviceAddress, request, linecoding, bind_method(this, &USBHostCDCChannel::HandleSetLineCodingResult));
}

/////////////////////////////////////////////////////////////////////////////////
///// \author Kurt Skauen
/////////////////////////////////////////////////////////////////////////////////

ssize_t USBHostCDCChannel::FlushInternal()
{
    kassert(m_HostHandler->GetMutex().IsLocked());

    if (m_HostHandler->GetURBState(m_DataPipeOut) == USB_URBState::Idle)
    {
        size_t length = std::min(m_OutEndpointBuffer.size(), m_TransmitFIFO.GetLength());
        if (length > 0)
        {
            length = m_TransmitFIFO.Read(m_OutEndpointBuffer.data(), length);

            if (length > 0)
            {
                m_TransmitCondition.WakeupAll();
                m_CurrentTxTransactionLength = length;

                m_HostHandler->BulkSendData(m_DataPipeOut, m_OutEndpointBuffer.data(), m_CurrentTxTransactionLength, true, bind_method(this, &USBHostCDCChannel::SendTransactionCallback));
            }
        }
        return length;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
///// \author Kurt Skauen
/////////////////////////////////////////////////////////////////////////////////

bool USBHostCDCChannel::SetLineCoding(const USB_CDC_LineCoding& lineCoding)
{
    CRITICAL_SCOPE(m_HostHandler->GetMutex());

    m_LineCoding = lineCoding;

    if (m_IsActive) {
        ReqSetLineCoding(&m_LineCoding);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////
///// \author Kurt Skauen
/////////////////////////////////////////////////////////////////////////////////

const USB_CDC_LineCoding& USBHostCDCChannel::GetLineCoding() const
{
    return m_LineCoding;
}

/////////////////////////////////////////////////////////////////////////////////
///// \author Kurt Skauen
/////////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::HandleSetLineCodingResult(bool result, uint8_t deviceAddr)
{
    if (!result) {
        kernel_log(LogCategoryUSBHost, KLogSeverity::ERROR, "USBH: CDC set line coding configuration failed.\n");
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::HandleEndpointHaltResult(bool result, uint8_t deviceAddr)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::SendTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (urbState == USB_URBState::Done)
    {
        FlushInternal();
    }
    else if (urbState == USB_URBState::NotReady)
    {
        m_HostHandler->BulkSendData(m_DataPipeOut, m_OutEndpointBuffer.data(), m_CurrentTxTransactionLength, true, bind_method(this, &USBHostCDCChannel::SendTransactionCallback));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostCDCChannel::ReceiveTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (urbState == USB_URBState::Done)
    {
        if (transactionLength > 0)
        {
            m_ReceiveFIFO.Write(m_InEndpointBuffer.data(), transactionLength);
            m_ReceiveCondition.WakeupAll();
        }
        const size_t maxLength = std::min(m_ReceiveFIFO.GetRemainingSpace(), m_InEndpointBuffer.size());
        if (maxLength > 0) {
            m_HostHandler->BulkReceiveData(m_DataPipeIn, m_InEndpointBuffer.data(), maxLength, bind_method(this, &USBHostCDCChannel::ReceiveTransactionCallback));
        }
    }
}

} // namespace kernel
