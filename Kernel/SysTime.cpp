// This file is part of PadOS.
//
// Copyright (C) 2020-2022 Kurt Skauen <http://kavionic.com/>
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

#include <time.h>
#include <math.h>

#include <System/SysTime.h>
#include <Threads/Threads.h>
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <Utils/Utils.h>

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValMicros get_system_time()
{
    bigtime_t time;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        time = Kernel::s_SystemTime;
    } CRITICAL_END;
    return TimeValMicros::FromMilliseconds(time);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode set_real_time(TimeValMicros time, bool updateRTC)
{
    Kernel::s_RealTime = time - get_system_time();

    if (updateRTC)
    {
        RealtimeClock::SetClock(time);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValMicros get_real_time()
{
    return get_system_time() + Kernel::s_RealTime;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_real_time_hires()
{
    return get_system_time_hires() + Kernel::s_RealTime;
}

///////////////////////////////////////////////////////////////////////////////
/// Return system time in nano seconds with the resolution of the core clock.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC TimeValNanos get_system_time_hires()
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
    return TimeValNanos::FromNanoseconds(time + bigtime_t(ticks) * TimeValNanos::TicksPerSecond / coreFrequency);  // Convert clock-cycles to nS and add to the time.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValMicros get_clock_time_offset(int clockID)
{
    switch (clockID)
    {
        case CLOCK_REALTIME_COARSE:
        case CLOCK_REALTIME:
        case CLOCK_REALTIME_ALARM:
            return Kernel::s_RealTime;
        case CLOCK_PROCESS_CPUTIME_ID:
            return -get_idle_time();
        case CLOCK_THREAD_CPUTIME_ID:
            return gk_CurrentThread->m_RunTime - get_system_time();
        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_RAW:
        case CLOCK_MONOTONIC_COARSE:
        case CLOCK_BOOTTIME:
        case CLOCK_BOOTTIME_ALARM:
            return TimeValMicros::zero;
    }
    return TimeValMicros::zero;
}


template<typename T>
T get_clock_time_internal(int clockID)
{
    switch(clockID)
    {
        case CLOCK_REALTIME_COARSE:     return get_real_time();
        case CLOCK_REALTIME:            return get_system_time_hires();
        case CLOCK_PROCESS_CPUTIME_ID:  return get_system_time_hires() - get_idle_time();
        case CLOCK_THREAD_CPUTIME_ID:   return gk_CurrentThread->m_RunTime;
        case CLOCK_MONOTONIC:           return get_system_time_hires();
        case CLOCK_MONOTONIC_RAW:       return get_system_time_hires();
        case CLOCK_MONOTONIC_COARSE:    return get_system_time();
        case CLOCK_BOOTTIME:            return get_system_time_hires();
        case CLOCK_REALTIME_ALARM:      return get_system_time_hires();
        case CLOCK_BOOTTIME_ALARM:      return get_system_time_hires();
    }
    return T::zero;
}

TimeValMicros get_clock_time(int clockID)
{
    return get_system_time() + get_clock_time_offset(clockID);
//    return get_clock_time_internal<TimeValMicros>(clockID);
}

TimeValNanos get_clock_time_hires(int clockID)
{
    return get_system_time_hires() + get_clock_time_offset(clockID);
//    return get_clock_time_internal<TimeValNanos>(clockID);
}

std::chrono::steady_clock::time_point get_monotonic_clock()
{
    bigtime_t time;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        time = Kernel::s_SystemTime;
    } CRITICAL_END;
    return std::chrono::steady_clock::time_point(std::chrono::milliseconds(time));
}

std::chrono::steady_clock::time_point get_monotonic_clock_hires()
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
//    time *= 1000000; // Convert system time from mS to nS.
    return std::chrono::steady_clock::time_point(std::chrono::milliseconds(time) + std::chrono::nanoseconds(bigtime_t(ticks) * TimeValNanos::TicksPerSecond / coreFrequency));
//    return TimeValNanos::FromNanoseconds(time + bigtime_t(ticks) * TimeValNanos::TicksPerSecond / coreFrequency);  // Convert clock-cycles to nS and add to the time.
}

std::chrono::system_clock::time_point get_realtime_clock()
{
    return std::chrono::system_clock::time_point(get_monotonic_clock().time_since_epoch() + std::chrono::microseconds(Kernel::s_RealTime.AsMicroSeconds()));
}

std::chrono::system_clock::time_point get_realtime_clock_hires()
{
    return std::chrono::system_clock::time_point(get_monotonic_clock_hires().time_since_epoch() + std::chrono::microseconds(Kernel::s_RealTime.AsMicroSeconds()));
}

///////////////////////////////////////////////////////////////////////////////
/// Return time in nano seconds where no threads or IRQ's have been executing.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC TimeValNanos get_idle_time()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    return gk_IdleThread->m_RunTime;
}

