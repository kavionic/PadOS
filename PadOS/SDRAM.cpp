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

#include "System/Platform.h"
#include "SDRAM.h"
//#include "SystemSetup.h"
//#include "Kernel/SpinTimer.h"

//using namespace kernel;

void SetupSDRAM(uint32_t configReg, uint32_t mode, uint32_t block1Bit, uint32_t refreshNS, uint32_t clkFrequency)
{
    volatile uint32_t i;
    volatile uint16_t *pSdram = (uint16_t *)(SDRAM_CS_ADDR);

    /* SDRAM device configuration */
    /* Step 1. */
    /* Set the features of SDRAM device into the Configuration Register */
    SDRAMC->SDRAMC_CR = configReg;

    /* Step 2. */

    /* For low-power SDRAM, Temperature-Compensated Self Refresh (TCSR),
       Drive Strength (DS) and Partial Array Self Refresh (PASR) must be set
       in the Low-power Register. */
    SDRAMC->SDRAMC_LPR = 0;

    /* Step 3. */
    /* Program the memory device type into the Memory Device Register */
    SDRAMC->SDRAMC_MDR = SDRAMC_MDR_MD_SDRAM;

    /* Step 4. */

    /* A minimum pause of 200 µs is provided to precede any signal toggle.
       (6 core cycles per iteration) */
    for (i = 0; i < ((clkFrequency / 1000000) * 1000 / 6); i++) {
        ;
    }

    /* Step 5. */

    /* A NOP command is issued to the SDR-SDRAM. Program NOP command into
       Mode Register, and the application must set Mode to 1 in the Mode
       Register. Perform a write access to any SDR-SDRAM address to
       acknowledge this command. Now the clock which drives SDR-SDRAM
       device is enabled. */
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_NOP;
    *pSdram = 0x0;

    /* Step 6. */

    /* An all banks precharge command is issued to the SDR-SDRAM. Program
       all banks precharge command into Mode Register, and the application
       must set Mode to 2 in the Mode Register. Perform a write access to
       any SDRSDRAM address to acknowledge this command. */
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_ALLBANKS_PRECHARGE;
    *pSdram = 0x0;

    /* Add some delays after precharge */
    for (i = 0; i < ((clkFrequency / 1000000) * 1000 / 6); i++) {
        ;
    }

    /* Step 7. */
    /* Eight auto-refresh (CBR) cycles are provided. Program the auto
       refresh command (CBR) into Mode Register, and the application
       must set Mode to 4 in the Mode Register. Once in the idle state,
       eight AUTO REFRESH cycles must be performed. */
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x1;

    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x2;

    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x3;

    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x4;

    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x5;

    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x6;

    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x7;

    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_AUTO_REFRESH;
    *pSdram = 0x8;

    /* Step 8. */
    /* A Mode Register Set (MRS) cycle is issued to program the parameters
       of the SDRAM devices, in particular CAS latency and burst length. */
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_LOAD_MODEREG;
    *((uint16_t *)(pSdram + mode)) = 0xcafe;
    
    /* Add some delays */
    for (i = 0; i < ((clkFrequency / 1000000) * 1000 / 6); i++) {
        ;
    }

    /* Step 9. */

    /* For low-power SDR-SDRAM initialization, an Extended Mode Register Set
       (EMRS) cycle is issued to program the SDR-SDRAM parameters (TCSR,
       PASR, DS). The write address must be chosen so that BA[1] is set to
       1 and BA[0] is set to 0. */
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_EXT_LOAD_MODEREG;
    *((uint16_t *)(pSdram + (1 << block1Bit))) = 0x0;

    /* Step 10. */
    /* The application must go into Normal Mode, setting Mode to 0 in the
       Mode Register and perform a write access at any location in the\
       SDRAM to acknowledge this command. */
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_NORMAL;
    *pSdram = 0x0;

    // Step 11.
    // Write the refresh rate into the count field in the SDRAMC Refresh Timer register.

    uint32_t refreshTimer = clkFrequency / 1000;
    refreshTimer *= refreshNS;
    refreshTimer /= 1000000;

    SDRAMC->SDRAMC_TR = SDRAMC_TR_COUNT(refreshTimer);

        
    SDRAMC->SDRAMC_CFR1 |= SDRAMC_CFR1_UNAL; // Enable unaligned access.
}

void ShutdownSDRAM()
{
    volatile uint16_t *pSdram = (uint16_t *)(SDRAM_CS_ADDR);
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_NOP;
    *pSdram = 0x0;
    SDRAMC->SDRAMC_MR = SDRAMC_MR_MODE_DEEP_POWERDOWN;    
    *pSdram = 0x0;
}
