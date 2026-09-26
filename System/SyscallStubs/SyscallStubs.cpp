// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.03.2026 16:00



#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <sys/stat.h>
#include <type_traits>
#include <sys/pados_syscalls.h>
#include <System/SyscallEpilogues.h>
#include <Threads/Thread.h>


#ifdef PADOS_MODULE_USER_SPACE

#define PEXPAND_SYSCALL(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  extern "C" __attribute__((naked, no_instrument_function)) RETTYPE_SYS __##FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  } \
  extern "C" RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { return EPILOGUE<RETTYPE>(__##FPREFIX##FNAME(PNAME_LIST(SIGNATURE))); }

#define PEXPAND_SYSCALL_VOID(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  extern "C" __attribute__((naked, no_instrument_function)) RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  }

#define PEXPAND_SYSCALL_NORET(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  extern "C" __attribute__((naked, noreturn, no_instrument_function)) RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ( \
        "ldr r12, =%0                           \n" \
        "svc 0                                  \n" \
        :: "i"(SYS_##FNAME) : "r12", "memory", "cc");                 \
  }

#include <PadOS/SyscallDefinitions.h>

#undef PEXPAND_SYSCALL
#undef PEXPAND_SYSCALL_VOID
#undef PEXPAND_SYSCALL_NORET

#else // PADOS_MODULE_USER_SPACE

#include <Kernel/Syscalls.h>

template<typename T>
T get_not_implemented_retval()
{
    if constexpr (std::is_same_v<T, PSysRetPair>) {
        return PMakeSysRetFail(PErrorCode::NOSYS);
    } else {
        return T(PErrorCode::NOSYS);
    }
}

#define PEXPAND_SYSCALL(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  __attribute__((naked, no_instrument_function)) RETTYPE_SYS __##FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ("b " __XSTRING(sys_##FNAME)); \
  } \
  extern "C" RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { return EPILOGUE<RETTYPE>(__##FPREFIX##FNAME(PNAME_LIST(SIGNATURE))); } \
  extern "C" __attribute__((naked, no_instrument_function)) RETTYPE_SYS ksys_##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ("b sys_" #FNAME); \
  } \
  extern "C" __attribute__((weak)) RETTYPE_SYS sys_##FNAME(PDECL_LIST(SIGNATURE)) { return get_not_implemented_retval<RETTYPE_SYS>(); }

#define PEXPAND_SYSCALL_VOID(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  __attribute__((naked, no_instrument_function)) RETTYPE FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ("b " __XSTRING(sys_##FNAME)); \
  } \
  extern "C" __attribute__((naked, no_instrument_function)) RETTYPE_SYS ksys_##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ("b sys_" #FNAME); \
  } \
  extern "C" __attribute__((weak)) RETTYPE_SYS sys_##FNAME(PDECL_LIST(SIGNATURE)) {}

#define PEXPAND_SYSCALL_NORET(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) \
  __attribute__((naked, noreturn, no_instrument_function)) RETTYPE __##FPREFIX##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ("b " __XSTRING(sys_##FNAME)); \
  } \
  extern "C" __attribute__((naked, no_instrument_function)) RETTYPE_SYS ksys_##FNAME(PDECL_LIST(SIGNATURE)) { \
    __asm volatile ("b sys_" #FNAME); \
  }

#include <PadOS/SyscallDefinitions.h>

#undef PEXPAND_SYSCALL
#undef PEXPAND_SYSCALL_VOID
#undef PEXPAND_SYSCALL_NORET

#endif // PADOS_MODULE_USER_SPACE
