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


#include <System/TimeValue.h>


TimeValMicros   get_system_time();
TimeValNanos    get_system_time_hires();
TimeValNanos    get_idle_time();
uint64_t        get_core_clock_cycles();

void            set_real_time(TimeValMicros time, bool updateRTC);
TimeValMicros   get_real_time();

namespace unit_test
{
void TestTimeValue();
}
