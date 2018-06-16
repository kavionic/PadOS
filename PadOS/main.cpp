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
#include "SDRAM.h"
#include "System/Utils/EventTimer.h"
#include "System/Threads.h"
#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"
#include "Kernel/VFS/KBlockCache.h"
#include "Kernel/Drivers/HSMCIDriver.h"
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
#include "Applications/WindowManager/WindowManager.h"
#include "Kernel/FSDrivers/FAT/FATFilesystem.h"
#include "Kernel/VFS/KFilesystem.h"

extern "C" void InitializeNewLibMutexes();

//kernel::HSMCIDriver g_SDCardBlockDevice(DigitalPin(e_DigitalPortID_A, 24));

ApplicationServer* g_ApplicationServer;


void NonMaskableInt_Handler()
{
    kernel::panic("NMI\n");
}
void HardFault_Handler()
{
    kernel::panic("HardFault\n");
}
void MemoryManagement_Handler()
{
    kernel::panic("MemManage\n");
}
void BusFault_Handler()
{
    kernel::panic("BusFault\n");
}
void UsageFault_Handler()
{
    kernel::panic("UsageFault\n");
}
void DebugMonitor_Handler()
{   
}

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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static Ptr<kernel::UARTDriver> g_UARTDriver = ptr_new<kernel::UARTDriver>();
static void SetupDevices()
{
    g_UARTDriver->Setup("uart/0", kernel::UART::Channels::Channel4_D18D19);
//    kernel::Kernel::RegisterDevice("uart/0", ptr_new<kernel::UARTDriver>(kernel::UART::Channels::Channel4_D18D19));

    int file = open("/dev/uart/0", O_WRONLY);
    
    if (file >= 0)
    {
        if (file != 0) {
            FileIO::Dupe(file, 0);
        }
        if (file != 1) {
            FileIO::Dupe(file, 1);
        }
        if (file != 2) {
            FileIO::Dupe(file, 2);
        }
        if (file != 0 && file != 1 && file != 2) {
            close(file);
        }
        write(0, "Testing testing\n", strlen("Testing testing\n"));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int main(void)
{
    __disable_irq();

    SAME70System::EnablePeripheralClock(ID_PIOA);
    SAME70System::EnablePeripheralClock(ID_PIOB);
    SAME70System::EnablePeripheralClock(ID_PIOC);
    SAME70System::EnablePeripheralClock(ID_PIOD);
    SAME70System::EnablePeripheralClock(ID_PIOE);

    SAME70System::EnablePeripheralClock(SYSTEM_TIMER_PERIPHERALID1);
//    SAME70System::EnablePeripheralClock(SYSTEM_TIMER_PERIPHERALID2);
//    SAME70System::EnablePeripheralClock(ID_TC0_CHANNEL2);

//    SYSTEM_TIMER->TC_BMR = TC_BMR_TC2XC2S_TIOA1_Val;// Clock XC2 is TIOA1

    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_EMR = TC_EMR_NODIVCLK_Msk; // Run at undivided peripheral clock.
    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CMR = TC_CMR_WAVE_Msk | TC_CMR_WAVESEL_UP; // | TC_CMR_ACPA_CLEAR | TC_CMR_ACPC_SET;
    
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RunTests(void* args)
{
    Tests tests;
    exit_thread(0);
}

bool PrintFileContent(const char* format, const char* path)
{
    int testFile = FileIO::Open(path, O_RDONLY);
    if (testFile != -1)
    {
        std::vector<char> buffer;
        buffer.resize(1024);
        
        int length = FileIO::Read(testFile, 0, buffer.data(), buffer.size() - 1);
        if (length >= 0) {
            buffer[length] = 0;
            printf(format, buffer.data());
        } else {
            printf(format, "*ERROR READING*");            
        }
        FileIO::Close(testFile);
        return true;
    }
    else
    {
        printf(format, "*ERROR OPENING*");
        return false;
    }
}

static void TestFilesystem()
{
    int dir = FileIO::Open("/sdcard", O_RDONLY);
    if (dir >= 0)
    {
        printf("Listing SD-card\n");
        kernel::dir_entry entry;
        for (int i = 0; FileIO::ReadDirectory(dir, i, &entry, sizeof(entry)) == 1; ++i) {
            printf("%02d: '%s'\n", i, entry.d_name);            
        }
    }
    int testFile = FileIO::Open("/sdcard/TestTextFile.txt", O_RDONLY);
    if (testFile != -1)
    {
        printf("Succeeded opening file\n");
        char buffer[512];
        int length = FileIO::Read(testFile, 0, buffer, sizeof(buffer) - 1);
        if (length >= 0) {
            buffer[length] = 0;
            printf("Content: '%s'\n", buffer);
        }
        FileIO::Close(testFile);
    }
    
    if (PrintFileContent("Existing write test file: '%s'\n", "/sdcard/WriteTestFile.txt"))
    {
        if (FileIO::Unlink("/sdcard/WriteTestFile.txt") >= 0) {
            printf("Successfully removed the previous write test file.\n");
        } else {            
            printf("ERROR: Failed to remove old write test file.\n");
        }        
    }
    
    testFile = FileIO::Open("/sdcard/WriteTestFile.txt", O_RDWR | O_CREAT);
    if (testFile != -1)
    {
        String buffer("Test write string");
        if (FileIO::Write(testFile, buffer.c_str(), buffer.size()) != buffer.size()) {
            printf("ERROR: Failed to write test file: %s\n", strerror(get_last_error()));
        }
        FileIO::Close(testFile);
    }
    PrintFileContent("Write test content: '%s'\n", "/sdcard/WriteTestFile.txt");
}
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<kernel::I2CDriver>     g_I2CDriver;
Ptr<kernel::FT5x0xDriver>  g_FT5x0xDriver;
Ptr<kernel::INA3221Driver> g_INA3221Driver;
Ptr<kernel::BME280Driver>  g_BME280Driver;
Ptr<kernel::HSMCIDriver>   g_HSMCIDriver;

void kernel::InitThreadMain(void* argument)
{
    SetupDevices();
    
    InitializeNewLibMutexes();

    printf("Initialize display.\n");
    kernel::GfxDriver::Instance.InitDisplay();

    KBlockCache::Initialize();

    g_I2CDriver = ptr_new<kernel::I2CDriver>();
    g_I2CDriver->Setup("i2c/0", kernel::I2CDriverINode::Channels::Channel0);
    g_I2CDriver->Setup("i2c/2", kernel::I2CDriverINode::Channels::Channel2);
    printf("Setup I2C drivers\n");
//    kernel::Kernel::RegisterDevice("i2c/0", ptr_new<kernel::I2CDriver>(kernel::I2CDriver::Channels::Channel0));
//    kernel::Kernel::RegisterDevice("i2c/2", ptr_new<kernel::I2CDriver>(kernel::I2CDriver::Channels::Channel2));

//    kernel::Kernel::RegisterDevice("ft5x0x/0", ptr_new<kernel::FT5x0xDriver>(LCD_TP_WAKE_Pin, LCD_TP_RESET_Pin, LCD_TP_INT_Pin, "/dev/i2c/0"));
    g_FT5x0xDriver = ptr_new<kernel::FT5x0xDriver>();
    g_FT5x0xDriver->Setup("ft5x0x/0", LCD_TP_WAKE_Pin, LCD_TP_RESET_Pin, LCD_TP_INT_Pin, "/dev/i2c/0");
    
//    kernel::Kernel::RegisterDevice("ina3221/0", ptr_new<kernel::INA3221Driver>("/dev/i2c/2"));
    g_INA3221Driver = ptr_new<kernel::INA3221Driver>();
    g_INA3221Driver->Setup("ina3221/0", "/dev/i2c/2");
    
//    kernel::Kernel::RegisterDevice("bme280/0", ptr_new<kernel::BME280Driver>("/dev/i2c/2"));
    g_BME280Driver = ptr_new<kernel::BME280Driver>();
    g_BME280Driver->Setup("bme280/0", "/dev/i2c/2");
    
//    kernel::Kernel::RegisterDevice("sdcard/0", ptr_new<kernel::HSMCIDriver>(DigitalPin(e_DigitalPortID_A, 24)));
    g_HSMCIDriver = ptr_new<kernel::HSMCIDriver>(DigitalPin(e_DigitalPortID_A, 24));
    g_HSMCIDriver->Setup("sdcard/");
    
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
//    spawn_thread("Tester", RunTests, 0, args);

//    printf("Start I2S clock output\n");
    SAME70System::EnablePeripheralClock(ID_PMC);
/*    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD; // | PMC_WPMR_WPEN_Msk; // Remove register write protection.
    PMC->PMC_PCK[1] = PMC_PCK_CSS_MAIN_CLK | PMC_PCK_PRES(0);
    PMC->PMC_SCER = PMC_SCER_PCK1_Msk;
    PMC->PMC_WPMR = PMC_WPMR_WPKEY_PASSWD | PMC_WPMR_WPEN_Msk; // Write protect registers.
    DigitalPort::SetPeripheralMux(e_DigitalPortID_A, PIN21_bm, DigitalPort::PeripheralID::B); // PCK1
    */
    snooze(bigtime_from_ms(1000));
#if 0
    std::vector<uint8_t> buffer1;
    std::vector<uint8_t> buffer2;
    
    for (int i = 0; i < 512 / 2; ++i) {
        buffer1.push_back((i+42) << 8);
        buffer1.push_back((i+42));
    }
    buffer2.resize(buffer1.size());
    
    int dev = FileIO::Open("/dev/sdcard/0", O_RDWR);
    if (dev >= 0)
    {
        off64_t pos = 1024LL*1024LL*1024LL*16LL;
        if (FileIO::Write(dev, pos, buffer1.data(), buffer1.size()) == buffer1.size())
        {
            if (FileIO::Read(dev, pos, buffer2.data(), buffer2.size()) == buffer2.size())
            {
                for (size_t i = 0; i < buffer1.size(); ++i)
                {
                    if (buffer1[i] != buffer2[i]) {
                        kprintf("ERROR: Failed to write block %" PRIu64 ". Mismatch at %d\n", pos, i);
                        break;
                    }
                }
            }            
        }
    }
#else
    printf("Initializeing the FAT filesystem driver\n");
    FileIO::RegisterFilesystem("fat", ptr_new<FATFilesystem>());

    printf("Mounting SD-card\n");
    
    FileIO::CreateDirectory("/sdcard", 0777);
    FileIO::Mount("/dev/sdcard/0", "/sdcard", "fat", 0, nullptr, 0);

    TestFilesystem();    
#endif


    printf("Start Application Server.\n");

    g_ApplicationServer = new ApplicationServer();
    g_ApplicationServer->Start("appserver");
    
    WindowManager* windowManager = new WindowManager();
    windowManager->Start("window_manager");

    Application* testApp = new TestApp();
    
    testApp->Start("test_app");

    exit_thread(0);
}
