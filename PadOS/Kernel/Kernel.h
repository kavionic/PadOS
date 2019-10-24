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

#pragma once

#include "sam.h"

#include <stdio.h>

#include <vector>
#include <atomic>
#include <cstdint>
#include <unistd.h>

#include "System/Ptr/PtrTarget.h"
#include "System/Ptr/Ptr.h"
#include "System/String.h"


#define DCACHE_LINE_SIZE 32 // Cortex-M7 size of cache line is fixed to 8 words (32 bytes)
#define DCACHE_LINE_SIZE_MASK (DCACHE_LINE_SIZE - 1)

class DigitalPin;

namespace kernel
{

class KINode;
class KFileNode;
class KFSVolume;
class KRootFilesystem;

typedef void KIRQHandler(IRQn_Type irq, void* userData);

struct KIRQAction
{

    KIRQHandler* m_Handler;
    void*        m_UserData;
    int32_t      m_Handle;
    KIRQAction*  m_Next;
};

template<typename ...ARGS> int kprintf(const char* fmt, ARGS&&... args) { return printf(fmt, args...); }

enum class KLogCategory
{
    General,
    VFS,
    BlockCache,
    Scheduler,
    COUNT
};

enum class KLogSeverity
{
    INFO_HIGH_VOL,
    INFO_LOW_VOL,
    WARNING,
    CRITICAL,
    ERROR,
    FATAL,
    NONE
};

int  kernel_log_alloc_category(KLogSeverity initialLogLevel);
void kernel_log_set_category_log_level(int category, KLogSeverity logLevel);
bool kernel_log_is_category_active(int category, KLogSeverity logLevel);

template<typename ...ARGS>
void kernel_log(int category, KLogSeverity severity, const char* fmt, ARGS&&... args) { if (kernel_log_is_category_active(category, severity)) kprintf(fmt, args...); }

template<typename ...ARGS>
void kernel_log(KLogCategory category, KLogSeverity severity, const char* fmt, ARGS&&... args) { if (kernel_log_is_category_active(int(category), severity)) kprintf(fmt, args...); }

void panic(const char* message);

template<typename FIRSTARG, typename... ARGS>
void panic(const char* fmt, FIRSTARG&& firstArg, ARGS&&... args)
{
    panic(os::String::format_string(fmt, firstArg, args...).c_str());
}

inline void kassert_function(const char* file, int line, const char* func, const char* expression)
{
    os::String message;
    message.format("KASSERT %s / %s:%d: %s -> ", func, file, line, expression);
    panic(message.c_str());
}

template<typename... ARGS>
void kassert_function(const char* file, int line, const char* func, const char* expression, const char* fmt, ARGS&&... args)
{
    os::String message;
    message.format("KASSERT %s / %s:%d: %s -> ", func, file, line, expression);
    message += os::String::format_string(fmt, args...);
    panic(message.c_str());
}

#define kassert(expression, ...) if (!(expression)) kassert_function(__FILE__, __LINE__, __func__, #expression __VA_ARGS__)

template<typename ...ARGS>
void kassure(bool expression, const char* fmt, ARGS&&... args)
{
    if (!expression) kprintf(fmt, args...);
}



class Kernel
{
public:

    static void PreBSSInitialize(uint32_t frequencyCrystal, uint32_t frequencyCore, uint32_t frequencyPeripheral);
    static void Initialize(uint32_t coreFrequency, TcChannel* spinTimerChannel, TcChannel* powerSwitchTimerChannel, const DigitalPin& pinPowerSwitch);
    static void SystemTick();
    static int RegisterDevice(const char* path, Ptr<KINode> deviceNode);
    static int RenameDevice(int handle, const char* newPath);
    static int RemoveDevice(int handle);

    static int RegisterIRQHandler(IRQn_Type irqNum, KIRQHandler* handler, void* userData);
    static int UnregisterIRQHandler(IRQn_Type irqNum, int handle);
    static void HandleIRQ(IRQn_Type irqNum);

//    static void SetError(int error) { s_LastError = error; }
//    static int  GetLastError() { return s_LastError; }

    static bigtime_t GetTime();


private:

    static volatile bigtime_t   s_SystemTime;
//    static int                  s_LastError;
    static KIRQAction*                   s_IRQHandlers[PERIPH_COUNT_IRQn];
};

} // namespace
