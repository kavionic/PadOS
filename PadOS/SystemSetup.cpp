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
// Created: 01.11.2017 09:57:07

#include "SystemSetup.h"
#include "MPU.h"
#include "SDRAM.h"
#include "MemTest.h"
#include "Kernel/HAL/SAME70System.h"


uint8_t sdram_access_test();

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

extern "C" void PreInitSetup()
{
    WDT->WDT_MR = WDT_MR_WDDIS;

    RGBLED_R.SetDirection(DigitalPinDirection_e::Out);
    RGBLED_G.SetDirection(DigitalPinDirection_e::Out);
    RGBLED_B.SetDirection(DigitalPinDirection_e::Out);

    // FPU:
    __DSB();
    SCB->CPACR |= 0xF << 20; // Full access to CP10 & CP 11
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | // Enable CONTROL.FPCA setting on execution of a floating-point instruction.
                  FPU_FPCCR_LSPEN_Msk;  // Enable automatic lazy state preservation for floating-point context.
    __DSB();
    __ISB();
       
    // TCM:
    __DSB();
    __ISB();
    SCB->ITCMCR &= ~(uint32_t)(1UL);
    SCB->DTCMCR &= ~(uint32_t)SCB_DTCMCR_EN_Msk;
    __DSB();
    __ISB();

    SCB_EnableDCache();
    SCB_EnableICache();
    SAME70System::SetupClock();

    SetupMemoryRegions();

    SetupEBIPeripherals();
    SetupSDRAM(SDRAMC_CR_NC_COL10     |
               SDRAMC_CR_NR_ROW13     | 
               SDRAMC_CR_NB_BANK4     | 
               SDRAMC_CR_CAS_LATENCY2 | 
               SDRAMC_CR_DBW_Msk      | 
               SDRAMC_CR_TWR(0/*5*/)       | 
               SDRAMC_CR_TRC_TRFC(9/*13*/) | 
               SDRAMC_CR_TRP(3)       | 
               SDRAMC_CR_TRCD(3)      | 
               SDRAMC_CR_TRAS(6)      | 
               SDRAMC_CR_TXSR(10/*15*/),
               0x27,
               25,
               7812,
               SAME70System::GetFrequencyPeripheral());
    SCB_CleanInvalidateDCache();

    volatile datum* sdramStart = (volatile datum*)SDRAM_START;

    RGBLED_B.Write(true);
    if (memTestDataBus(sdramStart))
    {
        RGBLED_B.Write(false);
        RGBLED_R.Write(true);
        for(;;);
    }
    RGBLED_R.Write(true);
    if (memTestAddressBus(sdramStart, SDRAM_SIZE) != nullptr)
    {
        RGBLED_B.Write(false);
        RGBLED_R.Write(true);
        for(;;);
    }
    RGBLED_R.Write(false);
    RGBLED_G.Write(true);

/*    for(;;)
    {
        datum* resultDev = memTestDevice(sdramStart, SDRAM_SIZE);
        if (resultDev != nullptr)
        {
            RGBLED_B.Write(false);
            RGBLED_G.Write(false);
            RGBLED_R.Write(true);
        }
        else
        {
            break;
        }
    }*/
    RGBLED_R.Write(false);
    RGBLED_G.Write(false);
    RGBLED_B.Write(false);


//    SCB_EnableDCache();

    sdram_access_test();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void SetupEBIPeripherals()
{
    SAME70System::EnablePeripheralClock(ID_SDRAMC);
    SAME70System::EnablePeripheralClock(ID_SMC);

    EBI_DB0_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB1_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB2_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB3_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB4_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB5_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB6_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB7_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB8_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB9_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB10_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB11_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB12_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB13_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB14_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DB15_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);

    EBI_AB2_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB3_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB4_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB5_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB6_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB7_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB8_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB9_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB10_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB11_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB12_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB13_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_AB14_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    
    EBI_DQMH_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_DQML_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    
    EBI_SDRAM_BANK0_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_SDRAM_BANK1_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    
    EBI_SDRAM_CAS_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_SDRAM_CKE_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_SDRAM_CLK_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_SDRAM_CS_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_SDRAM_RAS_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_SDRAM_WE_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_LCD_CS_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_LCD_RD_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_LCD_RS_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);
    EBI_LCD_WR_Pin.SetDriveStrength(DigitalPinDriveStrength_e::High);



    EBI_DB0_Pin.SetPeripheralMux(EBI_DB0_Periph);
    EBI_DB1_Pin.SetPeripheralMux(EBI_DB1_Periph);
    EBI_DB2_Pin.SetPeripheralMux(EBI_DB2_Periph);
    EBI_DB3_Pin.SetPeripheralMux(EBI_DB3_Periph);
    EBI_DB4_Pin.SetPeripheralMux(EBI_DB4_Periph);
    EBI_DB5_Pin.SetPeripheralMux(EBI_DB5_Periph);
    EBI_DB6_Pin.SetPeripheralMux(EBI_DB6_Periph);
    EBI_DB7_Pin.SetPeripheralMux(EBI_DB7_Periph);
    EBI_DB8_Pin.SetPeripheralMux(EBI_DB8_Periph);
    EBI_DB9_Pin.SetPeripheralMux(EBI_DB9_Periph);
    EBI_DB10_Pin.SetPeripheralMux(EBI_DB10_Periph);
    EBI_DB11_Pin.SetPeripheralMux(EBI_DB11_Periph);
    EBI_DB12_Pin.SetPeripheralMux(EBI_DB12_Periph);
    EBI_DB13_Pin.SetPeripheralMux(EBI_DB13_Periph);
    EBI_DB14_Pin.SetPeripheralMux(EBI_DB14_Periph);
    EBI_DB15_Pin.SetPeripheralMux(EBI_DB15_Periph);

    EBI_AB2_Pin.SetPeripheralMux(EBI_AB2_Periph);
    EBI_AB3_Pin.SetPeripheralMux(EBI_AB3_Periph);
    EBI_AB4_Pin.SetPeripheralMux(EBI_AB4_Periph);
    EBI_AB5_Pin.SetPeripheralMux(EBI_AB5_Periph);
    EBI_AB6_Pin.SetPeripheralMux(EBI_AB6_Periph);
    EBI_AB7_Pin.SetPeripheralMux(EBI_AB7_Periph);
    EBI_AB8_Pin.SetPeripheralMux(EBI_AB8_Periph);
    EBI_AB9_Pin.SetPeripheralMux(EBI_AB9_Periph);
    EBI_AB10_Pin.SetPeripheralMux(EBI_AB10_Periph);
    EBI_AB11_Pin.SetPeripheralMux(EBI_AB11_Periph);
    EBI_AB12_Pin.SetPeripheralMux(EBI_AB12_Periph);
    EBI_AB13_Pin.SetPeripheralMux(EBI_AB13_Periph);
    EBI_AB14_Pin.SetPeripheralMux(EBI_AB14_Periph);
    
    EBI_DQMH_Pin.SetPeripheralMux(EBI_DQMH_Periph);
    EBI_DQML_Pin.SetPeripheralMux(EBI_DQML_Periph);
    
    EBI_SDRAM_BANK0_Pin.SetPeripheralMux(EBI_SDRAM_BANK0_Periph);
    EBI_SDRAM_BANK1_Pin.SetPeripheralMux(EBI_SDRAM_BANK1_Periph);    
    EBI_SDRAM_CAS_Pin.SetPeripheralMux(EBI_SDRAM_CAS_Periph);
    EBI_SDRAM_CKE_Pin.SetPeripheralMux(EBI_SDRAM_CKE_Periph);
    EBI_SDRAM_CLK_Pin.SetPeripheralMux(EBI_SDRAM_CLK_Periph);
    EBI_SDRAM_CS_Pin.SetPeripheralMux(EBI_SDRAM_CS_Periph);
    EBI_SDRAM_RAS_Pin.SetPeripheralMux(EBI_SDRAM_RAS_Periph);
    EBI_SDRAM_WE_Pin.SetPeripheralMux(EBI_SDRAM_WE_Periph);
    EBI_LCD_CS_Pin.SetPeripheralMux(EBI_LCD_CS_Periph);
    EBI_LCD_RD_Pin.SetPeripheralMux(EBI_LCD_RD_Periph);
    EBI_LCD_RS_Pin.SetPeripheralMux(EBI_LCD_RS_Periph);
    EBI_LCD_WR_Pin.SetPeripheralMux(EBI_LCD_WR_Periph);

    MATRIX->CCFG_SMCNFCS = CCFG_SMCNFCS_SDRAMEN_Msk;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GfxDriverIO::Setup()
{
    NVIC_DisableIRQ(TC9_IRQn);

    
    LCD_RESET_Pin.SetDirection(DigitalPinDirection_e::Out);
    LCD_RESET_Pin.Write(true);

    LCD_BL_CTRL_Pin.SetDirection(DigitalPinDirection_e::Out);
    LCD_BL_CTRL_Pin = true;


    SMC->SMC_WPMR = SMC_WPMR_WPKEY_PASSWD;
    SMC->SMC_CS_NUMBER[3].SMC_SETUP = SMC_SETUP_NWE_SETUP(0) | SMC_SETUP_NCS_WR_SETUP(0) | SMC_SETUP_NRD_SETUP(0);
    SMC->SMC_CS_NUMBER[3].SMC_PULSE = SMC_PULSE_NWE_PULSE(5) | SMC_PULSE_NCS_WR_PULSE(10) | SMC_PULSE_NRD_PULSE(5) | SMC_PULSE_NCS_RD_PULSE(10);
    SMC->SMC_CS_NUMBER[3].SMC_CYCLE = SMC_CYCLE_NWE_CYCLE(10) | SMC_CYCLE_NRD_CYCLE(10);
    SMC->SMC_CS_NUMBER[3].SMC_MODE = SMC_MODE_READ_MODE_Msk | SMC_MODE_WRITE_MODE_Msk | SMC_MODE_EXNW_MODE_DISABLED | SMC_MODE_BAT_BYTE_WRITE | SMC_MODE_DBW_16_BIT | SMC_MODE_TDF_CYCLES(0); // | SMC_MODE_TDF_MODE_Msk;
}

