// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 07.12.2017 23:50:32

#include "sam.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "HSMCIDriver.h"
#include "Kernel/SpinTimer.h"
#include "Kernel/HAL/SAME70System.h"
#include "ASF/sam/drivers/xdmac/xdmac.h"

using namespace kernel;

static const uint32_t CONF_HSMCI_XDMAC_CHANNEL = 0;
// Commands available in SPI mode

#define CMD_GO_IDLE_STATE          0 // CMD0: response R1
#define CMD_SEND_OP_COND           1 // CMD1: response R1
#define CMD_SEND_IF_COND           8 // CMD8: response R7
#define CMD_SEND_CSD               9 // CMD9: response R1
#define CMD_SEND_CID              10 // CMD10: response R1
#define CMD_STOP_TRANSMISSION     12 // CMD12: response R1b
#define CMD_SEND_STATUS           13 // CMD13: response R2
#define CMD_SET_BLOCKLEN          16 // CMD16: arg0[31:0]: block length, response R1
#define CMD_READ_SINGLE_BLOCK     17 // CMD17: arg0[31:0]: data address, response R1
#define CMD_READ_MULTIPLE_BLOCK   18 // CMD18: arg0[31:0]: data address, response R1
#define CMD_WRITE_SINGLE_BLOCK    24 // CMD24: arg0[31:0]: data address, response R1
#define CMD_WRITE_MULTIPLE_BLOCK  25 // CMD25: arg0[31:0]: data address, response R1
#define CMD_PROGRAM_CSD           27 // CMD27: response R1
#define CMD_SET_WRITE_PROT        28 // CMD28: arg0[31:0]: data address, response R1b
#define CMD_CLR_WRITE_PROT        29 // CMD29: arg0[31:0]: data address, response R1b
#define CMD_SEND_WRITE_PROT       30 // CMD30: arg0[31:0]: write protect data address, response R1
#define CMD_TAG_SECTOR_START      32 // CMD32: arg0[31:0]: data address, response R1
#define CMD_TAG_SECTOR_END        33 // CMD33: arg0[31:0]: data address, response R1
#define CMD_UNTAG_SECTOR          34 // CMD34: arg0[31:0]: data address, response R1
#define CMD_TAG_ERASE_GROUP_START 35 // CMD35: arg0[31:0]: data address, response R1
#define CMD_TAG_ERASE_GROUP_END   36 // CMD36: arg0[31:0]: data address, response R1
#define CMD_UNTAG_ERASE_GROUP     37 // CMD37: arg0[31:0]: data address, response R1
#define CMD_ERASE                 38 // CMD38: arg0[31:0]: stuff bits, response R1b
#define CMD_SD_SEND_OP_COND       41 // ACMD41: arg0[31:0]: OCR contents, response R1
#define CMD_LOCK_UNLOCK           42 // CMD42: arg0[31:0]: stuff bits, response R1b
#define CMD_APP                   55 // CMD55: arg0[31:0]: stuff bits, response R1
#define CMD_READ_OCR              58 // CMD58: arg0[31:0]: stuff bits, response R3
#define CMD_CRC_ON_OFF            59 // CMD59: arg0[31:1]: stuff bits, arg0[0:0]: crc option, response R1

// Command response flags
// R1/R1b: size 1 byte
#define R1_IDLE_STATE_bp    0
#define R1_IDLE_STATE_bm    (1<<R1_IDLE_STATE_bp)

#define R1_ERASE_RESET_bp   1
#define R1_ERASE_RESET_bm   (1<<R1_ERASE_RESET_bp)

#define R1_ILLEGAL_COMMAND_bp   2
#define R1_ILLEGAL_COMMAND_bm   (1<<R1_ILLEGAL_COMMAND_bp)

#define R1_COM_CRC_ERR_bp   3
#define R1_COM_CRC_ERR_bm   (1<<R1_COM_CRC_ERR_bp)

#define R1_ERASE_SEQ_ERR_bp 4
#define R1_ERASE_SEQ_ERR_bm (1<<R1_ERASE_SEQ_ERR_bp)

#define R1_ADDR_ERR_bp      5
#define R1_ADDR_ERR_bm      (1<<R1_ADDR_ERR_bp)

#define R1_PARAM_ERR_bp     6
#define R1_PARAM_ERR_bm     (1<<R1_PARAM_ERR_bp)

// R2: size 2 bytes
#define R2_CARD_LOCKED 0
#define R2_WP_ERASE_SKIP 1
#define R2_ERR 2
#define R2_CARD_ERR 3
#define R2_CARD_ECC_FAIL 4
#define R2_WP_VIOLATION 5
#define R2_INVAL_ERASE 6
#define R2_OUT_OF_RANGE 7
#define R2_CSD_OVERWRITE 7
/*#define R2_IDLE_STATE (R1_IDLE_STATE + 8)
#define R2_ERASE_RESET (R1_ERASE_RESET + 8)
#define R2_ILL_COMMAND (R1_ILL_COMMAND + 8)
#define R2_COM_CRC_ERR (R1_COM_CRC_ERR + 8)
#define R2_ERASE_SEQ_ERR (R1_ERASE_SEQ_ERR + 8)
#define R2_ADDR_ERR (R1_ADDR_ERR + 8)
#define R2_PARAM_ERR (R1_PARAM_ERR + 8)*/
/* R3: size 5 bytes */
#define R3_OCR_MASK (0xffffffffUL)
#define R3_IDLE_STATE (R1_IDLE_STATE + 32)
#define R3_ERASE_RESET (R1_ERASE_RESET + 32)
/*#define R3_ILL_COMMAND (R1_ILL_COMMAND + 32)
#define R3_COM_CRC_ERR (R1_COM_CRC_ERR + 32)
#define R3_ERASE_SEQ_ERR (R1_ERASE_SEQ_ERR + 32)
#define R3_ADDR_ERR (R1_ADDR_ERR + 32)
#define R3_PARAM_ERR (R1_PARAM_ERR + 32)*/
/* Data Response: size 1 byte */
#define DR_STATUS_MASK 0x0e
#define DR_STATUS_ACCEPTED 0x05
#define DR_STATUS_CRC_ERR 0x0a
#define DR_STATUS_WRITE_ERR 0x0c

/* status bits for card types */

#define SD_RAW_SPEC_1 0
#define SD_RAW_SPEC_2 1
#define SD_RAW_SPEC_SDHC 2

#define hsmci_debug printf
#define sd_mmc_debug printf

#define SD_MMC_DEBOUNCE_TIMEOUT   1000 // Unit ms

#  define SD_MMC_START_TIMEOUT()  SpinTimer::SleepMS(SD_MMC_DEBOUNCE_TIMEOUT)
#  define SD_MMC_IS_TIMEOUT()     true
#  define SD_MMC_STOP_TIMEOUT()

//! This SD MMC stack supports only the high voltage
#define SD_MMC_VOLTAGE_SUPPORT \
(OCR_VDD_27_28 | OCR_VDD_28_29 | \
OCR_VDD_29_30 | OCR_VDD_30_31 | \
OCR_VDD_31_32 | OCR_VDD_32_33)


#define IS_SDIO()  (m_CardType & CARD_TYPE_SDIO)

//! Current position (byte) of the transfer started by hsmci_adtc_start()
static uint32_t hsmci_transfert_pos;
//! Size block requested by last hsmci_adtc_start()
static uint16_t hsmci_block_size;
//! Total number of block requested by last hsmci_adtc_start()
static uint16_t hsmci_nb_block;



//! Index of current slot configurated
static uint8_t sd_mmc_slot_sel;
//! Pointer on current slot configurated
//static struct sd_mmc_card *sd_mmc_card;
//! Number of block to read or write on the current transfer
static uint16_t sd_mmc_nb_block_to_tranfer = 0;
//! Number of block remaining to read or write on the current transfer
static uint16_t sd_mmc_nb_block_remaining = 0;

//! SD/MMC transfer rate unit codes (10K) list
const uint32_t sd_mmc_trans_units[7] = {
    10, 100, 1000, 10000, 0, 0, 0
};
//! SD transfer multiplier factor codes (1/10) list
const uint32_t sd_trans_multipliers[16] = {
    0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};
//! MMC transfer multiplier factor codes (1/10) list
const uint32_t mmc_trans_multipliers[16] = {
    0, 10, 12, 13, 15, 20, 26, 30, 35, 40, 45, 52, 55, 60, 70, 80
};

static void hsmci_reset(void);
static void hsmci_set_speed(uint32_t speed, uint32_t mck);
static bool hsmci_wait_busy(void);
static bool hsmci_send_cmd_execute(uint32_t cmdr, sdmmc_cmd_def_t cmd, uint32_t arg);

/**
 * \brief Reset the HSMCI interface
 */
static void hsmci_reset(void)
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
    HSMCI->HSMCI_DMA = 0;
    // Enable the HSMCI
    HSMCI->HSMCI_CR = HSMCI_CR_PWSEN | HSMCI_CR_MCIEN;
}

/**
 * \brief Set speed of the HSMCI clock.
 *
 * \param speed    HSMCI clock speed in Hz.
 * \param mck      MCK clock speed in Hz.
 */
static void hsmci_set_speed(uint32_t speed, uint32_t mck)
{
    uint32_t clkdiv = 0;
    uint32_t clkodd = 0;
    // clock divider, represent (((clkdiv << 1) + clkodd) + 2)
    uint32_t div = 0;

    // Speed = MCK clock / (((clkdiv << 1) + clkodd) + 2)
    if ((speed * 2) < mck)
    {
        div = (mck / speed) - 2;
        if (mck % speed) {
            div++; // Ensure that the card speed not be higher than expected.
        }
        clkdiv = div >> 1;
        // clkodd is the last significant bit of the clock divider (div).
        clkodd = div % 2;
    } else {
        clkdiv = 0;
        clkodd = 0;
    }

    HSMCI->HSMCI_MR &= ~HSMCI_MR_CLKDIV_Msk;
    HSMCI->HSMCI_MR |= HSMCI_MR_CLKDIV(clkdiv);
    if (clkodd) {
        HSMCI->HSMCI_MR |= HSMCI_MR_CLKODD;
    } else {
        HSMCI->HSMCI_MR &= ~HSMCI_MR_CLKODD;
    }

}

/** \brief Wait the end of busy signal on data line
 *
 * \return true if success, otherwise false
 */
static bool hsmci_wait_busy(void)
{
    uint32_t busy_wait = 0xFFFFFFFF;
    uint32_t sr;

    do {
        sr = HSMCI->HSMCI_SR;
        if (busy_wait-- == 0) {
            hsmci_debug("%s: timeout\n\r", __func__);
            hsmci_reset();
            return false;
        }
    } while (!((sr & HSMCI_SR_NOTBUSY) && ((sr & HSMCI_SR_DTIP) == 0)));
    return true;
}


/** \brief Send a command
 *
 * \param cmdr       CMDR register bit to use for this command
 * \param cmd        Command definition
 * \param arg        Argument of the command
 *
 * \return true if success, otherwise false
 */
