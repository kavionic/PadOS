// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 30.08.2025 15:00

#include <utility>

#include <Kernel/KTime.h>
#include <Kernel/Syscalls.h>
#include <System/TimeValue.h>
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KAddressValidation.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <Kernel/HAL/STM32/ResetAndClockControl.h>


namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


time_t sys_get_monotonic_time_ns()
{
    return kget_monotonic_time_ns();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t sys_get_monotonic_time_hires_ns()
{
    return kget_monotonic_time_hires_ns();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t sys_get_real_time_ns()
{
    return kget_real_time_ns();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t sys_get_real_time_hires_ns()
{
    return kget_real_time_hires_ns();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_set_real_time_ns(bigtime_t time, bool updateRTC)
{
    return kset_real_time(TimeValNanos::FromNanoseconds(time), updateRTC);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_get_clock_time_offset_ns(clockid_t clockID, bigtime_t* outTime)
{
    try
    {
        validate_user_write_pointer_trw(outTime);
        *outTime = kget_clock_time_offset_ns_trw(clockID);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_get_clock_time_ns(clockid_t clockID, bigtime_t* outTime)
{
    try
    {
        validate_user_write_pointer_trw(outTime);
        *outTime = kget_clock_time_ns_trw(clockID);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_get_clock_time_hires_ns(clockid_t clockID, bigtime_t* outTime)
{
    try
    {
        validate_user_write_pointer_trw(outTime);
        *outTime = kget_clock_time_hires_ns_trw(clockID);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t sys_get_idle_time_ns()
{
    return kget_idle_time_ns();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t sys_get_total_irq_time_ns()
{
    return kget_total_irq_time().AsNanoseconds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_get_clock_resolution_ns(clockid_t clockID, bigtime_t* outResolutionNanos)
{
    if (clockID == CLOCK_MONOTONIC_COARSE || clockID == CLOCK_REALTIME_COARSE)
    {
        *outResolutionNanos = TimeValNanos::FromMilliseconds(1).AsNanoseconds();
        return PErrorCode::Success;
    }
    else
    {
        const uint32_t coreFrequency = ResetAndClockControl::GetSysClockFrequency();
        *outResolutionNanos = TimeValNanos::FromNanoseconds((TimeValNanos::TicksPerSecond + coreFrequency - 1) / coreFrequency).AsNanoseconds();
        return PErrorCode::Success;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_set_clock_resolution_ns(clockid_t clockID, bigtime_t resolutionNanos)
{
    if (clockID != CLOCK_REALTIME)
    {
        return PErrorCode::INVAL;
    }
    return PErrorCode::Success;
}

} // extern "C"

} // namespace kernel
