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
#include <Utils/CircularBuffer.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/USB/USBProtocolCDC.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KINode.h>

namespace kernel
{
class USBDevice;

enum class USB_ControlStage : int;
enum class USB_TransferResult : uint8_t;

class USBClientCDCChannel : public KINode, public KFilesystemFileOps
{
public:
    USBClientCDCChannel(USBDevice* deviceHandler, int channelIndex, uint8_t endpointNotification, uint8_t endpointOut, uint8_t endpointIn, uint16_t endpointOutMaxSize, uint16_t endpointInMaxSize);

    // From KNamedObject:
    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

    void Close();

    ssize_t  GetReadBytesAvailable() const;
    IFLASHC virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    IFLASHC virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    IFLASHC virtual int     Sync(Ptr<KFileNode> file) override;
    IFLASHC virtual int     ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* result) override;
    ssize_t  GetWriteBytesAvailable() const;

    uint8_t  GetEndpointNotifications() const { return m_EndpointNotifications; }
    uint8_t  GetEndpointOut() const           { return m_EndpointOut; }
    uint8_t  GetEndpointIn() const            { return m_EndpointIn; }


    bool     HandleControlTransfer(USB_ControlStage stage, const USB_ControlRequest& request);
    bool     HandleDataTransfer(uint8_t endpointAddr, USB_TransferResult result, uint32_t length);

    Signal<void, const USB_CDC_LineCoding&/*lineCoding*/>   SignalLineCodingChanged;
    Signal<void, bool/*DTR*/, bool/*RTS*/>                  SignalControlLineStateChanged;
    Signal<void, TimeValMicros/*duration*/>                 SignalBreak;

private:
    uint32_t    FlushInternal();
    bool        StartOutTransaction();

    USBDevice*          m_DeviceHandler;
    KConditionVariable  m_ReceiveCondition;
    KConditionVariable  m_TransmitCondition;

    TimeValMicros       m_CreateTime;

    int                 m_DevNodeHandle = -1; // Handle for out node in the "/dev/" filesystem.
    uint8_t             m_EndpointNotifications = 0;
    uint8_t             m_EndpointOut = 0;
    uint8_t             m_EndpointIn  = 0;

    volatile bool       m_IsActive = true;
    bool                m_DTR = false;
    bool                m_RTS = false;

    USB_CDC_LineCoding  m_LineCoding;


    CircularBuffer<uint8_t, 1024, void> m_ReceiveFIFO;
    CircularBuffer<uint8_t, 1024, void> m_TransmitFIFO;

    std::vector<uint8_t> m_OutEndpointBuffer;
    std::vector<uint8_t> m_InEndpointBuffer;
};


} // namespace kernel
