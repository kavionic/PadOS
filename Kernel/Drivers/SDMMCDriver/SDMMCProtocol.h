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


#pragma once

namespace sdmmc
{

// Default clock frequency for initialization (400KHz)
static constexpr uint32_t SDMMC_CLOCK_INIT = 400000;


///////////////////////////////////////////////////////////////////////////////
// Function for masking away the command flags and return the command index.
static inline uint8_t SDMMC_CMD_GET_INDEX(uint32_t cmd) { return uint8_t(cmd & 0x3f); }


///////////////////////////////////////////////////////////////////////////////
// Flags defining misc properties in SD/MMC/SDIO commands. Merged in with the
// actual command index.

static constexpr uint32_t SDMMC_RESP_PRESENT      = 1UL << 8;	// Have response (MCI only).
static constexpr uint32_t SDMMC_RESP_8            = 1UL << 9;	// 8 bit response (SPI only).
static constexpr uint32_t SDMMC_RESP_32           = 1UL << 10;	// 32 bit response (SPI only).
static constexpr uint32_t SDMMC_RESP_136          = 1UL << 11;	// 136 bit response (MCI only).
static constexpr uint32_t SDMMC_RESP_CRC          = 1UL << 12;	// Expect valid CRC (MCI only).
static constexpr uint32_t SDMMC_RESP_BUSY         = 1UL << 13;	// Card may send busy.
static constexpr uint32_t SDMMC_CMD_OPENDRAIN     = 1UL << 14;	// Open drain for a broadcast command or to enter inactive state (MCI only).
static constexpr uint32_t SDMMC_CMD_WRITE         = 1UL << 15;	// To signal a data write operation.
static constexpr uint32_t SDMMC_CMD_SDIO_BYTE     = 1UL << 16;	// To signal a SDIO transfer in multi byte mode.
static constexpr uint32_t SDMMC_CMD_SDIO_BLOCK    = 1UL << 17;	// To signal a SDIO transfer in block mode.
static constexpr uint32_t SDMMC_CMD_STREAM        = 1UL << 18;	// To signal a data transfer in stream mode.
static constexpr uint32_t SDMMC_CMD_SINGLE_BLOCK  = 1UL << 19;	// To signal a data transfer in single block mode.
static constexpr uint32_t SDMMC_CMD_MULTI_BLOCK   = 1UL << 20;	// To signal a data transfer in multi block mode.

///////////////////////////////////////////////////////////////////////////////
// Flag-sets defining response types.
//
// Response types (R1, R3, R4 & R5 use a 48 bits response with 7bit CRC):
//  - R1 receive data not specified
//  - R3 receive OCR
//  - R4, R5 RCA management (MMC only)
//  - R6, R7 RCA management (SD only)
//
// R1b assert the BUSY signal and respond with R1.
// If the busy signal is asserted, it is done 2 clock cycles after the command
// end-bit. The D0 line is driven low. D1-D7 lines are also driven by the card
// but their values are undefined.
//
// R2 use a 136 bits response with 7bit CRC. The content is CID or CSD.
//
// Specific for MMC:
// - R4 (Fast I/O) return RCA
// - R5 (interrupt request) return RCA null
//
// Specific for SD:
// - R6 (Published RCA) return RCA
// - R7 (Card interface condition) return RCA null

static constexpr uint32_t SDMMC_CMD_NO_RESP = 0;
static constexpr uint32_t SDMMC_CMD_R1      = SDMMC_RESP_PRESENT | SDMMC_RESP_CRC;
static constexpr uint32_t SDMMC_CMD_R1B     = SDMMC_RESP_PRESENT | SDMMC_RESP_CRC | SDMMC_RESP_BUSY;
static constexpr uint32_t SDMMC_CMD_R2      = SDMMC_RESP_PRESENT | SDMMC_RESP_8 | SDMMC_RESP_136 | SDMMC_RESP_CRC;
static constexpr uint32_t SDMMC_CMD_R3      = SDMMC_RESP_PRESENT | SDMMC_RESP_32;
static constexpr uint32_t SDMMC_CMD_R4      = SDMMC_RESP_PRESENT | SDMMC_RESP_32;
static constexpr uint32_t SDMMC_CMD_R5      = SDMMC_RESP_PRESENT | SDMMC_RESP_8 | SDMMC_RESP_CRC;
static constexpr uint32_t SDMMC_CMD_R6      = SDMMC_RESP_PRESENT | SDMMC_RESP_CRC;
static constexpr uint32_t SDMMC_CMD_R7      = SDMMC_RESP_PRESENT | SDMMC_RESP_32 | SDMMC_RESP_CRC;

///////////////////////////////////////////////////////////////////////////////
// SPI mode commands:

// === Basic commands and read-stream command (Class 0 and 1) ===

// Cmd0(broadcast): Reset all cards to idle state.
static constexpr uint32_t SDMMC_SPI_CMD0_GO_IDLE_STATE = 0 | SDMMC_CMD_R1;
// MMC Cmd1(broadcast+r, R3): Ask the card to send its Operating Conditions.
static constexpr uint32_t MMC_SPI_CMD1_SEND_OP_COND    = 1 | SDMMC_CMD_R1;
// Cmd9 SPI (R1): Addressed card sends its card-specific data (CSD).
static constexpr uint32_t SDMMC_SPI_CMD9_SEND_CSD      = 9 | SDMMC_CMD_R1 | SDMMC_CMD_SINGLE_BLOCK;
// Cmd13(R2): Addressed card sends its status register.
static constexpr uint32_t SDMMC_SPI_CMD13_SEND_STATUS  = 13 | SDMMC_CMD_R2;
// Cmd58(R3): Reads the OCR register of a card.
static constexpr uint32_t SDMMC_SPI_CMD58_READ_OCR     = 58 | SDMMC_CMD_R3;
// Cmd59(R1): Turns the CRC option on or off.
static constexpr uint32_t SDMMC_SPI_CMD59_CRC_ON_OFF   = 59 | SDMMC_CMD_R1;


// === Application-specific commands (Class 8) ===

// ACMD41(R1): Send host capacity support information (HCS) and activates card initialization.
static constexpr uint32_t SD_SPI_ACMD41_SD_SEND_OP_COND = 41 | SDMMC_CMD_R1;


///////////////////////////////////////////////////////////////////////////////
// SD mode commands:

// === Basic commands and read-stream command (Class 0 and 1) ===

// Cmd0(broadcast): Reset all cards to idle state.
static constexpr uint32_t SDMMC_MCI_CMD0_GO_IDLE_STATE = 0 | SDMMC_CMD_NO_RESP | SDMMC_CMD_OPENDRAIN;

// MMC Cmd1(broadcast+r, R3): Ask the card to send its Operating Conditions.
static constexpr uint32_t MMC_MCI_CMD1_SEND_OP_COND    = 1 | SDMMC_CMD_R3 | SDMMC_CMD_OPENDRAIN;

// Cmd2(broadcast+r, R2): Request CID number from card.
static constexpr uint32_t SDMMC_CMD2_ALL_SEND_CID      = 2 | SDMMC_CMD_R2 | SDMMC_CMD_OPENDRAIN;

// SD Cmd3(broadcast+r, R6): Ask the card to publish a new relative address (RCA).
static constexpr uint32_t SD_CMD3_SEND_RELATIVE_ADDR   = 3 | SDMMC_CMD_R6 | SDMMC_CMD_OPENDRAIN;

// MMC Cmd3(addressed, R1): Assigns relative address to the card.
static constexpr uint32_t MMC_CMD3_SET_RELATIVE_ADDR   = 3 | SDMMC_CMD_R1;

// Cmd4(broadcast): Program the DSR of all cards (MCI only).
static constexpr uint32_t SDMMC_CMD4_SET_DSR           = 4 | SDMMC_CMD_NO_RESP;

// MMC Cmd5(addressed, R1b): Toggle the card between Sleep state and Standby state.
static constexpr uint32_t MMC_CMD5_SLEEP_AWAKE         = 5 | SDMMC_CMD_R1B;

// Cmd7(addressed, R1/R1b): Select/Deselect card
//    For SD:  R1b only from the selected card.
//    For MMC: R1 while selecting from stand-by state to transfer state;
//             R1b while selecting from disconnected state to programming state.
static constexpr uint32_t SDMMC_CMD7_SELECT_CARD_CMD   = 7 | SDMMC_CMD_R1B;
static constexpr uint32_t SDMMC_CMD7_DESELECT_CARD_CMD = 7 | SDMMC_CMD_R1;
// MMC Cmd8(addressed+d, R1): Send EXT_CSD register as a block of data.
static constexpr uint32_t MMC_CMD8_SEND_EXT_CSD        = 8 | SDMMC_CMD_R1 | SDMMC_CMD_SINGLE_BLOCK;
// SD Cmd8(broadcast+r, R7) : Send SD Memory Card interface condition.
static constexpr uint32_t SD_CMD8_SEND_IF_COND         = 8 | SDMMC_CMD_R7 | SDMMC_CMD_OPENDRAIN;
// Cmd9 MCI (addressed, R2): Addressed card sends its card-specific data (CSD).
static constexpr uint32_t SDMMC_MCI_CMD9_SEND_CSD      = 9 | SDMMC_CMD_R2;
// Cmd10(addressed, R2): Addressed card sends its card identification (CID).
static constexpr uint32_t SDMMC_CMD10_SEND_CID         = 10 | SDMMC_CMD_R2;

// MMC Cmd11(addressed+d, R1): Read data stream from the card from given address until a STOP_TRANSMISSION follows.
static constexpr uint32_t MMC_CMD11_READ_DAT_UNTIL_STOP = 11 | SDMMC_CMD_R1;
// SD Cmd11 MCI (addressed, R1): Voltage switching.
static constexpr uint32_t SD_CMD11_READ_DAT_UNTIL_STOP  = 11 | SDMMC_CMD_R1;
// Cmd12(addressed, R1b): Force the card to stop transmission.
static constexpr uint32_t SDMMC_CMD12_STOP_TRANSMISSION = 12 | SDMMC_CMD_R1B;
// Cmd13(addressed, R1): Addressed card sends its status register.
static constexpr uint32_t SDMMC_MCI_CMD13_SEND_STATUS   = 13 | SDMMC_CMD_R1;
// MMC Cmd14(addressed+d, R1): Read the reversed bus testing data pattern from a card.
static constexpr uint32_t MMC_CMD14_BUSTEST_R           = 14 | SDMMC_CMD_R1;
// Cmd15(addressed): Set an addressed card to the inactive state.
// NOTE: It is an addressed command, but it must be send like broadcast command to open drain.
static constexpr uint32_t SDMMC_CMD15_GO_INACTIVE_STATE = 15 | SDMMC_CMD_NO_RESP | SDMMC_CMD_OPENDRAIN;
// MMC Cmd19(addressed+d, R1): Send the bus test data pattern.
static constexpr uint32_t MMC_CMD19_BUSTEST_W           = 19 | SDMMC_CMD_R1;

// === Block-oriented read commands (Class 2) ===

// Cmd16(addressed, R1): Set block length in bytes.
static constexpr uint32_t SDMMC_CMD16_SET_BLOCKLEN        = 16 | SDMMC_CMD_R1;
// Cmd17(addressed+d, R1): Read single block.
static constexpr uint32_t SDMMC_CMD17_READ_SINGLE_BLOCK   = 17 | SDMMC_CMD_R1 | SDMMC_CMD_SINGLE_BLOCK;
// Cmd18(addressed+d, R1): Read multiple blocks.
static constexpr uint32_t SDMMC_CMD18_READ_MULTIPLE_BLOCK = 18 | SDMMC_CMD_R1 | SDMMC_CMD_MULTI_BLOCK;

// === Sequential write commands (Class 3) ===


// MMC Cmd20(addressed+d, R1): Write a data stream to the card from given address until a STOP_TRANSMISSION follows.
static constexpr uint32_t MMC_CMD20_WRITE_DAT_UNTIL_STOP = 20 | SDMMC_CMD_R1;

// === Block-oriented write commands (Class 4) ===

// MMC Cmd23(addressed, R1): Set block count.
static constexpr uint32_t MMC_CMD23_SET_BLOCK_COUNT        = 23 | SDMMC_CMD_R1;
// Cmd24(addressed+d, R1): Write block.
static constexpr uint32_t SDMMC_CMD24_WRITE_BLOCK          = 24 | SDMMC_CMD_R1 | SDMMC_CMD_WRITE | SDMMC_CMD_SINGLE_BLOCK;
// Cmd25(addressed+d, R1): Write multiple blocks.
static constexpr uint32_t SDMMC_CMD25_WRITE_MULTIPLE_BLOCK = 25 | SDMMC_CMD_R1 | SDMMC_CMD_WRITE | SDMMC_CMD_MULTI_BLOCK;
// MMC Cmd26(addressed+d, R1): Programming of the card identification register.
static constexpr uint32_t MMC_CMD26_PROGRAM_CID            = 26 | SDMMC_CMD_R1;
// Cmd27(addressed+d, R1): Programming of the programmable bits of the CSD.
static constexpr uint32_t SDMMC_CMD27_PROGRAM_CSD          = 27 | SDMMC_CMD_R1;

// === Erase commands  (Class 5) ===

// SD Cmd32(addressed, R1):
static constexpr uint32_t SD_CMD32_ERASE_WR_BLK_START = 32 | SDMMC_CMD_R1;
// SD Cmd33(addressed, R1):
static constexpr uint32_t SD_CMD33_ERASE_WR_BLK_END   = 33 | SDMMC_CMD_R1;
// MMC Cmd35(addressed, R1):
static constexpr uint32_t MMC_CMD35_ERASE_GROUP_START = 35 | SDMMC_CMD_R1;
// MMC Cmd36(addressed, R1):
static constexpr uint32_t MMC_CMD36_ERASE_GROUP_END   = 36 | SDMMC_CMD_R1;
// Cmd38(addressed, R1B):
static constexpr uint32_t SDMMC_CMD38_ERASE           = 38 | SDMMC_CMD_R1B;

// === Block Oriented Write Protection Commands (class 6) ===

// Cmd28(addressed, R1b): Set write protection.
static constexpr uint32_t SDMMC_CMD28_SET_WRITE_PROT  = 28 | SDMMC_CMD_R1B;
// Cmd29(addressed, R1b): Clr write protection.
static constexpr uint32_t SDMMC_CMD29_CLR_WRITE_PROT  = 29 | SDMMC_CMD_R1B;
// Cmd30(addressed+d, R1b): Send write protection.
static constexpr uint32_t SDMMC_CMD30_SEND_WRITE_PROT = 30 | SDMMC_CMD_R1;

// === Lock Card (Class 7) ===

// Cmd42(addressed+d, R1): Used to set/reset the password or lock/unlock the card.
static constexpr uint32_t SDMMC_CMD42_LOCK_UNLOCK = 42 | SDMMC_CMD_R1;

// === Application-specific commands (Class 8) ===

// Cmd55(addressed, R1): Indicate to the card that the next command is an application specific command rather than a standard command.
static constexpr uint32_t SDMMC_CMD55_APP_CMD    = 55 | SDMMC_CMD_R1;
// Cmd56(addressed+d, R1): Used either to transfer a data block to the card or to get
// a data block from the card for general purpose/application specific commands.
static constexpr uint32_t SDMMC_CMD56_GEN_CMD    = 56 | SDMMC_CMD_R1;
// MMC Cmd6(addressed, R1b) : Switch the mode of operation of the selected card or modifies the EXT_CSD registers.
static constexpr uint32_t MMC_CMD6_SWITCH        = 6 | SDMMC_CMD_R1B;
// SD Cmd6(addressed+d, R1) : Check switchable function (mode 0) and switch card function (mode 1).
static constexpr uint32_t SD_CMD6_SWITCH_FUNC    = 6 | SDMMC_CMD_R1 | SDMMC_CMD_SINGLE_BLOCK;
// ACmd6(addressed, R1): Define the data bus width.
static constexpr uint32_t SD_ACMD6_SET_BUS_WIDTH = 6 | SDMMC_CMD_R1;
// ACmd13(addressed+d, R1): Send the SD Status.
static constexpr uint32_t SD_ACMD13_SD_STATUS    = 13 | SDMMC_CMD_R1;
// ACmd22(addressed+d, R1): Send the number of the written (without errors) write blocks.
static constexpr uint32_t SD_ACMD22_SEND_NUM_WR_BLOCKS     = 22 | SDMMC_CMD_R1;
// ACmd23(addressed, R1): Set the number of write blocks to be pre-erased before writing:
static constexpr uint32_t SD_ACMD23_SET_WR_BLK_ERASE_COUNT = 23 | SDMMC_CMD_R1;

// ACmd41(broadcast+r, R3): Send host capacity support information (HCS) and asks the accessed
// card to send its operating condition register (OCR) content in the response
static constexpr uint32_t SD_MCI_ACMD41_SD_SEND_OP_COND = 41 | SDMMC_CMD_R3 | SDMMC_CMD_OPENDRAIN;
// ACmd42(addressed, R1): Connect[1]/Disconnect[0] the 50 KOhm pull-up resistor on CD/DAT3 (pin 1) of the card.
static constexpr uint32_t SD_ACMD42_SET_CLR_CARD_DETECT = 42 | SDMMC_CMD_R1;
// ACmd51(addressed+d, R1): Read the SD Configuration Register (SCR).
static constexpr uint32_t SD_ACMD51_SEND_SCR            = 51 | SDMMC_CMD_R1 | SDMMC_CMD_SINGLE_BLOCK;

// === I/O mode commands (class 9) ===

// MMC Cmd39(addressed, R4): Used to write and read 8 bit (register) data fields.
static constexpr uint32_t MMC_CMD39_FAST_IO              = 39 | SDMMC_CMD_R4;
// MMC Cmd40(broadcast+r, R5): Set the system into interrupt mode.
static constexpr uint32_t MMC_CMD40_GO_IRQ_STATE         = 40 | SDMMC_CMD_R5 | SDMMC_CMD_OPENDRAIN;
// SDIO Cmd5(R4): Send operation condition.
static constexpr uint32_t SDIO_CMD5_SEND_OP_COND         = 5 | SDMMC_CMD_R4 | SDMMC_CMD_OPENDRAIN;
// SDIO Cmd52(R5): Direct IO read/write.
static constexpr uint32_t SDIO_CMD52_IO_RW_DIRECT        = 52 | SDMMC_CMD_R5;
// SDIO Cmd53(R5): Extended IO read/write.
static constexpr uint32_t SDIO_CMD53_IO_R_BYTE_EXTENDED  = 53 | SDMMC_CMD_R5 | SDMMC_CMD_SDIO_BYTE;
static constexpr uint32_t SDIO_CMD53_IO_W_BYTE_EXTENDED  = 53 | SDMMC_CMD_R5 | SDMMC_CMD_SDIO_BYTE | SDMMC_CMD_WRITE;
static constexpr uint32_t SDIO_CMD53_IO_R_BLOCK_EXTENDED = 53 | SDMMC_CMD_R5 | SDMMC_CMD_SDIO_BLOCK;
static constexpr uint32_t SDIO_CMD53_IO_W_BLOCK_EXTENDED = 53 | SDMMC_CMD_R5 | SDMMC_CMD_SDIO_BLOCK | SDMMC_CMD_WRITE;

///////////////////////////////////////////////////////////////////////////////
// SDIO definitions

// SDIO state in R5:
static constexpr uint32_t SDIO_R5_COM_CRC_ERROR   = 1UL << 15; // CRC check error.
static constexpr uint32_t SDIO_R5_ILLEGAL_COMMAND = 1UL << 14; // Illegal command.
static constexpr uint32_t SDIO_R5_STATE           = 3UL << 12; // SDIO R5 state mask.
static constexpr uint32_t SDIO_R5_STATE_DIS       = 0UL << 12; // Disabled.
static constexpr uint32_t SDIO_R5_STATE_CMD       = 1UL << 12; // DAT lines free.
static constexpr uint32_t SDIO_R5_STATE_TRN       = 2UL << 12; // Transfer.
static constexpr uint32_t SDIO_R5_STATE_RFU       = 3UL << 12; // Reserved.
static constexpr uint32_t SDIO_R5_ERROR           = 1UL << 11; // General error.
static constexpr uint32_t SDIO_R5_FUNC_NUM        = 1UL << 9;  // Invalid function number.
static constexpr uint32_t SDIO_R5_OUT_OF_RANGE    = 1UL << 8;  // Argument out of range.
static constexpr uint32_t SDIO_R5_STATUS_ERR      = SDIO_R5_ERROR | SDIO_R5_FUNC_NUM | SDIO_R5_OUT_OF_RANGE; // Error status bits mask.

// SDIO state in R6:
static constexpr uint32_t SDIO_R6_COM_CRC_ERROR   = 1UL << 15; // The CRC check of the previous command failed.
static constexpr uint32_t SDIO_R6_ILLEGAL_COMMAND = 1UL << 14; // Command not legal for the card state.
static constexpr uint32_t SDIO_R6_ERROR           = 1UL << 13; // A general or an unknown error occurred during the operation.
static constexpr uint32_t SDIO_STATUS_R6          = SDIO_R6_COM_CRC_ERROR | SDIO_R6_ILLEGAL_COMMAND | SDIO_R6_ERROR; // Error status bits mask.

// SDIO CMD52 argument bit offsets:
static constexpr uint32_t SDIO_CMD52_WR_DATA      = 0;  // [ 7: 0] Write data or stuff bits.
static constexpr uint32_t SDIO_CMD52_STUFF0       = 8;  // [    8] Reserved.
static constexpr uint32_t SDIO_CMD52_REG_ADRR     = 9;  // [25: 9] Register address.
static constexpr uint32_t SDIO_CMD52_STUFF1       = 26; // [   26] Reserved.
static constexpr uint32_t SDIO_CMD52_RAW_FLAG     = 27; // [   27] Read after Write flag.
static constexpr uint32_t SDIO_CMD52_FUNCTION_NUM = 28; // [30:28] Number of the function.
static constexpr uint32_t SDIO_CMD52_RW_FLAG      = 31; // [   31] Direction, 1:write, 0:read.
static constexpr uint32_t   SDIO_CMD52_READ_FLAG  = 0;
static constexpr uint32_t   SDIO_CMD52_WRITE_FLAG = 1;

// SDIO CMD53 argument structure:
static constexpr uint32_t SDIO_CMD53_COUNT        = 0;  // [ 8: 0] Byte mode: number of bytes to transfer (0 == 512). Block mode: number of blocks to transfer, 0 means infinite.
static constexpr uint32_t SDIO_CMD53_REG_ADDR     = 9;  // [25: 9] Start Address I/O register.
static constexpr uint32_t SDIO_CMD53_OP_CODE      = 26; // [   26] 1:Incrementing address, 0: fixed.
static constexpr uint32_t SDIO_CMD53_BLOCK_MODE   = 27; // [   27] 1:block mode (Optional).
static constexpr uint32_t SDIO_CMD53_FUNCTION_NUM = 28; // [30:28] Number of the function.
static constexpr uint32_t SDIO_CMD53_RW_FLAG      = 31; // [   31] Direction, 1:WR, 0:RD.
static constexpr uint32_t   SDIO_CMD53_READ_FLAG  = 0;
static constexpr uint32_t   SDIO_CMD53_WRITE_FLAG = 1;

// SDIO Functions:
static constexpr uint32_t SDIO_CIA = 0; // SDIO Function 0 (CIA).
static constexpr uint32_t SDIO_FN0 = 0; // SDIO Function 0.
static constexpr uint32_t SDIO_FN1 = 1; // SDIO Function 1.
static constexpr uint32_t SDIO_FN2 = 2; // SDIO Function 2.
static constexpr uint32_t SDIO_FN3 = 3; // SDIO Function 3.
static constexpr uint32_t SDIO_FN4 = 4; // SDIO Function 4.
static constexpr uint32_t SDIO_FN5 = 5; // SDIO Function 5.
static constexpr uint32_t SDIO_FN6 = 6; // SDIO Function 6.
static constexpr uint32_t SDIO_FN7 = 7; // SDIO Function 7.

// SDIO Card Common Control Registers (CCCR):
static constexpr uint32_t SDIO_CCCR_SDIO_REV    = 0x00;         // CCCR/SDIO revision (RO).
static constexpr uint32_t   SDIO_CCCR_REV_1_00    = 0x0UL << 0; // CCCR/FBR Version 1.00.
static constexpr uint32_t   SDIO_CCCR_REV_1_10    = 0x1UL << 0; // CCCR/FBR Version 1.10.
static constexpr uint32_t   SDIO_CCCR_REV_2_00    = 0x2UL << 0; // CCCR/FBR Version 2.00.
static constexpr uint32_t   SDIO_CCCR_REV_3_00    = 0x3UL << 0; // CCCR/FBR Version 3.00.
static constexpr uint32_t   SDIO_SDIO_REV_1_00    = 0x0UL << 4; // SDIO Spec 1.00.
static constexpr uint32_t   SDIO_SDIO_REV_1_10    = 0x1UL << 4; // SDIO Spec 1.10.
static constexpr uint32_t   SDIO_SDIO_REV_1_20    = 0x2UL << 4; // SDIO Spec 1.20(unreleased).
static constexpr uint32_t   SDIO_SDIO_REV_2_00    = 0x3UL << 4; // SDIO Spec Version 2.00.
static constexpr uint32_t   SDIO_SDIO_REV_3_00    = 0x4UL << 4; // SDIO Spec Version 3.00.

static constexpr uint32_t SDIO_CCCR_SD_REV      = 0x01;         // SD Spec Revision (RO).
static constexpr uint32_t   SDIO_SD_REV_1_01      = 0x0UL << 0; // SD 1.01 (Mar 2000).
static constexpr uint32_t   SDIO_SD_REV_1_10      = 0x1UL << 0; // SD 1.10 (Oct 2004).
static constexpr uint32_t   SDIO_SD_REV_2_00      = 0x2UL << 0; // SD 2.00 (May 2006).
static constexpr uint32_t   SDIO_SD_REV_3_00      = 0x3UL << 0; // SD 3.00.

static constexpr uint32_t SDIO_CCCR_IOE         = 0x02;       // I/O Enable (R/W).
static constexpr uint32_t   SDIO_IOE_FN1          = 1UL << 1; // Function 1 Enable/Disable.
static constexpr uint32_t   SDIO_IOE_FN2          = 1UL << 2; // Function 2 Enable/Disable.
static constexpr uint32_t   SDIO_IOE_FN3          = 1UL << 3; // Function 3 Enable/Disable.
static constexpr uint32_t   SDIO_IOE_FN4          = 1UL << 4; // Function 4 Enable/Disable.
static constexpr uint32_t   SDIO_IOE_FN5          = 1UL << 5; // Function 5 Enable/Disable.
static constexpr uint32_t   SDIO_IOE_FN6          = 1UL << 6; // Function 6 Enable/Disable.
static constexpr uint32_t   SDIO_IOE_FN7          = 1UL << 7; // Function 7 Enable/Disable.
static constexpr uint32_t SDIO_CCCR_IOR         = 0x03;       // I/O Ready (RO).
static constexpr uint32_t   SDIO_IOR_FN1          = 1UL << 1; // Function 1 ready.
static constexpr uint32_t   SDIO_IOR_FN2          = 1UL << 2; // Function 2 ready.
static constexpr uint32_t   SDIO_IOR_FN3          = 1UL << 3; // Function 3 ready.
static constexpr uint32_t   SDIO_IOR_FN4          = 1UL << 4; // Function 4 ready.
static constexpr uint32_t   SDIO_IOR_FN5          = 1UL << 5; // Function 5 ready.
static constexpr uint32_t   SDIO_IOR_FN6          = 1UL << 6; // Function 6 ready.
static constexpr uint32_t   SDIO_IOR_FN7          = 1UL << 7; // Function 7 ready.
static constexpr uint32_t SDIO_CCCR_IEN         = 0x04;       // Int Enable.
static constexpr uint32_t   SDIO_IENM             = 1UL << 0; // Int Enable Master (R/W).
static constexpr uint32_t   SDIO_IEN_FN1          = 1UL << 1; // Function 1 Int Enable.
static constexpr uint32_t   SDIO_IEN_FN2          = 1UL << 2; // Function 2 Int Enable.
static constexpr uint32_t   SDIO_IEN_FN3          = 1UL << 3; // Function 3 Int Enable.
static constexpr uint32_t   SDIO_IEN_FN4          = 1UL << 4; // Function 4 Int Enable.
static constexpr uint32_t   SDIO_IEN_FN5          = 1UL << 5; // Function 5 Int Enable.
static constexpr uint32_t   SDIO_IEN_FN6          = 1UL << 6; // Function 6 Int Enable.
static constexpr uint32_t   SDIO_IEN_FN7          = 1UL << 7; // Function 7 Int Enable.
static constexpr uint32_t SDIO_CCCR_INT         = 0x05;       // Int Pending.
static constexpr uint32_t   SDIO_INT_FN1          = 1UL << 1; // Function 1 Int pending.
static constexpr uint32_t   SDIO_INT_FN2          = 1UL << 2; // Function 2 Int pending.
static constexpr uint32_t   SDIO_INT_FN3          = 1UL << 3; // Function 3 Int pending.
static constexpr uint32_t   SDIO_INT_FN4          = 1UL << 4; // Function 4 Int pending.
static constexpr uint32_t   SDIO_INT_FN5          = 1UL << 5; // Function 5 Int pending.
static constexpr uint32_t   SDIO_INT_FN6          = 1UL << 6; // Function 6 Int pending.
static constexpr uint32_t   SDIO_INT_FN7          = 1UL << 7; // Function 7 Int pending.
static constexpr uint32_t SDIO_CCCR_IOA         = 0x06;       // I/O Abort.
static constexpr uint32_t   SDIO_AS_FN1           = 1UL << 0; // Abort function 1 IO.
static constexpr uint32_t   SDIO_AS_FN2           = 2UL << 0; // Abort function 2 IO.
static constexpr uint32_t   SDIO_AS_FN3           = 3UL << 0; // Abort function 3 IO.
static constexpr uint32_t   SDIO_AS_FN4           = 4UL << 0; // Abort function 4 IO.
static constexpr uint32_t   SDIO_AS_FN5           = 5UL << 0; // Abort function 5 IO.
static constexpr uint32_t   SDIO_AS_FN6           = 6UL << 0; // Abort function 6 IO.
static constexpr uint32_t   SDIO_AS_FN7           = 7UL << 0; // Abort function 7 IO.
static constexpr uint32_t   SDIO_RES              = 1UL << 3; // IO CARD RESET (WO).
static constexpr uint32_t SDIO_CCCR_BUS_CTRL    = 0x07;       // Bus Interface Control.
static constexpr uint32_t   SDIO_BUSWIDTH_1B      = 0UL << 0; // 1-bit data bus.
static constexpr uint32_t   SDIO_BUSWIDTH_4B      = 2UL << 0; // 4-bit data bus.

static constexpr uint32_t   SDIO_BUS_ECSI         = 1UL << 5; // Enable Continuous SPI interrupt (R/W).
static constexpr uint32_t   SDIO_BUS_SCSI         = 1UL << 6; // Support Continuous SPI interrupt (RO).
static constexpr uint32_t   SDIO_BUS_CD_DISABLE   = 1UL << 7; // Connect(0)/Disconnect(1) pull-up on CD/DAT[3] (R/W).
static constexpr uint32_t SDIO_CCCR_CAP         = 0x08;       // Card Capability.
static constexpr uint32_t   SDIO_CAP_SDC          = 1UL << 0; // Support Direct Commands during data transfer (RO).
static constexpr uint32_t   SDIO_CAP_SMB          = 1UL << 1; // Support Multi-Block (RO).
static constexpr uint32_t   SDIO_CAP_SRW          = 1UL << 2; // Support Read Wait (RO).
static constexpr uint32_t   SDIO_CAP_SBS          = 1UL << 3; // Support Suspend/Resume (RO).
static constexpr uint32_t   SDIO_CAP_S4MI         = 1UL << 4; // Support interrupt between blocks of data in 4-bit SD mode (RO).
static constexpr uint32_t   SDIO_CAP_E4MI         = 1UL << 5; // Enable interrupt between blocks of data in 4-bit SD mode (R/W)
static constexpr uint32_t   SDIO_CAP_LSC          = 1UL << 6; // Low-Speed Card (RO).
static constexpr uint32_t   SDIO_CAP_4BLS         = 1UL << 7; // 4-bit support for Low-Speed Card (RO).
static constexpr uint32_t SDIO_CCCR_CIS_PTR     = 0x09;       // Pointer to CIS (3B, LSB first).
static constexpr uint32_t SDIO_CCCR_BUS_SUSPEND = 0x0c;       // Bus Suspend.
static constexpr uint32_t   SDIO_BS               = 1UL << 0; // Bus Status (transfer on DAT[x] lines) (RO).
static constexpr uint32_t   SDIO_BR               = 1UL << 1; // Bus Release Request/Status (R/W).
static constexpr uint32_t SDIO_CCCR_FUN_SEL     = 0x0d;       // Function select.
static constexpr uint32_t   SDIO_DF               = 1UL << 7; // Resume Data Flag (RO).
static constexpr uint32_t   SDIO_FS_CIA           = 0UL << 0; // Select CIA (function 0).
static constexpr uint32_t   SDIO_FS_FN1           = 1UL << 0; // Select Function 1.
static constexpr uint32_t   SDIO_FS_FN2           = 2UL << 0; // Select Function 2.
static constexpr uint32_t   SDIO_FS_FN3           = 3UL << 0; // Select Function 3.
static constexpr uint32_t   SDIO_FS_FN4           = 4UL << 0; // Select Function 4.
static constexpr uint32_t   SDIO_FS_FN5           = 5UL << 0; // Select Function 5.
static constexpr uint32_t   SDIO_FS_FN6           = 6UL << 0; // Select Function 6.
static constexpr uint32_t   SDIO_FS_FN7           = 7UL << 0; // Select Function 7.
static constexpr uint32_t   SDIO_FS_MEM           = 8UL << 0; // Select memory in combo card.
static constexpr uint32_t SDIO_CCCR_EXEC        = 0x0e;       // Exec Flags (RO).
static constexpr uint32_t   SDIO_EXM              = 1UL << 0; // Executing status of memory.
static constexpr uint32_t   SDIO_EX_FN1           = 1UL << 1; // Executing status of func 1.
static constexpr uint32_t   SDIO_EX_FN2           = 1UL << 2; // Executing status of func 2.
static constexpr uint32_t   SDIO_EX_FN3           = 1UL << 3; // Executing status of func 3.
static constexpr uint32_t   SDIO_EX_FN4           = 1UL << 4; // Executing status of func 4.
static constexpr uint32_t   SDIO_EX_FN5           = 1UL << 5; // Executing status of func 5.
static constexpr uint32_t   SDIO_EX_FN6           = 1UL << 6; // Executing status of func 6.
static constexpr uint32_t   SDIO_EX_FN7           = 1UL << 7; // Executing status of func 7.
static constexpr uint32_t SDIO_CCCR_READY       = 0x0f;       // Ready Flags (RO).
static constexpr uint32_t   SDIO_RFM              = 1UL << 0; // Ready Flag for memory.
static constexpr uint32_t   SDIO_RF_FN1           = 1UL << 1; // Ready Flag for function 1.
static constexpr uint32_t   SDIO_RF_FN2           = 1UL << 2; // Ready Flag for function 2.
static constexpr uint32_t   SDIO_RF_FN3           = 1UL << 3; // Ready Flag for function 3.
static constexpr uint32_t   SDIO_RF_FN4           = 1UL << 4; // Ready Flag for function 4.
static constexpr uint32_t   SDIO_RF_FN5           = 1UL << 5; // Ready Flag for function 5.
static constexpr uint32_t   SDIO_RF_FN6           = 1UL << 6; // Ready Flag for function 6.
static constexpr uint32_t   SDIO_RF_FN7           = 1UL << 7; // Ready Flag for function 7.
static constexpr uint32_t SDIO_CCCR_FN0_BLKSIZ  = 0x10;       // FN0 Block Size (2B, LSB first) (R/W).
static constexpr uint32_t SDIO_CCCR_POWER       = 0x12;       // Power Control.
static constexpr uint32_t   SDIO_POWER_SMPC       = 1UL << 0; // Support Master Power Control.
static constexpr uint32_t   SDIO_POWER_EMPC       = 1UL << 1; // Enable Master Power Control.
static constexpr uint32_t SDIO_CCCR_HS          = 0x13;       // High-Speed.
static constexpr uint32_t   SDIO_SHS              = 1UL << 0; // Support High-Speed (RO).
static constexpr uint32_t   SDIO_EHS              = 1UL << 1; // Enable High-Speed (R/W).

// SDIO Card Meta format:
static constexpr uint32_t SDIO_CISTPL_NULL     = 0x00; // Null tuple (PCMCIA 3.1.9).
static constexpr uint32_t SDIO_CISTPL_DEVICE   = 0x01; // Device tuple (PCMCIA 3.2.2).
static constexpr uint32_t SDIO_CISTPL_CHECKSUM = 0x10; // Checksum control (PCMCIA 3.1.1).
static constexpr uint32_t SDIO_CISTPL_VERS_1   = 0x15; // Level 1 version (PCMCIA 3.2.10).
static constexpr uint32_t SDIO_CISTPL_ALTSTR   = 0x16; // Alternate Language String (PCMCIA 3.2.1).
static constexpr uint32_t SDIO_CISTPL_MANFID   = 0x20; // Manufacturer Identification String (PCMCIA 3.2.9).
static constexpr uint32_t SDIO_CISTPL_FUNCID   = 0x21; // Function Identification (PCMCIA 3.2.7).
static constexpr uint32_t SDIO_CISTPL_FUNCE    = 0x22; // Function Extensions (PCMCIA 3.2.6).
static constexpr uint32_t SDIO_CISTPL_SDIO_STD = 0x91; // Additional information for SDIO (PCMCIA 6.1.2).
static constexpr uint32_t SDIO_CISTPL_SDIO_EXT = 0x92; // Reserved for future SDIO (PCMCIA 6.1.3).
static constexpr uint32_t SDIO_CISTPL_END      = 0xff; // The End-of-chain Tuple (PCMCIA 3.1.2).

///////////////////////////////////////////////////////////////////////////////
// CSD, OCR, SCR, Switch status, extend CSD definitions

// Function for extracting bit fields from a large SD MMC register. Used by CSD, SCR, Switch status.
static inline uint32_t SDMMC_ExtractBitField(const uint8_t* reg, int regSize, int fieldPos, int fieldSize)
{
	uint32_t value = reg[((regSize - fieldPos + 7) / 8) - 1] >> (fieldPos % 8);
	if (fieldPos % 8 + fieldSize > 8)  value |= uint32_t(reg[((regSize - fieldPos + 7) / 8) - 2]) << (8 - fieldPos % 8);
	if (fieldPos % 8 + fieldSize > 16) value |= uint32_t(reg[((regSize - fieldPos + 7) / 8) - 3]) << (16 - fieldPos % 8);
	if (fieldPos % 8 + fieldSize > 16) value |= uint32_t(reg[((regSize - fieldPos + 7) / 8) - 3]) << (16 - fieldPos % 8);
	return value & ((1UL << fieldSize) - 1);
}

// CSD Fields:
static constexpr uint32_t CSD_REG_SIZE_BITS  = 128;                   // 128 bits
static constexpr uint32_t CSD_REG_SIZE_BYTES = CSD_REG_SIZE_BITS / 8; // 16 bytes.

static inline uint32_t CSD_STRUCTURE(const uint8_t* csd, int pos, int size) { return SDMMC_ExtractBitField(csd, CSD_REG_SIZE_BITS, pos, size); }
static inline uint32_t CSD_STRUCTURE_VERSION(const uint8_t* csd) { return CSD_STRUCTURE(csd, 126, 2); }

static constexpr uint32_t SD_CSD_VER_1_0  = 0;
static constexpr uint32_t SD_CSD_VER_2_0  = 1;
static constexpr uint32_t MMC_CSD_VER_1_0 = 0;
static constexpr uint32_t MMC_CSD_VER_1_1 = 1;
static constexpr uint32_t MMC_CSD_VER_1_2 = 2;

static inline uint32_t CSD_TRAN_SPEED(const uint8_t* csd)         { return CSD_STRUCTURE(csd, 96, 8);  }
static inline uint32_t SD_CSD_1_0_C_SIZE(const uint8_t* csd)      { return CSD_STRUCTURE(csd, 62, 12); }
static inline uint32_t SD_CSD_1_0_C_SIZE_MULT(const uint8_t* csd) { return CSD_STRUCTURE(csd, 47, 3);  }
static inline uint32_t SD_CSD_1_0_READ_BL_LEN(const uint8_t* csd) { return CSD_STRUCTURE(csd, 80, 4);  }
static inline uint32_t SD_CSD_2_0_C_SIZE(const uint8_t* csd)      { return CSD_STRUCTURE(csd, 48, 22); }
static inline uint32_t MMC_CSD_C_SIZE(const uint8_t* csd)         { return CSD_STRUCTURE(csd, 62, 12); }
static inline uint32_t MMC_CSD_C_SIZE_MULT(const uint8_t* csd)    { return CSD_STRUCTURE(csd, 47, 3);  }
static inline uint32_t MMC_CSD_READ_BL_LEN(const uint8_t* csd)    { return CSD_STRUCTURE(csd, 80, 4);  }
static inline uint32_t MMC_CSD_SPEC_VERS(const uint8_t* csd)      { return CSD_STRUCTURE(csd, 122, 4); }

// OCR Register Fields:
static constexpr uint32_t OCR_REG_SIZE_BITS      = 32;                    // 32 bits.
static constexpr uint32_t OCR_REG_SIZE_BYTES     = OCR_REG_SIZE_BITS / 8; // 4 bytes.
static constexpr uint32_t OCR_VDD_170_195        = 1UL << 7;
static constexpr uint32_t OCR_VDD_20_21          = 1UL << 8;
static constexpr uint32_t OCR_VDD_21_22          = 1UL << 9;
static constexpr uint32_t OCR_VDD_22_23          = 1UL << 10;
static constexpr uint32_t OCR_VDD_23_24          = 1UL << 11;
static constexpr uint32_t OCR_VDD_24_25          = 1UL << 12;
static constexpr uint32_t OCR_VDD_25_26          = 1UL << 13;
static constexpr uint32_t OCR_VDD_26_27          = 1UL << 14;
static constexpr uint32_t OCR_VDD_27_28          = 1UL << 15;
static constexpr uint32_t OCR_VDD_28_29          = 1UL << 16;
static constexpr uint32_t OCR_VDD_29_30          = 1UL << 17;
static constexpr uint32_t OCR_VDD_30_31          = 1UL << 18;
static constexpr uint32_t OCR_VDD_31_32          = 1UL << 19;
static constexpr uint32_t OCR_VDD_32_33          = 1UL << 20;
static constexpr uint32_t OCR_VDD_33_34          = 1UL << 21;
static constexpr uint32_t OCR_VDD_34_35          = 1UL << 22;
static constexpr uint32_t OCR_VDD_35_36          = 1UL << 23;
static constexpr uint32_t OCR_SDIO_S18R          = 1UL << 24; // Switching to 1.8V Accepted.
static constexpr uint32_t OCR_SDIO_MP            = 1UL << 27; // Memory Present.
static constexpr uint32_t OCR_SDIO_NF            = 7UL << 28; // Number of I/O Functions.
static constexpr uint32_t OCR_ACCESS_MODE_MASK   = 3UL << 29; // (MMC) Access mode mask.
static constexpr uint32_t OCR_ACCESS_MODE_BYTE   = 0UL << 29; // (MMC) Byte access mode.
static constexpr uint32_t OCR_ACCESS_MODE_SECTOR = 2UL << 29; // (MMC) Sector access mode.
static constexpr uint32_t OCR_CCS                = 1UL << 30; // (SD) Card Capacity Status.
static constexpr uint32_t OCR_POWER_UP_BUSY      = 1UL << 31; // Card power up status bit.

// SD SCR Register Fields:
static constexpr uint32_t SD_SCR_REG_SIZE_BITS  = 64;                       // 64 bits.
static constexpr uint32_t SD_SCR_REG_SIZE_BYTES = SD_SCR_REG_SIZE_BITS / 8; // 8 bytes.

static inline uint32_t SD_SCR_STRUCTURE(const uint8_t* scr, int pos, int size) { return SDMMC_ExtractBitField(scr, SD_SCR_REG_SIZE_BITS, pos, size); }
static inline uint32_t SD_SCR_SCR_STRUCTURE(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 60, 4); }
static constexpr uint32_t SD_SCR_SCR_STRUCTURE_1_0 = 0;

