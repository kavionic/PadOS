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


#pragma once

#include <Signals/Signal.h>
#include <Utils/CircularBuffer.h>

#include <Kernel/KNamedObject.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/USB/USBProtocolCDC.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/VFS/KINode.h>
#include <Kernel/VFS/KFilesystem.h>

struct USB_DescriptorHeader;
struct USB_DescInterface;
struct USB_DescInterfaceAssociation;


namespace kernel
{
class USBHost;
class USBHostClassCDC;

enum class USB_URBState : uint8_t;


class USBHostCDCChannel : public KINode, public KFilesystemFileOps
{
public:
    IFLASHC USBHostCDCChannel(USBHost* hostHandler, USBHostClassCDC* classDriver);

    // From KNamedObject:
    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

    IFLASHC const USB_DescriptorHeader* Open(uint8_t deviceAddr, int channelIndex, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc);
    IFLASHC void Close();
    IFLASHC void Startup();


    IFLASHC ssize_t GetReadBytesAvailable() const;
    IFLASHC virtual size_t  Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    IFLASHC virtual size_t  Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    IFLASHC virtual void    Sync(Ptr<KFileNode> file) override;
    IFLASHC virtual void    ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* result) override;
    IFLASHC virtual void    DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

    IFLASHC bool SetLineCoding(const USB_CDC_LineCoding& lineCoding);
    IFLASHC const USB_CDC_LineCoding& GetLineCoding() const;

    Signal<void, USBHostClassCDC*, const USB_CDC_LineCoding&>  SignalLineCodingChanged;

private:
    IFLASHC void    ReqGetLineCoding(USB_CDC_LineCoding* linecoding);
    IFLASHC void    ReqSetLineCoding(USB_CDC_LineCoding* linecoding);
    IFLASHC void    FlushInternal();

    IFLASHC void    HandleSetLineCodingResult(bool result, uint8_t deviceAddr);
    IFLASHC void    HandleEndpointHaltResult(bool result, uint8_t deviceAddr);

    IFLASHC void    SendTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    IFLASHC void    ReceiveTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);


    USBHost*            m_HostHandler = nullptr;
    USBHostClassCDC*    m_ClassDriver = nullptr;

    int                 m_DevNodeHandle = -1;
    TimeValNanos        m_CreateTime;

    uint8_t             m_DeviceAddress = 0;
    volatile bool       m_IsActive      = false;
    USB_CDC_LineCoding  m_LineCoding;

    USB_PipeIndex       m_NotificationPipe  = USB_INVALID_PIPE;
    USB_PipeIndex       m_DataPipeOut       = USB_INVALID_PIPE;
    USB_PipeIndex       m_DataPipeIn        = USB_INVALID_PIPE;

    uint8_t             m_NotificationEndpoint  = USB_INVALID_ENDPOINT;
    uint8_t             m_DataEndpointOut       = USB_INVALID_ENDPOINT;
    uint8_t             m_DataEndpointIn        = USB_INVALID_ENDPOINT;

    size_t              m_NotificationEndpointSize  = 0;
    size_t              m_DataEndpointOutSize       = 0;
    size_t              m_DataEndpointInSize        = 0;

    KConditionVariable  m_ReceiveCondition;
    KConditionVariable  m_TransmitCondition;

    size_t              m_CurrentTxTransactionLength = 0;

    std::vector<uint8_t> m_OutEndpointBuffer;
    std::vector<uint8_t> m_InEndpointBuffer;

    CircularBuffer<uint8_t, 1024, void> m_ReceiveFIFO;
    CircularBuffer<uint8_t, 1024, void> m_TransmitFIFO;

};

} // namespace kernel


