// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.10.2025 18:00

#pragma once

#include <sys/pados_types.h>
#include <sys/pados_error_codes.h>
#include <System/TimeValue.h>

namespace kernel
{

time_t          kget_monotonic_time_ns() noexcept;
TimeValNanos    kget_monotonic_time() noexcept;

time_t          kget_system_ticks_hires() noexcept;

time_t          kget_monotonic_time_hires_ns() noexcept;
TimeValNanos    kget_monotonic_time_hires() noexcept;

time_t          kget_real_time_ns() noexcept;
TimeValNanos    kget_real_time() noexcept;

time_t          kget_real_time_hires_ns() noexcept;
TimeValNanos    kget_real_time_hires() noexcept;

PErrorCode      kset_real_time_ns(time_t time, bool updateRTC) noexcept;
PErrorCode      kset_real_time(TimeValNanos time, bool updateRTC) noexcept;

time_t          kget_idle_time_ns() noexcept;
TimeValNanos    kget_idle_time() noexcept;

time_t          kget_clock_time_ns_trw(clockid_t clockID);
PErrorCode      kget_clock_time_ns(clockid_t clockID, time_t& outTime) noexcept;

TimeValNanos    kget_clock_time_trw(int clockID);
PErrorCode      kget_clock_time(int clockID, TimeValNanos& outTime) noexcept;

time_t          kget_clock_time_hires_ns_trw(clockid_t clockID);
PErrorCode      kget_clock_time_hires_ns(clockid_t clockID, time_t& outTime) noexcept;

TimeValNanos    kget_clock_time_hires_trw(int clockID);
PErrorCode      kget_clock_time_hires(int clockID, TimeValNanos& outTime) noexcept;

time_t          kget_clock_time_offset_ns_trw(clockid_t clockID);
PErrorCode      kget_clock_time_offset_ns(clockid_t clockID, time_t& outTime) noexcept;

TimeValNanos    kget_clock_time_offset_trw(clockid_t clockID);
PErrorCode      kget_clock_time_offset(clockid_t clockID, TimeValNanos& outOffset) noexcept;

TimeValNanos    kconvert_clock_to_monotonic_trw(clockid_t clockID, TimeValNanos clockTime);
PErrorCode      kconvert_clock_to_monotonic(clockid_t clockID, TimeValNanos clockTime, TimeValNanos& outMonotonicTime) noexcept;

} // namespace kernel
