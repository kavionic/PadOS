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

#pragma once

#include <sys/pados_types.h>
#include <sys/pados_error_codes.h>
#include <System/TimeValue.h>

namespace kernel
{

time_t          kget_monotonic_time_ns() noexcept;
TimeValNanos    kget_monotonic_time() noexcept;

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
