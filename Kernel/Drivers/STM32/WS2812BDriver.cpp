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
// Created: 01.06.2020 16:30:05

#include "WS2812BDriver.h"

#include <malloc.h>
#include <algorithm>
#include <cstring>

#include "Ptr/Ptr.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/HAL/DMA.h"
#include "DeviceControl/WS2812B.h"
#include "Utils/Utils.h"
#include "GUI/Color.h"

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WS2812BDriverINode::WS2812BDriverINode(SPI_TypeDef* port, bool swapIOPins, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency, KFilesystemFileOps* fileOps)
	: KINode(nullptr, nullptr, fileOps, false)
	, m_Mutex("WS2812BDriverINodeWrite")
	, m_TransmitCondition("WS2812BDriverINodeTransmit")
	, m_DMARequestTX(dmaRequestTX)

{
	m_Port = port;

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

	if (swapIOPins) {
		m_Port->CFG2 |= SPI_CFG2_IOSWP;
	}

	m_Port->UDRDR = 0;
	m_Port->CR1 = SPI_CR1_HDDIR | SPI_CR1_CSTART | SPI_CR1_SPE;

	m_SendDMAChannel = dma_allocate_channel();

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

int WS2812BDriverINode::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
	CRITICAL_SCOPE(m_Mutex);

	int* inArg  = (int*)inData;
	int* outArg = (int*)outData;

	switch (request)
	{
		case WS2812BIOCTL_SET_LED_COUNT:
		    if (inArg == nullptr || inDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
		    return SetLEDCount(*inArg) ? 0 : -1;
		case WS2812BIOCTL_GET_LED_COUNT:
		    if (outArg == nullptr || outDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
		    *outArg = GetLEDCount();
		    return 0;
		case WS2812BIOCTL_SET_EXPONENTIAL:
		    if (inArg == nullptr || inDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
		    SetExponential(*inArg != 0);
		    return 0;
		case WS2812BIOCTL_GET_EXPONENTIAL:
		    if (outArg == nullptr || outDataLength != sizeof(int)) { set_last_error(EINVAL); return -1; }
		    *outArg = GetExponential() ? 1 : 0;
		    return 0;
		default: set_last_error(EINVAL); return -1;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t WS2812BDriverINode::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
	set_last_error(ENOSYS);
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t WS2812BDriverINode::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, const size_t length)
{
    if (position < 0 || (position & 3) || (length & 3)) {
	set_last_error(EINVAL);
	return -1;
    }
    CRITICAL_SCOPE(m_Mutex);

    size_t firstLED = position / 4;
    size_t lastLED  = std::min(m_LEDCount, firstLED + length / 4);
    if (lastLED < firstLED) {
	set_last_error(EINVAL);
	return -1;
    }
    const uint32_t* values = reinterpret_cast<const uint32_t*>(buffer);

    if (!WaitForIdle()) {
	return false;
    }

    // Expand the color bit patterns. Each source bit becomes 3 destination bits.
    // If the source is '0' one of the 3 is '1' to make a narrow pulse.
    // If the source is '1' two of the 3 is '1' to make a wide pulse.
    size_t targetPos = firstLED * 3;
    for (size_t i = firstLED; i < lastLED; ++i)
    {
	uint32_t color = values[i];

	if (m_Exponential)
	{
	    const Color tmp(color);
	    const float red	= float(tmp.GetRed()) / 255.0f;
	    const float green	= float(tmp.GetGreen()) / 255.0f;
	    const float blue	= float(tmp.GetBlue()) / 255.0f;
	    color = Color(uint8_t(red * red * 255.0f), uint8_t(green * green * 255.0f), uint8_t(blue * blue * 255.0f)).GetColor32();
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
    dma_setup(m_SendDMAChannel, DMAMode::MemToPeriph, m_DMARequestTX, &m_Port->TXDR, m_TransmitBuffer, bytesToSend);
    dma_start(m_SendDMAChannel);
    m_Port->CR1 |= SPI_CR1_CSTART;

    return length;
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

bool WS2812BDriverINode::WaitForIdle()
{
	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		while (m_State != State::Idle)
		{
            if (!m_TransmitCondition.IRQWaitTimeout(TimeValMicros::FromMilliseconds(100))) // Timeout in 100mS. Max transmit time is about 27mS (>7000LEDs)
			{
				if (get_last_error() != EINTR) {
					return false;
				}
			}
		}
	} CRITICAL_END;
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool WS2812BDriverINode::SetLEDCount(size_t count)
{
	if (count == m_LEDCount) {
		return true;
	}

	if (!WaitForIdle()) {
		return false;
	}

	const size_t bufferSize = (count * 3 * 3 + RESET_BYTE_COUNT + DCACHE_LINE_SIZE - 1) & ~DCACHE_LINE_SIZE_MASK;

	if (m_TransmitBuffer != nullptr) {
		free(m_TransmitBuffer);
	}
	m_TransmitBuffer = reinterpret_cast<uint8_t*>(memalign(DCACHE_LINE_SIZE, bufferSize));

	if (m_TransmitBuffer != nullptr) {
		memset(m_TransmitBuffer, 0, bufferSize);
		m_TransmitBufferSize = bufferSize;
		m_LEDCount = count;
		return true;
	} else {
		m_TransmitBufferSize = 0;
		m_LEDCount = 0;
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WS2812BDriver::WS2812BDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

WS2812BDriver::~WS2812BDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void WS2812BDriver::Setup(const char* devicePath, bool swapIOPins, SPI_TypeDef* port, DMAMUX1_REQUEST dmaRequestTX, uint32_t clockFrequency)
{
    Ptr<WS2812BDriverINode> node = ptr_new<WS2812BDriverINode>(port, swapIOPins, dmaRequestTX, clockFrequency, this);
    Kernel::RegisterDevice(devicePath, node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t WS2812BDriver::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    Ptr<WS2812BDriverINode> node = ptr_static_cast<WS2812BDriverINode>(file->GetINode());
	return node->Read(file, position, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t WS2812BDriver::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    Ptr<WS2812BDriverINode> node = ptr_static_cast<WS2812BDriverINode>(file->GetINode());
	return node->Write(file, position, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int WS2812BDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
	return ptr_static_cast<WS2812BDriverINode>(file->GetINode())->DeviceControl(file, request, inData, inDataLength, outData, outDataLength);
}



} // namespace