static inline uint32_t SD_SCR_SD_SPEC(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 56, 4); }
static constexpr uint32_t SD_SCR_SD_SPEC_1_0_01 = 0;
static constexpr uint32_t SD_SCR_SD_SPEC_1_10   = 1;
static constexpr uint32_t SD_SCR_SD_SPEC_2_00   = 2;

static inline uint32_t SD_SCR_DATA_STATUS_AFTER_ERASE(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 55, 1); }
static inline uint32_t SD_SCR_SD_SECURITY(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 52, 3); }
static constexpr uint32_t SD_SCR_SD_SECURITY_NO      = 0;
static constexpr uint32_t SD_SCR_SD_SECURITY_NOTUSED = 1;
static constexpr uint32_t SD_SCR_SD_SECURITY_1_01    = 2;
static constexpr uint32_t SD_SCR_SD_SECURITY_2_00    = 3;
static constexpr uint32_t SD_SCR_SD_SECURITY_3_00    = 4;

static inline uint32_t SD_SCR_SD_BUS_WIDTHS(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 48, 4); }
static constexpr uint32_t SD_SCR_SD_BUS_WIDTH_1BITS = 1UL << 0;
static constexpr uint32_t SD_SCR_SD_BUS_WIDTH_4BITS = 1UL << 2;

