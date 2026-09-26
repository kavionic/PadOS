// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
