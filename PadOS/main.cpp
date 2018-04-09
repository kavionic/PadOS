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
// Created: 21.10.2017 00:18:50

#include "sam.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>

#include <vector>
#include <deque>
#include <algorithm>

#include "component/pio.h"
#include "SystemSetup.h"

#include "System/Signals/Signal.h"
#include "Kernel/HAL/SAME70System.h"
#include "Kernel/Drivers/HSMCIDriver.h"
#include "FAT.h"
#include "SDRAM.h"
#include "System/Utils/EventTimer.h"
#include "System/Threads.h"
#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"
#include "Kernel/Drivers/UARTDriver.h"
#include "Kernel/Drivers/I2CDriver.h"
#include "Kernel/Drivers/FT5x0xDriver.h"
#include "Kernel/Drivers/INA3221Driver.h"
#include "Kernel/Drivers/BME280Driver.h"
#include "DeviceControl/INA3221.h"
#include "DeviceControl/BME280.h"
#include "Tests.h"
#include "ApplicationServer/ApplicationServer.h"
#include "Applications/TestApp/TestApp.h"

extern "C" void InitializeNewLibMutexes();

typedef enum {
    SDRAMC_OK = 0,
    SDRAMC_TIMEOUT = 1,
    SDRAMC_ERROR = 2,
} sdramc_status_t;

/** SDRAM benchmark test size */
#define SDRAMC_TEST_BUFF_SIZE        (30 * 1024)

/** SDRAM benchmark test page number */
#define SDRAMC_TEST_PAGE_NUM        (1024)

/** SDRAMC access test length */
#define SDRAMC_TEST_LENGTH        (1024 * 1024 * 64)

/** SDRAMC access test data in even address */
#define SDRAMC_TEST_EVEN_TAG        (0x55aaaa55)

/** SDRAMC access test data in odd address */
#define SDRAMC_TEST_ODD_TAG        (0xaa5555aa)

bigtime_t g_FrameTime = 0;

kernel::HSMCIDriver g_SDCardBlockDevice(DigitalPin(e_DigitalPortID_A, 24));
kernel::FAT         g_FileSystem;

ApplicationServer* g_ApplicationServer;

/*
E1: 7   PA00: LCD_DATA_B00
E2: 9   PA02: LCD_DATA_B01

E1: 11  PA03: LCD_DATA_B02
E2: 11  PA03: LCD_DATA_B02

E1: 12  PA04: LCD_DATA_B03
E2: 12  PA04: LCD_DATA_B03

E2: 5   PA06: LCD_DATA_B04
E1: 4   PA19: LCD_DATA_B05
E2: 13  PA21: LCD_DATA_B06
E2: 10  PA24: LCD_DATA_B07

E1: 13  PB00: LCD_DATA_B08
E1: 14  PB01: LCD_DATA_B09

E1: 6   PB02: 0 LEDA
E1: 5   PB03: 
E2: 14  PB04

E2: 4   PC13: 2 CS
E1: 10  PC17: 3 RD
E2: 7   PC19: 1 RESET
E1: 8   PC30: 4 WR
E1: 3   PC31: 5 RS

E2: 6   PD11: LCD_DATA_B10

E1: 17  PD20: MISO
E2: 17  PD20: MISO

E1: 16  PD21: MOSI
E2: 16  PD21: MOSI

E1: 18  PD22: SPCK
E2: 18  PD22: SPCK

E1: 15  PD25: LCD_DATA_B11
E2: 8   PD26: LCD_DATA_B12
E2: 15  PD27: LCD_DATA_B13
E1: 9   PD28: LCD_DATA_B14
E2: 3   PD30: LCD_DATA_B15


E1: 6   PB02: 0 LEDA
E1: 3   PC31: 1 RESET
E1: 8   PC30: 2 CS
E1: 10  PC17: 3 RD
E2: 4   PC13: 4 WR
E2: 7   PC19: 5 RS

E1: 6   PB02: 0 LEDA
E1: 3   PC31: 5 RS
E1: 8   PC30: 4 WR
E1: 10  PC17: 3 RD
E2: 4   PC13: 2 CS
E2: 7   PC19: 1 RESET

*/