static inline uint32_t SD_SCR_SD_SPEC3(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 47, 1); }
static constexpr uint32_t SD_SCR_SD_SPEC_3_00 = 1;

static inline uint32_t SD_SCR_SD_EX_SECURITY(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 43, 4); }
static inline uint32_t SD_SCR_SD_CMD_SUPPORT(const uint8_t* scr) { return SD_SCR_STRUCTURE(scr, 32, 2); }

// SD Switch status fields:
static constexpr uint32_t SD_SW_STATUS_SIZE_BITS  = 512;                        // 512 bits
static constexpr uint32_t SD_SW_STATUS_SIZE_BYTES = SD_SW_STATUS_SIZE_BITS / 8; // 64 bytes

static inline uint32_t SD_SW_STATUS_STRUCTURE(const uint8_t* sd_sw_status, int pos, int size) { return SDMMC_ExtractBitField(sd_sw_status, SD_SW_STATUS_SIZE_BITS, pos, size); }
static inline uint32_t SD_SW_STATUS_MAX_CURRENT_CONSUMPTION(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 496, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP6_INFO(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 480, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP5_INFO(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 464, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP4_INFO(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 448, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP3_INFO(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 432, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP2_INFO(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 416, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP1_INFO(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 400, 16); }

static inline uint32_t SD_SW_STATUS_FUN_GRP6_RC(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 396, 4); }
static inline uint32_t SD_SW_STATUS_FUN_GRP5_RC(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 392, 4); }
static inline uint32_t SD_SW_STATUS_FUN_GRP4_RC(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 388, 4); }
static inline uint32_t SD_SW_STATUS_FUN_GRP3_RC(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 384, 4); }
static inline uint32_t SD_SW_STATUS_FUN_GRP2_RC(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 380, 4); }
static inline uint32_t SD_SW_STATUS_FUN_GRP1_RC(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 376, 4); }

