// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 08.03.2018 23:02:13

#pragma once

#include <limits>

#include <sys/types.h>

#include <System/Types.h>
#include <System/ErrorCodes.h>
#include <System/TimeValue.h>

static constexpr TimeValNanos INFINIT_TIMEOUT = TimeValNanos::FromNanoseconds(std::numeric_limits<bigtime_t>::max());

extern "C" void launch_pados(uint32_t coreFrequency, size_t mainThreadStackSize);


int get_last_error();
void set_last_error(int error);
void set_last_error(PErrorCode error);

status_t set_input_event_port(port_id port);
port_id  get_input_event_port();