void NonMaskableInt_Handler() {kernel::panic("NMI\n");}
void HardFault_Handler() {kernel::panic("HardFault\n");}
void MemoryManagement_Handler() {kernel::panic("MemManage\n");}
void BusFault_Handler() {kernel::panic("BusFault\n");}
void UsageFault_Handler() {kernel::panic("UsageFault\n");}
void DebugMonitor_Handler() {}



/*!
 * \brief Initialize the SWO trace port for debug message printing
 * \param portBits Port bit mask to be configured
 * \param cpuCoreFreqHz CPU core clock frequency in Hz
 */
#if 0
void SWO_Init(uint32_t portBits, uint32_t cpuCoreFreqHz)
{
  uint32_t SWOSpeed = 2000000; /* default 64k baud rate */
  uint32_t SWOPrescaler = (cpuCoreFreqHz / SWOSpeed) - 1; /* SWOSpeed in Hz, note that cpuCoreFreqHz is expected to be match the CPU core clock */
 
#if 0
  CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; /* enable trace in core debug */
//  *((volatile unsigned *)(ITM_BASE + 0x400F0)) = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
//  *((volatile unsigned *)(ITM_BASE + 0x40010)) = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
//  *((volatile unsigned *)(ITM_BASE + 0x00FB0)) = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */

  TPI->SPPR = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
  TPI->FFCR = 0x100;
  TPI->ACPR = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
  ITM->LAR  = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */


  ITM->TCR = 0x00010000 /*ITM_TCR_TraceBusID_Msk*/ | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_ITMENA_Msk; /* ITM Trace Control Register */
  ITM->TER = portBits; /* ITM Trace Enable Register. Enabled tracing on stimulus ports. One bit per stimulus port. */
  ITM->TPR = ITM_TPR_PRIVMASK_Msk; /* ITM Trace Privilege Register */
  ITM->PORT[0].u32 = 0;

  // *((volatile unsigned *)(ITM_BASE + 0x01000)) = 0x400003FE; /* DWT_CTRL */
  // *((volatile unsigned *)(ITM_BASE + 0x40304)) = 0x00000100; /* Formatter and Flush Control Register */

//  DWT->CTRL = 0x400003FE; /* DWT_CTRL */
//  DWT->CTRL = 0x40000000; /* DWT_CTRL */
//  TPI->FFCR = 0x00000100; /* Formatter and Flush Control Register */
#endif
}

void SWO_PrintChar(char c, uint8_t portNo)
{
  volatile int timeout;
 
  /* Check if Trace Control Register (ITM->TCR at 0xE0000E80) is set */
  if ((ITM->TCR&ITM_TCR_ITMENA_Msk) == 0) { /* check Trace Control Register if ITM trace is enabled*/
    return; /* not enabled? */
  }
  /* Check if the requested channel stimulus port (ITM->TER at 0xE0000E00) is enabled */
  if ((ITM->TER & (1ul<<portNo))==0) { /* check Trace Enable Register if requested port is enabled */
    return; /* requested port not enabled? */
  }
  timeout = 5000; /* arbitrary timeout value */
  while (ITM->PORT[0].u32 == 0) {
    /* Wait until STIMx is ready, then send data */
    timeout--;
    if (timeout==0) {
      return; /* not able to send */
    }
  }
  ITM->PORT[0].u16 = 0x08 | (int(c)<<8);
}
#endif

uint8_t sdram_access_test()
{
    uint32_t i;
    uint32_t *pul = (uint32_t *)SDRAM_CS_ADDR;

    for (i = 0; i < SDRAMC_TEST_LENGTH / 4; ++i) {
        if (i & 1) {
            pul[i] = SDRAMC_TEST_ODD_TAG | (i);
        } else {
            pul[i] = SDRAMC_TEST_EVEN_TAG | (i);
        }
    }

        pul[0] = 0xdeadbabe;
    for (i = 1; i < SDRAMC_TEST_LENGTH / 4; ++i) {
        if (i & 1) {
            if (pul[i] != (SDRAMC_TEST_ODD_TAG | (i))) {
                        printf("MEMTEST failed at 0x%08" PRIx32 "\n", i*4);
                return SDRAMC_ERROR;
            }
        } else {
            if (pul[i] != (SDRAMC_TEST_EVEN_TAG | (i))) {
                        printf("MEMTEST failed at 0x%08" PRIx32 "\n", i*4);
                return SDRAMC_ERROR;
            }
        }
    }

    return SDRAMC_OK;
}