static bool hsmci_send_cmd_execute(uint32_t cmdr, sdmmc_cmd_def_t cmd, uint32_t arg)
{
    uint32_t sr;

    cmdr |= HSMCI_CMDR_CMDNB(cmd) | HSMCI_CMDR_SPCMD_STD;
    if (cmd & SDMMC_RESP_PRESENT) {
        cmdr |= HSMCI_CMDR_MAXLAT;
        if (cmd & SDMMC_RESP_136) {
            cmdr |= HSMCI_CMDR_RSPTYP_136_BIT;
        } else if (cmd & SDMMC_RESP_BUSY) {
            cmdr |= HSMCI_CMDR_RSPTYP_R1B;
        } else {
            cmdr |= HSMCI_CMDR_RSPTYP_48_BIT;
        }
    }
    if (cmd & SDMMC_CMD_OPENDRAIN) {
        cmdr |= HSMCI_CMDR_OPDCMD_OPENDRAIN;
    }

    // Write argument
    HSMCI->HSMCI_ARGR = arg;
    // Write and start command
    HSMCI->HSMCI_CMDR = cmdr;

    // Wait end of command
    do {
        sr = HSMCI->HSMCI_SR;
        if (cmd & SDMMC_RESP_CRC) {
            if (sr & (HSMCI_SR_CSTOE | HSMCI_SR_RTOE | HSMCI_SR_RENDE | HSMCI_SR_RCRCE | HSMCI_SR_RDIRE | HSMCI_SR_RINDE)) {
                hsmci_debug("%s: CMD 0x%08lx sr 0x%08lx error\n\r", __func__, cmd, sr);
                hsmci_reset();
                return false;
            }
        } else {
            if (sr & (HSMCI_SR_CSTOE | HSMCI_SR_RTOE | HSMCI_SR_RENDE | HSMCI_SR_RDIRE | HSMCI_SR_RINDE)) {
                hsmci_debug("%s: CMD 0x%08lx sr 0x%08lx error\n\r", __func__, cmd, sr);
                hsmci_reset();
                return false;
            }
        }
    } while (!(sr & HSMCI_SR_CMDRDY));

    if (cmd & SDMMC_RESP_BUSY) {
        if (!hsmci_wait_busy()) {
            return false;
        }
    }
    return true;
}


//-------------------------------------------------------------------
//--------------------- PUBLIC FUNCTIONS ----------------------------

void hsmci_init()
{
    SAME70System::EnablePeripheralClock(ID_HSMCI);

    // Enable clock for DMA controller
    SAME70System::EnablePeripheralClock(ID_XDMAC);

    // Set the Data Timeout Register to 2 Mega Cycles
    HSMCI->HSMCI_DTOR = HSMCI_DTOR_DTOMUL_1048576 | HSMCI_DTOR_DTOCYC(2);
    // Set Completion Signal Timeout to 2 Mega Cycles
    HSMCI->HSMCI_CSTOR = HSMCI_CSTOR_CSTOMUL_1048576 | HSMCI_CSTOR_CSTOCYC(2);
    // Set Configuration Register
    HSMCI->HSMCI_CFG = HSMCI_CFG_FIFOMODE | HSMCI_CFG_FERRCTRL;
    // Set power saving to maximum value
    HSMCI->HSMCI_MR = HSMCI_MR_PWSDIV_Msk;

    // Enable the HSMCI and the Power Saving
    HSMCI->HSMCI_CR = HSMCI_CR_MCIEN | HSMCI_CR_PWSEN;
}

void hsmci_select_device(uint32_t clock, uint8_t bus_width, bool high_speed)
{
    uint32_t hsmci_slot = HSMCI_SDCR_SDCSEL_SLOTA;
    uint32_t hsmci_bus_width = HSMCI_SDCR_SDCBUS_1;

    if (high_speed) {
        HSMCI->HSMCI_CFG |= HSMCI_CFG_HSMODE;
        } else {
        HSMCI->HSMCI_CFG &= ~HSMCI_CFG_HSMODE;
    }

    hsmci_set_speed(clock, SAME70System::GetFrequencyPeripheral());

    switch (bus_width)
    {
        case 1:
            hsmci_bus_width = HSMCI_SDCR_SDCBUS_1;
            break;
        case 4:
            hsmci_bus_width = HSMCI_SDCR_SDCBUS_4;
            break;

        case 8:
            hsmci_bus_width = HSMCI_SDCR_SDCBUS_8;
            break;

        default:
            assert(false); // Bus width wrong
    }
    HSMCI->HSMCI_SDCR = hsmci_slot | hsmci_bus_width;
}

void hsmci_deselect_device(uint8_t slot)
{
    (void)slot;
    // Nothing to do
}

void hsmci_send_clock(void)
{
    // Configure command
    HSMCI->HSMCI_MR &= ~(HSMCI_MR_WRPROOF | HSMCI_MR_RDPROOF | HSMCI_MR_FBYTE);
    // Write argument
    HSMCI->HSMCI_ARGR = 0;
    // Write and start initialization command
    HSMCI->HSMCI_CMDR = HSMCI_CMDR_RSPTYP_NORESP
    | HSMCI_CMDR_SPCMD_INIT
    | HSMCI_CMDR_OPDCMD_OPENDRAIN;
    // Wait end of initialization command
    while (!(HSMCI->HSMCI_SR & HSMCI_SR_CMDRDY));
}

bool hsmci_send_cmd(sdmmc_cmd_def_t cmd, uint32_t arg)
{
    // Configure command
    HSMCI->HSMCI_MR &= ~(HSMCI_MR_WRPROOF | HSMCI_MR_RDPROOF | HSMCI_MR_FBYTE);
    // Disable DMA for HSMCI
    HSMCI->HSMCI_DMA = 0;
    // Disable DMA for HSMCI
    HSMCI->HSMCI_DMA = 0;
    HSMCI->HSMCI_BLKR = 0;
    return hsmci_send_cmd_execute(0, cmd, arg);
}

uint32_t hsmci_get_response(void)
{
    return HSMCI->HSMCI_RSPR[0];
}

void hsmci_get_response_128(uint8_t* response)
{
    uint32_t response_32;

    for (uint8_t i = 0; i < 4; i++) {
        response_32 = HSMCI->HSMCI_RSPR[0];
        *response = (response_32 >> 24) & 0xFF;
        response++;
        *response = (response_32 >> 16) & 0xFF;
        response++;
        *response = (response_32 >>  8) & 0xFF;
        response++;
        *response = (response_32 >>  0) & 0xFF;
        response++;
    }
}

bool hsmci_adtc_start(sdmmc_cmd_def_t cmd, uint32_t arg, uint16_t block_size, uint16_t nb_block, bool access_block)
{
    uint32_t cmdr;

    if (access_block) {
        // Enable DMA for HSMCI
        HSMCI->HSMCI_DMA = HSMCI_DMA_DMAEN;
        } else {
        // Disable DMA for HSMCI
        HSMCI->HSMCI_DMA = 0;
    }

    if (access_block) {
        // Enable DMA for HSMCI
        HSMCI->HSMCI_DMA = HSMCI_DMA_DMAEN;
    } else {
        // Disable DMA for HSMCI
        HSMCI->HSMCI_DMA = 0;
    }
    // Enabling Read/Write Proof allows to stop the HSMCI Clock during
    // read/write  access if the internal FIFO is full.
    // This will guarantee data integrity, not bandwidth.
    HSMCI->HSMCI_MR |= HSMCI_MR_WRPROOF | HSMCI_MR_RDPROOF;
    // Force byte transfer if needed
    if (block_size & 0x3) {
        HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE;
        } else {
        HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
    }

    if (cmd & SDMMC_CMD_WRITE) {
        cmdr = HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRDIR_WRITE;
        } else {
        cmdr = HSMCI_CMDR_TRCMD_START_DATA | HSMCI_CMDR_TRDIR_READ;
    }

    if (cmd & SDMMC_CMD_SDIO_BYTE) {
        cmdr |= HSMCI_CMDR_TRTYP_BYTE;
        // Value 0 corresponds to a 512-byte transfer
        HSMCI->HSMCI_BLKR = ((block_size % 512) << HSMCI_BLKR_BCNT_Pos);
    } else {
        HSMCI->HSMCI_BLKR = (block_size << HSMCI_BLKR_BLKLEN_Pos) | (nb_block << HSMCI_BLKR_BCNT_Pos);
        if (cmd & SDMMC_CMD_SDIO_BLOCK) {
            cmdr |= HSMCI_CMDR_TRTYP_BLOCK;
        } else if (cmd & SDMMC_CMD_STREAM) {
            cmdr |= HSMCI_CMDR_TRTYP_STREAM;
        } else if (cmd & SDMMC_CMD_SINGLE_BLOCK) {
            cmdr |= HSMCI_CMDR_TRTYP_SINGLE;
        } else if (cmd & SDMMC_CMD_MULTI_BLOCK) {
            cmdr |= HSMCI_CMDR_TRTYP_MULTIPLE;
        } else {
            assert(false); // Incorrect flags
        }
    }
    hsmci_transfert_pos = 0;
    hsmci_block_size = block_size;
    hsmci_nb_block = nb_block;

    return hsmci_send_cmd_execute(cmdr, cmd, arg);
}

bool hsmci_adtc_stop(sdmmc_cmd_def_t cmd, uint32_t arg)
{
    return hsmci_send_cmd_execute(HSMCI_CMDR_TRCMD_STOP_DATA, cmd, arg);
}

bool hsmci_read_word(uint32_t* value)
{
    uint32_t sr;

    Assert(((uint32_t)hsmci_block_size * hsmci_nb_block) > hsmci_transfert_pos);

    // Wait data available
    do {
        sr = HSMCI->HSMCI_SR;
        if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE))
        {
            hsmci_debug("%s: DMA sr 0x%08lx error\n\r", __func__, sr);
            hsmci_reset();
            return false;
        }
    } while (!(sr & HSMCI_SR_RXRDY));

    // Read data
    *value = HSMCI->HSMCI_RDR;
    hsmci_transfert_pos += 4;
    if (((uint32_t)hsmci_block_size * hsmci_nb_block) > hsmci_transfert_pos) {
        return true;
    }

    // Wait end of transfer
    // Note: no need of timeout, because it is include in HSMCI
    do {
        sr = HSMCI->HSMCI_SR;
        if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE))
        {
            hsmci_debug("%s: DMA sr 0x%08lx error\n\r", __func__, sr);
            hsmci_reset();
            return false;
        }
    } while (!(sr & HSMCI_SR_XFRDONE));
    return true;
}

bool hsmci_write_word(uint32_t value)
{
    uint32_t sr;

    Assert(((uint32_t)hsmci_block_size * hsmci_nb_block) > hsmci_transfert_pos);

    // Wait data available
    do {
        sr = HSMCI->HSMCI_SR;
        if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE))
        {
            hsmci_debug("%s: DMA sr 0x%08lx error\n\r", __func__, sr);
            hsmci_reset();
            return false;
        }
    } while (!(sr & HSMCI_SR_TXRDY));

    // Write data
    HSMCI->HSMCI_TDR = value;
    hsmci_transfert_pos += 4;
    if (((uint32_t)hsmci_block_size * hsmci_nb_block) > hsmci_transfert_pos) {
        return true;
    }

    // Wait end of transfer
    // Note: no need of timeout, because it is include in HSMCI, see DTOE bit.
    do {
        sr = HSMCI->HSMCI_SR;
        if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | HSMCI_SR_DTOE | HSMCI_SR_DCRCE))
        {
            hsmci_debug("%s: DMA sr 0x%08lx error\n\r", __func__, sr);
            hsmci_reset();
            return false;
        }
    } while (!(sr & HSMCI_SR_NOTBUSY));
    Assert(HSMCI->HSMCI_SR & HSMCI_SR_FIFOEMPTY);
    return true;
}

