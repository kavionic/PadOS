// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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

#include "Kernel/Drivers/STM32/USARTDriver.h"

#include <malloc.h>
#include <algorithm>
#include <cstring>

#include "Ptr/Ptr.h"
#include "Kernel/IRQDispatcher.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/HAL/DMA.h"

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USARTDriverINode::USARTDriverINode(USART_TypeDef* port, DMAMUX1_REQUEST dmaRequestRX, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency, KFilesystemFileOps* fileOps)
	: KINode(nullptr, nullptr, fileOps, false)
	, m_MutexRead("USARTDriverINodeRead")
	, m_MutexWrite("USARTDriverINodeWrite")
	, m_ReceiveCondition("USARTDriverINodeReceive")
	, m_TransmitCondition("USARTDriverINodeTransmit")
	, m_DMARequestRX(dmaRequestRX)
	, m_DMARequestTX(dmaRequestTX)

{
	m_Port = port;

	m_Port->CR1 = USART_CR1_RE | USART_CR1_FIFOEN;
	m_Port->CR3 = USART_CR3_DMAR | USART_CR3_DMAT;

	m_Baudrate = 921600;
	m_Port->BRR = clockFrequency / m_Baudrate;

	m_Port->CR1 |= USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;

	m_ReceiveDMAChannel = dma_allocate_channel();
	m_SendDMAChannel = dma_allocate_channel();

	if (m_ReceiveDMAChannel != -1)
	{
		m_ReceiveBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, m_ReceiveBufferSize));

		auto irq = dma_get_channel_irq(m_ReceiveDMAChannel);
		NVIC_ClearPendingIRQ(irq);
		kernel::register_irq_handler(irq, IRQCallbackReceive, this);

		dma_setup(m_ReceiveDMAChannel, DMAMode::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer, m_ReceiveBufferSize);
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

ssize_t USARTDriverINode::Read(Ptr<KFileNode> file, void* buffer, size_t length)
{
    CRITICAL_SCOPE(m_MutexRead);

    uint8_t* currentTarget = reinterpret_cast<uint8_t*>(buffer);
    ssize_t remainingLen = length;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
	dma_stop(m_ReceiveDMAChannel);
	dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
    } CRITICAL_END;

    if (m_Port->ISR & USART_ISR_ORE) {
	m_Port->ICR = USART_ICR_ORECF;
	set_last_error(EIO);
	return -1;
    }

    int32_t bytesReceived = m_ReceiveBufferSize - dma_get_transfer_count(m_ReceiveDMAChannel);

    if (bytesReceived >= length)
    {
	SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(m_ReceiveBuffer), ((bytesReceived + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE);
	memcpy(currentTarget, m_ReceiveBuffer, length);

	if (length == bytesReceived)
	{
	    m_ReceiveBufferInPos = 0;
	    dma_setup(m_ReceiveDMAChannel, DMAMode::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer, m_ReceiveBufferSize);
	}
	else
	{
	    memmove(m_ReceiveBuffer, m_ReceiveBuffer + length, bytesReceived - length);
	    m_ReceiveBufferInPos = bytesReceived - length;
	    dma_setup(m_ReceiveDMAChannel, DMAMode::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer + m_ReceiveBufferInPos, m_ReceiveBufferSize - m_ReceiveBufferInPos);
	}
	dma_start(m_ReceiveDMAChannel);
	return length;
    }
    else
    {
	if (bytesReceived > 0)
	{
	    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(m_ReceiveBuffer), ((bytesReceived + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE);
	    memcpy(currentTarget, m_ReceiveBuffer, bytesReceived);
	    remainingLen -= bytesReceived;
	    currentTarget += bytesReceived;
	}
	for (size_t currentLen = std::min<size_t>(m_ReceiveBufferSize, remainingLen); remainingLen > 0; )
	{
	    dma_setup(m_ReceiveDMAChannel, DMAMode::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer, currentLen);
	    CRITICAL_BEGIN(CRITICAL_IRQ)
	    {
		dma_start(m_ReceiveDMAChannel);
		while (!m_ReceiveCondition.IRQWait() && get_last_error() == EINTR);
	    } CRITICAL_END;
	    SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(m_ReceiveBuffer), ((currentLen + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE);
	    memcpy(currentTarget, m_ReceiveBuffer, currentLen);

	    remainingLen -= currentLen;
	    currentTarget += currentLen;
	    currentLen = std::min<size_t>(m_ReceiveBufferSize, remainingLen);
	}
	m_ReceiveBufferInPos = 0;
	dma_setup(m_ReceiveDMAChannel, DMAMode::PeriphToMem, m_DMARequestRX, &m_Port->RDR, m_ReceiveBuffer, m_ReceiveBufferSize);
	dma_start(m_ReceiveDMAChannel);
    }
    return length;
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
		dma_setup(m_SendDMAChannel, DMAMode::MemToPeriph, m_DMARequestTX, &m_Port->TDR, currentTarget, currentLen);
		CRITICAL_BEGIN(CRITICAL_IRQ)
		{
			dma_start(m_SendDMAChannel);
            if (!m_TransmitCondition.IRQWaitTimeout(TimeValMicros::FromMicroseconds(bigtime_t(currentLen) * 10 * 2 * TimeValMicros::TicksPerSecond / m_Baudrate) + TimeValMicros::FromMilliseconds(100)))
			{
				return -1;
			}
		} CRITICAL_END;
	}
	return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult USARTDriverINode::HandleIRQReceive()
{
	if (dma_get_interrupt_flags(m_ReceiveDMAChannel) & DMA_LISR_TCIF0)
	{
		dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
		m_ReceiveCondition.Wakeup(1);
		KSWITCH_CONTEXT();
	}
	return IRQResult::HANDLED;
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

USARTDriver::~USARTDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriver::Setup(const char* devicePath, USART_TypeDef* port, DMAMUX1_REQUEST dmaRequestRX, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency)
{
    Ptr<USARTDriverINode> node = ptr_new<USARTDriverINode>(port, dmaRequestRX, dmaRequestTX, clockFrequency, this);
    Kernel::RegisterDevice(devicePath, node);
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
	return -1;
}



} // namespace