int g_CurrentSensor = -1;
int g_EnvSensor = -1;

void MeasureCurrent()
{
    INA3221Values values;
    if (INA3221_GetMeasurements(g_CurrentSensor, &values) >= 0)
    {
        printf("1: %.3f/%.3f (%.3f), 2: %.3f/%.3f (%.3f), 3: %.3f/%.3f (%.3f)\n", values.Voltages[0], values.Currents[0] * 1000.0, values.Voltages[0] * values.Currents[0] * 1000.0, values.Voltages[1], values.Currents[1] * 1000.0, values.Voltages[1] * values.Currents[1] * 1000.0, values.Voltages[2], values.Currents[2] * 1000.0, values.Voltages[2] * values.Currents[2] * 1000.0);
    }

    BME280Values envValues;
    if (BME280IOCTL_GetValues(g_EnvSensor, &envValues) >= 0) {
        printf("Temp: %.3f, Pres: %.3f, Hum: %.3f\n", envValues.m_Temperature, envValues.m_Pressure / 100.0, envValues.m_Humidity);
    }
}

static void SetupDevices()
{
    kernel::Kernel::RegisterDevice("uart/0", ptr_new<kernel::UARTDriver>(kernel::UART::Channels::Channel4_D18D19));

    int file = open("/dev/uart/0", O_WRONLY);
    
    if (file >= 0)
    {
        if (file != 0) {
            kernel::Kernel::DupeFile(file, 0);
        }
        if (file != 1) {
            kernel::Kernel::DupeFile(file, 1);
        }
        if (file != 2) {
            kernel::Kernel::DupeFile(file, 2);
        }
        if (file != 0 && file != 1 && file != 2) {
            close(file);
        }
        write(0, "Testing testing\n", strlen("Testing testing\n"));
    }
}

void MeasureThread(void* args)
{
//    __enable_irq();

/*    INA3221ShuntConfig shuntConfig;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_1] = 47.0e-3;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_2] = 47.0e-3;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_3] = 47.0e-3 * 0.5;
*/
    g_CurrentSensor = open("/dev/ina3221/0", O_RDWR);
    if (g_CurrentSensor < 0) {
//        INA3221_SetShuntConfig(g_CurrentSensor, shuntConfig);
//    } else {
        printf("ERROR: Failed to open current sensor\n");
    }
    g_EnvSensor = open("/dev/bme280/0", O_RDWR);
    if (g_EnvSensor < 0) {
        printf("ERROR: Failed to open environment sensor\n");
    }


    for (;;)
    {
        MeasureCurrent();
        snooze(bigtime_from_s(10));
//        printf("First task!!!\n");
//        Schedule();
    }
}

int main(void)
{
    __disable_irq();

    SAME70System::EnablePeripheralClock(ID_PIOA);
    SAME70System::EnablePeripheralClock(ID_PIOB);
    SAME70System::EnablePeripheralClock(ID_PIOC);
    SAME70System::EnablePeripheralClock(ID_PIOD);
    SAME70System::EnablePeripheralClock(ID_PIOE);

    SAME70System::EnablePeripheralClock(SYSTEM_TIMER_PERIPHERALID1);
    SAME70System::EnablePeripheralClock(SYSTEM_TIMER_PERIPHERALID2);
    SAME70System::EnablePeripheralClock(ID_TC0_CHANNEL2);

//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK2 | TC_CMR_WAVE_Msk | TC_CMR_WAVESEL_UP_RC;
//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_IER = TC_IER_CPCS_Msk; // IRQ on C compare.
//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_RC = SAME70System::GetFrequencyPeripheral() / 8000; // 18750; // Wrap every ms (150e6/8 = 1.8750.000)
    
//    SYSTEM_TIMER->TC_BMR = TC_BMR_TC2XC2S_TIOA1_Val;// Clock XC2 is TIOA1

    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_EMR = TC_EMR_NODIVCLK_Msk; // Run at undivided peripheral clock.
    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CMR = TC_CMR_WAVE_Msk | TC_CMR_WAVESEL_UP; // | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
    
//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_REALTIME_CHANNEL].TC_CCR = TC_CCR_CLKEN_Msk;
    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CCR = TC_CCR_SWTRG_Msk;
    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CCR = TC_CCR_CLKEN_Msk;
    SYSTEM_TIMER->TC_BCR = TC_BCR_SYNC_Msk;
    
/*    ITM->TCR |= ITM_TCR_ITMENA_Msk;
    ITM->TER |= 0x01;    */
//    SWO_Init(0x1, CLOCK_CPU_FREQUENCY);

    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('\n');
    ITM_SendChar(0);
  
    RGBLED_R.Write(false);
    RGBLED_G.Write(false);
    RGBLED_B.Write(false);
    
    SAME70System::ResetWatchdog();
    kernel::Kernel::Initialize();


    kernel::start_scheduler();
}

