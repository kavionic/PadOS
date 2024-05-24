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

#ifdef STM32H7

#include "Kernel/HAL/STM32/DMA_STM32.h"

#include "Kernel/KMutex.h"
#include "Utils/Utils.h"

namespace kernel
{
#define IS_DMA_UART_USART_REQUEST_BASE(__REQUEST__) ((((__REQUEST__) >= DMAMUX_REQUEST::REQ_USART1_RX)  &&  ((__REQUEST__) <= DMAMUX_REQUEST::REQ_USART3_TX)) || \
                                                    (((__REQUEST__) >= DMAMUX_REQUEST::REQ_UART4_RX)  &&  ((__REQUEST__) <= DMAMUX_REQUEST::REQ_UART5_TX )) || \
                                                    (((__REQUEST__) >= DMAMUX_REQUEST::REQ_USART6_RX) &&  ((__REQUEST__) <= DMAMUX_REQUEST::REQ_USART6_TX)) || \
                                                    (((__REQUEST__) >= DMAMUX_REQUEST::REQ_UART7_RX)  &&  ((__REQUEST__) <= DMAMUX_REQUEST::REQ_UART8_TX )))

#if defined(UART9)
#define IS_DMA_UART_USART_REQUEST(__REQUEST__) (IS_DMA_UART_USART_REQUEST_BASE(__REQUEST__) || ((__REQUEST__) >= DMAMUX_REQUEST::REQ_UART9_RX)  &&  ((__REQUEST__) <= DMAMUX_REQUEST::REQ_USART10_TX )))
#else
#define IS_DMA_UART_USART_REQUEST(__REQUEST__) IS_DMA_UART_USART_REQUEST_BASE(__REQUEST__)
#endif

static uint32_t g_UsedChannels = 0;

KMutex g_DMAMutex("kernel_dma", EMutexRecursionMode::RaiseError);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC int dma_allocate_channel()
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

