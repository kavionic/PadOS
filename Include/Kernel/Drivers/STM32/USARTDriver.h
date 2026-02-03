// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
#include <Kernel/VFS/KDriverParametersBase.h>
#include "Kernel/HAL/STM32/DMARequestID.h"
#include "Kernel/HAL/DigitalPort.h"
#include "DeviceControl/USART.h"

enum class USARTID : int;


struct USARTDriverParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "usart";

    USARTDriverParameters() = default;
    USARTDriverParameters(
        const PString&  devicePath,
        USARTID         portID,
        PinMuxTarget    pinRX,
        PinMuxTarget    pinTX
    )
        : KDriverParametersBase(devicePath),
        PortID(portID),
        PinRX(pinRX),
        PinTX(pinTX)
    {}

    USARTID         PortID;
    PinMuxTarget    PinRX;
    PinMuxTarget    PinTX;

    friend void to_json(Pjson& data, const USARTDriverParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
        data.update(Pjson{
            {"port_id",         value.PortID },
            {"pin_rx",          value.PinRX },
            {"pin_tx",          value.PinTX }
        });
    }
    friend void from_json(const Pjson& data, USARTDriverParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));

        data.at("port_id").get_to(outValue.PortID);
        data.at("pin_rx").get_to(outValue.PinRX);
        data.at("pin_tx").get_to(outValue.PinTX);
    }

};

namespace kernel
{

class USARTDriver;

class USARTDriverINode : public KINode, public KFilesystemFileOps
{
public:
    USARTDriverINode(const USARTDriverParameters& parameters);

    virtual size_t  Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t  Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void    DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual void    ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> inode, struct stat* statBuf) override;

    virtual bool    AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

private:
    void SetBaudrate(int baudrate);
    void SetIOControl(uint32_t flags);

    bool SetPinMode(const PinMuxTarget& pin, USARTPinMode mode);

    void SetSwapRXTX(bool doSwap);
    bool GetSwapRXTX() const;

    bool    RestartReceiveDMA(size_t maxLength);
    size_t  ReadReceiveBuffer(Ptr<KFileNode> file, void* buffer, const size_t length);

    static IRQResult IRQCallbackReceive(IRQn_Type irq, void* userData);
    IRQResult HandleIRQReceive();
    static IRQResult IRQCallbackSend(IRQn_Type irq, void* userData);
    IRQResult HandleIRQSend();

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
    TimeValNanos    m_ReadTimeout = TimeValNanos::infinit;
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

} // namespace