void RunTests(void* args)
{
    Tests tests;

    exit_thread(0);
}

void kernel::InitThreadMain(void* argument)
{
    SetupDevices();
    
    printf("InitializeNewLibMutexes()");
    InitializeNewLibMutexes();

    printf("Setup I2C drivers\n");

    kernel::Kernel::RegisterDevice("i2c/0", ptr_new<kernel::I2CDriver>(kernel::I2CDriver::Channels::Channel0));
    kernel::Kernel::RegisterDevice("i2c/2", ptr_new<kernel::I2CDriver>(kernel::I2CDriver::Channels::Channel2));
    kernel::Kernel::RegisterDevice("ft5x0x/0", ptr_new<kernel::FT5x0xDriver>(LCD_TP_WAKE_Pin, LCD_TP_RESET_Pin, LCD_TP_INT_Pin, "/dev/i2c/0"));
    
    kernel::Kernel::RegisterDevice("ina3221/0", ptr_new<kernel::INA3221Driver>("/dev/i2c/2"));
    kernel::Kernel::RegisterDevice("bme280/0", ptr_new<kernel::BME280Driver>("/dev/i2c/2"));

    INA3221ShuntConfig shuntConfig;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_1] = 47.0e-3;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_2] = 47.0e-3;
    shuntConfig.ShuntValues[INA3221_SENSOR_IDX_3] = 47.0e-3 * 0.5;

    int currentSensor = open("/dev/ina3221/0", O_RDWR);
    if (currentSensor >= 0) {
        INA3221_SetShuntConfig(currentSensor, shuntConfig);
        close(currentSensor);
    } else {
        printf("ERROR: Failed to open current sensor for configuration.\n");
    }

//    static int32_t args[] = {1, 2, 3};
//    spawn_thread("SensorDumper", MeasureThread, 0, args);
//    spawn_thread("Tester", RunTests, 0, args);

    printf("Start I2S clock output\n");
    SAME70System::EnablePeripheralClock(ID_PMC);
/*    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD; // | PMC_WPMR_WPEN_Msk; // Remove register write protection.
    PMC->PMC_PCK[1] = PMC_PCK_CSS_MAIN_CLK | PMC_PCK_PRES(0);
    PMC->PMC_SCER = PMC_SCER_PCK1_Msk;
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN_Msk; // Write protect registers.
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN21_bm, DigitalPort::PeripheralID::B); // PCK1
    */
    printf("Initialize display.\n");    

    Display::Instance.InitDisplay();

    g_SDCardBlockDevice.Initialize();;
    g_FileSystem.Initialize(&g_SDCardBlockDevice);

    kernel::Directory* rootDir = g_FileSystem.OpenDir("");
    if (rootDir != nullptr)
    {
        kernel::FileEntry entry;
        while(rootDir->ReadDir(&entry))
        {
            printf("%s : %" PRIu32 "\n", entry.m_Name, entry.m_Size);
        }
    }

    printf("Clear screen.\n");
    Display::Instance.SetFgColor(0xffff);
    Display::Instance.SetWindow(0, 0, 799, 479);
    Display::Instance.FillRect(0, 0, 799, 479);

    printf("Start Application Server.\n");

    g_ApplicationServer = new ApplicationServer();
    g_ApplicationServer->Start("appserver");


    Application* testApp = new TestApp();
    
    testApp->Start("test_app");

    exit_thread(0);
}
