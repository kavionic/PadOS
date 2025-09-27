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

#include <sys/pados_syscalls.h>
#include <System/SysTime.h>
#include <Kernel/Kernel.h>

using namespace os;
using namespace kernel;

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_system_time()
{
    return get_system_time().AsMicroSeconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_system_time_hires()
{
    return get_system_time_hires().AsNanoSeconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_real_time()
{
    return get_real_time().AsMicroSeconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t sys_set_real_time(bigtime_t time, bool updateRTC)
{
    return std::to_underlying(set_real_time(TimeValMicros::FromMicroseconds(time), updateRTC));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_clock_time_offset(clockid_t clockID)
{
    return get_clock_time_offset(clockID).AsMicroSeconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_clock_time(clockid_t clockID)
{
    return get_clock_time(clockID).AsMicroSeconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t sys_get_clock_time_hires(clockid_t clockID)
{
    return get_clock_time_hires(clockID).AsNanoSeconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t sys_get_clock_resolution(clockid_t clockID, bigtime_t* outResolutionNanos)
{
    if (clockID == CLOCK_MONOTONIC_COARSE || clockID == CLOCK_REALTIME_COARSE)
    {
        *outResolutionNanos = TimeValNanos::FromMilliseconds(1).AsNanoSeconds();
        return 0;
    }
    else
    {
        const uint32_t coreFrequency = Kernel::GetFrequencyCore();
        *outResolutionNanos = TimeValNanos::FromNanoseconds((TimeValNanos::TicksPerSecond + coreFrequency - 1) / coreFrequency).AsNanoSeconds();
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
