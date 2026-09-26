// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 03.01.2020 12:00:00

#pragma once

#include "System/Platform.h"
#include "Kernel/IRQDispatcher.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"
#include "Kernel/VFS/KInode.h"
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

class USARTDriverInode : public KInode, public KFilesystemFileOps
{
public:
    USARTDriverInode(const USARTDriverParameters& parameters);

    virtual size_t  Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t  Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void    DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
    virtual void    ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

    virtual bool    AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

private:
    static constexpr size_t RECEIVE_BUFFER_SIZE = 8 * 1024;
    static constexpr size_t RECEIVE_CHUNK_SIZE = 512;
    static constexpr uint32_t RECEIVE_DMA_INTERRUPT_FLAGS = DMA_LISR_TCIF0 | DMA_LISR_TEIF0 | DMA_LISR_DMEIF0;
    static_assert((RECEIVE_BUFFER_SIZE & (RECEIVE_BUFFER_SIZE - 1)) == 0);
    static_assert(RECEIVE_BUFFER_SIZE % RECEIVE_CHUNK_SIZE == 0);
    static_assert(RECEIVE_CHUNK_SIZE % DCACHE_LINE_SIZE == 0);

    void InitializeDMA();
    void InitializeReceiveDMA();
    void ConfigureReceiveDMA();
    void SetBaudrate(int baudrate);
    void SetIOControl(uint32_t flags);

    bool SetPinMode(const PinMuxTarget& pin, USARTPinMode mode);

    void SetSwapRXTX(bool doSwap);
    bool GetSwapRXTX() const;

    // Thread-side callers hold m_ReceiveDMAIRQ with KIRQGuard, which also masks the normal-latency USART IRQ.
    void    RefreshReceiveDMA();
    void    UpdateReceiveDMA();
    void    PublishReceivePosition(size_t position);
    void    QueueNextReceiveChunk();
    void    ResumeReceiveDMA();
    void    FailReceiveDMA();
    size_t  ReadReceiveBuffer(void* buffer, size_t length);

    static IRQResult IRQCallbackReceive(IRQn_Type irq, void* userData);
    static void DeferredIRQCallbackReceive(IRQn_Type irq, void* userData);
    IRQResult HandleIRQReceive();
    void HandleDeferredIRQReceive();
    static IRQResult IRQCallbackUSART(IRQn_Type irq, void* userData);
    IRQResult HandleIRQUSART();
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
    IRQn_Type       m_ReceiveDMAIRQ = static_cast<IRQn_Type>(-1);
    IRQn_Type       m_USARTIRQ = static_cast<IRQn_Type>(-1);
    int             m_ReceiveDMAChannel = -1;
    int             m_SendDMAChannel = -1;
    DMA_Stream_TypeDef* m_ReceiveDMAStream = nullptr;
    uint8_t*        m_ReceiveBuffer = nullptr;

    // Unsigned positions advance monotonically modulo size_t; the buffer size divides the counter range.
    volatile size_t m_ReceiveReadPosition = 0;
    volatile size_t m_ReceivePublishedPosition = 0;
    volatile size_t m_ReceiveCompletedPosition = 0;
    volatile uint32_t m_ReceiveActiveTarget = 0;
    volatile bool m_ReceivePaused = false;
    volatile bool m_ReceiveError = false;
    volatile bool m_ReceiveServicePending = false;
    volatile bool m_ReceiveWakeupPending = false;
};

} // namespace