///////////////////////////////////////////////////////////////////////////////
/// Return number of core clock cycles since last wrap. Wraps every ms.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC uint64_t get_core_clock_cycles()
{
    uint32_t timerLoadVal = SysTick->LOAD;

    CRITICAL_SCOPE(CRITICAL_IRQ);
    uint32_t ticks = SysTick->VAL;
    return timerLoadVal - ticks;
}



namespace unit_test
{

IFLASHC void TestTimeValue()
{
    TimeValMillis millis = TimeValMillis::FromNative(1000);
    TimeValMicros micros = TimeValMicros::FromNative(1000000);
    TimeValNanos  nanos  = TimeValNanos::FromNative(1000000000);

    _EXPECT_TRUE(millis.AsNative()  == 1000);
    _EXPECT_TRUE(micros.AsNative()  == 1000000);
    _EXPECT_TRUE(nanos.AsNative()  == 1000000000);
    _EXPECT_TRUE(micros == nanos);
    _EXPECT_TRUE(!(micros != nanos));

    _EXPECT_TRUE(micros == millis);
    _EXPECT_TRUE(!(micros != millis));

    TimeValMicros sum1 = micros + nanos;
    TimeValMicros sum2 = nanos + micros;
    TimeValNanos sum3 = micros + nanos;
    TimeValNanos sum4 = nanos + micros;
    TimeValNanos sum5 = micros + millis;
    TimeValNanos sum6 = nanos + millis;

    _EXPECT_TRUE(sum1 == sum2);
    _EXPECT_TRUE(sum1 == sum3);
    _EXPECT_TRUE(sum1 == sum4);
    _EXPECT_TRUE(sum1 == sum5);
    _EXPECT_TRUE(sum1 == sum6);
    _EXPECT_TRUE(sum2 == sum1);
    _EXPECT_TRUE(sum2 == sum3);
    _EXPECT_TRUE(sum2 == sum4);
    _EXPECT_TRUE(sum2 == sum5);
    _EXPECT_TRUE(sum2 == sum6);
    _EXPECT_TRUE(sum3 == sum1);
    _EXPECT_TRUE(sum3 == sum2);
    _EXPECT_TRUE(sum3 == sum4);
    _EXPECT_TRUE(sum3 == sum5);
    _EXPECT_TRUE(sum3 == sum6);
    _EXPECT_TRUE(sum4 == sum1);
    _EXPECT_TRUE(sum4 == sum2);
    _EXPECT_TRUE(sum4 == sum3);
    _EXPECT_TRUE(sum4 == sum5);
    _EXPECT_TRUE(sum4 == sum6);


    _EXPECT_TRUE(sum1 > millis);
    _EXPECT_TRUE(sum3 > millis);
    _EXPECT_TRUE(sum1 > micros);
    _EXPECT_TRUE(sum3 > micros);
    _EXPECT_TRUE(sum1 > nanos);
    _EXPECT_TRUE(sum3 > nanos);

    _EXPECT_TRUE(sum1 >= millis);
    _EXPECT_TRUE(sum3 >= millis);
    _EXPECT_TRUE(sum1 >= micros);
    _EXPECT_TRUE(sum3 >= micros);
    _EXPECT_TRUE(sum1 >= nanos);
    _EXPECT_TRUE(sum3 >= nanos);

    _EXPECT_TRUE(millis < sum1);
    _EXPECT_TRUE(millis < sum3);
    _EXPECT_TRUE(micros < sum1);
    _EXPECT_TRUE(micros < sum3);
    _EXPECT_TRUE(nanos  < sum1);
    _EXPECT_TRUE(nanos  < sum3);
    _EXPECT_TRUE(millis <= sum1);
    _EXPECT_TRUE(millis <= sum3);
    _EXPECT_TRUE(micros <= sum1);
    _EXPECT_TRUE(micros <= sum3);
    _EXPECT_TRUE(nanos  <= sum1);
    _EXPECT_TRUE(nanos  <= sum3);

    _EXPECT_TRUE(nanos <= micros);
    _EXPECT_TRUE(nanos >= micros);
    _EXPECT_TRUE(micros <= nanos);
    _EXPECT_TRUE(micros >= nanos);
    _EXPECT_TRUE(micros <= millis);
    _EXPECT_TRUE(micros >= millis);

    _EXPECT_NEAR(sum1.AsSeconds(), 2.0, 0.0000001);
    _EXPECT_NEAR(sum3.AsSeconds(), 2.0, 0.0000001);

    _EXPECT_TRUE(millis.AsMilliSeconds() == 1000LL);
    _EXPECT_TRUE(millis.AsMicroSeconds() == 1000000LL);
    _EXPECT_TRUE(millis.AsNanoSeconds() == 1000000000LL);

    _EXPECT_TRUE(micros.AsMilliSeconds() == 1000LL);
    _EXPECT_TRUE(micros.AsMicroSeconds() == 1000000LL);
    _EXPECT_TRUE(micros.AsNanoSeconds() == 1000000000LL);

    _EXPECT_TRUE(nanos.AsMilliSeconds() == 1000LL);
    _EXPECT_TRUE(nanos.AsMicroSeconds() == 1000000LL);
    _EXPECT_TRUE(nanos.AsNanoSeconds() == 1000000000LL);

    millis += 1.0;
    micros += 1.0;
    nanos += 1.0;

    _EXPECT_TRUE(millis.AsMilliSeconds() == 2000LL);
    _EXPECT_TRUE(millis.AsMicroSeconds() == 2000000LL);
    _EXPECT_TRUE(millis.AsNanoSeconds() == 2000000000LL);

    _EXPECT_TRUE(micros.AsMilliSeconds() == 2000LL);
    _EXPECT_TRUE(micros.AsMicroSeconds() == 2000000LL);
    _EXPECT_TRUE(micros.AsNanoSeconds() == 2000000000LL);

    _EXPECT_TRUE(nanos.AsMilliSeconds() == 2000LL);
    _EXPECT_TRUE(nanos.AsMicroSeconds() == 2000000LL);
    _EXPECT_TRUE(nanos.AsNanoSeconds() == 2000000000LL);

    _EXPECT_TRUE(TimeValMillis::FromSeconds(1LL).AsNative() == 1000);
    _EXPECT_TRUE(TimeValMillis::FromMilliseconds(1000LL).AsNative() == 1000);
    _EXPECT_TRUE(TimeValMillis::FromMicroseconds(1000000LL).AsNative() == 1000);
    _EXPECT_TRUE(TimeValMillis::FromNanoseconds(1000000000LL).AsNative() == 1000);

    _EXPECT_TRUE(TimeValMicros::FromSeconds(1LL).AsNative() == 1000000);
    _EXPECT_TRUE(TimeValMicros::FromMilliseconds(1000LL).AsNative() == 1000000);
    _EXPECT_TRUE(TimeValMicros::FromMicroseconds(1000000LL).AsNative() == 1000000);
    _EXPECT_TRUE(TimeValMicros::FromNanoseconds(1000000000LL).AsNative() == 1000000);

    _EXPECT_TRUE(TimeValNanos::FromSeconds(1LL).AsNative() == 1000000000);
    _EXPECT_TRUE(TimeValNanos::FromMilliseconds(1000LL).AsNative() == 1000000000);
    _EXPECT_TRUE(TimeValNanos::FromMicroseconds(1000000LL).AsNative() == 1000000000);
    _EXPECT_TRUE(TimeValNanos::FromNanoseconds(1000000000LL).AsNative() == 1000000000);
}

}
