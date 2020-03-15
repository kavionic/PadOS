#include "DMA_STM32.h"

#include "Kernel/KMutex.h"
#include "System/Utils/Utils.h"

namespace kernel
{

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

void dma_setup_mem_to_per(int channel, DMAMUX1_REQUEST requestID, volatile void* registerAddr, const void* buffer, int32_t length)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
//	auto dma         = (channel < DMA_CHANNELS_PER_UNIT) ? DMA1 : DMA2;
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	dmaStream->NDTR = length;
	dmaStream->PAR  = intptr_t(registerAddr);
	dmaStream->M0AR = intptr_t(buffer);
	dmaStream->CR   = DMA_SxCR_MINC | DMA_SxCR_DIR_0 | DMA_SxCR_TCIE;

	dma_clear_interrupt_flags(channel, DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0);
/*	if (localChannel < 4) {
		dma->LIFCR = (DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0) << (localChannel * 6);
	} else {
		dma->HIFCR = (DMA_HIFCR_CFEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTCIF4) << ((localChannel - 4) * 6);
	}*/
	DMAMUX1[channel].CCR = uint32_t(requestID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void dma_setup_per_to_mem(int channel, DMAMUX1_REQUEST requestID, void* buffer, volatile const void* registerAddr, int32_t length)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
//	auto dma         = (channel < DMA_CHANNELS_PER_UNIT) ? DMA1 : DMA2;
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	dmaStream->NDTR = length;
	dmaStream->PAR  = intptr_t(registerAddr);
	dmaStream->M0AR = intptr_t(buffer);
	dmaStream->CR   = DMA_SxCR_MINC | DMA_SxCR_TCIE;

	dma_clear_interrupt_flags(channel, DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0);
/*	if (localChannel < 4) {
		dma->LIFCR = (DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0) << (localChannel * 6);
	} else {
		dma->HIFCR = (DMA_HIFCR_CFEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTCIF4) << ((localChannel - 4) * 6);
	}*/
	DMAMUX1[channel].CCR = uint32_t(requestID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t dma_get_interrupt_flags(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dma         = (channel < DMA_CHANNELS_PER_UNIT) ? DMA1 : DMA2;

	if (localChannel < 4) {
		return dma->LISR >> (localChannel * 6);
	} else {
		return dma->HISR >> ((localChannel - 4) * 6);
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
		dma->LIFCR = flags << (localChannel * 6);
	} else {
		dma->HIFCR = flags << ((localChannel - 4) * 6);
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
	while(dmaStream->CR & DMA_SxCR_EN);
}



} // namespace
