// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 03.01.2020 12:00:00


#include <malloc.h>
#include <algorithm>
#include <cstring>

#include <Kernel/Drivers/STM32/USARTDriver.h>

#include <Ptr/Ptr.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>
#include <Utils/JSON.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KIRQGuard.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/HAL/DMA.h>
#include <Kernel/HAL/PeripheralMapping.h>

namespace kernel
{


PREGISTER_KERNEL_DRIVER(USARTDriverInode, USARTDriverParameters);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USARTDriverInode::USARTDriverInode(const USARTDriverParameters& parameters)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
    , m_MutexRead("USARTDriverInodeRead", PEMutexRecursionMode_RaiseError)
    , m_MutexWrite("USARTDriverInodeWrite", PEMutexRecursionMode_RaiseError)
    , m_ReceiveCondition("USARTDriverInodeReceive")
    , m_TransmitCondition("USARTDriverInodeTransmit")
    , m_PinRX(parameters.PinRX)
    , m_PinTX(parameters.PinTX)
{
    m_Port = get_usart_from_id(parameters.PortID);
    m_USARTIRQ = get_usart_irq(parameters.PortID);

    get_usart_dma_requests(parameters.PortID, m_DMARequestRX, m_DMARequestTX);

    DigitalPin::ActivatePeripheralMux(m_PinRX);
    DigitalPin::ActivatePeripheralMux(m_PinTX);
    
    m_Port->CR1 = USART_CR1_RE | USART_CR1_FIFOEN;
    m_Port->CR3 = USART_CR3_DMAR | USART_CR3_DMAT;

    m_ClockFrequency = get_usart_peripheral_clock_freq(parameters.PortID);
    SetBaudrate(921600);

    InitializeDMA();

    // All handlers and receive state are ready before the USART can produce DMA requests.
    KIRQGuard irqGuard(m_ReceiveDMAIRQ);
    m_Port->ICR = USART_ICR_IDLECF | USART_ICR_ORECF;
    dma_start(m_ReceiveDMAChannel);
    m_Port->CR1 |= USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_IDLEIE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USARTDriverInode::Read(Ptr<KFileNode> file, void* buffer, const size_t length, off64_t position)
{
    if (length == 0) {
        return 0;
    }
    CRITICAL_SCOPE(m_MutexRead);

    for (;;)
    {
        const size_t bytesRead = ReadReceiveBuffer(buffer, length);
        if (bytesRead != 0 || (file->GetOpenFlags() & O_NONBLOCK)) {
            return bytesRead;
        }

        PErrorCode result = PErrorCode::Success;
        {
            KIRQGuard irqGuard(m_ReceiveDMAIRQ);
            RefreshReceiveDMA();
            if (!m_ReceiveError && m_ReceivePublishedPosition == m_ReceiveReadPosition)
            {
                result = m_ReceiveCondition.IRQWaitTimeout(irqGuard, m_ReadTimeout);
                RefreshReceiveDMA();
            }

            if (m_ReceiveError)
            {
                result = PErrorCode::IO;
            }
            else if ((result == PErrorCode::TIMEDOUT || result == PErrorCode::INTR) &&
                m_ReceivePublishedPosition != m_ReceiveReadPosition)
            {
                result = PErrorCode::Success;
            }
        }
        if (result != PErrorCode::Success && result != PErrorCode::INTR) {
            PERROR_THROW_CODE(result);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USARTDriverInode::Write(Ptr<KFileNode> file, const void* buffer, const size_t length, off64_t position)
{
    const intptr_t startAddr = align_down<intptr_t>(intptr_t(buffer), DCACHE_LINE_SIZE);
    const intptr_t endAddr   = align_up<intptr_t>(intptr_t(buffer) + length, DCACHE_LINE_SIZE);
    SCB_CleanDCache_by_Addr(reinterpret_cast<intptr_t*>(startAddr), endAddr - startAddr);

    CRITICAL_SCOPE(m_MutexWrite);

    const uint8_t* currentTarget = reinterpret_cast<const uint8_t*>(buffer);
    size_t remainingLen = length;

    for (size_t currentLen = std::min(size_t(DMA_MAX_TRANSFER_LENGTH), remainingLen); remainingLen > 0; remainingLen -= currentLen, currentTarget += currentLen)
    {
        m_Port->ICR = USART_ICR_TCCF;

        dma_stop(m_SendDMAChannel);
        dma_setup(m_SendDMAChannel, DMADirection::MemToPeriph, m_DMARequestTX, &m_Port->TDR, currentTarget, currentLen);

        PErrorCode result;
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            dma_start(m_SendDMAChannel);
            result = m_TransmitCondition.IRQWaitTimeout(TimeValNanos::FromNanoseconds(bigtime_t(currentLen) * 10 * 2 * TimeValNanos::TicksPerSecond / m_Baudrate) + TimeValNanos::FromMilliseconds(100));
        } CRITICAL_END;
        if (result != PErrorCode::Success) {
            PERROR_THROW_CODE(result);
        }
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_MutexRead);
    CRITICAL_SCOPE(m_MutexWrite);

    while((m_Port->ISR & USART_ISR_TC) == 0);

    switch (request)
    {
        case USARTIOCTL_SET_BAUDRATE:
            if (inDataLength == sizeof(int))
            {
                int baudrate = *((const int*)inData);
                SetBaudrate(baudrate);
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_GET_BAUDRATE:
            if (outDataLength == sizeof(int))
            {
                int* baudrate = (int*)outData;
                *baudrate = m_Baudrate;
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
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
        case USARTIOCTL_SET_IOCTRL:
            if (inDataLength == sizeof(uint32_t))
            {
                uint32_t flags = *((const uint32_t*)inData);
                SetIOControl(flags);
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_GET_IOCTRL:
            if (outDataLength == sizeof(uint32_t))
            {
                uint32_t* flags = (uint32_t*)outData;
                *flags = m_IOControl;
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_SET_PINMODE:
            if (inDataLength == sizeof(int))
            {
                int arg = *((const int*)inData);
                USARTPin     pin  = USARTPin(arg >> 16);
                USARTPinMode mode = USARTPinMode(arg & 0xffff);

                switch (pin)
                {
                    case USARTPin::RX:
                        if (SetPinMode(m_PinRX, mode))
                        {
                            m_PinModeRX = mode;
                            m_Port->RQR = USART_RQR_RXFRQ;  // Flush receive buffer
                            return;
                        }
                        else
                        {
                            PERROR_THROW_CODE(PErrorCode::INVAL);
                        }
                    case USARTPin::TX:
                        if (SetPinMode(m_PinTX, mode))
                        {
                            m_PinModeTX = mode;
                            m_Port->RQR = USART_RQR_RXFRQ;  // Flush receive buffer
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
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_GET_PINMODE:
            if (inDataLength == sizeof(int) && outDataLength == sizeof(int))
            {
                int arg = *((const int*)inData);
                USARTPin     pin = USARTPin(arg);

                int* result = (int*)outData;

                switch (pin)
                {
                    case USARTPin::RX:
                        *result = int(m_PinModeRX);
                        return;
                    case USARTPin::TX:
                        *result = int(m_PinModeTX);
                        return;
                    default:
                        PERROR_THROW_CODE(PErrorCode::INVAL);
                }
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_SET_SWAPRXTX:
            if (inDataLength == sizeof(int))
            {
                int arg = *((const int*)inData);
                SetSwapRXTX(arg != 0);
                return;
            }
            else
            {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        case USARTIOCTL_GET_SWAPRXTX:
            if (outDataLength == sizeof(int))
            {
                int* result = (int*)outData;
                *result = GetSwapRXTX();
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

void USARTDriverInode::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USARTDriverInode::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    kassert(!m_MutexRead.IsLocked());
    CRITICAL_SCOPE(m_MutexRead);

    switch (mode)
    {
        case ObjectWaitMode::Read:
        case ObjectWaitMode::ReadWrite:
        {
            KIRQGuard irqGuard(m_ReceiveDMAIRQ);
            RefreshReceiveDMA();
            if (!m_ReceiveError && m_ReceivePublishedPosition == m_ReceiveReadPosition) {
                return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read);
            } else {
                return false; // Data or a receive error is ready.
            }
        }
        case ObjectWaitMode::Write:
            return false;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::InitializeDMA()
{
    m_ReceiveDMAChannel = dma_allocate_channel();
    if (m_ReceiveDMAChannel == -1) {
        PERROR_THROW_CODE(static_cast<PErrorCode>(get_last_error()));
    }
    PScopeFail releaseReceiveChannel([this]() { dma_free_channel(m_ReceiveDMAChannel); });

    m_SendDMAChannel = dma_allocate_channel();
    if (m_SendDMAChannel == -1) {
        PERROR_THROW_CODE(static_cast<PErrorCode>(get_last_error()));
    }
    PScopeFail releaseSendChannel([this]() { dma_free_channel(m_SendDMAChannel); });

    m_ReceiveBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, RECEIVE_BUFFER_SIZE));
    if (m_ReceiveBuffer == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOMEM);
    }
    PScopeFail releaseReceiveBuffer([this]() { free(m_ReceiveBuffer); });

    // The aligned buffer belongs exclusively to RX; discard any cache state before DMA first writes it.
    SCB_CleanInvalidateDCache_by_Addr(m_ReceiveBuffer, RECEIVE_BUFFER_SIZE);

    m_ReceiveDMAIRQ = dma_get_channel_irq(m_ReceiveDMAChannel);
    m_ReceiveDMAStream = dma_get_channel_stream(m_ReceiveDMAChannel);
    dma_stop(m_ReceiveDMAChannel);
    InitializeReceiveDMA();

    // Below SDMMC's LOW_LATENCY3, but above every scheduler-masked normal-latency IRQ.
    const int receiveIRQHandle = register_irq_handler(
        m_ReceiveDMAIRQ,
        IRQCallbackReceive,
        DeferredIRQCallbackReceive,
        this,
        KIRQ_PRI_LOW_LATENCY2);
    if (receiveIRQHandle < 0) {
        PERROR_THROW_CODE(static_cast<PErrorCode>(get_last_error()));
    }
    PScopeFail unregisterReceiveIRQ([this, receiveIRQHandle]() { unregister_irq_handler(m_ReceiveDMAIRQ, receiveIRQHandle); });

    // IDLE only requests RX service. Normal latency lets the RX guard also exclude this handler.
    m_Port->ICR = USART_ICR_IDLECF;
    NVIC_ClearPendingIRQ(m_USARTIRQ);
    const int usartIRQHandle = register_irq_handler(m_USARTIRQ, IRQCallbackUSART, this);
    if (usartIRQHandle < 0) {
        PERROR_THROW_CODE(static_cast<PErrorCode>(get_last_error()));
    }
    PScopeFail unregisterUSARTIRQ([this, usartIRQHandle]() { unregister_irq_handler(m_USARTIRQ, usartIRQHandle); });

    const IRQn_Type sendIRQ = dma_get_channel_irq(m_SendDMAChannel);
    dma_stop(m_SendDMAChannel);
    dma_clear_interrupt_flags(m_SendDMAChannel, DMA_LIFCR_CTCIF0);
    NVIC_ClearPendingIRQ(sendIRQ);
    if (register_irq_handler(sendIRQ, IRQCallbackSend, this) < 0) {
        PERROR_THROW_CODE(static_cast<PErrorCode>(get_last_error()));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::InitializeReceiveDMA()
{
    // Initialization/reset only: the stream is stopped and its callbacks cannot run.
    m_ReceiveReadPosition = 0;
    m_ReceivePublishedPosition = 0;
    m_ReceiveCompletedPosition = 0;
    m_ReceivePaused = false;
    m_ReceiveError = false;
    m_ReceiveServicePending = false;
    m_ReceiveWakeupPending = false;
    ConfigureReceiveDMA();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::ConfigureReceiveDMA()
{
    // The stream is stopped. Only completed chunks precede this new active chunk.
    const size_t bufferOffset = m_ReceiveCompletedPosition % RECEIVE_BUFFER_SIZE;
    const size_t nextOffset = (bufferOffset + RECEIVE_CHUNK_SIZE) % RECEIVE_BUFFER_SIZE;
    m_ReceiveActiveTarget = 0;
    dma_setup(
        m_ReceiveDMAChannel,
        DMADirection::PeriphToMem,
        m_DMARequestRX,
        &m_Port->RDR,
        m_ReceiveBuffer + bufferOffset,
        RECEIVE_CHUNK_SIZE);
    m_ReceiveDMAStream->FCR = 0; // Direct mode: short bursts must not remain buffered in the DMA FIFO.
    m_ReceiveDMAStream->M1AR = reinterpret_cast<uintptr_t>(m_ReceiveBuffer + nextOffset);
    m_ReceiveDMAStream->CR |= DMA_SxCR_DBM | DMA_SxCR_CIRC | DMA_SxCR_TEIE | DMA_SxCR_DMEIE;
    NVIC_ClearPendingIRQ(m_ReceiveDMAIRQ);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::SetBaudrate(int baudrate)
{
    if (baudrate != m_Baudrate)
    {
        m_Baudrate = baudrate;
        m_Port->BRR = m_ClockFrequency / m_Baudrate;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::SetIOControl(uint32_t flags)
{
    if (flags != m_IOControl)
    {
        m_IOControl = flags;

        uint32_t cr1 = m_Port->CR1;

        if (m_IOControl & USART_DISABLE_RX) {
            cr1 &= ~USART_CR1_RE;
        } else {
            cr1 |= USART_CR1_RE;
        }
        if (m_IOControl & USART_DISABLE_TX) {
            cr1 &= ~USART_CR1_TE;
        } else {
            cr1 |= USART_CR1_TE;
        }
        m_Port->CR1 = cr1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USARTDriverInode::SetPinMode(const PinMuxTarget& pin, USARTPinMode mode)
{
    while ((m_Port->ISR & USART_ISR_TC) == 0);
    if (mode == USARTPinMode::Normal)
    {
        DigitalPin::ActivatePeripheralMux(pin);
        return true;
    }
    else
    {
        DigitalPin ioPin(pin.PINID);
        switch (mode)
        {
            case USARTPinMode::Off:
                ioPin.SetPeripheralMux(DigitalPinPeripheralID::None);
                ioPin.SetDirection(DigitalPinDirection_e::Analog);
                return true;
            case USARTPinMode::Low:
                ioPin = false;
                ioPin.SetPeripheralMux(DigitalPinPeripheralID::None);
                ioPin.SetDirection(DigitalPinDirection_e::Out);
                return true;
            case USARTPinMode::High:
                ioPin = true;
                ioPin.SetPeripheralMux(DigitalPinPeripheralID::None);
                ioPin.SetDirection(DigitalPinDirection_e::Out);
                return true;
            default:
                set_last_error(EINVAL);
                return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::SetSwapRXTX(bool doSwap)
{
    if (doSwap != GetSwapRXTX())
    {
        // The write mutex excludes new transmissions while the existing TC wait completes.
        while ((m_Port->ISR & USART_ISR_TC) == 0);
        KIRQGuard irqGuard(m_ReceiveDMAIRQ);
        const uint32_t cr1 = m_Port->CR1;

        dma_stop(m_ReceiveDMAChannel);
        m_Port->CR1 &= ~(USART_CR1_TE | USART_CR1_RE);
        m_Port->CR1 &= ~USART_CR1_UE;

        if (doSwap) {
            m_Port->CR2 |= USART_CR2_SWAP;
        } else {
            m_Port->CR2 &= ~USART_CR2_SWAP;
        }

        m_Port->RQR = USART_RQR_RXFRQ;  // Flush receive buffer.
        m_Port->ICR = USART_ICR_IDLECF | USART_ICR_ORECF;
        NVIC_ClearPendingIRQ(m_USARTIRQ);
        InitializeReceiveDMA();
        m_Port->CR3 |= USART_CR3_DMAR;
        dma_start(m_ReceiveDMAChannel);
        m_Port->CR1 = cr1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USARTDriverInode::GetSwapRXTX() const
{
    return (m_Port->CR2 & USART_CR2_SWAP) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::RefreshReceiveDMA()
{
    // Thread context, with m_ReceiveDMAIRQ guarded. Never stop DMA to inspect a partial chunk.
    UpdateReceiveDMA();
    if (m_ReceiveWakeupPending) {
        NVIC_SetPendingIRQ(m_ReceiveDMAIRQ);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::UpdateReceiveDMA()
{
    if (m_ReceiveError) {
        return;
    }

    const uint32_t flags = dma_get_interrupt_flags(m_ReceiveDMAChannel);
    if ((flags & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0)) != 0)
    {
        FailReceiveDMA();
        return;
    }
    if ((flags & DMA_LISR_TCIF0) != 0)
    {
        dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
        const uint32_t activeTarget = m_ReceiveDMAStream->CR & DMA_SxCR_CT;
        if (m_ReceivePaused || activeTarget == m_ReceiveActiveTarget)
        {
            FailReceiveDMA();
            return;
        }

        m_ReceiveActiveTarget = activeTarget;
        m_ReceiveCompletedPosition = m_ReceiveCompletedPosition + RECEIVE_CHUNK_SIZE;
        PublishReceivePosition(m_ReceiveCompletedPosition);
        QueueNextReceiveChunk();
    }
    if (m_ReceiveError || m_ReceivePaused) {
        return;
    }

    // A target switch during the snapshot is serviced by its TC IRQ; do not guess which chunk NDTR describes.
    const size_t remainingBytes = m_ReceiveDMAStream->NDTR;
    const uint32_t activeTarget = m_ReceiveDMAStream->CR & DMA_SxCR_CT;
    const uint32_t pendingFlags = dma_get_interrupt_flags(m_ReceiveDMAChannel);
    if (activeTarget == m_ReceiveActiveTarget && remainingBytes != 0 && remainingBytes <= RECEIVE_CHUNK_SIZE &&
        (pendingFlags & RECEIVE_DMA_INTERRUPT_FLAGS) == 0)
    {
        PublishReceivePosition(m_ReceiveCompletedPosition + RECEIVE_CHUNK_SIZE - remainingBytes);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::PublishReceivePosition(size_t position)
{
    // Ignore stale samples at a target reload. Publication never moves backwards.
    const size_t newBytes = position - m_ReceivePublishedPosition;
    if (newBytes != 0 && newBytes <= RECEIVE_CHUNK_SIZE)
    {
        m_ReceivePublishedPosition = position;
        m_ReceiveWakeupPending = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::QueueNextReceiveChunk()
{
    const size_t nextPosition = m_ReceiveCompletedPosition + RECEIVE_CHUNK_SIZE;
    const size_t firstOccupiedChunk = m_ReceiveReadPosition - m_ReceiveReadPosition % RECEIVE_CHUNK_SIZE;
    if (nextPosition - firstOccupiedChunk >= RECEIVE_BUFFER_SIZE)
    {
        // Keep the active chunk as a guard. Its old alternate target must not overwrite unread data.
        m_Port->CR3 &= ~USART_CR3_DMAR;
        m_ReceivePaused = true;
    }
    else
    {
        if ((m_ReceiveDMAStream->CR & DMA_SxCR_CT) != m_ReceiveActiveTarget)
        {
            FailReceiveDMA();
            return;
        }
        volatile uint32_t* nextAddress = (m_ReceiveActiveTarget == 0) ? &m_ReceiveDMAStream->M1AR : &m_ReceiveDMAStream->M0AR;
        *nextAddress = reinterpret_cast<uintptr_t>(m_ReceiveBuffer + nextPosition % RECEIVE_BUFFER_SIZE);
    }
    if ((m_ReceiveDMAStream->CR & DMA_SxCR_CT) != m_ReceiveActiveTarget ||
        (dma_get_interrupt_flags(m_ReceiveDMAChannel) & RECEIVE_DMA_INTERRUPT_FLAGS) != 0) {
        FailReceiveDMA();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::ResumeReceiveDMA()
{
    // Thread context only, with the RX IRQ guarded. Retain unread completed chunks through overflow.
    const size_t firstOccupiedChunk = m_ReceiveReadPosition - m_ReceiveReadPosition % RECEIVE_CHUNK_SIZE;
    if (m_ReceiveCompletedPosition + RECEIVE_CHUNK_SIZE - firstOccupiedChunk < RECEIVE_BUFFER_SIZE)
    {
        dma_stop(m_ReceiveDMAChannel);
        if ((m_ReceiveDMAStream->CR & DMA_SxCR_CT) != m_ReceiveActiveTarget ||
            (dma_get_interrupt_flags(m_ReceiveDMAChannel) & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0)) != 0)
        {
            FailReceiveDMA();
            return;
        }
        // Drop the guard chunk and FIFO received while full, including any already-consumed IDLE notification.
        m_Port->RQR = USART_RQR_RXFRQ;
        m_Port->ICR = USART_ICR_IDLECF | USART_ICR_ORECF;
        ConfigureReceiveDMA();
        m_ReceiveServicePending = false;
        m_ReceivePaused = false;
        dma_start(m_ReceiveDMAChannel);
        m_Port->CR3 |= USART_CR3_DMAR;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::FailReceiveDMA()
{
    // Request an abort without waiting in the immediate handler. Reset stops the stream before reprogramming it.
    m_Port->CR3 &= ~USART_CR3_DMAR;
    m_ReceiveDMAStream->CR &= ~(DMA_SxCR_EN | DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE);
    dma_clear_interrupt_flags(m_ReceiveDMAChannel, RECEIVE_DMA_INTERRUPT_FLAGS);
    m_ReceiveError = true;
    m_ReceiveWakeupPending = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USARTDriverInode::ReadReceiveBuffer(void* buffer, size_t length)
{
    size_t readPosition;
    size_t bytesToRead;
    bool receiveError;
    {
        KIRQGuard irqGuard(m_ReceiveDMAIRQ);
        RefreshReceiveDMA();
        readPosition = m_ReceiveReadPosition;
        bytesToRead = std::min(length, m_ReceivePublishedPosition - readPosition);
        receiveError = m_ReceiveError;
    }
    if (receiveError) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (bytesToRead != 0)
    {
        // Published bytes stay reserved while copying, including prefixes of the active DMA chunk.
        uint8_t* target = static_cast<uint8_t*>(buffer);
        const size_t bufferOffset = readPosition % RECEIVE_BUFFER_SIZE;
        const size_t firstLength = std::min(bytesToRead, RECEIVE_BUFFER_SIZE - bufferOffset);
        SCB_InvalidateDCache_by_Addr(m_ReceiveBuffer + bufferOffset, firstLength);
        memcpy(target, m_ReceiveBuffer + bufferOffset, firstLength);
        if (firstLength < bytesToRead)
        {
            const size_t secondLength = bytesToRead - firstLength;
            SCB_InvalidateDCache_by_Addr(m_ReceiveBuffer, secondLength);
            memcpy(target + firstLength, m_ReceiveBuffer, secondLength);
        }

        {
            KIRQGuard irqGuard(m_ReceiveDMAIRQ);
            if (!m_ReceiveError)
            {
                m_ReceiveReadPosition = readPosition + bytesToRead;
                if (m_ReceivePaused) {
                    ResumeReceiveDMA();
                }
                RefreshReceiveDMA();
            }
            receiveError = m_ReceiveError;
        }
        if (receiveError) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }
    return bytesToRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverInode::IRQCallbackReceive(IRQn_Type irq, void* userData)
{
    return static_cast<USARTDriverInode*>(userData)->HandleIRQReceive();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::DeferredIRQCallbackReceive(IRQn_Type irq, void* userData)
{
    static_cast<USARTDriverInode*>(userData)->HandleDeferredIRQReceive();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverInode::HandleIRQReceive()
{
    const uint32_t flags = dma_get_interrupt_flags(m_ReceiveDMAChannel) & RECEIVE_DMA_INTERRUPT_FLAGS;
    const bool serviceRequested = m_ReceiveServicePending;
    m_ReceiveServicePending = false;
    if ((flags == 0 || m_ReceiveError) && !serviceRequested && !m_ReceiveWakeupPending) {
        return IRQResult::UNHANDLED;
    }

    UpdateReceiveDMA();
    return m_ReceiveWakeupPending ? IRQResult::HANDLED_DEFERRED : IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverInode::HandleDeferredIRQReceive()
{
    bool wakeReaders;
    {
        KIRQGuard irqGuard(m_ReceiveDMAIRQ);
        wakeReaders = m_ReceiveWakeupPending && (m_ReceiveError || m_ReceivePublishedPosition != m_ReceiveReadPosition);
        m_ReceiveWakeupPending = false;
    }

    // Retained readiness, rather than callback counts, handles coalescing and callbacks left over after a reset/read.
    if (wakeReaders) {
        m_ReceiveCondition.WakeupAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverInode::IRQCallbackUSART(IRQn_Type irq, void* userData)
{
    return static_cast<USARTDriverInode*>(userData)->HandleIRQUSART();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverInode::HandleIRQUSART()
{
    if ((m_Port->ISR & USART_ISR_IDLE) != 0 && (m_Port->CR1 & USART_CR1_IDLEIE) != 0)
    {
        // Let DMA retain ownership of RDR. Reading RDR here could steal a byte from the DMA stream.
        m_Port->ICR = USART_ICR_IDLECF;
        m_ReceiveServicePending = true;
        NVIC_SetPendingIRQ(m_ReceiveDMAIRQ);
        return IRQResult::HANDLED;
    }
    return IRQResult::UNHANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverInode::IRQCallbackSend(IRQn_Type irq, void* userData)
{
    return static_cast<USARTDriverInode*>(userData)->HandleIRQSend();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverInode::HandleIRQSend()
{
    if (dma_get_interrupt_flags(m_SendDMAChannel) & DMA_LISR_TCIF0)
    {
        dma_clear_interrupt_flags(m_SendDMAChannel, DMA_LIFCR_CTCIF0);
        m_TransmitCondition.Wakeup(1);
    }
    return IRQResult::HANDLED;
}


} // namespace
