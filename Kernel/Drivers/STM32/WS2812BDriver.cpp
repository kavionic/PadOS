// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 01.06.2020 16:30:05

#include "Kernel/Drivers/STM32/WS2812BDriver.h"

#include <malloc.h>
#include <algorithm>
#include <cstring>

#include <Ptr/Ptr.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/HAL/DMA.h>
#include <Kernel/HAL/PeripheralMapping.h>
#include <DeviceControl/WS2812B.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>
#include <GUI/Color.h>

using namespace os;

namespace kernel
{


PREGISTER_KERNEL_DRIVER(WS2812BDriverINode, WS2812BDriverParameters);

static const uint8_t g_GammaTable[] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WS2812BDriverINode::WS2812BDriverINode(const WS2812BDriverParameters& parameters)
	: KINode(nullptr, nullptr, this, false)
	, m_Mutex("WS2812BDriverINodeWrite", PEMutexRecursionMode_RaiseError)
	, m_TransmitCondition("WS2812BDriverINodeTransmit")
{
	m_Port = get_spi_from_id(parameters.PortID);

    DigitalPin(parameters.PinData.PINID).SetDriveStrength(DigitalPinDriveStrength_e::Low);
    DigitalPin::ActivatePeripheralMux(parameters.PinData);

    DMAMUX_REQUEST dmaRequestRX;

    get_spi_dma_requests(parameters.PortID, dmaRequestRX, m_DMARequestTX);

//	const uint32_t bitRate = 800000 * 3;
	uint32_t divider = get_first_bit_index(64) - 1;
    //	for (divider = 0; divider < 8 && clockFrequency / (2 << divider) <= bitRate; ++divider);
//	if (divider >= 8) divider = 7;

	m_Port->CFG1 = (divider << SPI_CFG1_MBR_Pos) | SPI_CFG1_TXDMAEN | ((8-1) << SPI_CFG1_DSIZE_Pos);
	m_Port->CFG2	= SPI_CFG2_AFCNTR 
//					| SPI_CFG2_LSBFRST
					| SPI_CFG2_MASTER
					| SPI_CFG2_SSIOP
					| SPI_CFG2_SSM
					| (0 << SPI_CFG2_SP_Pos)	// Motorola.
					| (1 << SPI_CFG2_COMM_Pos);		// Simplex transmitter.

	if (parameters.SwapIOPins) {
		m_Port->CFG2 |= SPI_CFG2_IOSWP;
	}

	m_Port->UDRDR = 0;
	m_Port->CR1 = SPI_CR1_HDDIR | SPI_CR1_CSTART | SPI_CR1_SPE;

	m_SendDMAChannel = dma_allocate_channel();

	if (m_SendDMAChannel != -1)
	{
		auto irq = dma_get_channel_irq(m_SendDMAChannel);
		NVIC_ClearPendingIRQ(irq);
		register_irq_handler(irq, IRQCallbackSend, this);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WS2812BDriverINode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
	CRITICAL_SCOPE(m_Mutex);

	int* inArg  = (int*)inData;
	int* outArg = (int*)outData;

	switch (request)
	{
		case WS2812BIOCTL_SET_LED_COUNT:
            if (inArg == nullptr || inDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
            SetLEDCount(*inArg);
            return;
		case WS2812BIOCTL_GET_LED_COUNT:
		    if (outArg == nullptr || outDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
		    *outArg = GetLEDCount();
		    return;
		case WS2812BIOCTL_SET_EXPONENTIAL:
		    if (inArg == nullptr || inDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
		    SetExponential(*inArg != 0);
		    return;
		case WS2812BIOCTL_GET_EXPONENTIAL:
		    if (outArg == nullptr || outDataLength != sizeof(int)) PERROR_THROW_CODE(PErrorCode::InvalidArg);
		    *outArg = GetExponential() ? 1 : 0;
		    return;
		default: PERROR_THROW_CODE(PErrorCode::InvalidArg);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t WS2812BDriverINode::Write(Ptr<KFileNode> file, const void* buffer, const size_t length, off64_t position)
{
    if (position < 0 || (position & 3) || (length & 3)) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    CRITICAL_SCOPE(m_Mutex);

    size_t firstLED = size_t(position / 4);
    size_t lastLED = std::min(m_LEDCount, firstLED + length / 4);
    if (lastLED < firstLED) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    const uint32_t* values = reinterpret_cast<const uint32_t*>(buffer);

    const PErrorCode result = WaitForIdle();

    if (result != PErrorCode::Success) {
        PERROR_THROW_CODE(result);
    }

    // Expand the color bit patterns. Each source bit becomes 3 destination bits.
    // If the source is '0' one of the 3 is '1' to make a narrow pulse. 1/(200e6/64) = 320nS
    // If the source is '1' two of the 3 is '1' to make a wide pulse.   2/(200e6/64) = 640nS
    size_t targetPos = firstLED * 3;
    for (size_t i = firstLED; i < lastLED; ++i)
    {
        uint32_t color = values[i];

        if (m_Exponential)
        {
            const Color tmp(color);
            color = Color(g_GammaTable[tmp.GetRed()], g_GammaTable[tmp.GetGreen()], g_GammaTable[tmp.GetBlue()]).GetColor32();
        }
        uint32_t patternR = 0;
        uint32_t patternG = 0;
        uint32_t patternB = 0;

        const uint32_t maskR = 0x800000;
        const uint32_t maskG = 0x8000;
        const uint32_t maskB = 0x80;

        // Expand each of the 8-bit color components to 24-bit pulse patterns.
        for (uint32_t i = 0; i < 8; ++i)
        {
            patternR <<= 3;
            patternG <<= 3;
            patternB <<= 3;
            patternR |= (color & maskR) ? 0b011 : 0b010;
            patternG |= (color & maskG) ? 0b011 : 0b010;
            patternB |= (color & maskB) ? 0b011 : 0b010;
            color <<= 1;
        }

        // Write each pulse pattern to the transmit buffer in order G, R, B.
        for (int i = 2; i >= 0; --i) {
            m_TransmitBuffer[targetPos++] = (patternG >> (i * 8)) & 0xff;
        }
        for (int i = 2; i >= 0; --i) {
            m_TransmitBuffer[targetPos++] = (patternR >> (i * 8)) & 0xff;
        }
        for (int i = 2; i >= 0; --i) {
            m_TransmitBuffer[targetPos++] = (patternB >> (i * 8)) & 0xff;
        }
    }

    SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(m_TransmitBuffer), m_TransmitBufferSize);

    m_State = State::Sending;

    const uint32_t bytesToSend = (m_LEDCount * 3 * 3 + RESET_BYTE_COUNT + 3) & ~3;
    dma_setup(m_SendDMAChannel, DMADirection::MemToPeriph, m_DMARequestTX, &m_Port->TXDR, m_TransmitBuffer, bytesToSend);
    dma_start(m_SendDMAChannel);
    m_Port->CR1 |= SPI_CR1_CSTART;

    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult WS2812BDriverINode::IRQCallbackSend(IRQn_Type irq, void* userData)
{
    return static_cast<WS2812BDriverINode*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult WS2812BDriverINode::HandleIRQ()
{
	if (dma_get_interrupt_flags(m_SendDMAChannel) & DMA_LISR_TCIF0)
	{
		dma_clear_interrupt_flags(m_SendDMAChannel, DMA_LIFCR_CTCIF0);
		m_State = State::Idle;
		m_TransmitCondition.WakeupAll();
	}
	return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode WS2812BDriverINode::WaitForIdle()
{
	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		while (m_State != State::Idle)
		{
            const PErrorCode result = m_TransmitCondition.IRQWaitTimeout(TimeValNanos::FromMilliseconds(100));
            if (result != PErrorCode::Success) // Timeout in 100mS. Max transmit time is about 27mS (>7000LEDs)
			{
				if (result != PErrorCode::Interrupted)
                {
					return result;
				}
			}
		}
	} CRITICAL_END;
	return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WS2812BDriverINode::SetLEDCount(size_t count)
{
	if (count == m_LEDCount) {
		return;
	}

    const PErrorCode result = WaitForIdle();
	if (result != PErrorCode::Success) {
        PERROR_THROW_CODE(result);
	}

	const size_t bufferSize = (count * 3 * 3 + RESET_BYTE_COUNT + DCACHE_LINE_SIZE - 1) & ~DCACHE_LINE_SIZE_MASK;

	if (m_TransmitBuffer != nullptr) {
		free(m_TransmitBuffer);
	}
	m_TransmitBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, bufferSize));

	if (m_TransmitBuffer != nullptr)
    {
		memset(m_TransmitBuffer, 0, bufferSize);
		m_TransmitBufferSize = bufferSize;
		m_LEDCount = count;
	}
    else
    {
		m_TransmitBufferSize = 0;
		m_LEDCount = 0;
        PERROR_THROW_CODE(PErrorCode::NoMemory);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t WS2812BDriverINode::GetLEDCount() const
{
    return m_LEDCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WS2812BDriverINode::SetExponential(bool exponential)
{
    m_Exponential = exponential;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool WS2812BDriverINode::GetExponential() const
{
    return m_Exponential;
}


} // namespace
