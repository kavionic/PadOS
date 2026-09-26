// This file is part of PadOS.
//
// Copyright (c) 2022-2023 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "System/Platform.h"
#include "DMARequestID.h"

namespace kernel
{

static const int32_t DMA_CHANNEL_COUNT = 16;
static const int32_t DMA_CHANNELS_PER_UNIT = 8;
static const int32_t DMA_MAX_TRANSFER_LENGTH = 65535;

enum class DMADirection
{
	PeriphToMem = 0,
	MemToPeriph = 1,
	MemToMem	= 2,
};

enum class DMAWordSize
{
    WS8     = 0,
    WS16    = 1,
    WS32    = 2
};

int dma_allocate_channel();
void dma_free_channel(int channel);

IRQn_Type dma_get_channel_irq(int channel);

void    dma_setup(int channel, DMADirection mode, DMAMUX_REQUEST requestID, volatile const void* registerAddr, const void* memAddr, int32_t length);

uint32_t dma_get_interrupt_flags(int channel);
void     dma_clear_interrupt_flags(int channel, uint32_t flags);

int32_t dma_get_transfer_count(int channel);

void dma_start(int channel);
void dma_stop(int channel);

class DMAChannel
{
public:
    ~DMAChannel();
    bool AllocateChannel();
    void FreeChannel();

    IRQn_Type GetChannelIRQ() const;

    bool SetDirection(DMADirection mode);
    bool SetMemoryWordSize(DMAWordSize size);
    bool SetRegisterWordSize(DMAWordSize size);
    bool SetRequestID(DMAMUX_REQUEST requestID);
    bool SetRegisterAddress(volatile const void* address);
    bool SetMemoryAddress(const void* address);
    bool SetTransferLength(int32_t length);

    uint32_t GetInterruptFlags() const;
    bool     ClearInterruptFlags(uint32_t flags);

    int32_t GetTransferCount() const;

    bool Start();
    bool Stop();

private:
    int                 m_Channel = -1;
    int                 m_LocalChannel = 0;
    DMA_TypeDef*        m_DMA = nullptr;
    DMA_Stream_TypeDef* m_Stream = nullptr;
};

} // namespace
