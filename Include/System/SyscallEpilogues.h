// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 22.03.2026 16:00

#pragma once

#include <sys/pados_error_codes.h>
#include <PadOS/SyscallReturns.h>
#include <Threads/Threads.h>


template<typename T, typename U>
T _SYSEPILOGUE_passthrough(U result) { return (T)result; }

template<typename T>
T _SYSEPILOGUE_errno_errorcode(PErrorCode result) { static_assert(sizeof(T) <= sizeof(PErrorCodeUpdateErrno_impl(result))); return (T)PErrorCodeUpdateErrno_impl(result); }

template<typename T>
T _SYSEPILOGUE_errno_sysretpair(PSysRetPair result) { static_assert(sizeof(T) <= sizeof(PSysRetUpdateErrno_impl(result))); return (T)PSysRetUpdateErrno_impl(result); }

template<typename T>
T _SYSEPILOGUE_cancelpnt(T result)
{
#ifdef PADOS_MODULE_POSIX_SIGNALS
    thread_testcancel();
#endif // PADOS_MODULE_POSIX_SIGNALS
    return result;
}
