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
// Created: 29.10.2017 19:51:50

#include "SAME70System.h"
#include "SystemSetup.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SAME70System::SetupClock()
{
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD; // | PMC_WPMR_WPEN_Msk; // Remove register write protection.
    
    PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCXTEN_Msk) | CKGR_MOR_KEY_PASSWD |
    CKGR_MOR_MOSCXTEN_Msk | // Enable main crystal oscillator.
    CKGR_MOR_MOSCXTST(250); // Startup time.
    
    while((PMC->PMC_SR & PMC_SR_MOSCXTS_Msk) == 0); // Wait for crystal oscillator to start up.

    PMC->CKGR_MCFR = (PMC->CKGR_MCFR & ~CKGR_MCFR_CCSS_Msk) | CKGR_MCFR_CCSS_Msk;    // Set frequency counter source to crystal oscillator

    
    for (;;)
    {
        PMC->CKGR_MCFR = (PMC->CKGR_MCFR & ~CKGR_MCFR_RCMEAS_Msk) | CKGR_MCFR_RCMEAS_Msk; // Start frequency measurement.
        while((PMC->CKGR_MCFR & CKGR_MCFR_MAINFRDY_Msk) == 0); // Wait for the measurement result.
        if (((PMC->CKGR_MCFR & CKGR_MCFR_MAINF_Msk) >> CKGR_MCFR_MAINF_Pos) > 16*CLOCK_CRYSTAL_FREQUENCY/2/32000)
        {
            break; // We have reach at least half the target frequency.
        }
    }

    //    printf("%d\n", mainFrequency);
    
    PMC->CKGR_MOR = (PMC->CKGR_MOR & ~CKGR_MOR_MOSCSEL_Msk) | CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCSEL_Msk; // Hook main clock to crystal oscillator.

    while((PMC->PMC_SR & PMC_SR_MOSCSELS_Msk) == 0);   // Wait for main clock to be stable.

    EFC->EEFC_FMR = (EFC->EEFC_FMR & ~EEFC_FMR_FWS_Msk) | EEFC_FMR_FWS(3); // Flash wait states required for 100MHz
    
    PMC->CKGR_PLLAR = CKGR_PLLAR_ONE_Msk |
    CKGR_PLLAR_DIVA(1) |          // Main clock divider.
    CKGR_PLLAR_MULA(CLOCK_CPU_FREQUENCY/CLOCK_CRYSTAL_FREQUENCY - 1) |          // Main clock multiplier - 1.
    CKGR_PLLAR_PLLACOUNT(63);     // Number of clock cycles before LOCKA is set.

    while((PMC->PMC_SR & PMC_SR_LOCKA_Msk) == 0); // Wait for PLL to lock.

    PMC->PMC_MCKR = (PMC->PMC_MCKR & ~PMC_MCKR_PRES_Msk) | PMC_MCKR_PRES_CLK_1; // No processor clock prescaling.
    while((PMC->PMC_SR & PMC_SR_MCKRDY_Msk) == 0);

    PMC->PMC_MCKR = (PMC->PMC_MCKR & ~PMC_MCKR_MDIV_Msk) | PMC_MCKR_MDIV_PCK_DIV3; // Master clock = prescaler output / 2.
    while((PMC->PMC_SR & PMC_SR_MCKRDY_Msk) == 0);

    PMC->PMC_MCKR = (PMC->PMC_MCKR & ~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_PLLA_CLK; // Use PLLA as master clock.
    while((PMC->PMC_SR & PMC_SR_MCKRDY_Msk) == 0);

    PMC->PMC_PCR = PMC_PCR_CMD_Msk | PMC_PCR_PID(11) | PMC_PCR_EN_Msk;



//    PMC->PMC_PCK[6]= (PMC->PMC_PCK[6] & ~PMC_PCK_PRES_Msk) | PMC_PCK_PRES(2); // No processor clock prescaling.
//    PMC->PMC_PCK[6] = (PMC->PMC_PCK[6] & ~PMC_PCK_CSS_Msk) | PMC_PCK_CSS_PLLA_CLK; // Use PLLA as master clock.

    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN_Msk; // Write protect registers.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SAME70System::EnablePeripheralClock( int prefID )
{
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD; // Write enable registers.
    if (prefID < 32) {
        PMC->PMC_PCER0 = BIT32(prefID, 1);
    } else {        
        PMC->PMC_PCER1 = BIT32(prefID-32, 1);
    }        
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN_Msk; // Write protect registers.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SAME70System::DisablePeripheralClock( int prefID )
{
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD; // Write enable registers.
    if (prefID < 32) {
        PMC->PMC_PCDR0 = BIT32(prefID, 1);
    } else {        
        PMC->PMC_PCDR1 = BIT32(prefID-32, 1);
    }        
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN_Msk; // Write protect registers.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t SAME70System::GetFrequencyCrystal()
{
    return CLOCK_CRYSTAL_FREQUENCY;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
uint32_t SAME70System::GetFrequencyCore()
{
    return CLOCK_CPU_FREQUENCY;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
uint32_t SAME70System::GetFrequencyPeripheral()
{
    return CLOCK_PERIF_FREQUENCY;
}


