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

#include "SDMMC_ATSAM.h"

#include "ASF/sam/drivers/xdmac/xdmac.h"

using namespace sdmmc;

static const uint32_t CONF_HSMCI_XDMAC_CHANNEL = 0;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static String GetStatusFlagNames(uint32_t flags)
{
    String result;
    
#define ADD_FLAG(name) if (flags & HSMCI_SR_##name##_Msk) { if (!result.empty()) result += ", "; result += #name; }
    
    ADD_FLAG(CMDRDY);             /**< (HSMCI_SR) Command Ready (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RXRDY);              /**< (HSMCI_SR) Receiver Ready (cleared by reading HSMCI_RDR) Mask */
    ADD_FLAG(TXRDY)               /**< (HSMCI_SR) Transmit Ready (cleared by writing in HSMCI_TDR) Mask */
    ADD_FLAG(BLKE)                /**< (HSMCI_SR) Data Block Ended (cleared on read) Mask */
    ADD_FLAG(DTIP)                /**< (HSMCI_SR) Data Transfer in Progress (cleared at the end of CRC16 calculation) Mask */
    ADD_FLAG(NOTBUSY)             /**< (HSMCI_SR) HSMCI Not Busy Mask */
    ADD_FLAG(SDIOIRQA)            /**< (HSMCI_SR) SDIO Interrupt for Slot A (cleared on read) Mask */
    ADD_FLAG(SDIOWAIT)            /**< (HSMCI_SR) SDIO Read Wait Operation Status Mask */
    ADD_FLAG(CSRCV)               /**< (HSMCI_SR) CE-ATA Completion Signal Received (cleared on read) Mask */
    ADD_FLAG(RINDE)               /**< (HSMCI_SR) Response Index Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RDIRE)               /**< (HSMCI_SR) Response Direction Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RCRCE)               /**< (HSMCI_SR) Response CRC Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RENDE)               /**< (HSMCI_SR) Response End Bit Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(RTOE)                /**< (HSMCI_SR) Response Time-out Error (cleared by writing in HSMCI_CMDR) Mask */
    ADD_FLAG(DCRCE)               /**< (HSMCI_SR) Data CRC Error (cleared on read) Mask */
    ADD_FLAG(DTOE)                /**< (HSMCI_SR) Data Time-out Error (cleared on read) Mask */
    ADD_FLAG(CSTOE)               /**< (HSMCI_SR) Completion Signal Time-out Error (cleared on read) Mask */
    ADD_FLAG(BLKOVRE)             /**< (HSMCI_SR) DMA Block Overrun Error (cleared on read) Mask */
    ADD_FLAG(FIFOEMPTY)           /**< (HSMCI_SR) FIFO empty flag Mask */
    ADD_FLAG(XFRDONE)             /**< (HSMCI_SR) Transfer Done flag Mask */
    ADD_FLAG(ACKRCV)              /**< (HSMCI_SR) Boot Operation Acknowledge Received (cleared on read) Mask */
    ADD_FLAG(ACKRCVE)             /**< (HSMCI_SR) Boot Operation Acknowledge Error (cleared on read) Mask */
    ADD_FLAG(OVRE)                /**< (HSMCI_SR) Overrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Mask */
    ADD_FLAG(UNRE)                /**< (HSMCI_SR) Underrun (if FERRCTRL = 1, cleared by writing in HSMCI_CMDR or cleared on read if FERRCTRL = 0) Mask */

#undef ADD_FLAG

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMC_ATSAM::SDMMC_ATSAM()
{
	DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN28_bm, DigitalPinPeripheralID::C);
	DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN25_bm, DigitalPinPeripheralID::D);
	DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN30_bm, DigitalPinPeripheralID::C);
	DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN31_bm, DigitalPinPeripheralID::C);
	DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN26_bm, DigitalPinPeripheralID::C);
	DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN27_bm, DigitalPinPeripheralID::C);

	DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN28_bm);
	DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN25_bm);
	DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN30_bm);
	DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN31_bm);
	DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN26_bm);
	DigitalPort::SetDriveStrength(e_DigitalPortID_A, DigitalPinDriveStrength_e::High, PIN27_bm);

	SAME70System::EnablePeripheralClock(ID_HSMCI);
	// Enable clock for DMA controller
	SAME70System::EnablePeripheralClock(ID_XDMAC);

	HSMCI->HSMCI_DTOR = HSMCI_DTOR_DTOMUL_1048576 | HSMCI_DTOR_DTOCYC(2);     // Set the Data Timeout Register to 2 Mega Cycles
	HSMCI->HSMCI_CSTOR = HSMCI_CSTOR_CSTOMUL_1048576 | HSMCI_CSTOR_CSTOCYC(2); // Set Completion Signal Timeout to 2 Mega Cycles
	HSMCI->HSMCI_CFG = HSMCI_CFG_FIFOMODE | HSMCI_CFG_FERRCTRL;              // Set Configuration Register
	HSMCI->HSMCI_MR = HSMCI_MR_PWSDIV_Msk;                                  // Set power saving to maximum value
	HSMCI->HSMCI_CR = HSMCI_CR_MCIEN_Msk | HSMCI_CR_PWSEN_Msk;              // Enable the HSMCI and the Power Saving

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SDMMC_ATSAM::~SDMMC_ATSAM()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMC_ATSAM::Setup(const os::String& devicePath, const DigitalPin& pinCD, IRQn_Type irqNum)
{
	if (!SetupBase(devicePath, pinCD)) return false;
	kernel::Kernel::RegisterIRQHandler(irqNum, IRQCallback, this);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult SDMMCDriver::HandleIRQ()
{
	for (;;)
	{
		uint32_t status = HSMCI->HSMCI_SR & HSMCI->HSMCI_IMR;

		static const uint32_t eventFlags = HSMCI_SR_XFRDONE_Msk | HSMCI_SR_CMDRDY_Msk | HSMCI_SR_NOTBUSY_Msk;
		static const uint32_t errorFlags = ~(eventFlags | HSMCI_SR_RXRDY_Msk);

		if (status & errorFlags)
		{
			HSMCI->HSMCI_IDR = ~0;
			m_IOError = EIO;
			kprintf("ERROR: SDMMCDriver::HandleIRQ() error: %s\n", GetStatusFlagNames(status).c_str());
			m_IOCondition.Wakeup(0);
			return IRQResult::HANDLED;
		}
		if (status & HSMCI_SR_RXRDY_Msk)
		{
			if (m_BytesToRead == 0)
			{
				HSMCI->HSMCI_IDR = ~0;
				m_IOError = EIO;
				kprintf("ERROR: SDMMCDriver::HandleIRQ() to many bytes received: %s\n", GetStatusFlagNames(status).c_str());
				m_IOCondition.Wakeup(0);
				return IRQResult::HANDLED;
			}
			if (HSMCI->HSMCI_MR & HSMCI_MR_FBYTE_Msk)
			{
				*reinterpret_cast<uint8_t*>(m_CurrentBuffer) = HSMCI->HSMCI_RDR;
				m_CurrentBuffer = reinterpret_cast<uint8_t*>(m_CurrentBuffer) + 1;
				m_BytesToRead--;

				if ((intptr_t(m_CurrentBuffer) & 3) == 0 && m_BytesToRead >= 4) {
					HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE_Msk;
				}
			}
			else
			{
				*reinterpret_cast<uint32_t*>(m_CurrentBuffer) = HSMCI->HSMCI_RDR;
				m_CurrentBuffer = reinterpret_cast<uint32_t*>(m_CurrentBuffer) + 1;
				m_BytesToRead -= 4;

				if (m_BytesToRead < 4 && m_BytesToRead != 0) {
					HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE_Msk;
				}
			}
			continue;
		}
		if (status & eventFlags) {
			HSMCI->HSMCI_IDR = ~0;
			m_IOError = 0;
			m_IOCondition.Wakeup(0);
		}
		break;
	}
	return IRQResult::HANDLED
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::WaitIRQ(uint32_t flags)
{
	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		HSMCI->HSMCI_IER = flags;
		while (!m_IOCondition.IRQWait())
		{
			if (get_last_error() != EINTR) {
				HSMCI->HSMCI_IDR = ~0;
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

bool SDMMCDriver::WaitIRQ(uint32_t flags, bigtime_t timeout)
{
	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		HSMCI->HSMCI_IER = flags;
		while (!m_IOCondition.IRQWaitTimeout(timeout))
		{
			if (get_last_error() != EINTR) {
				HSMCI->HSMCI_IDR = ~0;
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
/// \brief Reset the HSMCI interface
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::Reset()
{
	uint32_t mr = HSMCI->HSMCI_MR;
	uint32_t dtor = HSMCI->HSMCI_DTOR;
	uint32_t sdcr = HSMCI->HSMCI_SDCR;
	uint32_t cstor = HSMCI->HSMCI_CSTOR;
	uint32_t cfg = HSMCI->HSMCI_CFG;

	HSMCI->HSMCI_CR = HSMCI_CR_SWRST;
	HSMCI->HSMCI_MR = mr;
	HSMCI->HSMCI_DTOR = dtor;
	HSMCI->HSMCI_SDCR = sdcr;
	HSMCI->HSMCI_CSTOR = cstor;
	HSMCI->HSMCI_CFG = cfg;
	HSMCI->HSMCI_DMA = 0;
	HSMCI->HSMCI_CR = HSMCI_CR_PWSEN | HSMCI_CR_MCIEN;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::ReadNoDMA(void* buffer, uint16_t blockSize, size_t blockCount)
{
	m_CurrentBuffer = buffer;
	m_BytesToRead = blockSize * blockCount;
	m_IOError = 0;

	if ((intptr_t(buffer) & 3) || (m_BytesToRead < 4)) {
		HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE;
	}
	else {
		HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
	}

	return WaitIRQ(HSMCI_IER_RXRDY_Msk | HSMCI_IER_XFRDONE_Msk | HSMCI_IER_UNRE_Msk | HSMCI_IER_OVRE_Msk | HSMCI_IER_DTOE_Msk | HSMCI_IER_DCRCE_Msk);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::ReadDMA(void* buffer, uint16_t blockSize, size_t blockCount)
{
	if (blockCount == 0) {
		return true;
	}
	if (buffer == nullptr) {
		set_last_error(EINVAL);
		return false;
	}

	uint32_t bytesToRead = blockSize * blockCount;

	if ((intptr_t(buffer) & DCACHE_LINE_SIZE_MASK) || (bytesToRead & DCACHE_LINE_SIZE_MASK)) {
		kprintf("ERROR: SDMMCDriver::ReadDMA() called with unaligned buffer or size\n");
		set_last_error(EINVAL);
		return false;
	}

	HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
	xdmac_channel_config_t dmaCfg = { 0, 0, 0, 0, 0, 0, 0, 0 };

	dmaCfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
		| XDMAC_CC_MBSIZE_SINGLE
		| XDMAC_CC_DSYNC_PER2MEM
		| XDMAC_CC_CSIZE_CHK_1
		| XDMAC_CC_SIF_AHB_IF1
		| XDMAC_CC_DIF_AHB_IF0
		| XDMAC_CC_SAM_FIXED_AM
		| XDMAC_CC_DAM_INCREMENTED_AM
		| XDMAC_CC_DWIDTH_WORD
		| XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);

	dmaCfg.mbr_ubc = bytesToRead / 4;
	dmaCfg.mbr_sa = intptr_t(&HSMCI->HSMCI_FIFO[0]);
	dmaCfg.mbr_da = intptr_t(buffer);

	xdmac_configure_transfer(XDMAC, CONF_HSMCI_XDMAC_CHANNEL, &dmaCfg);
	xdmac_channel_enable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
	bool result = WaitIRQ(HSMCI_IER_XFRDONE_Msk | HSMCI_IER_UNRE_Msk | HSMCI_IER_OVRE_Msk | HSMCI_IER_DTOE_Msk | HSMCI_IER_DCRCE_Msk);
	xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);

	return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::WriteDMA(const void* buffer, uint16_t blockSize, size_t blockCount)
{
	if (blockCount == 0) {
		return true;
	}
	if (buffer == nullptr) {
		set_last_error(EINVAL);
		return false;
	}

	uint32_t bytesToWRite = blockSize * blockCount;

	xdmac_channel_config_t dmaCfg = { 0, 0, 0, 0, 0, 0, 0, 0 };

	dmaCfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
		| XDMAC_CC_MBSIZE_SINGLE
		| XDMAC_CC_DSYNC_MEM2PER
		| XDMAC_CC_CSIZE_CHK_1
		| XDMAC_CC_SIF_AHB_IF0
		| XDMAC_CC_DIF_AHB_IF1
		| XDMAC_CC_SAM_INCREMENTED_AM
		| XDMAC_CC_DAM_FIXED_AM
		| XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);

	if (intptr_t(buffer) & 3)
	{
		dmaCfg.mbr_cfg |= XDMAC_CC_DWIDTH_BYTE;
		dmaCfg.mbr_ubc = bytesToWRite;
		HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE;
	}
	else
	{
		dmaCfg.mbr_cfg |= XDMAC_CC_DWIDTH_WORD;
		dmaCfg.mbr_ubc = bytesToWRite / 4;
		HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
	}

	dmaCfg.mbr_sa = intptr_t(buffer);
	dmaCfg.mbr_da = intptr_t(&HSMCI->HSMCI_FIFO[0]);

	xdmac_configure_transfer(XDMAC, CONF_HSMCI_XDMAC_CHANNEL, &dmaCfg);
	xdmac_channel_enable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
	bool result = WaitIRQ(HSMCI_IER_XFRDONE_Msk | HSMCI_IER_UNRE_Msk | HSMCI_IER_OVRE_Msk | HSMCI_IER_DTOE_Msk | HSMCI_IER_DCRCE_Msk);
	xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Set HSMCI clock frequency.
///
/// \param frequency    HSMCI clock frequency in Hz.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::SetClockFrequency(uint32_t frequency)
{
	uint32_t peripheralClock = Kernel::GetFrequencyPeripheral();
	uint32_t divider = 0;

	// frequency = peripheralClock / (divider + 2)
	if (frequency * 2 < peripheralClock) {
		divider = (peripheralClock + frequency - 1) / frequency - 2;
	}

	uint32_t clockCfg = HSMCI_MR_CLKDIV(divider >> 1);
	if (divider & 1) clockCfg |= HSMCI_MR_CLKODD;

	HSMCI->HSMCI_MR = (HSMCI->HSMCI_MR & ~(HSMCI_MR_CLKDIV_Msk | HSMCI_MR_CLKODD)) | clockCfg;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::SendClock()
{
	// Configure command
	HSMCI->HSMCI_MR &= ~(HSMCI_MR_WRPROOF_Msk | HSMCI_MR_RDPROOF_Msk | HSMCI_MR_FBYTE_Msk);
	// Write argument
	HSMCI->HSMCI_ARGR = 0;
	// Write and start initialization command
	HSMCI->HSMCI_CMDR = HSMCI_CMDR_RSPTYP_NORESP | HSMCI_CMDR_SPCMD_INIT | HSMCI_CMDR_OPDCMD_OPENDRAIN;
	// Wait end of initialization command
	WaitIRQ(HSMCI_SR_CMDRDY_Msk);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Send a command
///
/// \param cmdr       CMDR register bit to use for this command
/// \param cmd        Command definition
/// \param arg        Argument of the command
///
/// \return true if success, otherwise false
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::ExecuteCmd(uint32_t cmdr, uint32_t cmd, uint32_t arg)
{
	cmdr |= HSMCI_CMDR_CMDNB(cmd) | HSMCI_CMDR_SPCMD_STD;
	if (cmd & SDMMC_RESP_PRESENT)
	{
		cmdr |= HSMCI_CMDR_MAXLAT;
		if (cmd & SDMMC_RESP_136) {
			cmdr |= HSMCI_CMDR_RSPTYP_136_BIT;
		}
		else if (cmd & SDMMC_RESP_BUSY) {
			cmdr |= HSMCI_CMDR_RSPTYP_R1B;
		}
		else {
			cmdr |= HSMCI_CMDR_RSPTYP_48_BIT;
		}
	}
	if (cmd & SDMMC_CMD_OPENDRAIN) {
		cmdr |= HSMCI_CMDR_OPDCMD_OPENDRAIN;
	}

	HSMCI->HSMCI_ARGR = arg;  // Write argument
	HSMCI->HSMCI_CMDR = cmdr; // Write and start command

	uint32_t errorFlags = HSMCI_SR_CSTOE_Msk | HSMCI_SR_RTOE_Msk | HSMCI_SR_RENDE_Msk | HSMCI_SR_RDIRE_Msk | HSMCI_SR_RINDE_Msk;
	if (cmd & SDMMC_RESP_CRC) {
		errorFlags |= HSMCI_SR_RCRCE_Msk;
	}
	// Wait end of command
	if (!WaitIRQ(HSMCI_SR_CMDRDY_Msk | errorFlags)) {
		return false;
	}
	if (cmd & SDMMC_RESP_BUSY)
	{
		// Should we have checked HSMCI_SR_DTIP_Msk to? Atmel do, but don't tell why.
		if (!WaitIRQ(HSMCI_SR_NOTBUSY_Msk, bigtime_from_s(1))) {
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::SendCmd(uint32_t cmd, uint32_t arg)
{
	// Configure command
	HSMCI->HSMCI_MR &= ~(HSMCI_MR_WRPROOF_Msk | HSMCI_MR_RDPROOF_Msk | HSMCI_MR_FBYTE_Msk);
	// Disable DMA for HSMCI
	HSMCI->HSMCI_DMA = 0;
	HSMCI->HSMCI_BLKR = 0;
	return ExecuteCmd(0, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t SDMMCDriver::GetResponse()
{
	return HSMCI->HSMCI_RSPR[0];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::GetResponse128(uint8_t* response)
{
	for (int i = 0; i < 4; ++i)
	{
		uint32_t response32 = GetResponse();
		*response++ = (response32 >> 24) & 0xff;
		*response++ = (response32 >> 16) & 0xff;
		*response++ = (response32 >> 8) & 0xff;
		*response++ = (response32 >> 0) & 0xff;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::StartAddressedDataTransCmd(uint32_t cmd, uint32_t arg, uint16_t blockSizePower, uint16_t blockCount, bool setupDMA)
{
	uint32_t cmdr;

	// Enable DMA for HSMCI
	HSMCI->HSMCI_DMA = (setupDMA) ? HSMCI_DMA_DMAEN : 0;

	// Enabling Read/Write Proof allows to stop the HSMCI Clock during
	// read/write  access if the internal FIFO is full.
	// This will guarantee data integrity, not bandwidth.
	HSMCI->HSMCI_MR |= HSMCI_MR_WRPROOF_Msk | HSMCI_MR_RDPROOF_Msk;
	// Force byte transfer if needed
	if (block_size & 0x3) {
		HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE_Msk;
	}
	else {
		HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE_Msk;
	}

	if (cmd & SDMMC_CMD_WRITE) {
		cmdr = HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRDIR_WRITE;
	}
	else {
		cmdr = HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRDIR_READ;
	}

	if (cmd & SDMMC_CMD_SDIO_BYTE)
	{
		cmdr |= HSMCI_CMDR_TRTYP_BYTE;
		// Value 0 corresponds to a 512-byte transfer
		HSMCI->HSMCI_BLKR = ((block_size % BLOCK_SIZE) << HSMCI_BLKR_BCNT_Pos);
	}
	else
	{
		HSMCI->HSMCI_BLKR = (block_size << HSMCI_BLKR_BLKLEN_Pos) | (blockCount << HSMCI_BLKR_BCNT_Pos);
		if (cmd & SDMMC_CMD_SDIO_BLOCK) {
			cmdr |= HSMCI_CMDR_TRTYP_BLOCK;
		}
		else if (cmd & SDMMC_CMD_STREAM) {
			cmdr |= HSMCI_CMDR_TRTYP_STREAM;
		}
		else if (cmd & SDMMC_CMD_SINGLE_BLOCK) {
			cmdr |= HSMCI_CMDR_TRTYP_SINGLE;
		}
		else if (cmd & SDMMC_CMD_MULTI_BLOCK) {
			cmdr |= HSMCI_CMDR_TRTYP_MULTIPLE;
		}
		else {
			kprintf("ERROR: StartAddressedDataTransCmd() invalid command flags: %lx\n", cmd);
			return false;
		}
	}
	return ExecuteCmd(cmdr, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool SDMMCDriver::StopAddressedDataTransCmd(uint32_t cmd, uint32_t arg)
{
	return ExecuteCmd(HSMCI_CMDR_TRCMD_STOP_DATA, cmd, arg);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief Configures the driver with the selected card configuration
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SDMMCDriver::ApplySpeedAndBusWidth()
{
	uint32_t busWidthCfg = HSMCI_SDCR_SDCBUS_1;

	if (m_HighSpeed) {
		HSMCI->HSMCI_CFG |= HSMCI_CFG_HSMODE;
	}
	else {
		HSMCI->HSMCI_CFG &= ~HSMCI_CFG_HSMODE;
	}

	SetClockFrequency(m_Clock);

	switch (m_BusWidth)
	{
	case 1: busWidthCfg = HSMCI_SDCR_SDCBUS_1; break;
	case 4: busWidthCfg = HSMCI_SDCR_SDCBUS_4; break;
	case 8: busWidthCfg = HSMCI_SDCR_SDCBUS_8; break;
	default:
		kprintf("ERROR: SDMMCDriver invalid bus width (%d) using 1-bit.\n", m_BusWidth);
		break;
	}
	HSMCI->HSMCI_SDCR = HSMCI_SDCR_SDCSEL_SLOTA | busWidthCfg;
}
