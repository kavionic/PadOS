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

#include "Kernel/HAL/STM32/DMA_STM32.h"

#include "Kernel/KMutex.h"
#include "Utils/Utils.h"

namespace kernel
{
#define IS_DMA_UART_USART_REQUEST_BASE(__REQUEST__) ((((__REQUEST__) >= DMAMUX1_REQUEST::REQ_USART1_RX)  &&  ((__REQUEST__) <= DMAMUX1_REQUEST::REQ_USART3_TX)) || \
                                                    (((__REQUEST__) >= DMAMUX1_REQUEST::REQ_UART4_RX)  &&  ((__REQUEST__) <= DMAMUX1_REQUEST::REQ_UART5_TX )) || \
                                                    (((__REQUEST__) >= DMAMUX1_REQUEST::REQ_USART6_RX) &&  ((__REQUEST__) <= DMAMUX1_REQUEST::REQ_USART6_TX)) || \
                                                    (((__REQUEST__) >= DMAMUX1_REQUEST::REQ_UART7_RX)  &&  ((__REQUEST__) <= DMAMUX1_REQUEST::REQ_UART8_TX )))

#if defined(UART9)
#define IS_DMA_UART_USART_REQUEST(__REQUEST__) (IS_DMA_UART_USART_REQUEST_BASE(__REQUEST__) || ((__REQUEST__) >= DMAMUX1_REQUEST::REQ_UART9_RX)  &&  ((__REQUEST__) <= DMAMUX1_REQUEST::REQ_USART10_TX )))
#else
#define IS_DMA_UART_USART_REQUEST(__REQUEST__) IS_DMA_UART_USART_REQUEST_BASE(__REQUEST__)
#endif

static uint32_t g_UsedChannels = 0;

KMutex g_DMAMutex("kernel_dma");

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int dma_allocate_channel()
{
	CRITICAL_SCOPE(g_DMAMutex);

	uint32_t mask = 1;
	for (int i = 0; i < DMA_CHANNEL_COUNT; ++i, mask <<=1)
	{
		if ((g_UsedChannels & mask) == 0)
		{
			g_UsedChannels |= mask;
			return i;
		}
	}
	set_last_error(ENOENT);
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void dma_free_channel(int channel)
{
	CRITICAL_SCOPE(g_DMAMutex);

	g_UsedChannels &= ~(1 << channel);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQn_Type dma_get_channel_irq(int channel)
{
	static IRQn_Type table[] =
	{
		DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn, DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn,
		DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn, DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn
	};
	if (channel >= 0 && channel < ARRAY_COUNT(table)) {
		return table[channel];
	} else {
		set_last_error(EINVAL);
		return IRQn_Type(-1);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void dma_setup(int channel, DMAMode mode, DMAMUX1_REQUEST requestID, volatile const void* registerAddr, const void* memAddr, int32_t length)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	DMAMUX1_ChannelStatus->CFR = 1 << channel;

	dmaStream->NDTR = length;
	dmaStream->PAR  = intptr_t(registerAddr);
	dmaStream->M0AR = intptr_t(memAddr);
	dmaStream->CR   = DMA_SxCR_MINC | DMA_SxCR_TCIE | ((uint32_t(mode) << DMA_SxCR_DIR_Pos) & DMA_SxCR_DIR_Msk);

	// Workaround for the "DMA stream locked when transferring data to/from USART/UART" errata.
	// Criteria taken from the ST HAL driver.
#if (STM32H7_DEV_ID == 0x450UL)
	if ((DBGMCU->IDCODE & 0xFFFF0000U) >= 0x20000000U)
#endif // STM32H7_DEV_ID == 0x450UL
	{
		if (IS_DMA_UART_USART_REQUEST(requestID)) {
			dmaStream->CR |= DMA_SxCR_TRBUFF;
		}
	}
	dma_clear_interrupt_flags(channel, DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0);
	DMAMUX1[channel].CCR = uint32_t(requestID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t dma_get_interrupt_flags(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dma         = (channel < DMA_CHANNELS_PER_UNIT) ? DMA1 : DMA2;

	if (localChannel < 4)
	{
		int offset = localChannel * 6;
		if (localChannel > 1) offset += 4;
		return dma->LISR >> offset;
	}
	else
	{
		localChannel -= 4;
		int offset = localChannel * 6;
		if (localChannel > 1) offset += 4;
		return dma->HISR >> offset;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void dma_clear_interrupt_flags(int channel, uint32_t flags)
{
	int  localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dma          = (channel < DMA_CHANNELS_PER_UNIT) ? DMA1 : DMA2;


	if (localChannel < 4) {
		int offset = localChannel * 6;
		if (localChannel > 1) offset += 4;
		dma->LIFCR = flags << offset;
	} else {
		localChannel -= 4;
		int offset = localChannel * 6;
		if (localChannel > 1) offset += 4;
		dma->HIFCR = flags << offset;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t dma_get_transfer_count(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	return dmaStream->NDTR;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void dma_start(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	dmaStream->CR |= DMA_SxCR_EN;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void dma_stop(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	dmaStream->CR &= ~DMA_SxCR_EN;
	while (dmaStream->CR & DMA_SxCR_EN);
}



} // namespace
