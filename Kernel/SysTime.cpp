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
#include <Kernel/Syscalls.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <Utils/Utils.h>

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_system_time()
{
    return TimeValNanos::FromNanoseconds(__get_system_time());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode set_real_time(TimeValNanos time, bool updateRTC)
{
    return __set_real_time(time.AsNanoseconds(), updateRTC);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_real_time()
{
    return TimeValNanos::FromNanoseconds(__get_real_time());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_real_time_hires()
{
    return TimeValNanos::FromNanoseconds(__get_system_time_hires());
}

///////////////////////////////////////////////////////////////////////////////
/// Return system time in nano seconds with the resolution of the core clock.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC TimeValNanos get_system_time_hires()
{
    return TimeValNanos::FromNanoseconds(__get_system_time_hires());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_clock_time_offset(int clockID)
{
    return TimeValNanos::FromNanoseconds(__get_clock_time_offset(clockID));
}

TimeValNanos get_clock_time(int clockID)
{
    return TimeValNanos::FromNanoseconds(__get_clock_time(clockID));
}

TimeValNanos get_clock_time_hires(int clockID)
{
    return TimeValNanos::FromNanoseconds(__get_clock_time_hires(clockID));
}

std::chrono::steady_clock::time_point get_monotonic_clock()
{
    const bigtime_t time = __get_system_time();
    return std::chrono::steady_clock::time_point(std::chrono::nanoseconds(time));
}

std::chrono::steady_clock::time_point get_monotonic_clock_hires()
{
    const bigtime_t time = __get_system_time_hires();
    return std::chrono::steady_clock::time_point(std::chrono::nanoseconds(time));
}

///////////////////////////////////////////////////////////////////////////////
/// Return time in nano seconds where no threads or IRQ's have been executing.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC TimeValNanos get_idle_time()
{
    return TimeValNanos::FromNanoseconds(__get_idle_time());
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_system_time()
{
    return TimeValNanos::FromNanoseconds(sys_get_system_time());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kset_real_time(TimeValNanos time, bool updateRTC)
{
    return sys_set_real_time(time.AsNanoseconds(), updateRTC);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_real_time()
{
    return TimeValNanos::FromNanoseconds(sys_get_real_time());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_real_time_hires()
{
    return TimeValNanos::FromNanoseconds(sys_get_system_time_hires());
}

///////////////////////////////////////////////////////////////////////////////
/// Return system time in nano seconds with the resolution of the core clock.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC TimeValNanos kget_system_time_hires()
{
    return TimeValNanos::FromNanoseconds(sys_get_system_time_hires());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_clock_time_offset(int clockID)
{
    return TimeValNanos::FromNanoseconds(sys_get_clock_time_offset(clockID));
}

TimeValNanos kget_clock_time(int clockID)
{
    return TimeValNanos::FromNanoseconds(sys_get_clock_time(clockID));
}

TimeValNanos kget_clock_time_hires(int clockID)
{
    return TimeValNanos::FromNanoseconds(sys_get_clock_time_hires(clockID));
}

///////////////////////////////////////////////////////////////////////////////
/// Return time in nano seconds where no threads or IRQ's have been executing.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_idle_time()
{
    return TimeValNanos::FromNanoseconds(sys_get_idle_time());
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

    _EXPECT_TRUE(millis.AsMilliseconds() == 1000LL);
    _EXPECT_TRUE(millis.AsMicroseconds() == 1000000LL);
    _EXPECT_TRUE(millis.AsNanoseconds() == 1000000000LL);

    _EXPECT_TRUE(micros.AsMilliseconds() == 1000LL);
    _EXPECT_TRUE(micros.AsMicroseconds() == 1000000LL);
    _EXPECT_TRUE(micros.AsNanoseconds() == 1000000000LL);

    _EXPECT_TRUE(nanos.AsMilliseconds() == 1000LL);
    _EXPECT_TRUE(nanos.AsMicroseconds() == 1000000LL);
    _EXPECT_TRUE(nanos.AsNanoseconds() == 1000000000LL);

    millis += 1.0;
    micros += 1.0;
    nanos += 1.0;

    _EXPECT_TRUE(millis.AsMilliseconds() == 2000LL);
    _EXPECT_TRUE(millis.AsMicroseconds() == 2000000LL);
    _EXPECT_TRUE(millis.AsNanoseconds() == 2000000000LL);

    _EXPECT_TRUE(micros.AsMilliseconds() == 2000LL);
    _EXPECT_TRUE(micros.AsMicroseconds() == 2000000LL);
    _EXPECT_TRUE(micros.AsNanoseconds() == 2000000000LL);

    _EXPECT_TRUE(nanos.AsMilliseconds() == 2000LL);
    _EXPECT_TRUE(nanos.AsMicroseconds() == 2000000LL);
    _EXPECT_TRUE(nanos.AsNanoseconds() == 2000000000LL);

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
