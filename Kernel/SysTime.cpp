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

#include <math.h>
#include "System/SysTime.h"
#include "Threads/Threads.h"
#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"

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

TimeValMicros get_real_time()
{
    return get_system_time() + TimeValMicros::FromSeconds(1000000000LL);
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
    return TimeValNanos::FromNanoseconds(time + bigtime_t(ticks) * TimeValNanos::TicksPerSecond / coreFrequency);  // Convert clock-cycles to nS and add to the time.
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

namespace unit_test
{
#define EXPECT_TRUE(expr) if (!(expr)) { printf("TEST FAILED: %s\n", #expr);}
#define EXPECT_NEAR(expr1, expr2, abs_error) if (fabs((expr1) - (expr2)) > abs_error) { printf("TEST FAILED: %s != %s\n", #expr1, #expr2); }

void TestTimeValue()
{
    TimeValMillis millis = TimeValMillis::FromNative(1000);
    TimeValMicros micros = TimeValMicros::FromNative(1000000);
    TimeValNanos  nanos  = TimeValNanos::FromNative(1000000000);

    EXPECT_TRUE(millis.AsNative()  == 1000);
    EXPECT_TRUE(micros.AsNative()  == 1000000);
    EXPECT_TRUE(nanos.AsNative()  == 1000000000);
    EXPECT_TRUE(micros == nanos);
    EXPECT_TRUE(!(micros != nanos));

    EXPECT_TRUE(micros == millis);
    EXPECT_TRUE(!(micros != millis));

    TimeValMicros sum1 = micros + nanos;
    TimeValMicros sum2 = nanos + micros;
    TimeValNanos sum3 = micros + nanos;
    TimeValNanos sum4 = nanos + micros;
    TimeValNanos sum5 = micros + millis;
    TimeValNanos sum6 = nanos + millis;

    EXPECT_TRUE(sum1 == sum2);
    EXPECT_TRUE(sum1 == sum3);
    EXPECT_TRUE(sum1 == sum4);
    EXPECT_TRUE(sum1 == sum5);
    EXPECT_TRUE(sum1 == sum6);
    EXPECT_TRUE(sum2 == sum1);
    EXPECT_TRUE(sum2 == sum3);
    EXPECT_TRUE(sum2 == sum4);
    EXPECT_TRUE(sum2 == sum5);
    EXPECT_TRUE(sum2 == sum6);
    EXPECT_TRUE(sum3 == sum1);
    EXPECT_TRUE(sum3 == sum2);
    EXPECT_TRUE(sum3 == sum4);
    EXPECT_TRUE(sum3 == sum5);
    EXPECT_TRUE(sum3 == sum6);
    EXPECT_TRUE(sum4 == sum1);
    EXPECT_TRUE(sum4 == sum2);
    EXPECT_TRUE(sum4 == sum3);
    EXPECT_TRUE(sum4 == sum5);
    EXPECT_TRUE(sum4 == sum6);


    EXPECT_TRUE(sum1 > millis);
    EXPECT_TRUE(sum3 > millis);
    EXPECT_TRUE(sum1 > micros);
    EXPECT_TRUE(sum3 > micros);
    EXPECT_TRUE(sum1 > nanos);
    EXPECT_TRUE(sum3 > nanos);

    EXPECT_TRUE(sum1 >= millis);
    EXPECT_TRUE(sum3 >= millis);
    EXPECT_TRUE(sum1 >= micros);
    EXPECT_TRUE(sum3 >= micros);
    EXPECT_TRUE(sum1 >= nanos);
    EXPECT_TRUE(sum3 >= nanos);

    EXPECT_TRUE(millis < sum1);
    EXPECT_TRUE(millis < sum3);
    EXPECT_TRUE(micros < sum1);
    EXPECT_TRUE(micros < sum3);
    EXPECT_TRUE(nanos  < sum1);
    EXPECT_TRUE(nanos  < sum3);
    EXPECT_TRUE(millis <= sum1);
    EXPECT_TRUE(millis <= sum3);
    EXPECT_TRUE(micros <= sum1);
    EXPECT_TRUE(micros <= sum3);
    EXPECT_TRUE(nanos  <= sum1);
    EXPECT_TRUE(nanos  <= sum3);

    EXPECT_TRUE(nanos <= micros);
    EXPECT_TRUE(nanos >= micros);
    EXPECT_TRUE(micros <= nanos);
    EXPECT_TRUE(micros >= nanos);
    EXPECT_TRUE(micros <= millis);
    EXPECT_TRUE(micros >= millis);

    EXPECT_NEAR(sum1.AsSeconds(), 2.0, 0.0000001);
    EXPECT_NEAR(sum3.AsSeconds(), 2.0, 0.0000001);

    EXPECT_TRUE(millis.AsMilliSeconds() == 1000LL);
    EXPECT_TRUE(millis.AsMicroSeconds() == 1000000LL);
    EXPECT_TRUE(millis.AsNanoSeconds() == 1000000000LL);

    EXPECT_TRUE(micros.AsMilliSeconds() == 1000LL);
    EXPECT_TRUE(micros.AsMicroSeconds() == 1000000LL);
    EXPECT_TRUE(micros.AsNanoSeconds() == 1000000000LL);

    EXPECT_TRUE(nanos.AsMilliSeconds() == 1000LL);
    EXPECT_TRUE(nanos.AsMicroSeconds() == 1000000LL);
    EXPECT_TRUE(nanos.AsNanoSeconds() == 1000000000LL);

    millis += 1.0;
    micros += 1.0;
    nanos += 1.0;

    EXPECT_TRUE(millis.AsMilliSeconds() == 2000LL);
    EXPECT_TRUE(millis.AsMicroSeconds() == 2000000LL);
    EXPECT_TRUE(millis.AsNanoSeconds() == 2000000000LL);

    EXPECT_TRUE(micros.AsMilliSeconds() == 2000LL);
    EXPECT_TRUE(micros.AsMicroSeconds() == 2000000LL);
    EXPECT_TRUE(micros.AsNanoSeconds() == 2000000000LL);

    EXPECT_TRUE(nanos.AsMilliSeconds() == 2000LL);
    EXPECT_TRUE(nanos.AsMicroSeconds() == 2000000LL);
    EXPECT_TRUE(nanos.AsNanoSeconds() == 2000000000LL);

    EXPECT_TRUE(TimeValMillis::FromSeconds(1LL).AsNative() == 1000);
    EXPECT_TRUE(TimeValMillis::FromMilliseconds(1000LL).AsNative() == 1000);
    EXPECT_TRUE(TimeValMillis::FromMicroseconds(1000000LL).AsNative() == 1000);
    EXPECT_TRUE(TimeValMillis::FromNanoseconds(1000000000LL).AsNative() == 1000);

    EXPECT_TRUE(TimeValMicros::FromSeconds(1LL).AsNative() == 1000000);
    EXPECT_TRUE(TimeValMicros::FromMilliseconds(1000LL).AsNative() == 1000000);
    EXPECT_TRUE(TimeValMicros::FromMicroseconds(1000000LL).AsNative() == 1000000);
    EXPECT_TRUE(TimeValMicros::FromNanoseconds(1000000000LL).AsNative() == 1000000);

    EXPECT_TRUE(TimeValNanos::FromSeconds(1LL).AsNative() == 1000000000);
    EXPECT_TRUE(TimeValNanos::FromMilliseconds(1000LL).AsNative() == 1000000000);
    EXPECT_TRUE(TimeValNanos::FromMicroseconds(1000000LL).AsNative() == 1000000000);
    EXPECT_TRUE(TimeValNanos::FromNanoseconds(1000000000LL).AsNative() == 1000000000);
}

}
