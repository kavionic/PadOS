// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 30.08.2025 15:00

#include <utility>

#include <Kernel/Syscalls.h>
#include <System/SysTime.h>
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>

using namespace os;
using namespace kernel;

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_system_time()
{
    bigtime_t time;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        time = Kernel::s_SystemTime;
    } CRITICAL_END;
    return time * 1000000;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_system_time_hires()
{
    const uint32_t coreFrequency = Kernel::GetFrequencyCore();
    bigtime_t   time;
    uint32_t    ticks;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        ticks = SysTick->VAL;
        time = Kernel::s_SystemTime;
        if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) || SysTick->VAL > ticks)
        {
            // If the SysTick exception is pending, or the timer wrapped around after reading
            // Kernel::s_SystemTime we need to add another tick and re-read the timer.
            ticks = SysTick->VAL;
            time++;
        }
    } CRITICAL_END;
    ticks = SysTick->LOAD - ticks;
    time *= 1000000; // Convert system time from mS to nS.
    return time + bigtime_t(ticks) * TimeValNanos::TicksPerSecond / coreFrequency;  // Convert clock-cycles to nS and add to the time.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_real_time()
{
    return sys_get_system_time() + Kernel::s_RealTime.AsNanoseconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_set_real_time(bigtime_t time, bool updateRTC)
{
    Kernel::s_RealTime = TimeValNanos::FromNanoseconds(time - sys_get_system_time());

    if (updateRTC)
    {
        RealtimeClock::SetClock(TimeValNanos::FromNanoseconds(time));
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_clock_time_offset(clockid_t clockID)
{
    switch (clockID)
    {
        case CLOCK_REALTIME_COARSE:
        case CLOCK_REALTIME:
        case CLOCK_REALTIME_ALARM:
            return Kernel::s_RealTime.AsNanoseconds();
        case CLOCK_PROCESS_CPUTIME_ID:
            return -sys_get_idle_time();
        case CLOCK_THREAD_CPUTIME_ID:
            return gk_CurrentThread->m_RunTime.AsNanoseconds() - sys_get_system_time();
        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_RAW:
        case CLOCK_MONOTONIC_COARSE:
        case CLOCK_BOOTTIME:
        case CLOCK_BOOTTIME_ALARM:
            return 0;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_clock_time(clockid_t clockID)
{
    return sys_get_system_time() + sys_get_clock_time_offset(clockID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_clock_time_hires(clockid_t clockID)
{
    return sys_get_system_time_hires() + sys_get_clock_time_offset(clockID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_idle_time()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    return gk_IdleThread->m_RunTime.AsNanoseconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t sys_get_clock_resolution(clockid_t clockID, bigtime_t* outResolutionNanos)
{
    if (clockID == CLOCK_MONOTONIC_COARSE || clockID == CLOCK_REALTIME_COARSE)
    {
        *outResolutionNanos = TimeValNanos::FromMilliseconds(1).AsNanoseconds();
        return 0;
    }
    else
    {
        const uint32_t coreFrequency = Kernel::GetFrequencyCore();
        *outResolutionNanos = TimeValNanos::FromNanoseconds((TimeValNanos::TicksPerSecond + coreFrequency - 1) / coreFrequency).AsNanoseconds();
        return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t sys_set_clock_resolution(clockid_t clockID, bigtime_t resolutionNanos)
{
    if (clockID != CLOCK_REALTIME)
    {
        return EINVAL;
    }
    return 0;
}

} // extern "C"
