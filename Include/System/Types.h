// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 07.03.2018 22:45:09

#pragma once

#include <stddef.h>
#include <inttypes.h>

typedef int      handle_id;
typedef int      thread_id;
typedef int      sem_id;
typedef int      port_id;
typedef int      handler_id;
typedef int      tls_id;
typedef int      fs_id;
typedef int64_t  bigtime_t;
typedef int      status_t;
typedef int64_t  off64_t;
typedef uint64_t size64_t;
typedef int64_t  ssize64_t;

typedef uint16_t wchar16_t;

constexpr handler_id	INVALID_HANDLE = -1;
constexpr size_t		INVALID_INDEX = size_t(-1);
