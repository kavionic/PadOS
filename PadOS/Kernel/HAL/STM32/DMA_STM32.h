#pragma once

#include "Platform.h"
#include "DMARequestID.h"

namespace kernel
{

static const int32_t DMA_CHANNEL_COUNT = 16;
static const int32_t DMA_CHANNELS_PER_UNIT = 8;
static const int32_t DMA_MAX_TRANSFER_LENGTH = 65535;

enum class DMAMode
{
	PeriphToMem = 0,
	MemToPeriph = 1,
	MemToMem	= 2,
};

int dma_allocate_channel();
void dma_free_channel(int channel);

IRQn_Type dma_get_channel_irq(int channel);

void    dma_setup(int channel, DMAMode mode, DMAMUX1_REQUEST requestID, volatile const void* registerAddr, const void* memAddr, int32_t length);

uint32_t dma_get_interrupt_flags(int channel);
void     dma_clear_interrupt_flags(int channel, uint32_t flags);

int32_t dma_get_transfer_count(int channel);

void dma_start(int channel);
void dma_stop(int channel);


} // namespace
