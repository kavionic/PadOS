// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 26.04.2022 21:00

#pragma once

#include <Kernel/HAL/STM32/QSPI_STM32.h>

static constexpr uint8_t QSPI_CMD_WRDI  = 0x04; // Write disable.
static constexpr uint8_t QSPI_CMD_WREN  = 0x06; // Write enable.
static constexpr uint8_t QSPI_CMD_RDSR  = 0x05; // Read Status Register.
static constexpr uint8_t QSPI_CMD_WRSR  = 0x01; // Write Status Register.
static constexpr uint8_t QSPI_CMD_RDFR  = 0x48; // Read Function Register.
static constexpr uint8_t QSPI_CMD_WRFR  = 0x42; // Write Function Register.
static constexpr uint8_t QSPI_CMD_QPIEN = 0x35; // Enter QPI.
static constexpr uint8_t QSPI_CMD_QPIDI = 0xF5; // Exit QPI.

static constexpr uint8_t QSPI_CMD_SRPNV = 0x65; // Set Read Parameter Bits (non-volatile).
static constexpr uint8_t QSPI_CMD_SRPV  = 0x63; // Set Read Parameter Bits (volatile).


static constexpr uint8_t QSPI_CMD_SERPNV = 0x85;  // Set Read Operational Driver Strength (non-volatile).
static constexpr uint8_t QSPI_CMD_SERPV  = 0x83;  // Set Read Operational Driver Strength (volatile).


static constexpr uint8_t QSPI_CMD_RDRP      = 0x61; // Read Read Parameters.
static constexpr uint8_t QSPI_CMD_RDERP     = 0x81; // Read Extended Read Parameters.
static constexpr uint8_t QSPI_CMD_CLERP     = 0x82; // Clear Extended Read Register.

static constexpr uint8_t QSPI_CMD_RDID      = 0xAB; // Read Product Identification

static constexpr uint8_t QSPI_CMD_RDJDID    = 0x9F; // Read Product Identification by JEDEC ID (SPI mode).
static constexpr uint8_t QSPI_CMD_RDJDIDQ   = 0xAF; // Read Product Identification by JEDEC ID (QPI mode).

static constexpr uint8_t QSPI_CMD_RDMDID    = 0x90; // Read Product Identification by RDMDID.
static constexpr uint8_t QSPI_CMD_RDUID     = 0x4B; // Read Unique ID Number.
static constexpr uint8_t QSPI_CMD_RDSFDP    = 0x5A; // Read Serial Flash Discoverable Parameters (SPI only).
static constexpr uint8_t QSPI_CMD_NOP       = 0x00; // No Operation


static constexpr uint8_t QSPI_CMD_RSTEN     = 0x66; // Software Reset Enable.
static constexpr uint8_t QSPI_CMD_RST       = 0x99; // Software Reset.

static constexpr uint8_t QSPI_CMD_IRER      = 0x64; // Information Row Erase.
static constexpr uint8_t QSPI_CMD_IRP       = 0x62; // Information Row Program.
static constexpr uint8_t QSPI_CMD_IRRD      = 0x68; // Information Row Read.

static constexpr uint8_t QSPI_CMD_NORD      = 0x03; // Normal Read (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4NORD     = 0x13; // Normal Read (4 byte address).

static constexpr uint8_t QSPI_CMD_FRD       = 0x0B; // Fast Read (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4FRD      = 0x0C; // Fast Read (4 byte address).

static constexpr uint8_t QSPI_CMD_FRQIO     = 0xEB; // Fast Read Quad I/O (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4FRQIO    = 0xEC; // Fast Read Quad I/O (4 byte address).

static constexpr uint8_t QSPI_CMD_FRDTR     = 0x0D; // Fast Read DTR Mode (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4FRDTR    = 0x0E; // Fast Read DTR Mode (4 byte address).

static constexpr uint8_t QSPI_CMD_FRDDTR    = 0xBD; // Fast Read Dual IO DTR Mode (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4FRDDTR   = 0xBE; // Fast Read Dual IO DTR Mode (4 byte address).

static constexpr uint8_t QSPI_CMD_FRQDTR    = 0xED; // Fast Read Quad IO DTR Mode (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4FRQDTR   = 0xEE; // Fast Read Quad IO DTR Mode (4 byte address).

static constexpr uint8_t QSPI_CMD_SER       = 0x20; // Sector Erase (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4SER      = 0x21; // Sector Erase (4 byte address).

static constexpr uint8_t QSPI_CMD_PP        = 0x02; // Page Program (3/4 byte address).
static constexpr uint8_t QSPI_CMD_4PP       = 0x12; // Page Program (4 byte address).



