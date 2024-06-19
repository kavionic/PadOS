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
// Created: 18.04.2022 20:30

#pragma once

#include <System/Sections.h>

enum class FMC_SDRAM_CommandMode
{
    NORMAL_MODE         = 0,
    CLK_ENABLE          = 1,
    PALL                = 2,
    AUTOREFRESH_MODE    = 3,
    LOAD_MODE           = 4,
    SELFREFRESH_MODE    = 5,
    POWERDOWN_MODE      = 6
};

enum class FMC_SDRAM_CommandTarget
{
    Bank2   = FMC_SDCMR_CTB2,
    Bank1   = FMC_SDCMR_CTB1,
    Bank1_2 = FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2
};

enum class FMC_SDRAM_DataWidth
{
    D8 = 0,
    D16 = 1,
    D32 = 2
};

enum class FMC_SDRAM_BankCount
{
    BC2 = 0,
    BC4 = 1
};

struct FMC_SDRAM_Init
{
    uint32_t            SDBank;             // Specifies the SDRAM memory device that will be used.
    uint32_t            ColumnBitsNumber;   // Defines the number of bits of column address.
    uint32_t            RowBitsNumber;      // Defines the number of bits of column address.
    FMC_SDRAM_DataWidth MemoryDataWidth;    // Defines the memory device width.
    FMC_SDRAM_BankCount InternalBankNumber; // Defines the number of the device's internal banks.
    uint32_t            CASLatency;         // Defines the SDRAM CAS latency in number of memory clock cycles.
    bool                WriteProtection;    // Enables the SDRAM device to be accessed in write mode.
    uint32_t            SDClockPeriod;      // Define the SDRAM Clock Period for both SDRAM devices and they allow to disable the clock before changing frequency.
    bool                ReadBurst;          // This bit enable the SDRAM controller to anticipate the next read commands during the CAS latency and stores data in the Read FIFO.
    uint32_t            ReadPipeDelay;      // Define the delay in system clock cycles on read data path.
};

struct FMC_SDRAM_Timing
{
    uint32_t LoadToActiveDelay;     // Defines the delay between a Load Mode Register command and an active or Refresh command in number of memory clock cycles. This parameter can be a value between 1 and 16.
    uint32_t ExitSelfRefreshDelay;  // Defines the delay from releasing the self refresh command to issuing the Activate command in number of memory clock cycles. This parameter can be a value between 1 and 16.
    uint32_t SelfRefreshTime;       // Defines the minimum Self Refresh period in number of memory clock cycles. This parameter can be a value between 1 and 16.
    uint32_t RowCycleDelay;         // Defines the delay between the Refresh command and the Activate command and the delay between two consecutive Refresh commands in number of memory clock cycles. This parameter can be a value between 1 and 16.
    uint32_t WriteRecoveryTime;     // Defines the Write recovery Time in number of memory clock cycles. This parameter can be a value between 1 and 16.
    uint32_t RPDelay;               // Defines the delay between a precharge Command and an other command in number of memory clock cycles. This parameter can be a value between 1 and 16.
    uint32_t RCDDelay;              // Defines the delay between the Activate Command and a Read/Write command in number of memory clock cycles. This parameter can be a value between 1 and 16.
};

class FMCSDRAM
{
public:
    static IFLASHC bool Setup(const FMC_SDRAM_Init& params, const FMC_SDRAM_Timing& timing);
    static IFLASHC void SetRefreshRate(uint32_t refreshRate);
    static IFLASHC void SendCommand(FMC_SDRAM_CommandMode commandMode, FMC_SDRAM_CommandTarget commandTarget, uint32_t autoRefreshNumber, uint32_t modeRegisterDefinition);
};

static constexpr uint32_t SDRAM_MODE_REG_BURST_LENGTH_Pos           = 0;
static constexpr uint32_t SDRAM_MODE_REG_BURST_LENGTH_Msk           = 0x07 << SDRAM_MODE_REG_BURST_LENGTH_Pos;
static constexpr uint32_t SDRAM_MODE_REG_BURST_LENGTH_1_Msk         = 0 << SDRAM_MODE_REG_BURST_LENGTH_Pos;
static constexpr uint32_t SDRAM_MODE_REG_BURST_LENGTH_2_Msk         = 1 << SDRAM_MODE_REG_BURST_LENGTH_Pos;
static constexpr uint32_t SDRAM_MODE_REG_BURST_LENGTH_4_Msk         = 2 << SDRAM_MODE_REG_BURST_LENGTH_Pos;
static constexpr uint32_t SDRAM_MODE_REG_BURST_LENGTH_8_Msk         = 3 << SDRAM_MODE_REG_BURST_LENGTH_Pos;
static constexpr uint32_t SDRAM_MODE_REG_BURST_TYPE_Pos             = 3;
static constexpr uint32_t SDRAM_MODE_REG_BURST_TYPE_SEQUENTIAL_Msk  = 0 << SDRAM_MODE_REG_BURST_TYPE_Pos;
static constexpr uint32_t SDRAM_MODE_REG_BURST_TYPE_INTERLEAVE_Msk  = 1 << SDRAM_MODE_REG_BURST_TYPE_Pos;
static constexpr uint32_t SDRAM_MODE_REG_CAS_LATENCY_Pos            = 4;
static constexpr uint32_t SDRAM_MODE_REG_CAS_LATENCY_Msk            = 0x07 << SDRAM_MODE_REG_CAS_LATENCY_Pos;
static constexpr uint32_t SDRAM_MODE_REG_CAS_LATENCY_2_Msk          = 2 << SDRAM_MODE_REG_CAS_LATENCY_Pos;
static constexpr uint32_t SDRAM_MODE_REG_CAS_LATENCY_3_Msk          = 3 << SDRAM_MODE_REG_CAS_LATENCY_Pos;
static constexpr uint32_t SDRAM_MODE_REG_TEST_MODE_Pos              = 7;
static constexpr uint32_t SDRAM_MODE_REG_TEST_MODE_Msk              = 0x03 << SDRAM_MODE_REG_TEST_MODE_Pos;
static constexpr uint32_t SDRAM_MODE_REG_WBURST_LENGTH_Pos          = 9;
static constexpr uint32_t SDRAM_MODE_REG_WBURST_LENGTH_BURST_Msk    = 0 << SDRAM_MODE_REG_WBURST_LENGTH_Pos;
static constexpr uint32_t SDRAM_MODE_REG_WBURST_LENGTH_SINGLE_Msk   = 1 << SDRAM_MODE_REG_WBURST_LENGTH_Pos;
