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

#include "System/Platform.h"
#include "Kernel/HAL/STM32/SDRAM_STM32.h"
#include "Utils/Utils.h"

//using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FMCSDRAM::Setup(const FMC_SDRAM_Init& params, const FMC_SDRAM_Timing& timing)
{
    FMC_Bank5_6_R->SDCR[0] // FIXME USE REQUESTED BANK NUM
        = ((params.ColumnBitsNumber - 8) << FMC_SDCRx_NC_Pos)
        | ((params.RowBitsNumber - 11) << FMC_SDCRx_NR_Pos)
        | (uint32_t(params.MemoryDataWidth)  << FMC_SDCRx_MWID_Pos)
        | (uint32_t(params.InternalBankNumber) << FMC_SDCRx_NB_Pos)
        | (params.CASLatency << FMC_SDCRx_CAS_Pos)
        | (params.WriteProtection ? FMC_SDCRx_WP : 0)
        | (params.SDClockPeriod << FMC_SDCRx_SDCLK_Pos)
        | (params.ReadBurst ? FMC_SDCRx_RBURST : 0)
        | (params.ReadPipeDelay << FMC_SDCRx_RPIPE_Pos);

    FMC_Bank5_6_R->SDTR[0] // FIXME USE REQUESTED BANK NUM
        = ((timing.LoadToActiveDelay - 1)       << FMC_SDTRx_TMRD_Pos)
        | ((timing.ExitSelfRefreshDelay - 1)    << FMC_SDTRx_TXSR_Pos)
        | ((timing.SelfRefreshTime - 1)         << FMC_SDTRx_TRAS_Pos)
        | ((timing.RowCycleDelay - 1)           << FMC_SDTRx_TRC_Pos)
        | ((timing.WriteRecoveryTime - 1)       << FMC_SDTRx_TWR_Pos)
        | ((timing.RPDelay - 1)                 << FMC_SDTRx_TRP_Pos)
        | ((timing.RCDDelay - 1)                << FMC_SDTRx_TRCD_Pos);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FMCSDRAM::SetRefreshRate(uint32_t refreshRate)
{
    set_bit_group(FMC_Bank5_6_R->SDRTR, FMC_SDRTR_COUNT_Msk, refreshRate << FMC_SDRTR_COUNT_Pos);

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FMCSDRAM::SendCommand(FMC_SDRAM_CommandMode commandMode, FMC_SDRAM_CommandTarget commandTarget, uint32_t autoRefreshNumber, uint32_t modeRegisterDefinition)
{
    set_bit_group(FMC_Bank5_6_R->SDCMR, FMC_SDCMR_MODE | FMC_SDCMR_CTB2 | FMC_SDCMR_CTB1 | FMC_SDCMR_NRFS | FMC_SDCMR_MRD,
            (uint32_t(commandMode) << FMC_SDCMR_MODE_Pos) | int32_t(commandTarget) | ((autoRefreshNumber - 1) << FMC_SDCMR_NRFS_Pos) | (modeRegisterDefinition << FMC_SDCMR_MRD_Pos));
}