bool hsmci_start_read_blocks(void *dest, uint16_t nb_block)
{
    xdmac_channel_config_t p_cfg = {0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t nb_data;

    Assert(nb_block);
    Assert(dest);

    xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);

    nb_data = nb_block * hsmci_block_size;

    if((uint32_t)dest & 3) {
        p_cfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
        | XDMAC_CC_MBSIZE_SINGLE
        | XDMAC_CC_DSYNC_PER2MEM
        | XDMAC_CC_CSIZE_CHK_1
        | XDMAC_CC_DWIDTH_BYTE
        | XDMAC_CC_SIF_AHB_IF1
        | XDMAC_CC_DIF_AHB_IF0
        | XDMAC_CC_SAM_FIXED_AM
        | XDMAC_CC_DAM_INCREMENTED_AM
        | XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);
        p_cfg.mbr_ubc = nb_data;
        HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE;
        } else {
        p_cfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
        | XDMAC_CC_MBSIZE_SINGLE
        | XDMAC_CC_DSYNC_PER2MEM
        | XDMAC_CC_CSIZE_CHK_1
        | XDMAC_CC_DWIDTH_WORD
        | XDMAC_CC_SIF_AHB_IF1
        | XDMAC_CC_DIF_AHB_IF0
        | XDMAC_CC_SAM_FIXED_AM
        | XDMAC_CC_DAM_INCREMENTED_AM
        | XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);
        p_cfg.mbr_ubc = nb_data / 4;
        HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
    }
    p_cfg.mbr_sa = (uint32_t)&(HSMCI->HSMCI_FIFO[0]);
    p_cfg.mbr_da = (uint32_t)dest;
    xdmac_configure_transfer(XDMAC, CONF_HSMCI_XDMAC_CHANNEL, &p_cfg);
    xdmac_channel_enable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
    hsmci_transfert_pos += nb_data;
    return true;
}

bool hsmci_wait_end_of_read_blocks(void)
{
    uint32_t sr;
    uint32_t dma_sr;
    // Wait end of transfer
    // Note: no need of timeout, because it is include in HSMCI
    do {
        sr = HSMCI->HSMCI_SR;
        if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | \
        HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
            hsmci_debug("%s: DMA sr 0x%08lx error\n\r",
            __func__, sr);
            hsmci_reset();
            // Disable XDMAC
            xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
            return false;
        }
        if (((uint32_t)hsmci_block_size * hsmci_nb_block) > hsmci_transfert_pos) {
            // It is not the end of all transfers
            // then just wait end of DMA
            dma_sr = xdmac_channel_get_interrupt_status(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
            if (dma_sr & XDMAC_CIS_BIS) {
                return true;
            }
        }
    } while (!(sr & HSMCI_SR_XFRDONE));
    return true;
}

bool hsmci_start_write_blocks(const void *src, uint16_t nb_block)
{
    xdmac_channel_config_t p_cfg = {0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t nb_data;

    Assert(nb_block);
    Assert(dest);

    xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);

    nb_data = nb_block * hsmci_block_size;

    if((uint32_t)src & 3) {
        p_cfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
        | XDMAC_CC_MBSIZE_SINGLE
        | XDMAC_CC_DSYNC_MEM2PER
        | XDMAC_CC_CSIZE_CHK_1
        | XDMAC_CC_DWIDTH_BYTE
        | XDMAC_CC_SIF_AHB_IF0
        | XDMAC_CC_DIF_AHB_IF1
        | XDMAC_CC_SAM_INCREMENTED_AM
        | XDMAC_CC_DAM_FIXED_AM
        | XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);
        p_cfg.mbr_ubc = nb_data;
        HSMCI->HSMCI_MR |= HSMCI_MR_FBYTE;
        } else {
        p_cfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
        | XDMAC_CC_MBSIZE_SINGLE
        | XDMAC_CC_DSYNC_MEM2PER
        | XDMAC_CC_CSIZE_CHK_1
        | XDMAC_CC_DWIDTH_WORD
        | XDMAC_CC_SIF_AHB_IF0
        | XDMAC_CC_DIF_AHB_IF1
        | XDMAC_CC_SAM_INCREMENTED_AM
        | XDMAC_CC_DAM_FIXED_AM
        | XDMAC_CC_PERID(CONF_HSMCI_XDMAC_CHANNEL);
        p_cfg.mbr_ubc = nb_data / 4;
        HSMCI->HSMCI_MR &= ~HSMCI_MR_FBYTE;
    }
    p_cfg.mbr_sa = (uint32_t)src;
    p_cfg.mbr_da = (uint32_t)&(HSMCI->HSMCI_FIFO[0]);
    xdmac_configure_transfer(XDMAC, CONF_HSMCI_XDMAC_CHANNEL, &p_cfg);
    xdmac_channel_enable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
    hsmci_transfert_pos += nb_data;
    return true;
}

bool hsmci_wait_end_of_write_blocks(void)
{
    uint32_t sr;
    uint32_t dma_sr;
    // Wait end of transfer
    // Note: no need of timeout, because it is include in HSMCI
    do {
        sr = HSMCI->HSMCI_SR;
        if (sr & (HSMCI_SR_UNRE | HSMCI_SR_OVRE | \
        HSMCI_SR_DTOE | HSMCI_SR_DCRCE)) {
            hsmci_debug("%s: DMA sr 0x%08lx error\n\r",
            __func__, sr);
            hsmci_reset();
            // Disable XDMAC
            xdmac_channel_disable(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
            return false;
        }
        if (((uint32_t)hsmci_block_size * hsmci_nb_block) > hsmci_transfert_pos) {
            // It is not the end of all transfers
            // then just wait end of DMA
            dma_sr = xdmac_channel_get_interrupt_status(XDMAC, CONF_HSMCI_XDMAC_CHANNEL);
            if (dma_sr & XDMAC_CIS_BIS) {
                return true;
            }
        }
    } while (!(sr & HSMCI_SR_XFRDONE));

    return true;
}
HSMCIDriver::HSMCIDriver(const DigitalPin& pinCD) : m_PinCD(pinCD)
{    
    m_CardType = 0; //e_CardTypeNone;
}


uint8_t HSMCIDriver::Initialize()
{
//    m_PinCS.Write(true);
//    m_PinCS.SetDirection(DigitalPinDirection_e::Out);
    m_PinCD.SetPullMode(PinPullMode_e::Up);

    SpinTimer::SleepMS(250);
    m_CardType = 0; //e_CardTypeNone;
    m_State = SD_MMC_CARD_STATE_NO_CARD;
    sd_mmc_slot_sel = 0xFF; // No slot configurated

    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN28_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN25_bm, DigitalPinPeripheralID::D);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN30_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN31_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN26_bm, DigitalPinPeripheralID::C);
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN27_bm, DigitalPinPeripheralID::C);
    
    hsmci_init();
    //hsmci_select_device(SDMMC_CLOCK_INIT, 4, true);
//    hsmci_set_speed(SDMMC_CLOCK_INIT, SAME70System::GetFrequencyPeripheral());
//    HSMCI.HSMCI_MR = HSMCI_MR_CLKDIV(255) | HSMCI_MR_PWSDIV(7) | HSMCI_MR_RDPROOF_Msk | HSMCI_MR_WRPROOF_Msk;
//    HSMCI.HSMCI_DTOR = HSMCI_DTOR_DTOCYC(15) | HSMCI_DTOR_DTOMUL_1048576;
//    HSMCI.HSMCI_CR = HSMCI_CR_MCIEN_Msk;
    //HSMCI.HSMCI_SDCR = HSMCI_SDCR_SDCSEL_SLOTA | HSMCI_SDCR_SDCBUS_4; // 4-bit bus width.

    for (;;)
    {
        sd_mmc_err_t result = sd_mmc_check(0);
        if (result == SD_MMC_OK || result == SD_MMC_INIT_ONGOING) break;
        if (result == SD_MMC_ERR_NO_CARD && m_State == SD_MMC_CARD_STATE_DEBOUNCE) continue;
        return false;
    }
    return true;
/*    if(!sd_raw_available())
    {
        printf("Error: %d\n", __LINE__);
        return 0;
    }
    uint8_t v2 = 0;
    uint8_t data = 0x08;

    // Card need minimum 74 clock cycles to start
    hsmci_send_clock();

    //select_card();

    return sd_mmc_mci_card_init();
//    return true;
*/
#if 0
    if ( m_CardType == e_CardTypeSD2 )
    {
        if ( SendCommand(CMD_READ_OCR, 0) )
        {
            unselect_card();
            printf("Error: %d\n", __LINE__);
            return 0;
        }

        if ( sd_raw_rec_byte() & 0x40 ) m_CardType = e_CardTypeSDHC;

        sd_raw_rec_byte();
        sd_raw_rec_byte();
        sd_raw_rec_byte();
    }

    /* set block size to 512 bytes */
    if(SendCommand(CMD_SET_BLOCKLEN, 512))
    {
        unselect_card();
        printf("Error: %d\n", __LINE__);
        return 0;
    }

    /* deaddress card */
    unselect_card();
    sd_raw_rec_byte(); // Run the clock for 8 cycles to complete transaction.

    /* switch to highest SPI frequency possible */

    //m_SPI.SetDivider(SD_CLK_DIVIDER_POST_INIT);

    printf("Card type: %d\n", m_CardType);
 #endif
    return 1;

}

/*void HSMCIDriver::select_card()
{
    m_PinCS.Write(false);
}

void HSMCIDriver::unselect_card()
{
    m_PinCS.Write(true);
//    m_SPI.Transfer(0xff);
}*/


/**
 * \ingroup sd_raw
 * Initializes memory card communication.
 *
 * \returns 0 on failure, 1 on success.
 */

/**
 * \ingroup sd_raw
 * Checks wether a memory card is located in the slot.
 *
 * \returns 1 if the card is available, 0 if it is not.
 */
uint8_t HSMCIDriver::sd_raw_available()
{
    return !m_PinCD.Read();
//    return true; //get_pin_available() == 0x00;
}

/**
 * \ingroup sd_raw
 * Checks wether the memory card is locked for write access.
 *
 * \returns 1 if the card is locked, 0 if it is not.
 */
uint8_t HSMCIDriver::sd_raw_locked()
{
    return false; //get_pin_locked() == 0x00;
}

/**
 * \ingroup sd_raw
 * Sends a raw byte to the memory card.
 *
 * \param[in] b The byte to sent.
 * \see sd_raw_rec_byte
 */
/*void HSMCIDriver::sd_raw_send_byte(uint8_t b)
{
    m_SPI.Transfer(b);
}*/

/**
 * \ingroup sd_raw
 * Receives a raw byte from the memory card.
 *
 * \returns The byte which should be read.
 * \see sd_raw_send_byte
 */
/*uint8_t HSMCIDriver::sd_raw_rec_byte()
{
    // send dummy data for receiving some.
    return m_SPI.Transfer(0xff);
}*/

/**
 * \ingroup sd_raw
 * Send a command to the memory card which responses with a R1 response (and possibly others).
 *
 * \param[in] command The command to send.
 * \param[in] arg The argument for command.
 * \returns The command answer.
 */
 #if 0