IFLASHC void dma_free_channel(int channel)
{
	CRITICAL_SCOPE(g_DMAMutex);

	g_UsedChannels &= ~(1 << channel);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC IRQn_Type dma_get_channel_irq(int channel)
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

IFLASHC void dma_setup(int channel, DMADirection mode, DMAMUX_REQUEST requestID, volatile const void* registerAddr, const void* memAddr, int32_t length)
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

IFLASHC uint32_t dma_get_interrupt_flags(int channel)
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

IFLASHC void dma_clear_interrupt_flags(int channel, uint32_t flags)
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

IFLASHC int32_t dma_get_transfer_count(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	return dmaStream->NDTR;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void dma_start(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	dmaStream->CR |= DMA_SxCR_EN;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void dma_stop(int channel)
{
	int localChannel = (channel < DMA_CHANNELS_PER_UNIT) ? channel : (channel - DMA_CHANNELS_PER_UNIT);
	auto dmaStream   = (channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + localChannel) : (DMA2_Stream0 + localChannel);

	dmaStream->CR &= ~DMA_SxCR_EN;
	while (dmaStream->CR & DMA_SxCR_EN);
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DMAChannel::~DMAChannel()
{
    if (m_Channel >= 0) {
        FreeChannel();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::AllocateChannel()
{
    if (m_Channel != -1) {
        return true;
    }

    CRITICAL_SCOPE(g_DMAMutex);

    uint32_t mask = 1;
    for (int i = 0; i < DMA_CHANNEL_COUNT; ++i, mask <<= 1)
    {
        if ((g_UsedChannels & mask) == 0)
        {
            g_UsedChannels |= mask;
            m_Channel = i;
            break;
        }
    }
    if (m_Channel == -1)
    {
        set_last_error(ENOENT);
        return false;
    }
    m_LocalChannel  = (m_Channel < DMA_CHANNELS_PER_UNIT) ? m_Channel : (m_Channel - DMA_CHANNELS_PER_UNIT);
    m_DMA           = (m_Channel < DMA_CHANNELS_PER_UNIT) ? DMA1 : DMA2;
    m_Stream        = (m_Channel < DMA_CHANNELS_PER_UNIT) ? (DMA1_Stream0 + m_LocalChannel) : (DMA2_Stream0 + m_LocalChannel);

    ClearInterruptFlags(DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0);
    m_Stream->CR = DMA_SxCR_MINC | DMA_SxCR_TCIE;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DMAChannel::FreeChannel()
{
    if (m_Channel != -1)
    {
        CRITICAL_SCOPE(g_DMAMutex);

        g_UsedChannels &= ~(1 << m_Channel);

        m_Channel   = -1;
        m_DMA       = nullptr;
        m_Stream    = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQn_Type DMAChannel::GetChannelIRQ() const
{
    static constexpr IRQn_Type table[] =
    {
        DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn, DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn,
        DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn, DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn
    };
    if (m_Channel >= 0 && m_Channel < ARRAY_COUNT(table))
    {
        return table[m_Channel];
    }
    else
    {
        set_last_error(ENODEV);
        return IRQn_Type(-1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::SetDirection(DMADirection mode)
{
    if (m_Channel >= 0)
    {
        set_bit_group(m_Stream->CR, DMA_SxCR_DIR_Msk, uint32_t(mode) << DMA_SxCR_DIR_Pos);
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::SetMemoryWordSize(DMAWordSize size)
{
    if (m_Channel >= 0)
    {
        set_bit_group(m_Stream->CR, DMA_SxCR_MSIZE_Msk, uint32_t(size) << DMA_SxCR_MSIZE_Pos);
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::SetRegisterWordSize(DMAWordSize size)
{
    if (m_Channel >= 0)
    {
        set_bit_group(m_Stream->CR, DMA_SxCR_PSIZE_Msk, uint32_t(size) << DMA_SxCR_PSIZE_Pos);
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::SetRequestID(DMAMUX_REQUEST requestID)
{
    if (m_Channel >= 0)
    {
        // Workaround for the "DMA stream locked when transferring data to/from USART/UART" errata.
    // Criteria taken from the ST HAL driver.
#if (STM32H7_DEV_ID == 0x450UL)
        if ((DBGMCU->IDCODE & 0xFFFF0000U) >= 0x20000000U)
#endif // STM32H7_DEV_ID == 0x450UL
        {
            if (IS_DMA_UART_USART_REQUEST(requestID)) {
                m_Stream->CR |= DMA_SxCR_TRBUFF;
            } else {
                m_Stream->CR &= ~DMA_SxCR_TRBUFF;
            }
        }
        DMAMUX1[m_Channel].CCR = uint32_t(requestID);

        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::SetRegisterAddress(volatile const void* address)
{
    if (m_Channel >= 0)
    {
        m_Stream->PAR = intptr_t(address);
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::SetMemoryAddress(const void* address)
{
    if (m_Channel >= 0)
    {
        m_Stream->M0AR = intptr_t(address);
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::SetTransferLength(int32_t length)
{
    if (m_Channel >= 0)
    {
        m_Stream->NDTR = length;
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t DMAChannel::GetInterruptFlags() const
{
    if (m_Channel < 0)
    {
        set_last_error(ENODEV);
        return 0;
    }
    if (m_LocalChannel < 4)
    {
        int offset = m_LocalChannel * 6;
        if (m_LocalChannel > 1) offset += 4;
        return m_DMA->LISR >> offset;
    }
    else
    {
        int offset = (m_LocalChannel - 4) * 6;
        if (m_LocalChannel > 5) offset += 4;
        return m_DMA->HISR >> offset;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::ClearInterruptFlags(uint32_t flags)
{
    if (m_Channel < 0)
    {
        set_last_error(ENODEV);
        return false;
    }
    if (m_LocalChannel < 4)
    {
        int offset = m_LocalChannel * 6;
        if (m_LocalChannel > 1) offset += 4;
        m_DMA->LIFCR = flags << offset;
    }
    else
    {
        int offset = (m_LocalChannel - 4) * 6;
        if (m_LocalChannel > 5) offset += 4;
        m_DMA->HIFCR = flags << offset;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t DMAChannel::GetTransferCount() const
{
    if (m_Channel >= 0)
    {
        return m_Stream->NDTR;
    }
    else
    {
        set_last_error(ENODEV);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::Start()
{
    if (m_Channel >= 0)
    {
        DMAMUX1_ChannelStatus->CFR = 1 << m_Channel;
        ClearInterruptFlags(DMA_LIFCR_CFEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTCIF0);
        m_Stream->CR |= DMA_SxCR_EN;
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DMAChannel::Stop()
{
    if (m_Channel >= 0)
    {
        m_Stream->CR &= ~DMA_SxCR_EN;
        while (m_Stream->CR & DMA_SxCR_EN);
        return true;
    }
    else
    {
        set_last_error(ENODEV);
        return false;
    }
}

} // namespace

#endif // STM32H7
