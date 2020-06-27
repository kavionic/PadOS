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
// Created: 20.05.2020 23:00:00

#include <malloc.h>
#include <string.h>

#include "SDMMCDriver_STM32.h"
#include "Kernel/SpinTimer.h"

using namespace kernel;
using namespace os;
using namespace sdmmc;

static constexpr uint32_t SDMMC_ICR_ALL_FLAGS = 
	  SDMMC_ICR_CCRCFAILC		
	| SDMMC_ICR_DCRCFAILC		
	| SDMMC_ICR_CTIMEOUTC		
	| SDMMC_ICR_DTIMEOUTC		
	| SDMMC_ICR_TXUNDERRC		
	| SDMMC_ICR_RXOVERRC		
	| SDMMC_ICR_CMDRENDC		
	| SDMMC_ICR_CMDSENTC		
	| SDMMC_ICR_DATAENDC		
	| SDMMC_ICR_DHOLDC		
	| SDMMC_ICR_DBCKENDC		
	| SDMMC_ICR_DABORTC		
	| SDMMC_ICR_BUSYD0ENDC	
	| SDMMC_ICR_SDIOITC		
	| SDMMC_ICR_ACKFAILC		
	| SDMMC_ICR_ACKTIMEOUTC	
	| SDMMC_ICR_VSWENDC		
	| SDMMC_ICR_CKSTOPC		
	| SDMMC_ICR_IDMATEC		
	| SDMMC_ICR_IDMABTCC;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::SDMMCDriver_STM32()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMCDriver_STM32::~SDMMCDriver_STM32()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::Setup(const os::String& devicePath, SDMMC_TypeDef* port, uint32_t peripheralClockFrequency, uint32_t clockCap, DigitalPinID pinCD, IRQn_Type irqNum)
{
	m_PeripheralClockFrequency = peripheralClockFrequency;
	m_ClockCap = clockCap;
	m_SDMMC = port;

	SetClockFrequency(SDMMC_CLOCK_INIT);
	m_SDMMC->POWER = 3 << SDMMC_POWER_PWRCTRL_Pos;

	if (!SetupBase(devicePath, pinCD)) return false;
	kernel::Kernel::RegisterIRQHandler(irqNum, IRQCallback, this);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Send a command
///
/// \param extraCmdRFlags	Extra CMD register bit to use for this command
/// \param cmd				Command definition
/// \param arg				Argument of the command
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::ExecuteCmd(uint32_t extraCmdRFlags, uint32_t cmd, uint32_t arg)
{
	uint32_t commandR = extraCmdRFlags | (SDMMC_CMD_GET_INDEX(cmd) << SDMMC_CMD_CMDINDEX_Pos) | SDMMC_CMD_CPSMEN;

	uint32_t response;

	uint32_t interrupts = SDMMC_MASK_CTIMEOUTIE;

	if (cmd & SDMMC_RESP_PRESENT)
	{
		m_SDMMC->DTIMER = 100000000;
		if (cmd & SDMMC_RESP_136) {
			response = 3; // Long response, expect CMDREND or CCRCFAIL flag
			interrupts |= SDMMC_MASK_CCRCFAILIE;
		} else if (cmd & SDMMC_RESP_CRC) {
			response = 1; // Short response, expect CMDREND or CCRCFAIL flag
			interrupts |= SDMMC_MASK_CCRCFAILIE;
		} else {
			response = 2; // Short response, expect CMDREND flag (No CRC)
		}
		interrupts |= SDMMC_MASK_CMDRENDIE; // ACKFAILIE | ACKTIMEOUTIE
	}
	else
	{
		response = 0; // No response, expect CMDSENT flag
		interrupts |= SDMMC_MASK_CMDSENTIE;
	}
	commandR |= response << SDMMC_CMD_WAITRESP_Pos;

	m_SDMMC->ICR = SDMMC_ICR_ALL_FLAGS;
	m_SDMMC->ARG = arg;
	m_SDMMC->CMD = commandR;

	if (!WaitIRQ(interrupts))
	{
		return false;
	}
	if ((cmd & SDMMC_RESP_BUSY) && (m_SDMMC->STA & SDMMC_STA_BUSYD0)) {
		if (!WaitIRQ(SDMMC_MASK_BUSYD0ENDIE, SDMMC_MASK_CTIMEOUTIE)) {
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::SendCmd(uint32_t cmd, uint32_t arg)
{
	m_SDMMC->DLEN = 0;
	return ExecuteCmd(0, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t SDMMCDriver_STM32::GetResponse()
{
	return m_SDMMC->RESP1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::GetResponse128(uint8_t* response)
{
	for (int i = 0; i < 4; ++i)
	{
		uint32_t response32 = (&m_SDMMC->RESP1)[i];
		*response++ = (response32 >> 24) & 0xff;
		*response++ = (response32 >> 16) & 0xff;
		*response++ = (response32 >> 8) & 0xff;
		*response++ = (response32 >> 0) & 0xff;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint32_t blockSizePower, uint16_t blockCount, const void* buffer)
{
	const uint32_t blockSize = 1 << blockSizePower;
	const uint32_t byteLength = blockSize * blockCount;

	void* dmaTarget = const_cast<void*>(buffer);

	uint32_t dataControl = (blockSizePower << SDMMC_DCTRL_DBLOCKSIZE_Pos);
	if (cmd & SDMMC_CMD_WRITE)
	{
		uint32_t* cacheStart = reinterpret_cast<uint32_t*>(intptr_t(dmaTarget) & DCACHE_LINE_SIZE_MASK);
		uint32_t  cacheLength = byteLength + intptr_t(dmaTarget) - intptr_t(cacheStart);
		cacheLength = ((cacheLength + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE;
		SCB_CleanDCache_by_Addr(cacheStart, cacheLength);
	}
	else
	{
		if ((intptr_t(buffer) & DCACHE_LINE_SIZE_MASK) || (byteLength & DCACHE_LINE_SIZE_MASK))
		{
			if (byteLength <= BLOCK_SIZE)
			{
				dmaTarget = m_CacheAlignedBuffer;
			}
			else
			{
				kprintf("ERROR: SDMMCDriver_STM32::StartAddressedDataTransCmd() called with unaligned buffer or size larger than 512 bytes.\n");
				set_last_error(EINVAL);
				return false;
			}
		}
		dataControl |= SDMMC_DCTRL_DTDIR; // From card to host (Read).
	}
	if (cmd & SDMMC_CMD_SDIO_BYTE)
	{
		dataControl |= 1 << SDMMC_DCTRL_DTMODE_Pos; // SDIO multibyte data transfer.
	}
	else
	{
		if (cmd & SDMMC_CMD_SDIO_BLOCK) {
			dataControl |= 0 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending on block count.
		} else if (cmd & SDMMC_CMD_STREAM) {
			dataControl |= 2 << SDMMC_DCTRL_DTMODE_Pos; // eMMC Stream data transfer. (WIDBUS shall select 1-bit wide bus mode)
		} else if (cmd & SDMMC_CMD_SINGLE_BLOCK) {
			dataControl |= 0 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending on block count.
		} else if (cmd & SDMMC_CMD_MULTI_BLOCK) {
//			dataControl |= 3 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending with STOP_TRANSMISSION command (not to be used with DTEN initiated data transfers).
			dataControl |= 0 << SDMMC_DCTRL_DTMODE_Pos; // Block data transfer ending with STOP_TRANSMISSION command (not to be used with DTEN initiated data transfers).
		} else {
			kprintf("ERROR: StartAddressedDataTransCmd() invalid command flags: %lx\n", cmd);
			return false;
		}
	}
	m_SDMMC->DTIMER = 100000000;
	m_SDMMC->CLKCR |= SDMMC_CLKCR_HWFC_EN; // Hardware flow-control enabled.
	m_SDMMC->IDMABASE0 = intptr_t(dmaTarget);
	m_SDMMC->IDMACTRL = SDMMC_IDMA_IDMAEN;
	m_SDMMC->DLEN = byteLength;
	m_SDMMC->DCTRL = dataControl;

	bool result = ExecuteCmd(SDMMC_CMD_CMDTRANS, cmd, arg);

	if (result)
	{
		result = WaitIRQ(SDMMC_MASK_DATAENDIE | SDMMC_MASK_IDMABTCIE | SDMMC_MASK_DABORTIE | SDMMC_MASK_DTIMEOUTIE | SDMMC_MASK_DCRCFAILIE);
	}
	m_SDMMC->IDMACTRL = 0;
	m_SDMMC->CLKCR &= ~SDMMC_CLKCR_HWFC_EN; // Hardware flow-control disabled.

	if (result)
	{
		if ((cmd & SDMMC_CMD_WRITE) == 0)
		{
			SCB_InvalidateDCache_by_Addr(reinterpret_cast<uint32_t*>(dmaTarget), ((byteLength + DCACHE_LINE_SIZE - 1) / DCACHE_LINE_SIZE) * DCACHE_LINE_SIZE);

			if (dmaTarget != buffer) {
				memcpy(const_cast<void*>(buffer), dmaTarget, byteLength);
			}
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg)
{
	return ExecuteCmd(SDMMC_CMD_CMDSTOP, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Configures the driver with the selected card configuration
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::ApplySpeedAndBusWidth()
{

	if (m_HighSpeed) {
		m_SDMMC->CLKCR |= SDMMC_CLKCR_NEGEDGE;
	} else {
		m_SDMMC->CLKCR &= ~SDMMC_CLKCR_NEGEDGE;
	}

	SetClockFrequency(m_Clock);

	uint32_t CLKCR = m_SDMMC->CLKCR;
	CLKCR &= ~SDMMC_CLKCR_WIDBUS_Msk;

	switch (m_BusWidth)
	{
		case 1: CLKCR |= 0 << SDMMC_CLKCR_WIDBUS_Pos; break;
		case 4: CLKCR |= 1 << SDMMC_CLKCR_WIDBUS_Pos; break;
		case 8: CLKCR |= 2 << SDMMC_CLKCR_WIDBUS_Pos; break;
		default:
			kprintf("ERROR: SDMMCDriver invalid bus width (%d) using 1-bit.\n", m_BusWidth);
			break;
	}
	m_SDMMC->CLKCR = CLKCR;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SDMMCDriver_STM32::HandleIRQ()
{
	uint32_t status = m_SDMMC->STA & m_SDMMC->MASK;

	static const uint32_t eventFlags =	  SDMMC_MASK_CMDRENDIE		// Command Response Received Interrupt Enable
										| SDMMC_MASK_CMDSENTIE      // Command Sent Interrupt Enable
										| SDMMC_MASK_DATAENDIE      // Data End Interrupt Enable
										| SDMMC_MASK_DHOLDIE        // Data Hold Interrupt Enable
										| SDMMC_MASK_DBCKENDIE      // Data Block End Interrupt Enable
										//| SDMMC_MASK_DABORTIE       // Data transfer aborted interrupt enable
										| SDMMC_MASK_TXFIFOHEIE     // Tx FIFO Half Empty interrupt Enable
										| SDMMC_MASK_RXFIFOHFIE     // Rx FIFO Half Full interrupt Enable
										| SDMMC_MASK_RXFIFOFIE      // Rx FIFO Full interrupt Enable
										| SDMMC_MASK_TXFIFOEIE      // Tx FIFO Empty interrupt Enable
										| SDMMC_MASK_BUSYD0ENDIE	// BUSYD0ENDIE interrupt Enable
										| SDMMC_MASK_SDIOITIE       // SDMMC Mode Interrupt Received interrupt Enable
										| SDMMC_MASK_VSWENDIE       // Voltage switch critical timing section completion Interrupt Enable
										| SDMMC_MASK_CKSTOPIE       // Voltage Switch clock stopped Interrupt Enable
										| SDMMC_MASK_IDMABTCIE;     // IDMA buffer transfer complete Interrupt Enable

	static const uint32_t errorFlags = ~eventFlags;

	if (status & errorFlags)
	{
		m_SDMMC->MASK = 0;
		m_IOError = EIO;
		m_IOCondition.Wakeup(0);
		return IRQResult::HANDLED;
	}
	if (status & eventFlags) {
		m_SDMMC->MASK = 0;
		m_IOError = 0;
		m_IOCondition.Wakeup(0);
	}
	return IRQResult::HANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::WaitIRQ(uint32_t flags)
{
	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		m_SDMMC->MASK = flags;
		while (!m_IOCondition.IRQWait())
		{
			if (get_last_error() != EINTR) {
				m_SDMMC->MASK = 0;
				m_IOError = get_last_error();
				break;
			}
		}
	} CRITICAL_END;
	if (m_IOError != 0) {
		Reset();
		set_last_error(m_IOError);
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver_STM32::WaitIRQ(uint32_t flags, bigtime_t timeout)
{
	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		m_SDMMC->MASK = flags;
		while (!m_IOCondition.IRQWaitTimeout(timeout))
		{
			if (get_last_error() != EINTR)
			{
				m_SDMMC->MASK = 0;
				m_IOError = get_last_error();
				break;
			}
		}
	} CRITICAL_END;
	if (m_IOError != 0)
	{
		Reset();
		set_last_error(m_IOError);
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Reset the SDMMC peripheral
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::Reset()
{
	RCC->AHB3RSTR |= RCC_AHB3RSTR_SDMMC1RST;
	RCC->AHB3RSTR &= ~RCC_AHB3RSTR_SDMMC1RST;
	ApplySpeedAndBusWidth();
	m_SDMMC->POWER = 3 << SDMMC_POWER_PWRCTRL_Pos;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Set SDMMC clock frequency.
///
/// \param frequency    SDMMC clock frequency in Hz.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::SetClockFrequency(uint32_t frequency)
{
    if (m_ClockCap != 0 && frequency > m_ClockCap) frequency = m_ClockCap;

    const uint32_t divider = (m_PeripheralClockFrequency + (frequency * 2) - 1) / (frequency * 2);

    uint32_t CLKCR = m_SDMMC->CLKCR;
    CLKCR &= ~SDMMC_CLKCR_CLKDIV_Msk;
    CLKCR |= divider << SDMMC_CLKCR_CLKDIV_Pos;
    m_SDMMC->CLKCR = CLKCR;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver_STM32::SendClock()
{
	uint32_t CLKCR = m_SDMMC->CLKCR;

	m_SDMMC->CLKCR &= ~SDMMC_CLKCR_PWRSAV;	// Disable power-save to make sure the clock is running.
	bigtime_t delay = (1000000 * 74 + m_Clock - 1) / m_Clock;	// Sleep for at least 74 SDMMC clock cycles.
	if (delay < 0) delay = 1;
	SpinTimer::SleepuS(delay);

	m_SDMMC->CLKCR = CLKCR; // Restore power-save.
}
