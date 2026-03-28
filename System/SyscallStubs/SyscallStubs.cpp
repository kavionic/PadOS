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



#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/pados_syscalls.h>
#include <Threads/Thread.h>

template<typename T>
T _SYSEPILOGUE_passthrough(T result) { return result; }

int _SYSEPILOGUE_errno_errorcode(PErrorCode result) { return PErrorCodeUpdateErrno_impl(result); }
int _SYSEPILOGUE_errno_sysretpair(PSysRetPair result) { return PSysRetUpdateErrno_impl(result); }

template<typename T>
T _SYSEPILOGUE_cancelpnt(T result)
{
    thread_testcancel();
    return result;
}

#define PEXPAND_SYSCALL(RETTYPE, FPREFIX, FNAME, SIGNATURE) \
  __attribute__((naked)) RETTYPE FPREFIX##FNAME SIGNATURE { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  }

#define PEXPAND_SYSCALL2(EPILOGUE, RETTYPE, FPREFIX, FNAME, SIGNATURE) \
  extern "C" __attribute__((naked)) RETTYPE __##FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  } \
  extern "C" RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { return EPILOGUE(__##FPREFIX##FNAME(PNAME_LIST(SIGNATURE))); }

#include <PadOS/SyscallDefinitions.h>

#undef PEXPAND_SYSCALL
#undef PEXPAND_SYSCALL2
