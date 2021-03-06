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

#include <sys/types.h>

#include <limits>

#include "System/Types.h"
#include "SysTime.h"

static constexpr TimeValMicros INFINIT_TIMEOUT = TimeValMicros::FromMicroseconds(std::numeric_limits<bigtime_t>::max());
static constexpr int OS_NAME_LENGTH = 32;


int get_last_error();
void set_last_error(int error);


port_id  create_message_port(const char* name, int maxCount);
port_id  duplicate_message_port(port_id handle);
status_t delete_message_port(port_id handle);
status_t send_message(port_id handle, handler_id targetHandler, int32_t code, const void* data, size_t length, bigtime_t timeout = TimeValMicros::infinit.AsMicroSeconds());
ssize_t  receive_message(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize);
ssize_t  receive_message_timeout(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t timeout);
ssize_t  receive_message_deadline(port_id handle, handler_id* targetHandler, int32_t* code, void* buffer, size_t bufferSize, bigtime_t deadline);

status_t set_input_event_port(port_id port);
port_id  get_input_event_port();
