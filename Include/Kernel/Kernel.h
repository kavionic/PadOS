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

#pragma once

#include <iostream>
#include <print>
#include <vector>
#include <atomic>
#include <cstdint>

#include "System/Platform.h"

#include <stdio.h>

#include <sys/pados_error_codes.h>

#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include <Utils/String.h>
#include <System/TimeValue.h>
#include <Kernel/HAL/DigitalPort.h>


#define DCACHE_LINE_SIZE __SCB_DCACHE_LINE_SIZE
#define DCACHE_LINE_SIZE_MASK (DCACHE_LINE_SIZE - 1U)

class DigitalPin;

extern "C" size_t get_heap_size();
extern "C" size_t get_max_heap_size();

static constexpr uint32_t PBackupReg_BootMode = 0;
static constexpr uint32_t PBackupReg_BootCount = 1;
static constexpr uint32_t PBackupReg_SYSTEM_COUNT = 2;
namespace kernel
{

class KINode;
class KFileNode;
class KFSVolume;
class KRootFilesystem;

void handle_panic(const char* message); // __attribute__((__noreturn__)); // To be implemented by user code.


static constexpr uint32_t SYS_TICKS_PER_SEC = 1000;


template<typename ...ARGS> int kprintf(const char* fmt, ARGS&&... args) { return printf(fmt, args...); }


void panic(const char* message); // __attribute__((__noreturn__));

template<typename... ARGS>
void panic(PFormatString<ARGS...>&& fmt, ARGS&&... args)
{
    panic(PString::format_string(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...).c_str());
}

bool is_in_isr();
bool kis_debugger_attached();

PErrorCode kdigital_pin_set_direction(DigitalPinID pinID, DigitalPinDirection_e dir);
PErrorCode kdigital_pin_set_drive_strength(DigitalPinID pinID, DigitalPinDriveStrength_e strength);
PErrorCode kdigital_pin_set_pull_mode(DigitalPinID pinID, PinPullMode_e mode);
PErrorCode kdigital_pin_set_peripheral_mux(DigitalPinID pinID, DigitalPinPeripheralID peripheral);
PErrorCode kdigital_pin_read(DigitalPinID pinID, bool& outValue);
PErrorCode kdigital_pin_write(DigitalPinID pinID, bool value);

void     kwrite_backup_register_trw(size_t registerID, uint32_t value);
uint32_t kread_backup_register_trw(size_t registerID);


class Kernel
{
public:
    static void     SetupGlobals();
    static uint32_t GetFrequencyCore();
    static uint32_t GetFrequencyPeripheral();

#if defined(__SAME70Q21__)
    static void ResetWatchdog();
#elif defined(STM32H7) || defined(STM32G0)
    static void ResetWatchdog();
#endif
    static void PreBSSInitialize(uint32_t frequencyCrystal, uint32_t frequencyCore, uint32_t frequencyPeripheral);
    static void Initialize(uint32_t coreFrequency, size_t mainThreadStackSize/*, MCU_Timer16_t* powerSwitchTimerChannel, const DigitalPin& pinPowerSwitch*/);

//private:

    static uint32_t s_FrequencyCore;
    static uint32_t s_FrequencyPeripheral;
    static volatile bigtime_t   s_SystemTime;
    static TimeValNanos         s_RealTime;
};

} // namespace


inline void kassert_function(const char* file, int line, const char* func, const char* expression)
{
    PString message;
    message.format("KASSERT {} / {}:{}: {}", func, file, line, expression);
    kernel::kprintf("%s\n", message.c_str());
    kernel::panic(message.c_str());
}

template<typename... ARGS>
void kassert_function(const char* file, int line, const char* func, const char* expression, PFormatString<ARGS...> fmt, ARGS&&... args)
{
    PString message;
    message.format("KASSERT {} / {}:{}: {} -> ", func, file, line, expression);
    message += PString::format_string(fmt, std::forward<ARGS>(args)...);
    kernel::kprintf("%s\n", message.c_str());
    kernel::panic(message.c_str());
}

#define kassert(expression) if (!(expression)) kassert_function(__FILE__, __LINE__, __func__, #expression)
#define kassert2(expression, ...) if (!(expression)) kassert_function(__FILE__, __LINE__, __func__, #expression __VA_ARGS__)

template<typename ...ARGS>
void kassure(bool expression, const char* fmt, ARGS&&... args)
{
    if (!expression) kernel::kprintf(fmt, args...);
}
