// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 19.07.2020 20:19

#include "System/SysTime.h"
#include "Kernel.h"
#include "Scheduler.h"
#include "Threads/Threads.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t get_system_time()
{
    bigtime_t time;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        time = Kernel::s_SystemTime;
    } CRITICAL_END;
    return time * 1000;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t get_real_time()
{
    return get_system_time() + bigtime_from_s(1000000000);
}

///////////////////////////////////////////////////////////////////////////////
/// Return system time in nano seconds with the resolution of the core clock.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_system_time_hires()
{
    const uint32_t    coreFrequency = Kernel::GetFrequencyCore();
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
    return time + bigtime_t(ticks) * 1000000000LL / coreFrequency;  // Convert clock-cycles to nS and add to the time.
}

///////////////////////////////////////////////////////////////////////////////
/// Return time in nano seconds where no threads or IRQ's have been executing.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_idle_time()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    return gk_IdleThread->m_RunTime;
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