uint8_t HSMCIDriver::SendCommand(uint8_t command, uint32_t arg)
{
    uint8_t response;

    /* wait some clock cycles */
//    sd_raw_rec_byte();

    /* send command via SPI */
    sd_raw_send_byte(0x40 | command);
    sd_raw_send_byte((arg >> 24) & 0xff);
    sd_raw_send_byte((arg >> 16) & 0xff);
    sd_raw_send_byte((arg >> 8) & 0xff);
    sd_raw_send_byte((arg >> 0) & 0xff);
    switch(command)
    {
        case CMD_GO_IDLE_STATE:
           sd_raw_send_byte(0x95);
           break;
        case CMD_SEND_IF_COND:
           sd_raw_send_byte(0x87);
           break;
        default:
           sd_raw_send_byte(0xff);
           break;
    }
    
    /* receive response */
    for(uint8_t i = 0; i < 10; ++i)
    {
        response = sd_raw_rec_byte();
        if(response != 0xff)
            break;
    }

    return response;
}
#endif

int8_t HSMCIDriver::ReadBlocks(offset_t offset, uint8_t* buffer, int8_t length)
{
    sd_mmc_err_t result = sd_mmc_init_read_blocks(0, offset, length);
    result = sd_mmc_start_read_blocks(buffer, length);
    result = sd_mmc_wait_end_of_read_blocks(false);
    return length;
}

bool HSMCIDriver::StartReadBlocks(offset_t offset, uint16_t blockCount)
{
    sd_mmc_err_t result = sd_mmc_init_read_blocks(0, offset, blockCount);
    return result == SD_MMC_OK;
}

bool HSMCIDriver::ReadNextBlocks(uint8_t* buffer, int32_t length)
{
    return sd_mmc_start_read_blocks(buffer, length) == SD_MMC_OK;
}

bool HSMCIDriver::EndReadBlocks()
{
    return sd_mmc_wait_end_of_read_blocks(false) == SD_MMC_OK;
}

#if 0
int8_t HSMCIDriver::ReadBlocks(offset_t offset, uint8_t* buffer, int8_t length)
{
    //m_SPI.SetDivider(SD_CLK_DIVIDER_POST_INIT);

    while ( length > 0 )
    {
        select_card();
        uint8_t status;
        if ( (status = SendCommand(CMD_READ_SINGLE_BLOCK, (m_CardType == e_CardTypeSDHC) ? offset : (offset * 512))) )
        {
            unselect_card();
            printf("Error: %d (Failed to read sector %ld, %02x\n", __LINE__, offset, status);
            return 0;
        }

        m_SPI.StartBurst(0xff);

        // Setup the CRC generator.
        CRC.CTRL = CRC_RESET_RESET0_gc;
        CRC.CTRL = CRC_SOURCE_IO_gc;
        
        // Wait for data block start byte (0xfe)
        while ( m_SPI.ReadBurst(0xff) != 0xfe );

        //m_SPI.SetDivider(SD_CLK_DIVIDER_BURST);
        uint8_t i = 255;
        do
        {
            uint8_t data = m_SPI.ReadBurst(0xff);
            *buffer++ = data;
            CRC.DATAIN = data;
            data = m_SPI.ReadBurst(0xff);
            *buffer++ = data;
            CRC.DATAIN = data;
        } while(i--);
            
        // Read CRC.
        CRC.DATAIN = m_SPI.ReadBurst(0xff);
        CRC.DATAIN = m_SPI.EndBurst();

        // Tell the CRC generator that we are done.
        CRC.STATUS |= CRC_BUSY_bm;
        
//        return (CRC.STATUS & CRC_ZERO_bm) != 0 || 1;
            
        unselect_card();
    //m_SPI.SetDivider(SD_CLK_DIVIDER_POST_INIT);

        /* let card some time to finish */
        sd_raw_rec_byte();
        length--;
        offset++;
    }

    return 1;
}

bool HSMCIDriver::StartReadBlocks(offset_t offset)
{
    //m_SPI.SetDivider(SD_CLK_DIVIDER_POST_INIT);

    select_card();
    if ( SendCommand(CMD_READ_MULTIPLE_BLOCK, (m_CardType == e_CardTypeSDHC) ? offset : (offset * 512)) )
    {
        unselect_card();
        return false;
    }
    return true;
}


int8_t HSMCIDriver::ReadNextBlocks(uint8_t* buffer, int8_t length)
{
//    m_SPI.SetDivider(SD_CLK_DIVIDER_POST_INIT);
    while ( length )
    {
        StartBlock();
        uint8_t i = 255;
        do
        {
            uint8_t data = ReadNextByte();// m_SPI.Read();
            *buffer++ = data;
            CRC.DATAIN = data;
            data = ReadNextByte();
            *buffer++ = data;
            CRC.DATAIN = data;
        } while(i--);
        EndBlock();
        length--;
    }
    return 1;
}

bool HSMCIDriver::EndReadBlocks()
{
    SendCommand(CMD_STOP_TRANSMISSION, 0);
    
    // Wait for the busy flag to be cleared.
    while( m_SPI.Transfer(0xff) != 0xff );

    unselect_card();

    // Send 8 clock cycles to allow the card to settle
    m_SPI.Transfer(0xff);

    return true;
}


#if DOXYGEN || SD_RAW_WRITE_SUPPORT
/**
 * \ingroup sd_raw
 * Writes raw data to the card.
 *
 * \note If write buffering is enabled, you might have to
 *       call sd_raw_sync() before disconnecting the card
 *       to ensure all remaining data has been written.
 *
 * \param[in] offset The offset where to start writing.
 * \param[in] buffer The buffer containing the data to be written.
 * \param[in] length The number of bytes to write.
 * \returns 0 on failure, 1 on success.
 * \see sd_raw_write_interval, sd_raw_read, sd_raw_read_interval
 */

int8_t HSMCIDriver::WriteBlocks(offset_t offset, const uint8_t* buffer, int8_t length)
{
    if ( sd_raw_locked() ) return -1;

    while(length > 0)
    {
        /* address card */
        select_card();

        /* send single block request */
        if(SendCommand(CMD_WRITE_SINGLE_BLOCK, (m_CardType == e_CardTypeSDHC) ? block_address / 512 : block_address)))
        {
            unselect_card();
            return 0;
        }

        /* send start byte */
        sd_raw_send_byte(0xfe);

        /* write byte block */
        uint8_t* cache = raw_block;
        for(uint16_t i = 0; i < 512; ++i)
            sd_raw_send_byte(*cache++);

        /* write dummy crc16 */
        sd_raw_send_byte(0xff);
        sd_raw_send_byte(0xff);

        // Wait for the busy flag to be cleared.
        while( m_SPI.Transfer(0xff) != 0xff );
        sd_raw_rec_byte();

        /* deaddress card */
        unselect_card();

        buffer += write_length;
        offset += write_length;
        length -= write_length;

#if SD_RAW_WRITE_BUFFERING
        raw_block_written = 1;
#endif
    }

    return 1;
}
#endif

/**
 * \ingroup sd_raw
 * Reads informational data from the card.
 *
 * This function reads and returns the card's registers
 * containing manufacturing and status information.
 *
 * \note: The information retrieved by this function is
 *        not required in any way to operate on the card,
 *        but it might be nice to display some of the data
 *        to the user.
 *
 * \param[in] info A pointer to the structure into which to save the information.
 * \returns 0 on failure, 1 on success.
 */
uint8_t HSMCIDriver::sd_raw_get_info(struct sd_raw_info* info)
{
    if(!info || !sd_raw_available())
        return 0;

    memset(info, 0, sizeof(*info));

    select_card();

    /* read cid register */
    if(SendCommand(CMD_SEND_CID, 0))
    {
        unselect_card();
        return 0;
    }
    while(sd_raw_rec_byte() != 0xfe);
    for(uint8_t i = 0; i < 18; ++i)
    {
        uint8_t b = sd_raw_rec_byte();

        switch(i)
        {
            case 0:
                info->manufacturer = b;
                break;
            case 1:
            case 2:
                info->oem[i - 1] = b;
                break;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                info->product[i - 3] = b;
                break;
            case 8:
                info->revision = b;
                break;
            case 9:
            case 10:
            case 11:
            case 12:
                info->serial |= (uint32_t) b << ((12 - i) * 8);
                break;
            case 13:
                info->manufacturing_year = b << 4;
                break;
            case 14:
                info->manufacturing_year |= b >> 4;
                info->manufacturing_month = b & 0x0f;
                break;
        }
    }

    /* read csd register */
    uint8_t csd_read_bl_len = 0;
    uint8_t csd_c_size_mult = 0;
//#if SD_RAW_SDHC
    uint16_t csd_c_size = 0;
//#else
//    uint32_t csd_c_size = 0;
//#endif
    uint8_t csd_structure = 0;
    if(SendCommand(CMD_SEND_CSD, 0))
    {
        unselect_card();
        return 0;
    }
    while(sd_raw_rec_byte() != 0xfe);
    for(uint8_t i = 0; i < 18; ++i)
    {
        uint8_t b = sd_raw_rec_byte();

        if(i == 0)
        {
            csd_structure = b >> 6;
        }
        else if(i == 14)
        {
            if(b & 0x40)
                info->flag_copy = 1;
            if(b & 0x20)
                info->flag_write_protect = 1;
            if(b & 0x10)
                info->flag_write_protect_temp = 1;
            info->format = (b & 0x0c) >> 2;
        }
        else
        {
            if(csd_structure == 0x01)
            {
                switch(i)
                {
                    case 7:
                        b &= 0x3f;
                    case 8:
                    case 9:
                        csd_c_size <<= 8;
                        csd_c_size |= b;
                        break;
                }
                if(i == 9)
                {
                    ++csd_c_size;
                    info->capacity = (offset_t) csd_c_size * 512;
                }
            }
            else if(csd_structure == 0x00)
            {
                switch(i)
                {
                    case 5:
                        csd_read_bl_len = b & 0x0f;
                        break;
                    case 6:
                        csd_c_size = b & 0x03;
                        csd_c_size <<= 8;
                        break;
                    case 7:
                        csd_c_size |= b;
                        csd_c_size <<= 2;
                        break;
                    case 8:
                        csd_c_size |= b >> 6;
                        ++csd_c_size;
                        break;
                    case 9:
                        csd_c_size_mult = b & 0x03;
                        csd_c_size_mult <<= 1;
                        break;
                    case 10:
                        csd_c_size_mult |= b >> 7;

                        info->capacity = (uint32_t) csd_c_size << (csd_c_size_mult + csd_read_bl_len + 2);
                        break;
                }
            }
        }
    }

    unselect_card();

    return 1;
}
#endif // #if 0
/**
 * \brief Ask to all cards to send their operations conditions (MCI only).
 * - ACMD41 sends operation condition command.
 * - ACMD41 reads OCR
 *
 * \param v2   Shall be 1 if it is a SD card V2
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_mci_op_cond(uint8_t v2)
{
    uint32_t arg, retry, resp;

    /*
     * Timeout 1s = 400KHz / ((6+6+6+6)*8) cylces = 2100 retry
     * 6 = cmd byte size
     * 6 = response byte size
     * 6 = cmd byte size
     * 6 = response byte size
     */
    retry = 2100;
    do {
        // CMD55 - Indicate to the card that the next command is an
        // application specific command rather than a standard command.
        if (!hsmci_send_cmd(SDMMC_CMD55_APP_CMD, 0)) {
            sd_mmc_debug("%s: CMD55 Fail\n\r", __func__);
            return false;
        }

        // (ACMD41) Sends host OCR register
        arg = SD_MMC_VOLTAGE_SUPPORT;
        if (v2) {
            arg |= SD_ACMD41_HCS;
        }
        // Check response
        if (!hsmci_send_cmd(SD_MCI_ACMD41_SD_SEND_OP_COND, arg)) {
            sd_mmc_debug("%s: ACMD41 Fail\n\r", __func__);
            return false;
        }
        resp = hsmci_get_response();
        if (resp & OCR_POWER_UP_BUSY) {
            // Card is ready
            if ((resp & OCR_CCS) != 0) {
                m_CardType |= CARD_TYPE_HC;
            }
            break;
        }
        if (retry-- == 0) {
            sd_mmc_debug("%s: ACMD41 Timeout on busy, resp32 0x%08lx \n\r", __func__, resp);
            return false;
        }
    } while (1);
    return true;
}