///////////////////////////////////////////////////////////////////////////////
/// Default memory map
/// Address range        Memory region      Memory type   Shareability  Cache policy
/// 0x00000000- 0x1FFFFFFF Code             Normal        Non-shareable  WT
/// 0x20000000- 0x3FFFFFFF SRAM             Normal        Non-shareable  WBWA
/// 0x40000000- 0x5FFFFFFF Peripheral       Device        Non-shareable  -
/// 0x60000000- 0x7FFFFFFF RAM              Normal        Non-shareable  WBWA
/// 0x80000000- 0x9FFFFFFF RAM              Normal        Non-shareable  WT
/// 0xA0000000- 0xBFFFFFFF Device           Device        Shareable
/// 0xC0000000- 0xDFFFFFFF Device           Device        Non Shareable
/// 0xE0000000- 0xFFFFFFFF System           -                  -
///////////////////////////////////////////////////////////////////////////////

void SetupMemoryRegions()
{
    uint32_t dw_region_base_addr;
    uint32_t dw_region_attr;

    __DMB();

    // ITCM memory region --- Normal
    // START_Addr:-  0x00000000UL
    // END_Addr:-    0x00400000UL
 
    dw_region_base_addr = ITCM_START_ADDRESS | MPU_REGION_VALID | MPU_DEFAULT_ITCM_REGION;
    dw_region_attr = MPU_AP_PRIVILEGED_READ_WRITE | mpu_cal_mpu_region_size(ITCM_END_ADDRESS - ITCM_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);

    // Internal flash memory region --- Normal read-only
    // (update to Strongly ordered in write accesses)
    // START_Addr:-  0x00400000UL
    // END_Addr:-    0x00600000UL

    dw_region_base_addr = IFLASH_START_ADDRESS | MPU_REGION_VALID | MPU_DEFAULT_IFLASH_REGION;
    dw_region_attr = MPU_AP_READONLY | INNER_NORMAL_WB_NWA_TYPE( NON_SHAREABLE ) | mpu_cal_mpu_region_size(IFLASH_END_ADDRESS - IFLASH_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);

    // DTCM memory region --- Normal
    // START_Addr:-  0x20000000L
    // END_Addr:-    0x20400000UL

    dw_region_base_addr = DTCM_START_ADDRESS | MPU_REGION_VALID | MPU_DEFAULT_DTCM_REGION;
    dw_region_attr = MPU_AP_PRIVILEGED_READ_WRITE | mpu_cal_mpu_region_size(DTCM_END_ADDRESS - DTCM_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);

    // SRAM Cacheable memory region --- Normal
    // START_Addr:-  0x20400000UL
    // END_Addr:-    0x2043FFFFUL
 
    dw_region_base_addr = SRAM_FIRST_START_ADDRESS | MPU_REGION_VALID | MPU_DEFAULT_SRAM_REGION_1;
    dw_region_attr = MPU_AP_FULL_ACCESS | INNER_NORMAL_WB_NWA_TYPE( NON_SHAREABLE ) | mpu_cal_mpu_region_size(SRAM_FIRST_END_ADDRESS - SRAM_FIRST_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);


    // Internal SRAM second partition memory region --- Normal
    // START_Addr:-  0x20440000UL
    // END_Addr:-    0x2045FFFFUL

    dw_region_base_addr = SRAM_SECOND_START_ADDRESS | MPU_REGION_VALID | MPU_DEFAULT_SRAM_REGION_2;
    dw_region_attr = MPU_AP_FULL_ACCESS | INNER_NORMAL_WB_NWA_TYPE( NON_SHAREABLE ) | mpu_cal_mpu_region_size(SRAM_SECOND_END_ADDRESS - SRAM_SECOND_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region( dw_region_base_addr, dw_region_attr);

#ifdef MPU_HAS_NOCACHE_REGION
    dw_region_base_addr = SRAM_NOCACHE_START_ADDRESS | MPU_REGION_VALID | MPU_NOCACHE_SRAM_REGION;
    dw_region_attr = MPU_AP_FULL_ACCESS | INNER_OUTER_NORMAL_NOCACHE_TYPE( SHAREABLE ) | mpu_cal_mpu_region_size(NOCACHE_SRAM_REGION_SIZE) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);
#endif

    // Peripheral memory region --- DEVICE Shareable
    // START_Addr:-  0x40000000UL
    // END_Addr:-    0x5FFFFFFFUL

    dw_region_base_addr = PERIPHERALS_START_ADDRESS | MPU_REGION_VALID | MPU_PERIPHERALS_REGION;
    dw_region_attr = MPU_AP_FULL_ACCESS | MPU_REGION_EXECUTE_NEVER | SHAREABLE_DEVICE_TYPE | mpu_cal_mpu_region_size(PERIPHERALS_END_ADDRESS - PERIPHERALS_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);


    // External EBI memory  memory region --- Strongly Ordered
    // START_Addr:-  0x60000000UL
    // END_Addr:-    0x6FFFFFFFUL

    dw_region_base_addr = EXT_EBI_START_ADDRESS | MPU_REGION_VALID | MPU_EXT_EBI_REGION;
    // External memory Must be defined with 'Device' or 'Strongly Ordered' attribute for write accesses (AXI)
    dw_region_attr = MPU_AP_FULL_ACCESS | STRONGLY_ORDERED_SHAREABLE_TYPE | mpu_cal_mpu_region_size(EXT_EBI_END_ADDRESS - EXT_EBI_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);

    // SDRAM cacheable memory region --- Normal
    // START_Addr:-  0x70000000UL
    // END_Addr:-    0x7FFFFFFFUL
 
    dw_region_base_addr = SDRAM_START_ADDRESS | MPU_REGION_VALID | MPU_DEFAULT_SDRAM_REGION;
    dw_region_attr = MPU_AP_FULL_ACCESS | INNER_NORMAL_WB_RWA_TYPE( SHAREABLE ) | mpu_cal_mpu_region_size(SDRAM_END_ADDRESS - SDRAM_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);

    // QSPI memory region --- Strongly ordered
    // START_Addr:-  0x80000000UL
    // END_Addr:-    0x9FFFFFFFUL

    dw_region_base_addr = QSPI_START_ADDRESS | MPU_REGION_VALID | MPU_QSPIMEM_REGION;
    dw_region_attr = MPU_AP_FULL_ACCESS | STRONGLY_ORDERED_SHAREABLE_TYPE | mpu_cal_mpu_region_size(QSPI_END_ADDRESS - QSPI_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);


    // USB RAM Memory region --- Device
    // START_Addr:-  0xA0100000UL
    // END_Addr:-    0xA01FFFFFUL

    dw_region_base_addr = USBHSRAM_START_ADDRESS | MPU_REGION_VALID | MPU_USBHSRAM_REGION;
    dw_region_attr = MPU_AP_FULL_ACCESS | MPU_REGION_EXECUTE_NEVER | SHAREABLE_DEVICE_TYPE | mpu_cal_mpu_region_size(USBHSRAM_END_ADDRESS - USBHSRAM_START_ADDRESS) | MPU_REGION_ENABLE;
    mpu_set_region(dw_region_base_addr, dw_region_attr);


    // Enable the memory management fault , Bus Fault, Usage Fault exception
    SCB->SHCSR |= (SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk);

    // Enable the MPU region
    mpu_enable( MPU_ENABLE | MPU_PRIVDEFENA);

    __DSB();
    __ISB();
}
