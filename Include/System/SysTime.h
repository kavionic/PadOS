// This file is part of PadOS.
//
// Copyright (C) 2020-2024 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include <chrono>
#include <System/TimeValue.h>
#include <System/ErrorCodes.h>


TimeValNanos    get_system_time();
TimeValNanos    get_system_time_hires();
TimeValNanos    get_idle_time();

PErrorCode      set_real_time(TimeValNanos time, bool updateRTC);
TimeValNanos    get_real_time();
TimeValNanos    get_real_time_hires();
TimeValNanos    get_clock_time_offset(int clockID);
TimeValNanos    get_clock_time(int clockID);
TimeValNanos    get_clock_time_hires(int clockID);


TimeValNanos    kget_system_time();
TimeValNanos    kget_system_time_hires();
TimeValNanos    kget_idle_time();

PErrorCode      kset_real_time(TimeValNanos time, bool updateRTC);
TimeValNanos    kget_real_time();
TimeValNanos    kget_real_time_hires();
TimeValNanos    kget_clock_time_offset(int clockID);
TimeValNanos    kget_clock_time(int clockID);
TimeValNanos    kget_clock_time_hires(int clockID);


std::chrono::steady_clock::time_point get_monotonic_clock();
std::chrono::steady_clock::time_point get_monotonic_clock_hires();

namespace unit_test
{
void TestTimeValue();
}
