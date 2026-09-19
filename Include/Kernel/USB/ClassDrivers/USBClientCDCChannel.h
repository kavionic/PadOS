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

#pragma once

#include <Signals/Signal.h>
#include <Kernel/USB/ClassDrivers/USBCDCBuffers.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/USB/USBProtocolCDC.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KInode.h>

namespace kernel
{
class USBDevice;

enum class USB_ControlStage : int;
enum class USB_TransferResult : uint8_t;

class USBClientCDCChannel : public KInode, public KFilesystemFileOps
{
public:
    USBClientCDCChannel(USBDevice* deviceHandler, uint8_t endpointNotification, uint8_t endpointOut, uint8_t endpointIn, uint16_t endpointOutMaxSize, uint16_t endpointInMaxSize);

    void Start_pl(int channelIndex);

    // From KNamedObject:
    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

    int Close();

    virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags) override;
    virtual void    CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;
    virtual size_t  Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t  Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void    Sync(Ptr<KFileNode> file) override;
    virtual void    ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;
    virtual void    DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

    uint8_t  GetEndpointNotifications() const { return m_EndpointNotifications; }
    uint8_t  GetEndpointOut() const           { return m_EndpointOut; }
    uint8_t  GetEndpointIn() const            { return m_EndpointIn; }


    bool     HandleControlTransfer(USB_ControlStage stage, const USB_ControlRequest& request);
    bool     HandleDataTransfer(uint8_t endpointAddr, USB_TransferResult result, uint32_t length);

    Signal<void, const USB_CDC_LineCoding&/*lineCoding*/>   SignalLineCodingChanged;
    Signal<void, bool/*DTR*/, bool/*RTS*/>                  SignalControlLineStateChanged;
    Signal<void, TimeValNanos/*duration*/>                  SignalBreak;

private:
    uint32_t    FlushInternal_pl();
    bool        StartOutTransaction_pl();

    USBDevice*          m_DeviceHandler;
    KConditionVariable  m_ReceiveCondition;
    KConditionVariable  m_TransmitCondition;

    int                 m_DevNodeHandle = -1; // Handle for out node in the "/dev/" filesystem.
    uint8_t             m_EndpointNotifications = 0;
    uint8_t             m_EndpointOut = 0;
    uint8_t             m_EndpointIn  = 0;

    bool                m_IsActive = true;
    bool                m_DTR = false;
    bool                m_RTS = false;

    USB_CDC_LineCoding  m_LineCoding;

    TimeValNanos    m_ReadTimeout = TimeValNanos::infinit;
    TimeValNanos    m_WriteTimeout = TimeValNanos::infinit;

    size_t m_ReceivePacketSize;
    size_t m_TransmitPacketSize;
    USBCDCBuffers m_Buffers;
    size_t m_TransmitLength = 0;
    bool m_ReceiveActive = false;
    bool m_TransmitActive = false;
    bool m_ReceiveError = false;
    bool m_TransmitError = false;
};


} // namespace kernel