/**
 * \brief Sends operation condition command and read OCR (MCI only)
 * - CMD1 sends operation condition command
 * - CMD1 reads OCR
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::mmc_mci_op_cond()
{
    uint32_t retry, resp;

    /*
     * Timeout 1s = 400KHz / ((6+6)*8) cylces = 4200 retry
     * 6 = cmd byte size
     * 6 = response byte size
     */
    retry = 4200;
    do {
        if (!hsmci_send_cmd(MMC_MCI_CMD1_SEND_OP_COND, SD_MMC_VOLTAGE_SUPPORT | OCR_ACCESS_MODE_SECTOR))
        {
            sd_mmc_debug("%s: CMD1 MCI Fail - Busy retry %d\n\r", __func__, (int)(4200 - retry));
            return false;
        }
        // Check busy flag
        resp = hsmci_get_response();
        if (resp & OCR_POWER_UP_BUSY)
        {
            // Check OCR value
            if ((resp & OCR_ACCESS_MODE_MASK) == OCR_ACCESS_MODE_SECTOR)
            {
                m_CardType |= CARD_TYPE_HC;
            }
            break;
        }
        if (retry-- == 0) {
            sd_mmc_debug("%s: CMD1 Timeout on busy\n\r", __func__);
            return false;
        }
    } while (1);
    return true;
}

/**
 * \brief Try to get the SDIO card's operating condition
 * - CMD5 to read OCR NF field
 * - CMD5 to wait OCR power up busy
 * - CMD5 to read OCR MP field
 *   m_CardType is updated
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sdio_op_cond()
{
    uint32_t resp;

    // CMD5 - SDIO send operation condition (OCR) command.
    if (!hsmci_send_cmd(SDIO_CMD5_SEND_OP_COND, 0)) {
        sd_mmc_debug("%s: CMD5 Fail\n\r", __func__);
        return true; // No error but card type not updated
    }
    resp = hsmci_get_response();
    if ((resp & OCR_SDIO_NF) == 0) {
        return true; // No error but card type not updated
    }

    /*
     * Wait card ready
     * Timeout 1s = 400KHz / ((6+4)*8) cylces = 5000 retry
     * 6 = cmd byte size
     * 4(SPI) 6(MCI) = response byte size
     */
    uint32_t cmd5_retry = 5000;
    while (1) {
        // CMD5 - SDIO send operation condition (OCR) command.
        if (!hsmci_send_cmd(SDIO_CMD5_SEND_OP_COND, resp & SD_MMC_VOLTAGE_SUPPORT)) {
            sd_mmc_debug("%s: CMD5 Fail\n\r", __func__);
            return false;
        }
        resp = hsmci_get_response();
        if ((resp & OCR_POWER_UP_BUSY) == OCR_POWER_UP_BUSY) {
            break;
        }
        if (cmd5_retry-- == 0) {
            sd_mmc_debug("%s: CMD5 Timeout on busy\n\r", __func__);
            return false;
        }
    }
    // Update card type at the end of busy
    if ((resp & OCR_SDIO_MP) > 0) {
        m_CardType = CARD_TYPE_SD_COMBO;
    } else {
        m_CardType = CARD_TYPE_SDIO;
    }
    return true; // No error and card type updated with SDIO type
}

/**
 * \brief CMD52 - SDIO IO_RW_DIRECT command
 *
 * \param rw_flag   Direction, 1:write, 0:read.
 * \param func_nb   Number of the function.
 * \param rd_after_wr Read after Write flag.
 * \param reg_addr  register address.
 * \param io_data   Pointer to input argument and response buffer.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sdio_cmd52(uint8_t rw_flag, uint8_t func_nb, uint32_t reg_addr, uint8_t rd_after_wr, uint8_t *io_data)
{
    assert(io_data != nullptr);
    if (!hsmci_send_cmd(SDIO_CMD52_IO_RW_DIRECT,
        ((uint32_t)*io_data << SDIO_CMD52_WR_DATA)
        | ((uint32_t)rw_flag << SDIO_CMD52_RW_FLAG)
        | ((uint32_t)func_nb << SDIO_CMD52_FUNCTION_NUM)
        | ((uint32_t)rd_after_wr << SDIO_CMD52_RAW_FLAG)
        | ((uint32_t)reg_addr << SDIO_CMD52_REG_ADRR))) {
        return false;
    }
    *io_data = hsmci_get_response() & 0xFF;
    return true;
}

/**
 * \brief CMD53 - SDIO IO_RW_EXTENDED command
 * This implementation support only the SDIO multi-byte transfer mode which is
 * similar to the single block transfer on memory.
 * Note: The SDIO block transfer mode is optional for SDIO card.
 *
 * \param rw_flag   Direction, 1:write, 0:read.
 * \param func_nb   Number of the function.
 * \param reg_addr  Register address.
 * \param inc_addr  1:Incrementing address, 0: fixed.
 * \param size      Transfer data size.
 * \param access_block  true, if the block access (DMA) is used
 *
 * \return true if success, otherwise false
 */
static bool sdio_cmd53(uint8_t rw_flag, uint8_t func_nb, uint32_t reg_addr, uint8_t inc_addr, uint32_t size, bool access_block)
{
    assert(size != 0);
    assert(size <= 512);

    return hsmci_adtc_start((rw_flag == SDIO_CMD53_READ_FLAG) ? SDIO_CMD53_IO_R_BYTE_EXTENDED : SDIO_CMD53_IO_W_BYTE_EXTENDED,
              ((size % 512) << SDIO_CMD53_COUNT)
            | ((uint32_t)reg_addr << SDIO_CMD53_REG_ADDR)
            | ((uint32_t)inc_addr << SDIO_CMD53_OP_CODE)
            | ((uint32_t)0 << SDIO_CMD53_BLOCK_MODE)
            | ((uint32_t)func_nb << SDIO_CMD53_FUNCTION_NUM)
            | ((uint32_t)rw_flag << SDIO_CMD53_RW_FLAG),
            size, 1, access_block);
}

/**
 * \brief Get SDIO max transfer speed in Hz.
 * - CMD53 reads CIS area address in CCCR area.
 * - Nx CMD53 search Fun0 tuple in CIS area
 * - CMD53 reads TPLFE_MAX_TRAN_SPEED in Fun0 tuple
 * - Compute maximum speed of SDIO
 *   and update m_Clock
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sdio_get_max_speed()
{
    uint32_t addr_new, addr_old;
    uint8_t buf[6];
    uint32_t unit;
    uint32_t mul;
    uint8_t tplfe_max_tran_speed, i;
    uint8_t addr_cis[4];

    /* Read CIS area address in CCCR area */
    addr_old = SDIO_CCCR_CIS_PTR;
    for(i = 0; i < 4; i++) {
        sdio_cmd52(SDIO_CMD52_READ_FLAG, SDIO_CIA, addr_old, 0, &addr_cis[i]);
        addr_old++;
    }
    addr_old = addr_cis[0] + (addr_cis[1] << 8) + (addr_cis[2] << 16) + (addr_cis[3] << 24);
    addr_new = addr_old;

    while (1) {
        /* Read a sample of CIA area */
        for(i=0; i<3; i++) {
            sdio_cmd52(SDIO_CMD52_READ_FLAG, SDIO_CIA, addr_new, 0, &buf[i]);
            addr_new++;
        }
        if (buf[0] == SDIO_CISTPL_END) {
            return false; /* Tuple error */
        }
        if (buf[0] == SDIO_CISTPL_FUNCE && buf[2] == 0x00) {
            break; /* Fun0 tuple found */
        }
        if (buf[1] == 0) {
            return false; /* Tuple error */
        }
        /* Next address */
        addr_new += buf[1]-1;
        if (addr_new > (addr_old + 256)) {
            return false; /* Outoff CIS area */
        }
    }

    /* Read all Fun0 tuple fields: fn0_blk_siz & max_tran_speed */
    addr_new -= 3;
    for(i = 0; i < 6; i++) {
        sdio_cmd52(SDIO_CMD52_READ_FLAG, SDIO_CIA, addr_new, 0, &buf[i]);
        addr_new++;
    }

    tplfe_max_tran_speed = buf[5];
    if (tplfe_max_tran_speed > 0x32) {
        /* Error on SDIO register, the high speed is not activated
         * and the clock can not be more than 25MHz.
         * This error is present on specific SDIO card
         * (H&D wireless card - HDG104 WiFi SIP).
         */
        tplfe_max_tran_speed = 0x32; /* 25Mhz */
    }

    /* Decode transfer speed in Hz.*/
    unit = sd_mmc_trans_units[tplfe_max_tran_speed & 0x7];
    mul = sd_trans_multipliers[(tplfe_max_tran_speed >> 3) & 0xF];
    m_Clock = unit * mul * 1000;
    /**
     * Note: A combo card shall be a Full-Speed SDIO card
     * which supports upto 25MHz.
     * A SDIO card alone can be:
     * - a Low-Speed SDIO card which supports 400Khz minimum
     * - a Full-Speed SDIO card which supports upto 25MHz
     */
    return true;
}

/**
 * \brief CMD52 for SDIO - Switches the bus width mode to 4
 *
 * \note m_BusWidth is updated.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sdio_cmd52_set_bus_width()
{
    /**
     * A SD memory card always supports bus 4bit
     * A SD COMBO card always supports bus 4bit
     * A SDIO Full-Speed alone always supports 4bit
     * A SDIO Low-Speed alone can supports 4bit (Optional)
     */
    uint8_t u8_value = 0;

    // Check 4bit support in 4BLS of "Card Capability" register
    if (!sdio_cmd52(SDIO_CMD52_READ_FLAG, SDIO_CIA, SDIO_CCCR_CAP, 0, &u8_value)) {
        return false;
    }
    if ((u8_value & SDIO_CAP_4BLS) != SDIO_CAP_4BLS) {
        // No supported, it is not a protocol error
        return true;
    }
    // HS mode possible, then enable
    u8_value = SDIO_BUSWIDTH_4B;
    if (!sdio_cmd52(SDIO_CMD52_WRITE_FLAG, SDIO_CIA, SDIO_CCCR_BUS_CTRL, 1, &u8_value)) {
        return false;
    }
    m_BusWidth = 4;
    sd_mmc_debug("%d-bit bus width enabled.\n\r", (int)m_BusWidth);
    return true;
}

