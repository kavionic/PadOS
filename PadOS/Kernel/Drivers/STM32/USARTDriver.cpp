/*
 * USARTDriver.cpp
 *
 *  Created on: Jan 3, 2020
 *      Author: kurts
 */

#include "USARTDriver.h"

#include <malloc.h>
#include <algorithm>
#include <cstring>

#include "System/Ptr/Ptr.h"
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

	m_Port->BRR = clockFrequency / 921600;

	m_Port->CR1 |= USART_CR1_UE | USART_CR1_RE | USART_CR1_TE;

	m_ReceiveDMAChannel = dma_allocate_channel();
	m_SendDMAChannel = dma_allocate_channel();

	if (m_ReceiveDMAChannel != -1)
	{
		m_ReceiveBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, m_ReceiveBufferSize));

		auto irq = dma_get_channel_irq(m_ReceiveDMAChannel);
		NVIC_ClearPendingIRQ(irq);
		kernel::Kernel::RegisterIRQHandler(irq, IRQCallbackReceive, this);

		dma_setup_per_to_mem(m_ReceiveDMAChannel, m_DMARequestRX, m_ReceiveBuffer, &m_Port->RDR, m_ReceiveBufferSize);
		dma_start(m_ReceiveDMAChannel);
	}
	if (m_SendDMAChannel != -1)
	{
		auto irq = dma_get_channel_irq(m_SendDMAChannel);
		NVIC_ClearPendingIRQ(irq);
		kernel::Kernel::RegisterIRQHandler(irq, IRQCallbackSend, this);
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

	int32_t bytesReceived = m_ReceiveBufferSize - dma_get_transfer_count(m_ReceiveDMAChannel);

	if (bytesReceived >= length)
	{
		SCB_CleanInvalidateDCache();
		memcpy(currentTarget, m_ReceiveBuffer, length);

		if (length == bytesReceived)
		{
			m_ReceiveBufferInPos = 0;
			dma_setup_per_to_mem(m_ReceiveDMAChannel, m_DMARequestRX, m_ReceiveBuffer, &m_Port->RDR, m_ReceiveBufferSize);
		}
		else
		{
			memmove(m_ReceiveBuffer, m_ReceiveBuffer + length, bytesReceived - length);
			SCB_CleanInvalidateDCache();
			m_ReceiveBufferInPos = bytesReceived - length;
			dma_setup_per_to_mem(m_ReceiveDMAChannel, m_DMARequestRX, m_ReceiveBuffer + m_ReceiveBufferInPos, &m_Port->RDR, m_ReceiveBufferSize - m_ReceiveBufferInPos);
		}
		dma_start(m_ReceiveDMAChannel);
		return length;
	}
	else
	{
		if (bytesReceived > 0)
		{
			SCB_CleanInvalidateDCache();
			memcpy(currentTarget, m_ReceiveBuffer, bytesReceived);
			remainingLen -= bytesReceived;
			currentTarget += bytesReceived;
		}
		for (size_t currentLen = std::min<size_t>(m_ReceiveBufferSize, remainingLen); remainingLen > 0; remainingLen -= currentLen, currentTarget += currentLen)
		{
			dma_setup_per_to_mem(m_ReceiveDMAChannel, m_DMARequestRX, m_ReceiveBuffer, &m_Port->RDR, currentLen);
			CRITICAL_BEGIN(CRITICAL_IRQ)
			{
				dma_start(m_ReceiveDMAChannel);
				while (!m_ReceiveCondition.IRQWait() && get_last_error() == EINTR);
			} CRITICAL_END;
			SCB_CleanInvalidateDCache();
			memcpy(currentTarget, m_ReceiveBuffer, currentLen);
		}
		m_ReceiveBufferInPos = 0;
		dma_setup_per_to_mem(m_ReceiveDMAChannel, m_DMARequestRX, m_ReceiveBuffer, &m_Port->RDR, m_ReceiveBufferSize);
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

		dma_setup_mem_to_per(m_SendDMAChannel, m_DMARequestTX, &m_Port->TDR, currentTarget, currentLen);
		CRITICAL_BEGIN(CRITICAL_IRQ)
		{
			dma_start(m_SendDMAChannel);
			m_TransmitCondition.IRQWait();
		} CRITICAL_END;
	}
	return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverINode::HandleIRQReceive()
{
	if (dma_get_interrupt_flags(m_ReceiveDMAChannel) & DMA_LISR_TCIF0)
	{
		dma_clear_interrupt_flags(m_ReceiveDMAChannel, DMA_LIFCR_CTCIF0);
		m_ReceiveCondition.Wakeup();
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USARTDriverINode::HandleIRQSend()
{
	if (dma_get_interrupt_flags(m_SendDMAChannel) & DMA_LISR_TCIF0)
	{
		dma_clear_interrupt_flags(m_SendDMAChannel, DMA_LIFCR_CTCIF0);
		m_TransmitCondition.Wakeup();
	}
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