static constexpr uint32_t SD_SW_STATUS_FUN_GRP_RC_ERROR = 0xf;

static inline uint32_t SD_SW_STATUS_DATA_STRUCT_VER(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 368, 8); }
static inline uint32_t SD_SW_STATUS_FUN_GRP6_BUSY(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 352, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP5_BUSY(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 336, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP4_BUSY(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 320, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP3_BUSY(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 304, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP2_BUSY(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 288, 16); }
static inline uint32_t SD_SW_STATUS_FUN_GRP1_BUSY(const uint8_t* status) { return SD_SW_STATUS_STRUCTURE(status, 272, 16); }

// Card status bits:
static constexpr uint32_t CARD_STATUS_APP_CMD           = 1UL << 5;
static constexpr uint32_t CARD_STATUS_SWITCH_ERROR      = 1UL << 7;
static constexpr uint32_t CARD_STATUS_READY_FOR_DATA    = 1UL << 8;
static constexpr uint32_t CARD_STATUS_STATE_Pos			= 9;
static constexpr uint32_t CARD_STATUS_STATE_Msk         = 0xfUL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_IDLE        = 0UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_READY       = 1UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_IDENT       = 2UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_STBY        = 3UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_TRAN        = 4UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_DATA        = 5UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_RCV         = 6UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_PRG         = 7UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_STATE_DIS         = 8UL << CARD_STATUS_STATE_Pos;
static constexpr uint32_t CARD_STATUS_ERASE_RESET       = 1UL << 13;
static constexpr uint32_t CARD_STATUS_WP_ERASE_SKIP     = 1UL << 15;
static constexpr uint32_t CARD_STATUS_CIDCSD_OVERWRITE  = 1UL << 16;
static constexpr uint32_t CARD_STATUS_OVERRUN           = 1UL << 17;
static constexpr uint32_t CARD_STATUS_UNERRUN           = 1UL << 18;
static constexpr uint32_t CARD_STATUS_ERROR             = 1UL << 19;
static constexpr uint32_t CARD_STATUS_CC_ERROR          = 1UL << 20;
static constexpr uint32_t CARD_STATUS_CARD_ECC_FAILED   = 1UL << 21;
static constexpr uint32_t CARD_STATUS_ILLEGAL_COMMAND   = 1UL << 22;
static constexpr uint32_t CARD_STATUS_COM_CRC_ERROR     = 1UL << 23;
static constexpr uint32_t CARD_STATUS_UNLOCK_FAILED     = 1UL << 24;
static constexpr uint32_t CARD_STATUS_CARD_IS_LOCKED    = 1UL << 25;
static constexpr uint32_t CARD_STATUS_WP_VIOLATION      = 1UL << 26;
static constexpr uint32_t CARD_STATUS_ERASE_PARAM       = 1UL << 27;
static constexpr uint32_t CARD_STATUS_ERASE_SEQ_ERROR   = 1UL << 28;
static constexpr uint32_t CARD_STATUS_BLOCK_LEN_ERROR   = 1UL << 29;
static constexpr uint32_t CARD_STATUS_ADDRESS_MISALIGN  = 1UL << 30;
static constexpr uint32_t CARD_STATUS_ADDR_OUT_OF_RANGE = 1UL << 31;
static constexpr uint32_t CARD_STATUS_ERR_RD_WR = CARD_STATUS_ADDR_OUT_OF_RANGE | CARD_STATUS_ADDRESS_MISALIGN | CARD_STATUS_BLOCK_LEN_ERROR | CARD_STATUS_WP_VIOLATION | CARD_STATUS_ILLEGAL_COMMAND | CARD_STATUS_CC_ERROR | CARD_STATUS_ERROR;

// SD status field.
static constexpr uint32_t SD_STATUS_SIZE_BYTES = 512 / 8;  // 512 bits, 64bytes.

// MMC Extended CSD Register Field.
static constexpr uint32_t EXT_CSD_SIZE_BYTES = 512;  // 512 bytes.

// Properties Segment:
static constexpr uint32_t EXT_CSD_S_CMD_SET_INDEX            = 504;
static constexpr uint32_t EXT_CSD_BOOT_INFO_INDEX            = 228;
static constexpr uint32_t EXT_CSD_BOOT_SIZE_MULTI_INDEX      = 226;
static constexpr uint32_t EXT_CSD_ACC_SIZE_INDEX             = 225;
static constexpr uint32_t EXT_CSD_HC_ERASE_GRP_SIZE_INDEX    = 224;
static constexpr uint32_t EXT_CSD_ERASE_TIMEOUT_MULT_INDEX   = 223;
static constexpr uint32_t EXT_CSD_REL_WR_SEC_C_INDEX         = 222;
static constexpr uint32_t EXT_CSD_HC_WP_GRP_SIZE_INDEX       = 221;
static constexpr uint32_t EXT_CSD_S_C_VCC_INDEX              = 220;
static constexpr uint32_t EXT_CSD_S_C_VCCQ_INDEX             = 219;
static constexpr uint32_t EXT_CSD_S_A_TIMEOUT_INDEX          = 217;
static constexpr uint32_t EXT_CSD_SEC_COUNT_INDEX            = 212;
static constexpr uint32_t EXT_CSD_MIN_PERF_W_8_52_INDEX      = 210;
static constexpr uint32_t EXT_CSD_MIN_PERF_R_8_52_INDEX      = 209;
static constexpr uint32_t EXT_CSD_MIN_PERF_W_8_26_4_52_INDEX = 208;
static constexpr uint32_t EXT_CSD_MIN_PERF_R_8_26_4_52_INDEX = 207;
static constexpr uint32_t EXT_CSD_MIN_PERF_W_4_26_INDEX      = 206;
static constexpr uint32_t EXT_CSD_MIN_PERF_R_4_26_INDEX      = 205;
static constexpr uint32_t EXT_CSD_PWR_CL_26_360_INDEX        = 203;
static constexpr uint32_t EXT_CSD_PWR_CL_52_360_INDEX        = 202;
static constexpr uint32_t EXT_CSD_PWR_CL_26_195_INDEX        = 201;
static constexpr uint32_t EXT_CSD_PWR_CL_52_195_INDEX        = 200;
static constexpr uint32_t EXT_CSD_CARD_TYPE_INDEX            = 196;

// MMC card type.
static constexpr uint32_t MMC_CTYPE_26MHZ = 0x1;
static constexpr uint32_t MMC_CTYPE_52MHZ = 0x2;

static constexpr uint32_t EXT_CSD_CSD_STRUCTURE_INDEX        = 194;
static constexpr uint32_t EXT_CSD_EXT_CSD_REV_INDEX          = 192;

// Mode Segment:
static constexpr uint32_t EXT_CSD_CMD_SET_INDEX              = 191;
static constexpr uint32_t EXT_CSD_CMD_SET_REV_INDEX          = 189;
static constexpr uint32_t EXT_CSD_POWER_CLASS_INDEX          = 187;
static constexpr uint32_t EXT_CSD_HS_TIMING_INDEX            = 185;
static constexpr uint32_t EXT_CSD_BUS_WIDTH_INDEX            = 183;
static constexpr uint32_t EXT_CSD_ERASED_MEM_CONT_INDEX      = 181;
static constexpr uint32_t EXT_CSD_BOOT_CONFIG_INDEX          = 179;
static constexpr uint32_t EXT_CSD_BOOT_BUS_WIDTH_INDEX       = 177;
static constexpr uint32_t EXT_CSD_ERASE_GROUP_DEF_INDEX      = 175;


///////////////////////////////////////////////////////////////////////////////
// Definition for SPI mode only:

// SPI commands start with a start bit "0" and a transmit bit "1"
static inline uint8_t SPI_CMD_ENCODE(uint32_t x) { return uint8_t(0x40 | SDMMC_CMD_GET_INDEX(x)); }

// Register R1 definition for SPI mode. The R1 register is always send after a command.
static constexpr uint32_t R1_SPI_IDLE            = 1UL << 0;
static constexpr uint32_t R1_SPI_ERASE_RESET     = 1UL << 1;
static constexpr uint32_t R1_SPI_ILLEGAL_COMMAND = 1UL << 2;
static constexpr uint32_t R1_SPI_COM_CRC         = 1UL << 3;
static constexpr uint32_t R1_SPI_ERASE_SEQ       = 1UL << 4;
static constexpr uint32_t R1_SPI_ADDRESS         = 1UL << 5;
static constexpr uint32_t R1_SPI_PARAMETER       = 1UL << 6;
// R1 bit 7 is always zero, reuse this bit for error
static constexpr uint32_t R1_SPI_ERROR           = 1UL << 7;


// Register R2 definition for SPI mode. The R2 register can be sent after R1.
static constexpr uint32_t R2_SPI_CARD_LOCKED      = 1UL << 0;
static constexpr uint32_t R2_SPI_WP_ERASE_SKIP    = 1UL << 1;
static constexpr uint32_t R2_SPI_LOCK_UNLOCK_FAIL = R2_SPI_WP_ERASE_SKIP;
static constexpr uint32_t R2_SPI_ERROR            = 1UL << 2;
static constexpr uint32_t R2_SPI_CC_ERROR         = 1UL << 3;
static constexpr uint32_t R2_SPI_CARD_ECC_ERROR   = 1UL << 4;
static constexpr uint32_t R2_SPI_WP_VIOLATION     = 1UL << 5;
static constexpr uint32_t R2_SPI_ERASE_PARAM      = 1UL << 6;
static constexpr uint32_t R2_SPI_OUT_OF_RANGE     = 1UL << 7;
static constexpr uint32_t R2_SPI_CSD_OVERWRITE    = R2_SPI_OUT_OF_RANGE;


///////////////////////////////////////////////////////////////////////////////
// Control Tokens in SPI Mode:

// Tokens used for a read operations.
static constexpr uint32_t SPI_TOKEN_SINGLE_MULTI_READ = 0xfe;
static inline bool SPI_TOKEN_DATA_ERROR_VALID(uint8_t token) { return (token & 0xf0) == 0; }
static constexpr uint32_t SPI_TOKEN_DATA_ERROR_ERRORS    = 0x0f;
static constexpr uint32_t SPI_TOKEN_DATA_ERROR_ERROR     = 1UL << 0;
static constexpr uint32_t SPI_TOKEN_DATA_ERROR_CC_ERROR  = 1UL << 1;
static constexpr uint32_t SPI_TOKEN_DATA_ERROR_ECC_ERROR = 1UL << 2;
static constexpr uint32_t SPI_TOKEN_DATA_ERROR_OUT_RANGE = 1UL << 3;

// Tokens used for a write operations.
static constexpr uint32_t SPI_TOKEN_SINGLE_WRITE = 0xfe;
static constexpr uint32_t SPI_TOKEN_MULTI_WRITE  = 0xfc;
static constexpr uint32_t SPI_TOKEN_STOP_TRAN    = 0xfd;
static inline bool    SPI_TOKEN_DATA_RESP_VALID(uint8_t token) { return (token & (1 << 4)) == 0 && (token & (1 << 0)) != 0; }
static inline uint8_t SPI_TOKEN_DATA_RESP_CODE(uint8_t token) { return token & 0x1e; }

static constexpr uint32_t SPI_TOKEN_DATA_RESP_ACCEPTED  = 2UL << 1;
static constexpr uint32_t SPI_TOKEN_DATA_RESP_CRC_ERR   = 5UL << 1;
static constexpr uint32_t SPI_TOKEN_DATA_RESP_WRITE_ERR = 6UL << 1;

///////////////////////////////////////////////////////////////////////////////
// Command argument definition

// MMC CMD6 argument structure
// [31:26] Set to 0
static constexpr uint32_t MMC_CMD6_ACCESS_Pos              = 24; // [25:24] Access
static constexpr uint32_t MMC_CMD6_ACCESS_COMMAND_SET      = 0UL << MMC_CMD6_ACCESS_Pos;
static constexpr uint32_t MMC_CMD6_ACCESS_SET_BITS         = 1UL << MMC_CMD6_ACCESS_Pos;
static constexpr uint32_t MMC_CMD6_ACCESS_CLEAR_BITS       = 2UL << MMC_CMD6_ACCESS_Pos;
static constexpr uint32_t MMC_CMD6_ACCESS_WRITE_BYTE       = 3UL << MMC_CMD6_ACCESS_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_Pos               = 16; // [23:16] Index for Mode Segment
static constexpr uint32_t MMC_CMD6_INDEX_CMD_SET           = EXT_CSD_CMD_SET_INDEX         << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_CMD_SET_REV       = EXT_CSD_CMD_SET_REV_INDEX     << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_POWER_CLASS       = EXT_CSD_POWER_CLASS_INDEX     << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_HS_TIMING         = EXT_CSD_HS_TIMING_INDEX       << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_BUS_WIDTH         = EXT_CSD_BUS_WIDTH_INDEX       << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_ERASED_MEM_CONT   = EXT_CSD_ERASED_MEM_CONT_INDEX << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_BOOT_CONFIG       = EXT_CSD_BOOT_CONFIG_INDEX     << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_BOOT_BUS_WIDTH    = EXT_CSD_BOOT_BUS_WIDTH_INDEX  << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_INDEX_ERASE_GROUP_DEF   = EXT_CSD_ERASE_GROUP_DEF_INDEX << MMC_CMD6_INDEX_Pos;
static constexpr uint32_t MMC_CMD6_VALUE_Pos               = 8; // [15:8] Value
static constexpr uint32_t MMC_CMD6_VALUE_BUS_WIDTH_1BIT    = 0x0UL << MMC_CMD6_VALUE_Pos;
static constexpr uint32_t MMC_CMD6_VALUE_BUS_WIDTH_4BIT    = 0x1UL << MMC_CMD6_VALUE_Pos;
static constexpr uint32_t MMC_CMD6_VALUE_BUS_WIDTH_8BIT    = 0x2UL << MMC_CMD6_VALUE_Pos;
static constexpr uint32_t MMC_CMD6_VALUE_HS_TIMING_ENABLE  = 0x1UL << MMC_CMD6_VALUE_Pos;
static constexpr uint32_t MMC_CMD6_VALUE_HS_TIMING_DISABLE = 0x0UL << MMC_CMD6_VALUE_Pos;
// [7:3] Set to 0
// [2:0] Cmd Set


// SD CMD6 argument structure.
static constexpr uint32_t SD_CMD6_GRP1_Pos          = 0;   // [ 3: 0] function group 1, access mode.
static constexpr uint32_t SD_CMD6_GRP1_HIGH_SPEED   = 0x1UL << SD_CMD6_GRP1_Pos;
static constexpr uint32_t SD_CMD6_GRP1_DEFAULT      = 0x0UL << SD_CMD6_GRP1_Pos;
static constexpr uint32_t SD_CMD6_GRP2_Pos          = 4;   // [ 7: 4] function group 2, command system.
static constexpr uint32_t SD_CMD6_GRP2_NO_INFLUENCE = 0xfUL << SD_CMD6_GRP2_Pos;
static constexpr uint32_t SD_CMD6_GRP2_DEFAULT      = 0x0UL << SD_CMD6_GRP2_Pos;
static constexpr uint32_t SD_CMD6_GRP3_Pos          = 8;   // [11: 8] function group 3, 0xf or 0x0.
static constexpr uint32_t SD_CMD6_GRP3_NO_INFLUENCE = 0xfUL << SD_CMD6_GRP3_Pos;
static constexpr uint32_t SD_CMD6_GRP3_DEFAULT      = 0x0UL << SD_CMD6_GRP3_Pos;
static constexpr uint32_t SD_CMD6_GRP4_Pos          = 12;  // [15:12] function group 4, 0xf or 0x0.
static constexpr uint32_t SD_CMD6_GRP4_NO_INFLUENCE = 0xfUL << SD_CMD6_GRP4_Pos;
static constexpr uint32_t SD_CMD6_GRP4_DEFAULT      = 0x0UL << SD_CMD6_GRP4_Pos;
static constexpr uint32_t SD_CMD6_GRP5_Pos          = 16;  // [19:16] function group 5, 0xf or 0x0.
static constexpr uint32_t SD_CMD6_GRP5_NO_INFLUENCE = 0xfUL << SD_CMD6_GRP5_Pos;
static constexpr uint32_t SD_CMD6_GRP5_DEFAULT      = 0x0UL << SD_CMD6_GRP5_Pos;
static constexpr uint32_t SD_CMD6_GRP6_Pos          = 20;  // [23:20] function group 6, 0xf or 0x0.
static constexpr uint32_t SD_CMD6_GRP6_NO_INFLUENCE = 0xfUL << SD_CMD6_GRP6_Pos;
static constexpr uint32_t SD_CMD6_GRP6_DEFAULT      = 0x0UL << SD_CMD6_GRP6_Pos;
// [30:24] reserved 0.
static constexpr uint32_t SD_CMD6_MODE_Pos          = 31;  // [31   ] Mode, 0: Check, 1: Switch.
static constexpr uint32_t SD_CMD6_MODE_CHECK        = 0UL << SD_CMD6_MODE_Pos;
static constexpr uint32_t SD_CMD6_MODE_SWITCH       = 1UL << SD_CMD6_MODE_Pos;

// SD CMD8 argument structure
static constexpr uint32_t SD_CMD8_PATTERN      = 0xaa;
static constexpr uint32_t SD_CMD8_MASK_PATTERN = 0xff;
static constexpr uint32_t SD_CMD8_HIGH_VOLTAGE = 0x100;
static constexpr uint32_t SD_CMD8_MASK_VOLTAGE = 0xf00;

// SD ACMD41 arguments
static constexpr uint32_t SD_ACMD41_HCS_Pos = 30;
static constexpr uint32_t SD_ACMD41_HCS     = 1UL << SD_ACMD41_HCS_Pos; // (SD) Host Capacity Support.

} // namespace sdmmc