/**
 * \brief CMD6 for SD - Switch card in high speed mode
 *
 * \note CMD6 for SD is valid under the "trans" state.
 * \note m_HighSpeed is updated.
 * \note m_Clock is updated.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_cm6_set_high_speed()
{
    uint8_t switch_status[SD_SW_STATUS_BSIZE] = {0};

    if (!hsmci_adtc_start(SD_CMD6_SWITCH_FUNC,
            SD_CMD6_MODE_SWITCH
            | SD_CMD6_GRP6_NO_INFLUENCE
            | SD_CMD6_GRP5_NO_INFLUENCE
            | SD_CMD6_GRP4_NO_INFLUENCE
            | SD_CMD6_GRP3_NO_INFLUENCE
            | SD_CMD6_GRP2_DEFAULT
            | SD_CMD6_GRP1_HIGH_SPEED,
            SD_SW_STATUS_BSIZE, 1, true)) {
        return false;
    }
    if (!hsmci_start_read_blocks(switch_status, 1)) {
        return false;
    }
    if (!hsmci_wait_end_of_read_blocks()) {
        return false;
    }

    if (hsmci_get_response() & CARD_STATUS_SWITCH_ERROR) {
        sd_mmc_debug("%s: CMD6 CARD_STATUS_SWITCH_ERROR\n\r", __func__);
        return false;
    }
    if (SD_SW_STATUS_FUN_GRP1_RC(switch_status) == SD_SW_STATUS_FUN_GRP_RC_ERROR) {
        // No supported, it is not a protocol error
        return true;
    }
    if (SD_SW_STATUS_FUN_GRP1_BUSY(switch_status)) {
        sd_mmc_debug("%s: CMD6 SD_SW_STATUS_FUN_GRP1_BUSY\n\r", __func__);
        return false;
    }
    // CMD6 function switching period is within 8 clocks
    // after the end bit of status data.
    hsmci_send_clock();
    m_HighSpeed = true;
    m_Clock *= 2;
    return true;
}

/**
 * \brief CMD6 for MMC - Switches the bus width mode
 *
 * \note CMD6 is valid under the "trans" state.
 * \note m_BusWidth is updated.
 *
 * \param bus_width   Bus width to set
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::mmc_cmd6_set_bus_width(uint8_t bus_width)
{
    uint32_t arg;

    switch (bus_width) {
    case 8:
        arg = MMC_CMD6_ACCESS_SET_BITS | MMC_CMD6_INDEX_BUS_WIDTH | MMC_CMD6_VALUE_BUS_WIDTH_8BIT;
        break;
    case 4:
        arg = MMC_CMD6_ACCESS_SET_BITS | MMC_CMD6_INDEX_BUS_WIDTH | MMC_CMD6_VALUE_BUS_WIDTH_4BIT;
        break;
    default:
        arg = MMC_CMD6_ACCESS_SET_BITS | MMC_CMD6_INDEX_BUS_WIDTH | MMC_CMD6_VALUE_BUS_WIDTH_1BIT;
        break;
    }
    if (!hsmci_send_cmd(MMC_CMD6_SWITCH, arg)) {
        return false;
    }
    if (hsmci_get_response() & CARD_STATUS_SWITCH_ERROR) {
        // No supported, it is not a protocol error
        sd_mmc_debug("%s: CMD6 CARD_STATUS_SWITCH_ERROR\n\r", __func__);
        return false;
    }
    m_BusWidth = bus_width;
    sd_mmc_debug("%d-bit bus width enabled.\n\r", (int)m_BusWidth);
    return true;
}

/**
 * \brief CMD6 for MMC - Switches in high speed mode
 *
 * \note CMD6 is valid under the "trans" state.
 * \note m_HighSpeed is updated.
 * \note m_Clock is updated.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::mmc_cmd6_set_high_speed()
{
    if (!hsmci_send_cmd(MMC_CMD6_SWITCH, MMC_CMD6_ACCESS_WRITE_BYTE | MMC_CMD6_INDEX_HS_TIMING | MMC_CMD6_VALUE_HS_TIMING_ENABLE)) {
        return false;
    }
    if (hsmci_get_response() & CARD_STATUS_SWITCH_ERROR) {
        // No supported, it is not a protocol error
        sd_mmc_debug("%s: CMD6 CARD_STATUS_SWITCH_ERROR\n\r", __func__);
        return false;
    }
    m_HighSpeed = true;
    m_Clock = 52000000lu;
    return true;
}

/**
 * \brief CMD9: Addressed card sends its card-specific
 * data (CSD) on the CMD line mci.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_mmc_cmd9_mci()
{
    if (!hsmci_send_cmd(SDMMC_MCI_CMD9_SEND_CSD, (uint32_t)m_RCA << 16)) {
        return false;
    }
    hsmci_get_response_128(m_CSD);
    return true;
}

/**
 * \brief Decodes MMC CSD register
 */
void HSMCIDriver::mmc_decode_csd()
{
    uint32_t unit;
    uint32_t mul;
    uint32_t tran_speed;

    // Get MMC System Specification version supported by the card
    switch (MMC_CSD_SPEC_VERS(m_CSD)) {
    default:
    case 0:
        m_CardVersion = CARD_VER_MMC_1_2;
        break;

    case 1:
        m_CardVersion = CARD_VER_MMC_1_4;
        break;

    case 2:
        m_CardVersion = CARD_VER_MMC_2_2;
        break;

    case 3:
        m_CardVersion = CARD_VER_MMC_3;
        break;

    case 4:
        m_CardVersion = CARD_VER_MMC_4;
        break;
    }

    // Get MMC memory max transfer speed in Hz.
    tran_speed = CSD_TRAN_SPEED(m_CSD);
    unit = sd_mmc_trans_units[tran_speed & 0x7];
    mul = mmc_trans_multipliers[(tran_speed >> 3) & 0xF];
    m_Clock = unit * mul * 1000;

    /*
     * Get card capacity.
     * ----------------------------------------------------
     * For normal SD/MMC card:
     * memory capacity = BLOCKNR * BLOCK_LEN
     * Where
     * BLOCKNR = (C_SIZE+1) * MULT
     * MULT = 2 ^ (C_SIZE_MULT+2)       (C_SIZE_MULT < 8)
     * BLOCK_LEN = 2 ^ READ_BL_LEN      (READ_BL_LEN < 12)
     * ----------------------------------------------------
     * For high capacity SD/MMC card:
     * memory capacity = SEC_COUNT * 512 byte
     */
    if (MMC_CSD_C_SIZE(m_CSD) != 0xFFF) {
        uint32_t blocknr = ((MMC_CSD_C_SIZE(m_CSD) + 1) * (1 << (MMC_CSD_C_SIZE_MULT(m_CSD) + 2)));
        m_Capacity = blocknr * (1 << MMC_CSD_READ_BL_LEN(m_CSD)) / 1024;
    }
}

/**
 * \brief Decodes SD CSD register
 */
void HSMCIDriver::sd_decode_csd()
{
    uint32_t unit;
    uint32_t mul;
    uint32_t tran_speed;

    // Get SD memory maximum transfer speed in Hz.
    tran_speed = CSD_TRAN_SPEED(m_CSD);
    unit = sd_mmc_trans_units[tran_speed & 0x7];
    mul = sd_trans_multipliers[(tran_speed >> 3) & 0xF];
    m_Clock = unit * mul * 1000;

    /*
     * Get card capacity.
     * ----------------------------------------------------
     * For normal SD/MMC card:
     * memory capacity = BLOCKNR * BLOCK_LEN
     * Where
     * BLOCKNR = (C_SIZE+1) * MULT
     * MULT = 2 ^ (C_SIZE_MULT+2)       (C_SIZE_MULT < 8)
     * BLOCK_LEN = 2 ^ READ_BL_LEN      (READ_BL_LEN < 12)
     * ----------------------------------------------------
     * For high capacity SD card:
     * memory capacity = (C_SIZE+1) * 512K byte
     */
    if (CSD_STRUCTURE_VERSION(m_CSD) >= SD_CSD_VER_2_0) {
        m_Capacity = (SD_CSD_2_0_C_SIZE(m_CSD) + 1) * 512;
    } else {
        uint32_t blocknr = ((SD_CSD_1_0_C_SIZE(m_CSD) + 1) * (1 << (SD_CSD_1_0_C_SIZE_MULT(m_CSD) + 2)));
        m_Capacity = blocknr * (1 << SD_CSD_1_0_READ_BL_LEN(m_CSD)) / 1024;
    }
}

/**
 * \brief CMD13 - Addressed card sends its status register.
 * This function waits the clear of the busy flag
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_mmc_cmd13()
{
    uint32_t nec_timeout;

    /* Wait for data ready status.
     * Nec timing: 0 to unlimited
     * However a timeout is used.
     * 200 000 * 8 cycles
     */
    nec_timeout = 200000;
    do {
        if (!hsmci_send_cmd(SDMMC_MCI_CMD13_SEND_STATUS, (uint32_t)m_RCA << 16)) {
            return false;
        }
        // Check busy flag
        if (hsmci_get_response() & CARD_STATUS_READY_FOR_DATA) {
            break;
        }
        if (nec_timeout-- == 0) {
            sd_mmc_debug("%s: CMD13 Busy timeout\n\r", __func__);
            return false;
        }
    } while (1);

    return true;
}

/**
 * \brief CMD52 for SDIO - Enable the high speed mode
 *
 * \note m_HighSpeed is updated.
 * \note m_Clock is updated.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sdio_cmd52_set_high_speed()
{
    uint8_t u8_value = 0;

    // Check CIA.HS
    if (!sdio_cmd52(SDIO_CMD52_READ_FLAG, SDIO_CIA, SDIO_CCCR_HS, 0, &u8_value)) {
        return false;
    }
    if ((u8_value & SDIO_SHS) != SDIO_SHS) {
        // No supported, it is not a protocol error
        return true;
    }
    // HS mode possible, then enable
    u8_value = SDIO_EHS;
    if (!sdio_cmd52(SDIO_CMD52_WRITE_FLAG, SDIO_CIA, SDIO_CCCR_HS, 1, &u8_value)) {
        return false;
    }
    m_HighSpeed = true;
    m_Clock *= 2;
    return true;
}

/**
 * \brief Select a card slot and initialize the associated driver
 *
 * \param slot  Card slot number
 *
 * \retval SD_MMC_ERR_SLOT     Wrong slot number
 * \retval SD_MMC_ERR_NO_CARD  No card present on slot
 * \retval SD_MMC_ERR_UNUSABLE Unusable card
 * \retval SD_MMC_INIT_ONGOING Card initialization requested
 * \retval SD_MMC_OK           Card present
 */
sd_mmc_err_t HSMCIDriver::sd_mmc_select_slot(uint8_t slot)
{
    if (slot != 0) {
        return SD_MMC_ERR_SLOT;
    }
    Assert(sd_mmc_nb_block_remaining == 0);

    //! Card Detect pins
    if (m_PinCD.Read()) {
        if (m_State == SD_MMC_CARD_STATE_DEBOUNCE) {
            SD_MMC_STOP_TIMEOUT();
        }
        m_State = SD_MMC_CARD_STATE_NO_CARD;
        return SD_MMC_ERR_NO_CARD;
    }
    if (m_State == SD_MMC_CARD_STATE_NO_CARD) {
        // A card plug on going, but this is not initialized
        m_State = SD_MMC_CARD_STATE_DEBOUNCE;
        // Debounce + Power On Setup
        SD_MMC_START_TIMEOUT();
        return SD_MMC_ERR_NO_CARD;
    }
    if (m_State == SD_MMC_CARD_STATE_DEBOUNCE) {
        if (!SD_MMC_IS_TIMEOUT()) {
            // Debounce on going
            return SD_MMC_ERR_NO_CARD;
        }
        // Card is not initialized
        m_State = SD_MMC_CARD_STATE_INIT;
        // Set 1-bit bus width and low clock for initialization
        m_Clock = SDMMC_CLOCK_INIT;
        m_BusWidth = 1;
        m_HighSpeed = false;
    }
    if (m_State == SD_MMC_CARD_STATE_UNUSABLE) {
        return SD_MMC_ERR_UNUSABLE;
    }

    // Initialize interface
    sd_mmc_slot_sel = slot;
    //sd_mmc_card = &sd_mmc_cards[slot];
    sd_mmc_configure_slot();
    return (m_State == SD_MMC_CARD_STATE_INIT) ? SD_MMC_INIT_ONGOING : SD_MMC_OK;
}

