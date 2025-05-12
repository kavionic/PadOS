// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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

#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"
#include "Kernel/KPowerManager.h"
#include "Kernel/VFS/KDeviceNode.h"
#include "Kernel/VFS/KRootFilesystem.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Utils/Utils.h"
#include "Kernel/HAL/SAME70System.h"
#include "Kernel/VFS/FileIO.h"
#include "Kernel/HAL/ATSAM/SAME70TimerDefines.h"
#include "Kernel/SpinTimer.h"
#include "System/SysTime.h"

using namespace kernel;
using namespace os;

uint32_t            Kernel::s_FrequencyCore;
uint32_t            Kernel::s_FrequencyPeripheral;
volatile bigtime_t  Kernel::s_SystemTime = 0;
TimeValMicros       Kernel::s_RealTime;

static KMutex												gk_KernelLogMutex("kernel_log", EMutexRecursionMode::RaiseError);
static port_id                                              gk_InputEventPort = INVALID_HANDLE;

static std::map<int, std::pair<KLogSeverity, os::String>>& get_kernel_log_levels_map()
{
    static std::map<int, std::pair<KLogSeverity, os::String>>	kernelLogLevels;
    return kernelLogLevels;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC bool kernel::kernel_log_register_category(uint32_t categoryHash, const char* categoryName, KLogSeverity initialLogLevel)
{
	CRITICAL_SCOPE(gk_KernelLogMutex, gk_CurrentThread != nullptr);
    get_kernel_log_levels_map()[categoryHash] = std::make_pair(initialLogLevel, os::String(categoryName));
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void kernel::kernel_log_set_category_log_level(uint32_t categoryHash, KLogSeverity logLevel)
{
	CRITICAL_SCOPE(gk_KernelLogMutex);
	auto i = get_kernel_log_levels_map().find(categoryHash);
	if (i != get_kernel_log_levels_map().end()) {
        i->second.first = logLevel;
    } else {
		kprintf("ERROR: kernel_log_set_category_log_level() called with unknown categoryHash %08x\n", categoryHash);
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC bool kernel::kernel_log_is_category_active(uint32_t categoryHash, KLogSeverity logLevel)
{
	CRITICAL_SCOPE(gk_KernelLogMutex);
	
	auto i = get_kernel_log_levels_map().find(categoryHash);
    if (i != get_kernel_log_levels_map().end()) {
        return logLevel >= i->second.first;
	} else {
		kprintf("ERROR: kernel_log_is_category_active() called with unknown categoryHash %08x\n", categoryHash);
	}
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void kernel::panic(const char* message)
{
    handle_panic(message);
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
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::Initialize(uint32_t coreFrequency, size_t mainThreadStackSize/*, MCU_Timer16_t* powerSwitchTimerChannel, const DigitalPin& pinPowerSwitch*/)
{
    ResetWatchdog();

	REGISTER_KERNEL_LOG_CATEGORY(LogCatKernel_General,    KLogSeverity::INFO_HIGH_VOL);
	REGISTER_KERNEL_LOG_CATEGORY(LogCatKernel_VFS,        KLogSeverity::INFO_HIGH_VOL);
	REGISTER_KERNEL_LOG_CATEGORY(LogCatKernel_BlockCache, KLogSeverity::INFO_LOW_VOL);
	REGISTER_KERNEL_LOG_CATEGORY(LogCatKernel_Scheduler,  KLogSeverity::INFO_HIGH_VOL);

//    FileIO::Initialze();
//    KPowerManager::GetInstance().Initialize(powerSwitchTimerChannel, pinPowerSwitch);
    kernel::start_scheduler(coreFrequency, mainThreadStackSize);
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

IFLASHC int get_last_error()
{
    return gk_CurrentThread->m_NewLibreent._errno;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void set_last_error(int error)
{
    gk_CurrentThread->m_NewLibreent._errno = error;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC status_t set_input_event_port(port_id port)
{
    gk_InputEventPort = port;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC port_id get_input_event_port()
{
    return gk_InputEventPort;
}
