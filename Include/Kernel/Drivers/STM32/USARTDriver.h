// This file is part of PadOS.
//
// Copyright (C) 2020-2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 03.01.2020 12:00:00

#pragma once

#include "System/Platform.h"
#include "Kernel/IRQDispatcher.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/VFS/KINode.h"
#include "Kernel/VFS/KFilesystem.h"
#include "Kernel/HAL/STM32/DMARequestID.h"
#include "Kernel/HAL/DigitalPort.h"
#include "DeviceControl/USART.h"

namespace kernel
{

class USARTDriver;
enum class USARTID : int;

class USARTDriverINode : public KINode
{
public:
    IFLASHC USARTDriverINode(USARTID             portID,
                             const PinMuxTarget& pinRX,
                             const PinMuxTarget& pinTX,
                             uint32_t            clockFrequency,
                             USARTDriver*        driver);

    IFLASHC ssize_t Read(Ptr<KFileNode> file, void* buffer, size_t length);
    IFLASHC ssize_t Write(Ptr<KFileNode> file, const void* buffer, size_t length);
    IFLASHC virtual bool    AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;
    IFLASHC int     DeviceControl(int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

private:
    IFLASHC void SetBaudrate(int baudrate);
    IFLASHC void SetIOControl(uint32_t flags);

    IFLASHC bool SetPinMode(const PinMuxTarget& pin, USARTPinMode mode);

    IFLASHC void SetSwapRXTX(bool doSwap);
    IFLASHC bool GetSwapRXTX() const;

    IFLASHC bool    RestartReceiveDMA(size_t maxLength);
    IFLASHC ssize_t ReadReceiveBuffer(Ptr<KFileNode> file, void* buffer, const size_t length);

    static IFLASHC IRQResult IRQCallbackReceive(IRQn_Type irq, void* userData);
    IFLASHC IRQResult HandleIRQReceive();
    static IFLASHC IRQResult IRQCallbackSend(IRQn_Type irq, void* userData);
    IFLASHC IRQResult HandleIRQSend();

    Ptr<USARTDriver> m_Driver;

    KMutex m_MutexRead;
    KMutex m_MutexWrite;

    KConditionVariable m_ReceiveCondition;
    KConditionVariable m_TransmitCondition;


    USART_TypeDef*  m_Port;
    PinMuxTarget    m_PinRX;
    PinMuxTarget    m_PinTX;
    DMAMUX_REQUEST  m_DMARequestRX;
    DMAMUX_REQUEST  m_DMARequestTX;

    int             m_ClockFrequency = 0;
    int             m_Baudrate = 0;
    USARTPinMode    m_PinModeRX = USARTPinMode::Normal;
    USARTPinMode    m_PinModeTX = USARTPinMode::Normal;
    uint32_t        m_IOControl = 0;
    TimeValMicros   m_ReadTimeout = TimeValMicros::infinit;
    int             m_ReceiveDMAChannel = -1;
    int             m_SendDMAChannel = -1;
    int32_t         m_ReceiveBufferSize = 1024 * 8;
    int32_t         m_ReceiveBufferOutPos = 0;
    volatile int32_t         m_ReceiveBufferInPos = 0;
    volatile int32_t         m_PendingReceiveBytes = 0;
    volatile std::atomic_int32_t         m_ReceiveBytesInBuffer = 0;
    uint8_t* m_ReceiveBuffer;

    volatile bool m_ReceiveTransactionActive = false;
};

struct USARTDriverSetup
{
    os::String      DevicePath;
    USARTID         PortID;
    PinMuxTarget    PinRX;
    PinMuxTarget    PinTX;
    uint32_t        ClockFrequency;
};

class USARTDriver : public PtrTarget, public KFilesystemFileOps
{
public:
    IFLASHC USARTDriver();

    IFLASHC void Setup( const char*         devicePath,
                USARTID             portID,
                const PinMuxTarget& pinRX,
                const PinMuxTarget& pinTX,
                uint32_t            clockFrequency);

    IFLASHC void Setup(const USARTDriverSetup& setup);

    IFLASHC virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    IFLASHC virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    IFLASHC virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    USARTDriver(const USARTDriver &other) = delete;
    USARTDriver& operator=(const USARTDriver &other) = delete;
};

} // namespace