/**
 * \brief Configures the driver with the selected card configuration
 */
void HSMCIDriver::sd_mmc_configure_slot()
{
    hsmci_select_device(m_Clock, m_BusWidth, m_HighSpeed);
}

/**
 * \brief ACMD6 - Define the data bus width to 4 bits bus
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_acmd6()
{
    // CMD55 - Indicate to the card that the next command is an
    // application specific command rather than a standard command.
    if (!hsmci_send_cmd(SDMMC_CMD55_APP_CMD, (uint32_t)m_RCA << 16)) {
        return false;
    }
    // 10b = 4 bits bus
    if (!hsmci_send_cmd(SD_ACMD6_SET_BUS_WIDTH, 0x2)) {
        return false;
    }
    m_BusWidth = 4;
    sd_mmc_debug("%d-bit bus width enabled.\n\r", (int)m_BusWidth);
    return true;
}

/**
 * \brief ACMD51 - Read the SD Configuration Register.
 *
 * \note
 * SD Card Configuration Register (SCR) provides information on the SD Memory
 * Card's special features that were configured into the given card. The size
 * of SCR register is 64 bits.
 *
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_acmd51()
{
    uint8_t scr[SD_SCR_REG_BSIZE];

    // CMD55 - Indicate to the card that the next command is an
    // application specific command rather than a standard command.
    if (!hsmci_send_cmd(SDMMC_CMD55_APP_CMD, (uint32_t)m_RCA << 16)) {
        return false;
    }
    if (!hsmci_adtc_start(SD_ACMD51_SEND_SCR, 0, SD_SCR_REG_BSIZE, 1, true)) {
        return false;
    }
    if (!hsmci_start_read_blocks(scr, 1)) {
        return false;
    }
    if (!hsmci_wait_end_of_read_blocks()) {
        return false;
    }

    // Get SD Memory Card - Spec. Version
    switch (SD_SCR_SD_SPEC(scr)) {
    case SD_SCR_SD_SPEC_1_0_01:
        m_Version = CARD_VER_SD_1_0;
        break;

    case SD_SCR_SD_SPEC_1_10:
        m_Version = CARD_VER_SD_1_10;
        break;

    case SD_SCR_SD_SPEC_2_00:
        if (SD_SCR_SD_SPEC3(scr) == SD_SCR_SD_SPEC_3_00) {
            m_Version = CARD_VER_SD_3_0;
        } else {
            m_Version = CARD_VER_SD_2_0;
        }
        break;

    default:
        m_Version = CARD_VER_SD_1_0;
        break;
    }
    return true;
}

/**
 * \brief Deselect the current card slot
 */
void HSMCIDriver::sd_mmc_deselect_slot()
{
    hsmci_deselect_device(0);
}

/**
 * \brief CMD8 for SD card - Send Interface Condition Command.
 *
 * \note
 * Send SD Memory Card interface condition, which includes host supply
 * voltage information and asks the card whether card supports voltage.
 * Should be performed at initialization time to detect the card type.
 *
 * \param v2 Pointer to v2 flag to update
 *
 * \return true if success, otherwise false
 *         with a update of \ref sd_mmc_err.
 */
bool HSMCIDriver::sd_cmd8(uint8_t * v2)
{
    uint32_t resp;

    *v2 = 0;
    // Test for SD version 2
    if (!hsmci_send_cmd(SD_CMD8_SEND_IF_COND,
            SD_CMD8_PATTERN | SD_CMD8_HIGH_VOLTAGE)) {
        return true; // It is not a V2
    }
    // Check R7 response
    resp = hsmci_get_response();
    if (resp == 0xFFFFFFFF) {
        // No compliance R7 value
        return true; // It is not a V2
    }
    if ((resp & (SD_CMD8_MASK_PATTERN | SD_CMD8_MASK_VOLTAGE)) != (SD_CMD8_PATTERN | SD_CMD8_HIGH_VOLTAGE)) {
        sd_mmc_debug("%s: CMD8 resp32 0x%08lx UNUSABLE CARD\n\r", __func__, resp);
        return false;
    }
    sd_mmc_debug("SD card V2\n\r");
    *v2 = 1;
    return true;
}

/**
 * \brief CMD8 - The card sends its EXT_CSD register as a block of data.
 *
 * \param b_authorize_high_speed Pointer to update with the high speed
 * support information
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::mmc_cmd8(uint8_t *b_authorize_high_speed)
{
    uint16_t i;
    uint32_t ext_csd;
    uint32_t sec_count;

    if (!hsmci_adtc_start(MMC_CMD8_SEND_EXT_CSD, 0, EXT_CSD_BSIZE, 1, false)) {
        return false;
    }
    //** Read and decode Extended Extended CSD
    // Note: The read access is done in byte to avoid a buffer
    // of EXT_CSD_BSIZE Byte in stack.

    // Read card type
    for (i = 0; i < (EXT_CSD_CARD_TYPE_INDEX + 4) / 4; i++) {
        if (!hsmci_read_word(&ext_csd)) {
            return false;
        }
    }
    *b_authorize_high_speed = (ext_csd >> ((EXT_CSD_CARD_TYPE_INDEX % 4) * 8)) & MMC_CTYPE_52MHZ;

    if (MMC_CSD_C_SIZE(m_CSD) == 0xFFF) {
        // For high capacity SD/MMC card,
        // memory capacity = SEC_COUNT * 512 byte
        for (; i <(EXT_CSD_SEC_COUNT_INDEX + 4) / 4; i++) {
            if (!hsmci_read_word(&sec_count)) {
                return false;
            }
        }
        m_Capacity = sec_count / 2;
    }
    for (; i < EXT_CSD_BSIZE / 4; i++) {
        if (!hsmci_read_word(&sec_count)) {
            return false;
        }
    }
    return true;
}

/**
 * \brief Initialize the MMC card in MCI mode.
 *
 * \note
 * This function runs the initialization procedure and the identification
 * process, then it sets the SD/MMC card in transfer state.
 * At last, it will automaticly enable maximum bus width and transfer speed.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_mmc_mci_install_mmc()
{
    uint8_t b_authorize_high_speed;

    // CMD0 - Reset all cards to idle state.
    if (!hsmci_send_cmd(SDMMC_MCI_CMD0_GO_IDLE_STATE, 0)) {
        return false;
    }

    if (!mmc_mci_op_cond()) {
        return false;
    }

    // Put the Card in Identify Mode
    // Note: The CID is not used in this stack
    if (!hsmci_send_cmd(SDMMC_CMD2_ALL_SEND_CID, 0)) {
        return false;
    }
    // Assign relative address to the card.
    m_RCA = 1;
    if (!hsmci_send_cmd(MMC_CMD3_SET_RELATIVE_ADDR, (uint32_t)m_RCA << 16)) {
        return false;
    }
    // Get the Card-Specific Data
    if (!sd_mmc_cmd9_mci()) {
        return false;
    }
    mmc_decode_csd();
    // Select the and put it into Transfer Mode
    if (!hsmci_send_cmd(SDMMC_CMD7_SELECT_CARD_CMD, (uint32_t)m_RCA << 16)) {
        return false;
    }
    if (m_CardVersion >= CARD_VER_MMC_4) {
        // For MMC 4.0 Higher version
        // Get EXT_CSD
        if (!mmc_cmd8(&b_authorize_high_speed)) {
            return false;
        }
//      if (4 <= hsmci_get_bus_width(sd_mmc_slot_sel)) {
            // Enable more bus width
            if (!mmc_cmd6_set_bus_width(4/*hsmci_get_bus_width(sd_mmc_slot_sel)*/)) {
                return false;
            }
            // Reinitialize the slot with the bus width
            sd_mmc_configure_slot();
//      }
        if (b_authorize_high_speed) {
            // Enable HS
            if (!mmc_cmd6_set_high_speed()) {
                return false;
            }
            // Reinitialize the slot with the new speed
            sd_mmc_configure_slot();
        }
    } else {
        // Reinitialize the slot with the new speed
        sd_mmc_configure_slot();
    }

    uint8_t retry = 10;
    while (retry--) {
        // Retry is a WORKAROUND for no compliance card (Atmel Internal ref. MMC19):
        // These cards seem not ready immediatly
        // after the end of busy of mmc_cmd6_set_high_speed()

        // Set default block size
        if (hsmci_send_cmd(SDMMC_CMD16_SET_BLOCKLEN, SD_MMC_BLOCK_SIZE)) {
            return true;
        }
    }
    return false;
}

/**
 * \brief Initialize the SD card in MCI mode.
 *
 * \note
 * This function runs the initialization procedure and the identification
 * process, then it sets the SD/MMC card in transfer state.
 * At last, it will automaticly enable maximum bus width and transfer speed.
 *
 * \return true if success, otherwise false
 */
bool HSMCIDriver::sd_mmc_mci_card_init()
{
    uint8_t v2 = 0;
    uint8_t data = 0x08;

    // In first, try to install SD/SDIO card
    m_CardType = CARD_TYPE_SD;
    m_Version = CARD_VER_UNKNOWN;
    m_RCA = 0;
    sd_mmc_debug("Start SD card install\n\r");

    // Card need of 74 cycles clock minimum to start
    hsmci_send_clock();

    /* CMD52 Reset SDIO */
    sdio_cmd52(SDIO_CMD52_WRITE_FLAG, SDIO_CIA,SDIO_CCCR_IOA, 0, &data);

    // CMD0 - Reset all cards to idle state.
    if (!hsmci_send_cmd(SDMMC_MCI_CMD0_GO_IDLE_STATE, 0)) {
        return false;
    }
    if (!sd_cmd8(&v2)) {
        return false;
    }
    // Try to get the SDIO card's operating condition
    if (!sdio_op_cond()) {
        return false;
    }

    if (m_CardType & CARD_TYPE_SD) {
        // Try to get the SD card's operating condition
        if (!sd_mci_op_cond(v2)) {
            // It is not a SD card
            sd_mmc_debug("Start MMC Install\n\r");
            m_CardType = CARD_TYPE_MMC;
            return sd_mmc_mci_install_mmc();
        }
    }

    if (m_CardType & CARD_TYPE_SD) {
        // SD MEMORY, Put the Card in Identify Mode
        // Note: The CID is not used in this stack
        if (!hsmci_send_cmd(SDMMC_CMD2_ALL_SEND_CID, 0)) {
            return false;
        }
    }
    // Ask the card to publish a new relative address (RCA).
    if (!hsmci_send_cmd(SD_CMD3_SEND_RELATIVE_ADDR, 0)) {
        return false;
    }
    m_RCA = (hsmci_get_response() >> 16) & 0xFFFF;

    // SD MEMORY, Get the Card-Specific Data
    if (m_CardType & CARD_TYPE_SD) {
        if (!sd_mmc_cmd9_mci()) {
            return false;
        }
        sd_decode_csd();
    }
    // Select the and put it into Transfer Mode
    if (!hsmci_send_cmd(SDMMC_CMD7_SELECT_CARD_CMD,
            (uint32_t)m_RCA << 16)) {
        return false;
    }
    // SD MEMORY, Read the SCR to get card version
    if (m_CardType & CARD_TYPE_SD) {
        if (!sd_acmd51()) {
            return false;
        }
    }
    if (IS_SDIO()) {
        if (!sdio_get_max_speed()) {
            return false;
        }
    }
    // TRY to enable 4-bit mode
    if (IS_SDIO()) {
        if (!sdio_cmd52_set_bus_width()) {
            return false;
        }
    }
    if (m_CardType & CARD_TYPE_SD) {
        if (!sd_acmd6()) {
            return false;
        }
    }
    // Switch to selected bus mode
    sd_mmc_configure_slot();
    // TRY to enable High-Speed Mode
    if (IS_SDIO()) {
        if (!sdio_cmd52_set_high_speed()) {
            return false;
        }
    }
    if (m_CardType & CARD_TYPE_SD) {
        if (m_Version > CARD_VER_SD_1_0) {
            if (!sd_cm6_set_high_speed()) {
                return false;
            }
        }
    }
    // Valid new configuration
    sd_mmc_configure_slot();
    // SD MEMORY, Set default block size
    if (m_CardType & CARD_TYPE_SD) {
        if (!hsmci_send_cmd(SDMMC_CMD16_SET_BLOCKLEN, SD_MMC_BLOCK_SIZE)) {
            return false;
        }
    }
    return true;
}

