// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 10.08.2022 19:30


#pragma once

#include <Signals/Signal.h>
#include <optional>
#include <Kernel/USB/ClassDrivers/USBCDCBuffers.h>

#include <Kernel/KNamedObject.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/USB/USBProtocolCDC.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>

struct USB_DescriptorHeader;
struct USB_DescInterface;
struct USB_DescInterfaceAssociation;


namespace kernel
{
class USBHost;
class USBHostClassCDC;


class USBHostCDCChannel : public KInode, public KFilesystemFileOps
{
public:
    USBHostCDCChannel(USBHost* hostHandler, USBHostClassCDC* classDriver);

    // From KNamedObject:
    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

    const USB_DescriptorHeader* Open(uint8_t deviceAddr, int channelIndex, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc);
    void Close();
    void Startup();

    uint8_t GetDeviceAddress() const { return m_DeviceAddress; }
    bool    IsActive() const { return m_IsActive; }

    virtual void    CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;
    virtual size_t  Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t  Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void    Sync(Ptr<KFileNode> file) override;
    virtual void    ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;
    virtual void    DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

    bool SetLineCoding(const USB_CDC_LineCoding& lineCoding);
    const USB_CDC_LineCoding& GetLineCoding() const;

    Signal<void, USBHostClassCDC*, const USB_CDC_LineCoding&>  SignalLineCodingChanged;

private:
    friend class USBHostCDCReceiveTest;

    void    ReqGetLineCoding(USB_CDC_LineCoding* linecoding);
    void    ReqSetLineCoding(USB_CDC_LineCoding* linecoding);
    void    FlushInternal_pl();
    void    StartReceive_pl();
    void    SubmitTransmit_pl();
    void    SubmitReceive_pl();

    void    HandleSetLineCodingResult(bool result, uint8_t deviceAddr);
    void    HandleEndpointHaltResult(bool result, uint8_t deviceAddr);

    void    SendTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    void    ReceiveTransactionCallback(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);


    USBHost*            m_HostHandler = nullptr;
    USBHostClassCDC*    m_ClassDriver = nullptr;

    int                 m_DevNodeHandle = -1;

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

    std::optional<USBCDCBuffers> m_Buffers;
    uint8_t* m_TransmitBuffer = nullptr;
    uint8_t* m_ReceiveBuffer = nullptr;
    size_t m_TransmitLength = 0;
    bool m_TransmitError = false;
    bool m_ReceiveError = false;

};

} // namespace kernel
