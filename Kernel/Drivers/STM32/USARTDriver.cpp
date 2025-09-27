// This file is part of PadOS.
//
// Copyright (C) 2020-2024 Kurt Skauen <http://kavionic.com/>
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


#include <malloc.h>
#include <algorithm>
#include <cstring>

#include <Kernel/Drivers/STM32/USARTDriver.h>

#include <Ptr/Ptr.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/HAL/DMA.h>
#include <Kernel/HAL/PeripheralMapping.h>

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USARTDriverINode::USARTDriverINode( USARTID      portID,
                                    const PinMuxTarget& pinRX,
                                    const PinMuxTarget& pinTX,
                                    uint32_t            clockFrequency,
                                    USARTDriver*        driver)
    : KINode(nullptr, nullptr, driver, false)
    , m_MutexRead("USARTDriverINodeRead", PEMutexRecursionMode_RaiseError)
    , m_MutexWrite("USARTDriverINodeWrite", PEMutexRecursionMode_RaiseError)
    , m_ReceiveCondition("USARTDriverINodeReceive")
    , m_TransmitCondition("USARTDriverINodeTransmit")
    , m_PinRX(pinRX)
    , m_PinTX(pinTX)
{
    m_Driver = ptr_tmp_cast(driver);
    m_Port = get_usart_from_id(portID);

    get_usart_dma_requests(portID, m_DMARequestRX, m_DMARequestTX);

    DigitalPin::ActivatePeripheralMux(m_PinRX);
    DigitalPin::ActivatePeripheralMux(m_PinTX);
    
    m_Port->CR1 = USART_CR1_RE | USART_CR1_FIFOEN;
    m_Port->CR3 = USART_CR3_DMAR | USART_CR3_DMAT;

    m_ClockFrequency = clockFrequency;
    SetBaudrate(921600);

    m_Port->CR1 |= USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;

    m_ReceiveDMAChannel = dma_allocate_channel();
    m_SendDMAChannel = dma_allocate_channel();

    if (m_ReceiveDMAChannel != -1)
    {
        m_ReceiveBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, m_ReceiveBufferSize));

        auto irq = dma_get_channel_irq(m_ReceiveDMAChannel);
        NVIC_ClearPendingIRQ(irq);
        kernel::register_irq_handler(irq, IRQCallbackReceive, this);

        m_PendingReceiveBytes = m_ReceiveBufferSize;
        dma_setup(m_ReceiveDMAChannel, DMADirection::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer, m_ReceiveBufferSize);
        dma_start(m_ReceiveDMAChannel);
    }
    if (m_SendDMAChannel != -1)
    {
        auto irq = dma_get_channel_irq(m_SendDMAChannel);
        NVIC_ClearPendingIRQ(irq);
        kernel::register_irq_handler(irq, IRQCallbackSend, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USARTDriverINode::Read(Ptr<KFileNode> file, void* buffer, const size_t length)
{
    if (length == 0) {
        return 0;
    }
    CRITICAL_SCOPE(m_MutexRead);

    uint8_t* currentTarget = reinterpret_cast<uint8_t*>(buffer);

    for (;;)
    {
        ssize_t curLen = ReadReceiveBuffer(file, currentTarget, length);
        if (curLen > 0 || (file->GetOpenFlags() & O_NONBLOCK)) {
            return curLen;
        }
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            dma_stop(m_ReceiveDMAChannel);
            dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
            RestartReceiveDMA(1);

            if (m_ReceiveBytesInBuffer == 0)
            {
                const PErrorCode result = m_ReceiveCondition.IRQWaitTimeout(m_ReadTimeout);
                if (result != PErrorCode::Success)
                {
                    if (result != PErrorCode::Interrupted)
                    {
                        set_last_error(result);
                        return -1;
                    }
                }
            }
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USARTDriverINode::Write(Ptr<KFileNode> file, const void* buffer, const size_t length)
{
    SCB_CleanInvalidateDCache();
    CRITICAL_SCOPE(m_MutexWrite);

    const uint8_t* currentTarget = reinterpret_cast<const uint8_t*>(buffer);
    ssize_t remainingLen = length;

    for (size_t currentLen = std::min(ssize_t(DMA_MAX_TRANSFER_LENGTH), remainingLen); remainingLen > 0; remainingLen -= currentLen, currentTarget += currentLen)
    {
        m_Port->ICR = USART_ICR_TCCF;

        dma_stop(m_SendDMAChannel);
        dma_setup(m_SendDMAChannel, DMADirection::MemToPeriph, m_DMARequestTX, &m_Port->TDR, currentTarget, currentLen);
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            dma_start(m_SendDMAChannel);
            const PErrorCode result = m_TransmitCondition.IRQWaitTimeout(TimeValMicros::FromMicroseconds(bigtime_t(currentLen) * 10 * 2 * TimeValMicros::TicksPerSecond / m_Baudrate) + TimeValMicros::FromMilliseconds(100));
            if (result != PErrorCode::Success)
            {
                set_last_error(result);
                return -1;
            }
        } CRITICAL_END;
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USARTDriverINode::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    kassert(!m_MutexRead.IsLocked());
    CRITICAL_SCOPE(m_MutexRead);

    switch (mode)
    {
        case ObjectWaitMode::Read:
        case ObjectWaitMode::ReadWrite:
            CRITICAL_BEGIN(CRITICAL_IRQ)
            {
                if (m_ReceiveBytesInBuffer == 0)
                {
                    dma_stop(m_ReceiveDMAChannel);
                    dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
                    RestartReceiveDMA(1);
                }

                if (m_ReceiveBytesInBuffer == 0) {
                    return m_ReceiveCondition.AddListener(waitNode, ObjectWaitMode::Read);
                } else {
                    return false; // Will not block.
                }
            } CRITICAL_END;
        case ObjectWaitMode::Write:
            return false;
        default:
            return false;
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USARTDriverINode::DeviceControl(int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    CRITICAL_SCOPE(m_MutexRead);
    CRITICAL_SCOPE(m_MutexWrite);

    while((m_Port->ISR & USART_ISR_TC) == 0);

    switch (request)
    {
        case USARTIOCTL_SET_BAUDRATE:
            if (inDataLength == sizeof(int)) {
                int baudrate = *((const int*)inData);
                SetBaudrate(baudrate);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_GET_BAUDRATE:
            if (outDataLength == sizeof(int)) {
                int* baudrate = (int*)outData;
                *baudrate = m_Baudrate;
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_SET_READ_TIMEOUT:
            if (inDataLength == sizeof(bigtime_t)) {
                bigtime_t micros = *((const bigtime_t*)inData);
                m_ReadTimeout = TimeValMicros::FromMicroseconds(micros);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_GET_READ_TIMEOUT:
            if (outDataLength == sizeof(bigtime_t)) {
                bigtime_t* micros = (bigtime_t*)outData;
                *micros = m_ReadTimeout.AsMicroSeconds();
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_SET_IOCTRL:
            if (inDataLength == sizeof(uint32_t)) {
                uint32_t flags = *((const uint32_t*)inData);
                SetIOControl(flags);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_GET_IOCTRL:
            if (outDataLength == sizeof(uint32_t)) {
                uint32_t* flags = (uint32_t*)outData;
                *flags = m_IOControl;
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
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
                        if (SetPinMode(m_PinRX, mode)) {
                            m_PinModeRX = mode;
                            m_Port->RQR = USART_RQR_RXFRQ;  // Flush receive buffer
                            return 0;
                        } else {
                            set_last_error(EINVAL);
                            return -1;
                        }
                    case USARTPin::TX:
                        if (SetPinMode(m_PinTX, mode)) {
                            m_PinModeTX = mode;
                            m_Port->RQR = USART_RQR_RXFRQ;  // Flush receive buffer
                            return 0;
                        } else {
                            set_last_error(EINVAL);
                            return -1;
                        }
                    default:
                        set_last_error(EINVAL);
                        return -1;
                }
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
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
                        return 0;
                    case USARTPin::TX:
                        *result = int(m_PinModeTX);
                        return 0;
                    default:
                        set_last_error(EINVAL);
                        return -1;
                }
            }
            else
            {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_SET_SWAPRXTX:
            if (inDataLength == sizeof(int)) {
                int arg = *((const int*)inData);
                SetSwapRXTX(arg != 0);
                return 0;
            } else {
                set_last_error(EINVAL);
                return -1;
            }
        case USARTIOCTL_GET_SWAPRXTX:
            if (outDataLength == sizeof(int)) {
                int* result = (int*)outData;
                *result = GetSwapRXTX();
                return 0;
            } else {
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

void USARTDriverINode::SetBaudrate(int baudrate)
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

void USARTDriverINode::SetIOControl(uint32_t flags)
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

bool USARTDriverINode::SetPinMode(const PinMuxTarget& pin, USARTPinMode mode)
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

void USARTDriverINode::SetSwapRXTX(bool doSwap)
{
    if (doSwap != GetSwapRXTX())
    {
        uint32_t cr1 = m_Port->CR1;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            dma_stop(m_ReceiveDMAChannel);
            dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);

            m_Port->CR1 &= ~(USART_CR1_TE | USART_CR1_RE);
            while ((m_Port->ISR & USART_ISR_TC) == 0);
            m_Port->CR1 &= ~USART_CR1_UE;

            if (doSwap) {
                m_Port->CR2 |= USART_CR2_SWAP;
            } else {
                m_Port->CR2 &= ~USART_CR2_SWAP;
            }
            m_Port->CR1 = cr1;

            m_Port->RQR = USART_RQR_RXFRQ;  // Flush receive buffer

            m_ReceiveBufferInPos = 0;
            m_ReceiveBufferOutPos = 0;
            m_ReceiveBytesInBuffer = 0;
            m_PendingReceiveBytes = m_ReceiveBufferSize;
            dma_setup(m_ReceiveDMAChannel, DMADirection::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer, m_ReceiveBufferSize);
            dma_start(m_ReceiveDMAChannel);
        } CRITICAL_END;

    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USARTDriverINode::GetSwapRXTX() const
{
    return (m_Port->CR2 & USART_CR2_SWAP) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USARTDriverINode::RestartReceiveDMA(size_t maxLength)
{
    const int32_t bytesReceived = m_PendingReceiveBytes - dma_get_transfer_count(m_ReceiveDMAChannel);

    m_ReceiveBytesInBuffer += bytesReceived;
    m_ReceiveBufferInPos = (m_ReceiveBufferInPos + bytesReceived) % m_ReceiveBufferSize;

    m_PendingReceiveBytes = std::min(m_ReceiveBufferSize - m_ReceiveBytesInBuffer, m_ReceiveBufferSize - m_ReceiveBufferInPos);
    
    if (m_ReceiveBytesInBuffer < maxLength && (m_ReceiveBytesInBuffer + m_PendingReceiveBytes) > maxLength)
    {
        m_PendingReceiveBytes = maxLength - m_ReceiveBytesInBuffer;
    }

    if (m_PendingReceiveBytes > 0)
    {
        dma_setup(m_ReceiveDMAChannel, DMADirection::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer + m_ReceiveBufferInPos, m_PendingReceiveBytes);
        dma_start(m_ReceiveDMAChannel);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USARTDriverINode::ReadReceiveBuffer(Ptr<KFileNode> file, void* buffer, const size_t length)
{
    uint8_t* currentTarget = reinterpret_cast<uint8_t*>(buffer);

    if (m_ReceiveBytesInBuffer == 0)
    {
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            dma_stop(m_ReceiveDMAChannel);
            dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
            RestartReceiveDMA(0);
        } CRITICAL_END;
    }

    const int32_t bytesToRead = std::min<int32_t>(length, m_ReceiveBytesInBuffer);
    if (bytesToRead > 0)
    {
        SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(m_ReceiveBuffer), ((m_ReceiveBufferSize + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE);

        const int32_t postLength = std::min(bytesToRead, m_ReceiveBufferSize - m_ReceiveBufferOutPos);
        memcpy(currentTarget, m_ReceiveBuffer + m_ReceiveBufferOutPos, postLength);
        if (postLength < bytesToRead)
        {
            const int32_t preLength = bytesToRead - postLength;
            memcpy(currentTarget + postLength, m_ReceiveBuffer, preLength);
        }
        m_ReceiveBufferOutPos = (m_ReceiveBufferOutPos + bytesToRead) % m_ReceiveBufferSize;
        m_ReceiveBytesInBuffer -= bytesToRead;
    }
    return bytesToRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC IRQResult USARTDriverINode::IRQCallbackReceive(IRQn_Type irq, void* userData)
{
    return static_cast<USARTDriverINode*>(userData)->HandleIRQReceive();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverINode::HandleIRQReceive()
{
    if (dma_get_interrupt_flags(m_ReceiveDMAChannel) & DMA_LISR_TCIF0)
    {
        dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
        dma_stop(m_ReceiveDMAChannel);
        RestartReceiveDMA(0);
        m_ReceiveCondition.WakeupAll();
        KSWITCH_CONTEXT();
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC IRQResult USARTDriverINode::IRQCallbackSend(IRQn_Type irq, void* userData)
{
    return static_cast<USARTDriverINode*>(userData)->HandleIRQSend();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverINode::HandleIRQSend()
{
    if (dma_get_interrupt_flags(m_SendDMAChannel) & DMA_LISR_TCIF0)
    {
        dma_clear_interrupt_flags(m_SendDMAChannel, DMA_LIFCR_CTCIF0);
        m_TransmitCondition.Wakeup(1);
    }
    return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USARTDriver::USARTDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriver::Setup(const char*         devicePath,
                        USARTID             portID,
                        const PinMuxTarget& pinRX,
                        const PinMuxTarget& pinTX,
                        uint32_t            clockFrequency)
{
    Ptr<USARTDriverINode> node = ptr_new<USARTDriverINode>(portID, pinRX, pinTX, clockFrequency, this);
    Kernel::RegisterDevice(devicePath, node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriver::Setup(const USARTDriverSetup& setup)
{
    Ptr<USARTDriverINode> node = ptr_new<USARTDriverINode>(setup.PortID, setup.PinRX, setup.PinTX, setup.ClockFrequency, this);
    Kernel::RegisterDevice(setup.DevicePath.c_str(), node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USARTDriver::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    Ptr<USARTDriverINode> node = ptr_static_cast<USARTDriverINode>(file->GetINode());
    return node->Read(file, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t USARTDriver::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    Ptr<USARTDriverINode> node = ptr_static_cast<USARTDriverINode>(file->GetINode());
    return node->Write(file, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USARTDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<USARTDriverINode> node = ptr_static_cast<USARTDriverINode>(file->GetINode());
    return node->DeviceControl(request, inData, inDataLength, outData, outDataLength);
}


} // namespace
