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
// Created: 11.10.2025 18:00

#include <Kernel/KTime.h>
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <System/ExceptionHandling.h>

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_monotonic_time_ns() noexcept
{
    time_t time;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        time = Kernel::s_SystemTime;
    } CRITICAL_END;
    return time * 1000000;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_monotonic_time() noexcept
{
    return TimeValNanos::FromNanoseconds(kget_monotonic_time_ns());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_monotonic_time_hires_ns() noexcept
{
    const uint32_t coreFrequency = Kernel::GetFrequencyCore();
    time_t   time;
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
    return time + time_t(ticks) * TimeValNanos::TicksPerSecond / coreFrequency;  // Convert clock-cycles to nS and add to the time.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_monotonic_time_hires() noexcept
{
    return TimeValNanos::FromNanoseconds(kget_monotonic_time_hires_ns());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_real_time_ns() noexcept
{
    return kget_monotonic_time_ns() + Kernel::s_RealTime.AsNanoseconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_real_time() noexcept
{
    return TimeValNanos::FromNanoseconds(kget_real_time_ns());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_real_time_hires_ns() noexcept
{
    return kget_monotonic_time_hires_ns() + Kernel::s_RealTime.AsNanoseconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_real_time_hires() noexcept
{
    return TimeValNanos::FromNanoseconds(kget_real_time_hires_ns());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kset_real_time_ns(time_t time, bool updateRTC) noexcept
{
    return kset_real_time(TimeValNanos::FromNanoseconds(time), updateRTC);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kset_real_time(TimeValNanos time, bool updateRTC) noexcept
{
    Kernel::s_RealTime = time - kget_monotonic_time();

    if (updateRTC)
    {
        RealtimeClock::SetClock(time);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_idle_time_ns() noexcept
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    return gk_IdleThread->m_RunTime.AsNanoseconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_idle_time() noexcept
{
    return TimeValNanos::FromNanoseconds(kget_idle_time_ns());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_clock_time_ns_trw(clockid_t clockID)
{
    return kget_monotonic_time_ns() + kget_clock_time_offset_ns_trw(clockID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_clock_time_ns(clockid_t clockID, time_t& outTime) noexcept
{
    time_t offset;
    const PErrorCode result = kget_clock_time_offset_ns(clockID, offset);
    if (result != PErrorCode::Success) {
        return result;
    }
    outTime = kget_monotonic_time_ns() + offset;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_clock_time_trw(int clockID)
{
    return TimeValNanos::FromNanoseconds(kget_clock_time_ns_trw(clockID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_clock_time(int clockID, TimeValNanos& outTime) noexcept
{
    time_t time;
    const PErrorCode result = kget_clock_time_ns(clockID, time);
    if (result != PErrorCode::Success) {
        return result;
    }
    outTime = TimeValNanos::FromNanoseconds(time);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_clock_time_hires_ns_trw(clockid_t clockID)
{
    return kget_monotonic_time_hires_ns() + kget_clock_time_offset_ns_trw(clockID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_clock_time_hires_ns(clockid_t clockID, time_t& outTime) noexcept
{
    time_t offset;
    const PErrorCode result = kget_clock_time_offset_ns(clockID, offset);
    if (result != PErrorCode::Success) {
        return result;
    }
    outTime = kget_monotonic_time_hires_ns() + offset;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_clock_time_hires_trw(int clockID)
{
    return TimeValNanos::FromNanoseconds(kget_clock_time_hires_ns_trw(clockID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_clock_time_hires(int clockID, TimeValNanos& outTime) noexcept
{
    time_t time;
    const PErrorCode result = kget_clock_time_hires_ns(clockID, time);
    if (result != PErrorCode::Success) {
        return result;
    }
    outTime = TimeValNanos::FromNanoseconds(time);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t kget_clock_time_offset_ns_trw(clockid_t clockID)
{
    time_t offset;
    const PErrorCode result = kget_clock_time_offset_ns(clockID, offset);
    if (result != PErrorCode::Success) {
        PERROR_THROW_CODE(result);
    }
    return offset;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_clock_time_offset_ns(clockid_t clockID, time_t& outOffset) noexcept
{
    switch (clockID)
    {
        case CLOCK_REALTIME_COARSE:
        case CLOCK_REALTIME:
        case CLOCK_REALTIME_ALARM:
            outOffset = Kernel::s_RealTime.AsNanoseconds();
            return PErrorCode::Success;
        case CLOCK_PROCESS_CPUTIME_ID:
            outOffset = -kget_idle_time_ns();
            return PErrorCode::Success;
        case CLOCK_THREAD_CPUTIME_ID:
            outOffset = (gk_CurrentThread->m_RunTime - kget_monotonic_time()).AsNanoseconds();
            return PErrorCode::Success;
        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_RAW:
        case CLOCK_MONOTONIC_COARSE:
        case CLOCK_BOOTTIME:
        case CLOCK_BOOTTIME_ALARM:
            outOffset = 0;
            return PErrorCode::Success;
    }
    return PErrorCode::InvalidArg;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_clock_time_offset_trw(clockid_t clockID)
{
    return TimeValNanos::FromNanoseconds(kget_clock_time_offset_ns_trw(clockID));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_clock_time_offset(clockid_t clockID, TimeValNanos& outOffset) noexcept
{
    time_t offset;
    const PErrorCode result = kget_clock_time_offset_ns(clockID, offset);
    if (result != PErrorCode::Success) {
        return result;
    }
    outOffset = TimeValNanos::FromNanoseconds(offset);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kconvert_clock_to_monotonic_trw(clockid_t clockID, TimeValNanos clockTime)
{
    if (!clockTime.IsInfinit()) {
        return clockTime - kget_clock_time_offset_trw(clockID);
    } else {
        return TimeValNanos::infinit;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kconvert_clock_to_monotonic(clockid_t clockID, TimeValNanos clockTime, TimeValNanos& outMonotonicTime) noexcept
{
    TimeValNanos offset;

    const PErrorCode result = kget_clock_time_offset(clockID, offset);
    if (result != PErrorCode::Success) {
        return result;
    }
    outMonotonicTime = clockTime - offset;
    return PErrorCode::Success;
}