sd_mmc_err_t HSMCIDriver::sd_mmc_check(uint8_t slot)
{
    sd_mmc_err_t sd_mmc_err;

    sd_mmc_err = sd_mmc_select_slot(slot);
    if (sd_mmc_err != SD_MMC_INIT_ONGOING) {
        sd_mmc_deselect_slot();
        return sd_mmc_err;
    }

    // Initialization of the card requested
    if (sd_mmc_mci_card_init())
    {
        sd_mmc_debug("SD/MMC card ready\n\r");
        m_State = SD_MMC_CARD_STATE_READY;
        sd_mmc_deselect_slot();
        // To notify that the card has been just initialized
        // It is necessary for USB Device MSC
        return SD_MMC_INIT_ONGOING;
    }
    sd_mmc_debug("SD/MMC card initialization failed\n\r");
    m_State = SD_MMC_CARD_STATE_UNUSABLE;
    sd_mmc_deselect_slot();
    return SD_MMC_ERR_UNUSABLE;
}

card_type_t HSMCIDriver::sd_mmc_get_type(uint8_t slot)
{
    if (SD_MMC_OK != sd_mmc_select_slot(slot)) {
        return CARD_TYPE_UNKNOWN;
    }
    sd_mmc_deselect_slot();
    return m_CardType;
}

card_version_t HSMCIDriver::sd_mmc_get_version(uint8_t slot)
{
    if (SD_MMC_OK != sd_mmc_select_slot(slot)) {
        return CARD_VER_UNKNOWN;
    }
    sd_mmc_deselect_slot();
    return m_Version;
}

uint32_t HSMCIDriver::sd_mmc_get_capacity(uint8_t slot)
{
    if (SD_MMC_OK != sd_mmc_select_slot(slot)) {
        return 0;
    }
    sd_mmc_deselect_slot();
    return m_Capacity;
}

sd_mmc_err_t HSMCIDriver::sd_mmc_init_read_blocks(uint8_t slot, uint32_t start, uint16_t nb_block)
{
    sd_mmc_err_t sd_mmc_err;
    uint32_t cmd, arg, resp;

    sd_mmc_err = sd_mmc_select_slot(slot);
    if (sd_mmc_err != SD_MMC_OK) {
        return sd_mmc_err;
    }

    // Wait for data ready status
    if (!sd_mmc_cmd13()) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }

    if (nb_block > 1) {
        cmd = SDMMC_CMD18_READ_MULTIPLE_BLOCK;
    } else {
        cmd = SDMMC_CMD17_READ_SINGLE_BLOCK;
    }
    /*
    * SDSC Card (CCS=0) uses byte unit address,
    * SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
    */
    if (m_CardType & CARD_TYPE_HC) {
        arg = start;
    } else {
        arg = (start * SD_MMC_BLOCK_SIZE);
    }

    if (!hsmci_adtc_start(cmd, arg, SD_MMC_BLOCK_SIZE, nb_block, true)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    // Check response
    resp = hsmci_get_response();
    if (resp & CARD_STATUS_ERR_RD_WR) {
        sd_mmc_debug("%s: Read blocks %02d resp32 0x%08lx CARD_STATUS_ERR_RD_WR\n\r", __func__, (int)SDMMC_CMD_GET_INDEX(cmd), resp);
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    sd_mmc_nb_block_remaining = nb_block;
    sd_mmc_nb_block_to_tranfer = nb_block;
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sd_mmc_start_read_blocks(void *dest, uint16_t nb_block)
{
    Assert(sd_mmc_nb_block_remaining >= nb_block);

    if (!hsmci_start_read_blocks(dest, nb_block)) {
        sd_mmc_nb_block_remaining = 0;
        return SD_MMC_ERR_COMM;
    }
    sd_mmc_nb_block_remaining -= nb_block;
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sd_mmc_wait_end_of_read_blocks(bool abort)
{
    if (!hsmci_wait_end_of_read_blocks()) {
        return SD_MMC_ERR_COMM;
    }
    if (abort) {
        sd_mmc_nb_block_remaining = 0;
    } else if (sd_mmc_nb_block_remaining) {
        return SD_MMC_OK;
    }

    // All blocks are transfered then stop read operation
    if (sd_mmc_nb_block_to_tranfer == 1) {
        // Single block transfer, then nothing to do
        sd_mmc_deselect_slot();
        return SD_MMC_OK;
    }
    // WORKAROUND for no compliance card (Atmel Internal ref. !MMC7 !SD19):
    // The errors on this command must be ignored
    // and one retry can be necessary in SPI mode for no compliance card.
    if (!hsmci_adtc_stop(SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
        hsmci_adtc_stop(SDMMC_CMD12_STOP_TRANSMISSION, 0);
    }
    sd_mmc_deselect_slot();
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sd_mmc_init_write_blocks(uint8_t slot, uint32_t start, uint16_t nb_block)
{
    sd_mmc_err_t sd_mmc_err;
    uint32_t cmd, arg, resp;

    sd_mmc_err = sd_mmc_select_slot(slot);
    if (sd_mmc_err != SD_MMC_OK) {
        return sd_mmc_err;
    }
//  if (sd_mmc_is_write_protected(slot)) {
//      sd_mmc_deselect_slot();
//      return SD_MMC_ERR_WP;
//  }

    if (nb_block > 1) {
        cmd = SDMMC_CMD25_WRITE_MULTIPLE_BLOCK;
    } else {
        cmd = SDMMC_CMD24_WRITE_BLOCK;
    }
    /*
     * SDSC Card (CCS=0) uses byte unit address,
     * SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit).
     */
    if (m_CardType & CARD_TYPE_HC) {
        arg = start;
    } else {
        arg = (start * SD_MMC_BLOCK_SIZE);
    }
    if (!hsmci_adtc_start(cmd, arg, SD_MMC_BLOCK_SIZE, nb_block, true)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    // Check response
    resp = hsmci_get_response();
    if (resp & CARD_STATUS_ERR_RD_WR) {
        sd_mmc_debug("%s: Write blocks %02d r1 0x%08lx CARD_STATUS_ERR_RD_WR\n\r", __func__, (int)SDMMC_CMD_GET_INDEX(cmd), resp);
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    sd_mmc_nb_block_remaining = nb_block;
    sd_mmc_nb_block_to_tranfer = nb_block;
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sd_mmc_start_write_blocks(const void *src, uint16_t nb_block)
{
    Assert(sd_mmc_nb_block_remaining >= nb_block);
    if (!hsmci_start_write_blocks(src, nb_block)) {
        sd_mmc_nb_block_remaining = 0;
        return SD_MMC_ERR_COMM;
    }
    sd_mmc_nb_block_remaining -= nb_block;
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sd_mmc_wait_end_of_write_blocks(bool abort)
{
    if (!hsmci_wait_end_of_write_blocks()) {
        return SD_MMC_ERR_COMM;
    }
    if (abort) {
        sd_mmc_nb_block_remaining = 0;
    } else if (sd_mmc_nb_block_remaining) {
        return SD_MMC_OK;
    }

    // All blocks are transfered then stop write operation
    if (sd_mmc_nb_block_to_tranfer == 1) {
        // Single block transfer, then nothing to do
        sd_mmc_deselect_slot();
        return SD_MMC_OK;
    }

    // Note: SPI multiblock writes terminate using a special
    // token, not a STOP_TRANSMISSION request.
    if (!hsmci_adtc_stop(SDMMC_CMD12_STOP_TRANSMISSION, 0)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    sd_mmc_deselect_slot();
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sdio_read_direct(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t *dest)
{
    sd_mmc_err_t sd_mmc_err;

    if (dest == nullptr) {
        return SD_MMC_ERR_PARAM;
    }

    sd_mmc_err = sd_mmc_select_slot(slot);
    if (sd_mmc_err != SD_MMC_OK) {
        return sd_mmc_err;
    }

    if (!sdio_cmd52(SDIO_CMD52_READ_FLAG, func_num, addr, 0, dest)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    sd_mmc_deselect_slot();
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sdio_write_direct(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t data)
{
    sd_mmc_err_t sd_mmc_err;

    sd_mmc_err = sd_mmc_select_slot(slot);
    if (sd_mmc_err != SD_MMC_OK) {
        return sd_mmc_err;
    }

    if (!sdio_cmd52(SDIO_CMD52_WRITE_FLAG, func_num, addr, 0, &data)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }

    sd_mmc_deselect_slot();
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sdio_read_extended(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t inc_addr, uint8_t *dest, uint16_t size)
{
    sd_mmc_err_t sd_mmc_err;

    if ((size == 0) || (size > 512)) {
        return SD_MMC_ERR_PARAM;
    }

    sd_mmc_err = sd_mmc_select_slot(slot);
    if (sd_mmc_err != SD_MMC_OK) {
        return sd_mmc_err;
    }

    if (!sdio_cmd53(SDIO_CMD53_READ_FLAG, func_num, addr, inc_addr, size, true)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    if (!hsmci_start_read_blocks(dest, 1)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    if (!hsmci_wait_end_of_read_blocks()) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }

    sd_mmc_deselect_slot();
    return SD_MMC_OK;
}

sd_mmc_err_t HSMCIDriver::sdio_write_extended(uint8_t slot, uint8_t func_num, uint32_t addr, uint8_t inc_addr, uint8_t *src, uint16_t size)
{
    sd_mmc_err_t sd_mmc_err;

    if ((size == 0) || (size > 512)) {
        return SD_MMC_ERR_PARAM;
    }

    sd_mmc_err = sd_mmc_select_slot(slot);
    if (sd_mmc_err != SD_MMC_OK) {
        return sd_mmc_err;
    }

    if (!sdio_cmd53(SDIO_CMD53_WRITE_FLAG, func_num, addr, inc_addr, size, true)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    if (!hsmci_start_write_blocks(src, 1)) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }
    if (!hsmci_wait_end_of_write_blocks()) {
        sd_mmc_deselect_slot();
        return SD_MMC_ERR_COMM;
    }

    sd_mmc_deselect_slot();
    return SD_MMC_OK;
}
