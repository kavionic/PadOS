// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.02.2018 01:41:28

#include <assert.h>
#include <string.h>

#include "Kernel.h"
#include "Scheduler.h"
#include "KPowerManager.h"
#include "VFS/KDeviceNode.h"
#include "VFS/KRootFilesystem.h"
#include "VFS/KFSVolume.h"
#include "VFS/KFileHandle.h"
#include "System/Utils/Utils.h"
#include "HAL/SAME70System.h"
#include "VFS/FileIO.h"
#include "SAME70TimerDefines.h"
#include "SpinTimer.h"

using namespace kernel;
using namespace os;

uint32_t                      Kernel::s_FrequencyCore;
uint32_t                      Kernel::s_FrequencyPeripheral;
volatile bigtime_t            Kernel::s_SystemTime = 0;
//int                           Kernel::s_LastError = 0;
KIRQAction*                   Kernel::s_IRQHandlers[IRQ_COUNT];

static std::map<int, KLogSeverity> gk_KernelLogLevels;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int  kernel::kernel_log_alloc_category(KLogSeverity initialLogLevel)
{
    static int prevCategoryID = int(KLogCategory::COUNT);
    int category = ++prevCategoryID;
    if (initialLogLevel != KLogSeverity::NONE) {
        gk_KernelLogLevels[category] = initialLogLevel;
    }        
    return category;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::kernel_log_set_category_log_level(int category, KLogSeverity logLevel)
{
    if (logLevel != KLogSeverity::NONE) {
        gk_KernelLogLevels[category] = logLevel;
    } else {
        auto i = gk_KernelLogLevels.find(category);
        if (i != gk_KernelLogLevels.end()) {
            gk_KernelLogLevels.erase(i);
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool kernel::kernel_log_is_category_active(int category, KLogSeverity logLevel)
{
    auto i = gk_KernelLogLevels.find(category);
    if (i != gk_KernelLogLevels.end()) {
        return logLevel >= i->second;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::panic(const char* message)
{
//    RGBLED_R.Write(true);
//    RGBLED_G.Write(false);
//    RGBLED_B.Write(false);

//    write(1, message, strlen(message));
    volatile bool freeze = true;
    while(freeze);
//    RGBLED_R.Write(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t get_system_time()
{
    return Kernel::GetTime();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t get_real_time()
{
    return Kernel::GetTime() + bigtime_from_s(1000000000);
}

///////////////////////////////////////////////////////////////////////////////
/// Return system time in nano seconds with the resolution of the core clock.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t get_system_time_hires()
{
	uint32_t coreFrequency = Kernel::GetFrequencyCore();
    for (;;)
    {
        uint32_t  ticks = SysTick->VAL;
        bigtime_t time = Kernel::GetTime();
        if (SysTick->VAL <= ticks) { // If timer didn't wrap while reading the timer-tick count, convert sys-ticks since last wrap to nS.
        	time *= 1000; // Convert system time from uS to nS.
            return time + bigtime_t(SysTick->LOAD - ticks) * 1000000000LL / coreFrequency;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Return number of core clock cycles since last wrap. Wraps every ms.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint64_t get_core_clock_cycles()
{
	uint32_t timerLoadVal = SysTick->LOAD;

	CRITICAL_SCOPE(CRITICAL_IRQ);
	uint32_t ticks = SysTick->VAL;
	return timerLoadVal - ticks;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::SetupFrequencies(uint32_t frequencyCore, uint32_t frequencyPeripheral)
{
    s_FrequencyCore       = frequencyCore;
    s_FrequencyPeripheral = frequencyPeripheral;
    SpinTimer::Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t Kernel::GetFrequencyCore()
{
    return s_FrequencyCore;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t Kernel::GetFrequencyPeripheral()
{
    return s_FrequencyPeripheral;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::PreBSSInitialize(uint32_t frequencyCrystal, uint32_t frequencyCore, uint32_t frequencyPeripheral)
{
#if defined(__SAME70Q21__)
    SUPC->SUPC_MR = SUPC_MR_KEY_PASSWD | SUPC_MR_BODRSTEN_Msk | SUPC_MR_ONREG_Msk;
    SUPC->SUPC_WUMR = SUPC_WUMR_SMEN_Msk | SUPC_WUMR_WKUPDBC_4096_SLCK;
    SUPC->SUPC_WUIR = SUPC_WUIR_WKUPEN9_Msk;
    
    WDT->WDT_MR = WDT_MR_WDDIS;
#elif defined(STM32H743xx)
#else
#error Unknown platform
#endif

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
#if defined(__SAME70Q21__)
    SAME70System::SetupClock(frequencyCrystal, frequencyCore, frequencyPeripheral);
#elif defined(STM32H743xx)
#else
#error Unknown platform
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::Initialize(uint32_t coreFrequency, MCU_Timer16_t* powerSwitchTimerChannel, const DigitalPin& pinPowerSwitch)
{
    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('a');
    ITM_SendChar('\n');
    ITM_SendChar(0);
  
//    RGBLED_R.Write(false);
//    RGBLED_G.Write(false);
//    RGBLED_B.Write(false);
    
    ResetWatchdog();
    
    gk_KernelLogLevels[int(KLogCategory::General)]    = KLogSeverity::INFO_HIGH_VOL;
    gk_KernelLogLevels[int(KLogCategory::VFS)]        = KLogSeverity::INFO_HIGH_VOL;
    gk_KernelLogLevels[int(KLogCategory::BlockCache)] = KLogSeverity::INFO_HIGH_VOL;
    gk_KernelLogLevels[int(KLogCategory::Scheduler)]  = KLogSeverity::INFO_HIGH_VOL;
//    FileIO::Initialze();
//    KPowerManager::GetInstance().Initialize(powerSwitchTimerChannel, pinPowerSwitch);
    kernel::start_scheduler(coreFrequency);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::SystemTick()
{
    s_SystemTime++;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::RegisterDevice(const char* path, Ptr<KINode> deviceNode)
{
    return FileIO::s_RootFilesystem->RegisterDevice(path, deviceNode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::RenameDevice(int handle, const char* newPath)
{
    return FileIO::s_RootFilesystem->RenameDevice(handle, newPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::RemoveDevice(int handle)
{
    return FileIO::s_RootFilesystem->RemoveDevice(handle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::RegisterIRQHandler(IRQn_Type irqNum, KIRQHandler* handler, void* userData)
{
    if (irqNum < 0 || irqNum >= IRQ_COUNT || handler == nullptr)
    {
        set_last_error(EINVAL);
        return -1;
    }
    KIRQAction* action = new KIRQAction;
    if (action == nullptr) {
        set_last_error(ENOMEM);
        return -1;
    }
    static int currentHandle = 0;
    int handle = ++currentHandle;

    action->m_Handle = handle;
    action->m_Handler = handler;
    action->m_UserData = userData;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        bool needEnabled = s_IRQHandlers[irqNum] == nullptr;
        
        action->m_Next = s_IRQHandlers[irqNum];
        s_IRQHandlers[irqNum] = action;

        if (needEnabled) {
            NVIC_SetPriority(irqNum, KIRQ_PRI_NORMAL_LATENCY2);
            NVIC_EnableIRQ(irqNum);
        }
    } CRITICAL_END;
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::UnregisterIRQHandler(IRQn_Type irqNum, int handle)
{
    if (irqNum < 0 || irqNum >= IRQ_COUNT)
    {
        set_last_error(EINVAL);
        return -1;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        KIRQAction* prev = nullptr;
        for (KIRQAction* action = s_IRQHandlers[irqNum]; action != nullptr; action = action->m_Next)
        {
            if (action->m_Handle == handle)
            {
                if (prev != nullptr) {
                    prev->m_Next = action->m_Next;
                } else {
                    s_IRQHandlers[irqNum] = action->m_Next;
                }
                delete action;
                if (s_IRQHandlers[irqNum] == nullptr) {
                    NVIC_DisableIRQ(irqNum);
                }
                return 0;
            }
        }
    } CRITICAL_END;
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::HandleIRQ(IRQn_Type irqNum)
{
	if (irqNum < IRQ_COUNT)
	{
		for (KIRQAction* action = s_IRQHandlers[irqNum]; action != nullptr; action = action->m_Next) {
			if (action->m_Handler(irqNum, action->m_UserData) == IRQResult::HANDLED) break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t Kernel::GetTime()
{
	bigtime_t time;
	CRITICAL_BEGIN(CRITICAL_IRQ)
	{
		time = s_SystemTime;
    } CRITICAL_END;
	return time * 1000;

/*    for (;;)
    {
        volatile uint32_t* timer = reinterpret_cast<volatile uint32_t*>(&s_SystemTime);

#ifdef LITTLE_ENDIAN
        uint32_t time1LO = timer[0];
        __DMB();
        uint32_t time1HI = timer[1];
        __DMB();
        uint32_t time2LO = timer[0];
        // Make sure we re-fetch s_SystemTime if the previous increment caused
        // a wrapping, and we got interrupted after reading the first word.
        if (time1LO <= time2LO) {
            uint64_t result;
            uint32_t* resultPtr = reinterpret_cast<uint32_t*>(&result);
            resultPtr[0] = time1LO;
            resultPtr[1] = time1HI;
            return result * 1000;
        }
#else // LITTLE_ENDIAN
#error
#endif // LITTLE_ENDIAN
    }*/
}



///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int get_last_error()
{
    return gk_CurrentThread->m_NewLibreent._errno;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void set_last_error(int error)
{
    gk_CurrentThread->m_NewLibreent._errno = error;
}

extern "C" void KernelHandleIRQ(int irq)
{
	Kernel::HandleIRQ(IRQn_Type(irq));
}

/*
void SUPC_Handler         ( void ) { Kernel::HandleIRQ(SUPC_IRQn); }
void RSTC_Handler         ( void ) { Kernel::HandleIRQ(RSTC_IRQn); }
void RTC_Handler          ( void ) { Kernel::HandleIRQ(RTC_IRQn); }
void RTT_Handler          ( void ) { Kernel::HandleIRQ(RTT_IRQn); }
void WDT_Handler          ( void ) { Kernel::HandleIRQ(WDT_IRQn); }
void PMC_Handler          ( void ) { Kernel::HandleIRQ(PMC_IRQn); }
void EFC_Handler          ( void ) { Kernel::HandleIRQ(EFC_IRQn); }
//void UART0_Handler        ( void ) { Kernel::HandleIRQ(UART0_IRQn); }
//void UART1_Handler        ( void ) { Kernel::HandleIRQ(UART1_IRQn); }
void PIOA_Handler         ( void ) { Kernel::HandleIRQ(PIOA_IRQn); }
void PIOB_Handler         ( void ) { Kernel::HandleIRQ(PIOB_IRQn); }
#ifdef PIOC
void PIOC_Handler         ( void ) { Kernel::HandleIRQ(PIOC_IRQn); }
#endif
void USART0_Handler       ( void ) { Kernel::HandleIRQ(USART0_IRQn); }
void USART1_Handler       ( void ) { Kernel::HandleIRQ(USART1_IRQn); }
void USART2_Handler       ( void ) { Kernel::HandleIRQ(USART2_IRQn); }
void PIOD_Handler         ( void ) { Kernel::HandleIRQ(PIOD_IRQn); }
#ifdef PIOE
void PIOE_Handler         ( void ) { Kernel::HandleIRQ(PIOE_IRQn); }
#endif
void HSMCI_Handler        ( void ) { Kernel::HandleIRQ(HSMCI_IRQn); }
void TWIHS0_Handler       ( void ) { Kernel::HandleIRQ(TWIHS0_IRQn); }
void TWIHS1_Handler       ( void ) { Kernel::HandleIRQ(TWIHS1_IRQn); }
void SPI0_Handler         ( void ) { Kernel::HandleIRQ(SPI0_IRQn); }
void SSC_Handler          ( void ) { Kernel::HandleIRQ(SSC_IRQn); }
void TC0_Handler          ( void ) { Kernel::HandleIRQ(TC0_IRQn); }
void TC1_Handler          ( void ) { Kernel::HandleIRQ(TC1_IRQn); }
void TC2_Handler          ( void ) { Kernel::HandleIRQ(TC2_IRQn); }
void TC3_Handler          ( void ) { Kernel::HandleIRQ(TC3_IRQn); }
void TC4_Handler          ( void ) { Kernel::HandleIRQ(TC4_IRQn); }
void TC5_Handler          ( void ) { Kernel::HandleIRQ(TC5_IRQn); }
void AFEC0_Handler        ( void ) { Kernel::HandleIRQ(AFEC0_IRQn); }
void DACC_Handler         ( void ) { Kernel::HandleIRQ(DACC_IRQn); }
void PWM0_Handler         ( void ) { Kernel::HandleIRQ(PWM0_IRQn); }
void ICM_Handler          ( void ) { Kernel::HandleIRQ(ICM_IRQn); }
void ACC_Handler          ( void ) { Kernel::HandleIRQ(ACC_IRQn); }
void USBHS_Handler        ( void ) { Kernel::HandleIRQ(USBHS_IRQn); }
void MCAN0_INT0_Handler        ( void ) { Kernel::HandleIRQ(MCAN0_INT0_IRQn); }
void MCAN0_INT1_Handler        ( void ) { Kernel::HandleIRQ(MCAN0_INT1_IRQn); }
void MCAN1_INT0_Handler        ( void ) { Kernel::HandleIRQ(MCAN1_INT0_IRQn); }
void MCAN1_INT1_Handler        ( void ) { Kernel::HandleIRQ(MCAN1_INT1_IRQn); }
void GMAC_Handler         ( void ) { Kernel::HandleIRQ(GMAC_IRQn); }
void AFEC1_Handler        ( void ) { Kernel::HandleIRQ(AFEC1_IRQn); }
void TWIHS2_Handler       ( void ) { Kernel::HandleIRQ(TWIHS2_IRQn); }
#ifdef SPI1
void SPI1_Handler         ( void ) { Kernel::HandleIRQ(SPI1_IRQn); }
#endif
void QSPI_Handler         ( void ) { Kernel::HandleIRQ(QSPI_IRQn); }
//void UART2_Handler        ( void ) { Kernel::HandleIRQ(UART2_IRQn); }
//void UART3_Handler        ( void ) { Kernel::HandleIRQ(UART3_IRQn); }
//void UART4_Handler        ( void ) { Kernel::HandleIRQ(UART4_IRQn); }
void TC6_Handler          ( void ) { Kernel::HandleIRQ(TC6_IRQn); }
void TC7_Handler          ( void ) { Kernel::HandleIRQ(TC7_IRQn); }
void TC8_Handler          ( void ) { Kernel::HandleIRQ(TC8_IRQn); }
void TC9_Handler          ( void ) { Kernel::HandleIRQ(TC9_IRQn); }
void TC10_Handler         ( void ) { Kernel::HandleIRQ(TC10_IRQn); }
void TC11_Handler         ( void ) { Kernel::HandleIRQ(TC11_IRQn); }
void AES_Handler          ( void ) { Kernel::HandleIRQ(AES_IRQn); }
void TRNG_Handler         ( void ) { Kernel::HandleIRQ(TRNG_IRQn); }
void XDMAC_Handler        ( void ) { Kernel::HandleIRQ(XDMAC_IRQn); }
void ISI_Handler          ( void ) { Kernel::HandleIRQ(ISI_IRQn); }
void PWM1_Handler         ( void ) { Kernel::HandleIRQ(PWM1_IRQn); }
#ifdef SDRAMC
void SDRAMC_Handler       ( void ) { Kernel::HandleIRQ(SDRAMC_IRQn); }
#endif
void RSWDT_Handler        ( void ) { Kernel::HandleIRQ(RSWDT_IRQn); }
*/
