// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include <utility>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KPowerManager.h>
#include <Kernel/VFS/KRootFilesystem.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Utils/Utils.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/SpinTimer.h>
#include <System/TimeValue.h>
#include <Threads/Mutex.h>


extern uint32_t _vectors;

extern "C" void newlib_retarget_locks_initialize();

namespace kernel
{

uint32_t        Kernel::s_FrequencyCore;
uint32_t        Kernel::s_FrequencyPeripheral;
double          Kernel::s_CoreFrequencyToNanosecondScale;
bigtime_t       Kernel::s_SystemTicks = 0;
bigtime_t       Kernel::s_SystemTimeNS = 0;
TimeValNanos    Kernel::s_RealTime;


void handle_panic(const char* message)
{
    //    write(1, message, strlen(message));

    if (kis_debugger_attached())
    {
        __BKPT(0);
    }
    else
    {
        //        NVIC_SystemReset();
        volatile bool freeze = true;
        if (is_in_isr()) {
            while (freeze);
        }
        else {
            while (freeze) snooze(TimeValNanos::FromSeconds(1.0));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void panic(const char* message)
{
    handle_panic(message);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool is_in_isr()
{
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool kis_debugger_attached()
{
    return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kdigital_pin_set_direction(DigitalPinID pinID, DigitalPinDirection_e dir)
{
    DigitalPin(pinID).SetDirection(dir);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kdigital_pin_set_drive_strength(DigitalPinID pinID, DigitalPinDriveStrength_e strength)
{
    DigitalPin(pinID).SetDriveStrength(strength);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kdigital_pin_set_pull_mode(DigitalPinID pinID, PinPullMode_e mode)
{
    DigitalPin(pinID).SetPullMode(mode);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kdigital_pin_set_peripheral_mux(DigitalPinID pinID, DigitalPinPeripheralID peripheral)
{
    DigitalPin(pinID).SetPeripheralMux(peripheral);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kdigital_pin_read(DigitalPinID pinID, bool& outValue)
{
    outValue = DigitalPin(pinID).Read();
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kdigital_pin_write(DigitalPinID pinID, bool value)
{
    DigitalPin(pinID).Write(value);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kwrite_backup_register_trw(size_t registerID, uint32_t value)
{
    RealtimeClock::WriteBackupRegister_trw(registerID, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t kread_backup_register_trw(size_t registerID)
{
    return RealtimeClock::ReadBackupRegister_trw(registerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::SetupGlobals()
{
    SCB->VTOR = ((uintptr_t)&_vectors) & SCB_VTOR_TBLOFF_Msk;

    kernel::KNamedObject::InitializeStatics();
    kernel::initialize_scheduler_statics();

    newlib_retarget_locks_initialize();
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

#if defined(__SAME70Q21__)
void Kernel::ResetWatchdog()
{
    WDT->WDT_CR = WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk;
}
#elif defined(STM32H7) || defined(STM32G0)
void Kernel::ResetWatchdog()
{
    /*IWDG1->KR = 0xaaaa;*/
}
#endif

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
#elif defined(STM32H7) || defined(STM32G0)
#else
#error Unknown platform
#endif

#if defined(STM32H7)
    // FPU:
    __DSB();
    SCB->CPACR |= 0xF << 20; // Full access to CP10 & CP 11
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | // Enable CONTROL.FPCA setting on execution of a floating-point instruction.
        FPU_FPCCR_LSPEN_Msk;  // Enable automatic lazy state preservation for floating-point context.
#endif // defined(STM32H7)
    __DSB();
    __ISB();
#if defined(__SAME70Q21__)
    SAME70System::SetupClock(frequencyCrystal, frequencyCore, frequencyPeripheral);
#elif defined(STM32H7) || defined(STM32G0)
#else
#error Unknown platform
#endif
    s_FrequencyCore = frequencyCore;
    s_FrequencyPeripheral = frequencyPeripheral;

    s_CoreFrequencyToNanosecondScale = 1.0e9 / double(s_FrequencyCore);

    SpinTimer::Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::Initialize(uint32_t coreFrequency, size_t mainThreadStackSize/*, MCU_Timer16_t* powerSwitchTimerChannel, const DigitalPin& pinPowerSwitch*/)
{
    ResetWatchdog();

    //    KPowerManager::GetInstance().Initialize(powerSwitchTimerChannel, pinPowerSwitch);
    start_scheduler(coreFrequency, mainThreadStackSize);
}

} // namespace kernel

extern "C"
{

void launch_pados(uint32_t coreFrequency, size_t mainThreadStackSize)
{
    kernel::Kernel::Initialize(coreFrequency, mainThreadStackSize);
}

} // extern "C"
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int get_last_error()
{
    return errno;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void set_last_error(int error)
{
    errno = error;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void set_last_error(PErrorCode error)
{
    errno = std::to_underlying(error);
}