static constexpr uint8_t QSPI_STATUS_WIP    = 0x01; //  Write In Progress Bit : "0" indicates the device is ready(default) "1" indicates a write cycle is in progress and the device is busy (R)
static constexpr uint8_t QSPI_STATUS_WEL    = 0x02; // Write Enable Latch : "0" indicates the device is not write enabled(default) "1" indicates the device is write enabled (R/W)
static constexpr uint8_t QSPI_STATUS_BP0    = 0x04; // Block Protection Bit : (See Tables 6.4 for details) "0" indicates the specific blocks are not write - protected (default) "1" indicates the specific blocks are write - protected (R/W)
static constexpr uint8_t QSPI_STATUS_BP1    = 0x08;
static constexpr uint8_t QSPI_STATUS_BP2    = 0x10;
static constexpr uint8_t QSPI_STATUS_BP3    = 0x20;
static constexpr uint8_t QSPI_STATUS_QE     = 0x40; // Quad Enable bit : “0” indicates the Quad output function disable(default) “1” indicates the Quad output function enable (R/W).
static constexpr uint8_t QSPI_STATUS_SRWD   = 0x80; // Status Register Write Disable : (See Table 7.1 for details) "0" indicates the Status Register is not write - protected (default) "1" indicates the Status Register is write - protected

static constexpr uint8_t QSPI_READR_BurstLength_Pos     = 0;
static constexpr uint8_t QSPI_READR_BurstLength_Msk     = 3 << QSPI_READR_BurstLength_Pos;
static constexpr uint8_t QSPI_READR_BurstLengthWrap_Pos = 2;
static constexpr uint8_t QSPI_READR_BurstLengthWrap     = 1 << QSPI_READR_BurstLengthWrap_Pos;  // Enable wrap at burst-length.
static constexpr uint8_t QSPI_READR_DummyCycles_Pos     = 3;
static constexpr uint8_t QSPI_READR_DummyCycles_Msk     = 0xf << QSPI_READR_DummyCycles_Pos;
static constexpr uint8_t QSPI_READR_Hold_Reset_Pos      = 7;
static constexpr uint8_t QSPI_READR_Hold_Reset          = 1 << QSPI_READR_Hold_Reset_Pos;  // Enable wrap at burst-length.

static constexpr uint8_t QSPI_READR_BurstLength_8  = 0;
static constexpr uint8_t QSPI_READR_BurstLength_16 = 1;
static constexpr uint8_t QSPI_READR_BurstLength_32 = 2;
static constexpr uint8_t QSPI_READR_BurstLength_64 = 3;


static constexpr uint8_t QSPI_EXT_READR_PROT_E_Pos = 1;
static constexpr uint8_t QSPI_EXT_READR_PROT_E = 1 << QSPI_EXT_READR_PROT_E_Pos;    // Indicates protection error in an Erase or a Program operation.

static constexpr uint8_t QSPI_EXT_READR_P_ERR_Pos = 2;
static constexpr uint8_t QSPI_EXT_READR_P_ERR = 1 << QSPI_EXT_READR_P_ERR_Pos;    // Indicates a Program operation failure or protection error.

static constexpr uint8_t QSPI_EXT_READR_E_ERR_Pos = 3;
static constexpr uint8_t QSPI_EXT_READR_E_ERR = 1 << QSPI_EXT_READR_E_ERR_Pos;    // Indicates a Erase operation failure or protection error.

static constexpr uint8_t QSPI_EXT_READR_DLPEN_Pos = 4;
static constexpr uint8_t QSPI_EXT_READR_DLPEN = 1 << QSPI_EXT_READR_DLPEN_Pos;    // Data Learning Pattern Enable.

static constexpr uint8_t QSPI_EXT_READR_ODS_Pos = 5;
static constexpr uint8_t QSPI_EXT_READR_ODS_Msk = 0x7 << QSPI_EXT_READR_ODS_Pos;    // Data Learning Pattern Enable.

static constexpr uint8_t QSPI_EXT_READR_ODS_12_5    = 1;
static constexpr uint8_t QSPI_EXT_READR_ODS_25      = 2;
static constexpr uint8_t QSPI_EXT_READR_ODS_37_5    = 3;
static constexpr uint8_t QSPI_EXT_READR_ODS_75      = 5;
static constexpr uint8_t QSPI_EXT_READR_ODS_100     = 6;
static constexpr uint8_t QSPI_EXT_READR_ODS_50      = 7;

static constexpr uint8_t QSPI_READ_DUMMY_CYCLES = 12; // 12->120MHz 14->133MHz
static constexpr uint8_t QSPI_READ_BURST_LEN = QSPI_READR_BurstLength_32;

class QSPI_STM32_IS25LP512M : public QSPI_STM32
{
public:
    virtual bool Setup(uint32_t spiFrequency, uint32_t addressBits, PinMuxTarget pinD0, PinMuxTarget pinD1, PinMuxTarget pinD2, PinMuxTarget pinD3, PinMuxTarget pinCLK, PinMuxTarget pinNCS) override;

    virtual void EnableMemoryMapping(bool useContinousRead) override;

    virtual void Erase(uint32_t address, uint32_t length) override;
    virtual void Read(void* data, uint32_t address, uint32_t length) override;
    virtual void Write(const void* data, uint32_t address, uint32_t length) override;
    virtual void WaitWriteInProgress() override;

    uint8_t ReadFunctionRegister(bool quadMode = true) const;

    void ReadProductID(uint8_t& manufacturerID, uint8_t& memoryType, uint8_t& capacity, bool quadMode = true);
    uint32_t ReadProductID(bool quadMode = true);
};


