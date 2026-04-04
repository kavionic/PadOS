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
#include <System/SyscallEpilogues.h>
#include <Threads/Thread.h>



#define PEXPAND_SYSCALL(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  extern "C" __attribute__((naked)) RETTYPE_SYS __##FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  } \
  extern "C" RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { return EPILOGUE<RETTYPE>(__##FPREFIX##FNAME(PNAME_LIST(SIGNATURE))); }

#define PEXPAND_SYSCALL_VOID(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  extern "C" __attribute__((naked)) RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  }

#define PEXPAND_SYSCALL_NORET(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  extern "C" __attribute__((naked, noreturn)) RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  }

#include <PadOS/SyscallDefinitions.h>

#undef PEXPAND_SYSCALL
#undef PEXPAND_SYSCALL_VOID
#undef PEXPAND_SYSCALL_NORET
