// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.09.2025 17:30

#include <stdlib.h>
#include <sys/unistd.h>
#include <string.h>
#include <sys/pados_syscalls.h>
#include <PadOS/SyscallReturns.h>

#include <System/AppDefinition.h>
#include <System/ErrorCodes.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KProcess.h>
#include <Kernel/Syscalls.h>


using namespace os;

extern unsigned char* _sheap;
extern unsigned char* _eheap;

namespace kernel
{

void* process_entry(void* arguments)
{
    const PAppDefinition* app = static_cast<const PAppDefinition*>(__app_thread_data->Ptr1);

    char** argv = static_cast<char**>(__app_thread_data->Ptr2);
    int argc = 0;
    if (argv != nullptr) {
        for (; argv[argc] != nullptr; ++argc);
    }
    app->MainEntry(argc, argv);

    return nullptr;
}

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_spawn_execve(const char* name, int priority, PThreadControlBlock* const tlsBlock, char* const argv[], char* const envp[])
{
    const PAppDefinition* const app = PAppDefinition::FindApplication(name);

    if (app == nullptr) {
        return PErrorCode::NoEntry;
    }

    try
    {
        tlsBlock->Ptr1 = const_cast<void*>(static_cast<const void*>(app));
        tlsBlock->Ptr2 = const_cast<void*>(static_cast<const void*>(argv));

        const PThreadAttribs attrs(name, priority, PThreadDetachState_Detached, app->StackSize);
        kthread_spawn_trw(&attrs, tlsBlock, false, process_entry, tlsBlock);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void sys_exit(int exitCode)
{
    if (kis_debugger_attached())
    {
        __BKPT(0);
    }
    NVIC_SystemReset();
    for (;;);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_getpid(void)
{
    return PMakeSysRetSuccess(gk_CurrentThread->GetProcessID());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_kill(pid_t pid, int sig)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_sysconf(int name, long* outValue)
{
    switch (name)
    {
        case _SC_ARG_MAX:                       return PErrorCode::InvalidArg;
        case _SC_CHILD_MAX:                     return PErrorCode::InvalidArg;
        case _SC_CLK_TCK:
            *outValue = 1000000;
            return PErrorCode::Success;
        case _SC_NGROUPS_MAX:                   return PErrorCode::InvalidArg;
        case _SC_OPEN_MAX:                      return PErrorCode::InvalidArg;
        case _SC_JOB_CONTROL:                   return PErrorCode::InvalidArg;
        case _SC_SAVED_IDS:                     return PErrorCode::InvalidArg;
        case _SC_VERSION:                       return PErrorCode::InvalidArg;
        case _SC_PAGESIZE:
            *outValue = 4096;
            return PErrorCode::Success;
        case _SC_NPROCESSORS_CONF:
            *outValue = 1;
            return PErrorCode::Success;
        case _SC_NPROCESSORS_ONLN:
            *outValue = 1;
            return PErrorCode::Success;
        case _SC_PHYS_PAGES:
            *outValue = get_max_heap_size() / 4096;
            return PErrorCode::Success;
        case _SC_AVPHYS_PAGES:                  return PErrorCode::InvalidArg;
            *outValue = (get_max_heap_size() - get_heap_size()) / 4096;
            return PErrorCode::Success;
        case _SC_MQ_OPEN_MAX:                   return PErrorCode::InvalidArg;
        case _SC_MQ_PRIO_MAX:                   return PErrorCode::InvalidArg;
        case _SC_RTSIG_MAX:                     return PErrorCode::InvalidArg;
        case _SC_SEM_NSEMS_MAX:                 return PErrorCode::InvalidArg;
        case _SC_SEM_VALUE_MAX:                 return PErrorCode::InvalidArg;
        case _SC_SIGQUEUE_MAX:                  return PErrorCode::InvalidArg;
        case _SC_TIMER_MAX:                     return PErrorCode::InvalidArg;
        case _SC_TZNAME_MAX:                    return PErrorCode::InvalidArg;
        case _SC_ASYNCHRONOUS_IO:               return PErrorCode::InvalidArg;
        case _SC_FSYNC:                         return PErrorCode::InvalidArg;
        case _SC_MAPPED_FILES:                  return PErrorCode::InvalidArg;
        case _SC_MEMLOCK:                       return PErrorCode::InvalidArg;
        case _SC_MEMLOCK_RANGE:                 return PErrorCode::InvalidArg;
        case _SC_MEMORY_PROTECTION:             return PErrorCode::InvalidArg;
        case _SC_MESSAGE_PASSING:               return PErrorCode::InvalidArg;
        case _SC_PRIORITIZED_IO:                return PErrorCode::InvalidArg;
        case _SC_REALTIME_SIGNALS:              return PErrorCode::InvalidArg;
        case _SC_SEMAPHORES:                    return PErrorCode::InvalidArg;
        case _SC_SHARED_MEMORY_OBJECTS:         return PErrorCode::InvalidArg;
        case _SC_SYNCHRONIZED_IO:               return PErrorCode::InvalidArg;
        case _SC_TIMERS:                        return PErrorCode::InvalidArg;
        case _SC_AIO_LISTIO_MAX:                return PErrorCode::InvalidArg;
        case _SC_AIO_MAX:                       return PErrorCode::InvalidArg;
        case _SC_AIO_PRIO_DELTA_MAX:            return PErrorCode::InvalidArg;
        case _SC_DELAYTIMER_MAX:                return PErrorCode::InvalidArg;
        case _SC_THREAD_KEYS_MAX:
            *outValue = THREAD_MAX_TLS_SLOTS;
            return PErrorCode::Success;
        case _SC_THREAD_STACK_MIN:              return PErrorCode::InvalidArg;
        case _SC_THREAD_THREADS_MAX:            return PErrorCode::InvalidArg;
        case _SC_TTY_NAME_MAX:                  return PErrorCode::InvalidArg;
        case _SC_THREADS:                       return PErrorCode::InvalidArg;
        case _SC_THREAD_ATTR_STACKADDR:         return PErrorCode::InvalidArg;
        case _SC_THREAD_ATTR_STACKSIZE:         return PErrorCode::InvalidArg;
        case _SC_THREAD_PRIORITY_SCHEDULING:    return PErrorCode::InvalidArg;
        case _SC_THREAD_PRIO_INHERIT:           return PErrorCode::InvalidArg;
        case _SC_THREAD_PRIO_PROTECT:           return PErrorCode::InvalidArg;
        case _SC_THREAD_PROCESS_SHARED:         return PErrorCode::InvalidArg;
        case _SC_THREAD_SAFE_FUNCTIONS:         return PErrorCode::InvalidArg;
        case _SC_GETGR_R_SIZE_MAX:              return PErrorCode::InvalidArg;
        case _SC_GETPW_R_SIZE_MAX:              return PErrorCode::InvalidArg;
        case _SC_LOGIN_NAME_MAX:                return PErrorCode::InvalidArg;
        case _SC_THREAD_DESTRUCTOR_ITERATIONS:  return PErrorCode::InvalidArg;
        case _SC_ADVISORY_INFO:                 return PErrorCode::InvalidArg;
        case _SC_ATEXIT_MAX:                    return PErrorCode::InvalidArg;
        case _SC_BARRIERS:                      return PErrorCode::InvalidArg;
        case _SC_BC_BASE_MAX:                   return PErrorCode::InvalidArg;
        case _SC_BC_DIM_MAX:                    return PErrorCode::InvalidArg;
        case _SC_BC_SCALE_MAX:                  return PErrorCode::InvalidArg;
        case _SC_BC_STRING_MAX:                 return PErrorCode::InvalidArg;
        case _SC_CLOCK_SELECTION:               return PErrorCode::InvalidArg;
        case _SC_COLL_WEIGHTS_MAX:              return PErrorCode::InvalidArg;
        case _SC_CPUTIME:                       return PErrorCode::InvalidArg;
        case _SC_EXPR_NEST_MAX:                 return PErrorCode::InvalidArg;
        case _SC_HOST_NAME_MAX:                 return PErrorCode::InvalidArg;
        case _SC_IOV_MAX:                       return PErrorCode::InvalidArg;
        case _SC_IPV6:                          return PErrorCode::InvalidArg;
        case _SC_LINE_MAX:                      return PErrorCode::InvalidArg;
        case _SC_MONOTONIC_CLOCK:               return PErrorCode::InvalidArg;
        case _SC_RAW_SOCKETS:                   return PErrorCode::InvalidArg;
        case _SC_READER_WRITER_LOCKS:           return PErrorCode::InvalidArg;
        case _SC_REGEXP:                        return PErrorCode::InvalidArg;
        case _SC_RE_DUP_MAX:                    return PErrorCode::InvalidArg;
        case _SC_SHELL:                         return PErrorCode::InvalidArg;
        case _SC_SPAWN:                         return PErrorCode::InvalidArg;
        case _SC_SPIN_LOCKS:                    return PErrorCode::InvalidArg;
        case _SC_SPORADIC_SERVER:               return PErrorCode::InvalidArg;
        case _SC_SS_REPL_MAX:                   return PErrorCode::InvalidArg;
        case _SC_SYMLOOP_MAX:                   return PErrorCode::InvalidArg;
        case _SC_THREAD_CPUTIME:                return PErrorCode::InvalidArg;
        case _SC_THREAD_SPORADIC_SERVER:        return PErrorCode::InvalidArg;
        case _SC_TIMEOUTS:                      return PErrorCode::InvalidArg;
        case _SC_TRACE:                         return PErrorCode::InvalidArg;
        case _SC_TRACE_EVENT_FILTER:            return PErrorCode::InvalidArg;
        case _SC_TRACE_EVENT_NAME_MAX:          return PErrorCode::InvalidArg;
        case _SC_TRACE_INHERIT:                 return PErrorCode::InvalidArg;
        case _SC_TRACE_LOG:                     return PErrorCode::InvalidArg;
        case _SC_TRACE_NAME_MAX:                return PErrorCode::InvalidArg;
        case _SC_TRACE_SYS_MAX:                 return PErrorCode::InvalidArg;
        case _SC_TRACE_USER_EVENT_MAX:          return PErrorCode::InvalidArg;
        case _SC_TYPED_MEMORY_OBJECTS:          return PErrorCode::InvalidArg;
        case _SC_V7_ILP32_OFF32:                return PErrorCode::InvalidArg;
        case _SC_V7_ILP32_OFFBIG:               return PErrorCode::InvalidArg;
        case _SC_V7_LP64_OFF64:                 return PErrorCode::InvalidArg;
        case _SC_V7_LPBIG_OFFBIG:               return PErrorCode::InvalidArg;
        case _SC_XOPEN_CRYPT:                   return PErrorCode::InvalidArg;
        case _SC_XOPEN_ENH_I18N:                return PErrorCode::InvalidArg;
        case _SC_XOPEN_LEGACY:                  return PErrorCode::InvalidArg;
        case _SC_XOPEN_REALTIME:                return PErrorCode::InvalidArg;
        case _SC_STREAM_MAX:                    return PErrorCode::InvalidArg;
        case _SC_PRIORITY_SCHEDULING:           return PErrorCode::InvalidArg;
        case _SC_XOPEN_REALTIME_THREADS:        return PErrorCode::InvalidArg;
        case _SC_XOPEN_SHM:                     return PErrorCode::InvalidArg;
        case _SC_XOPEN_STREAMS:                 return PErrorCode::InvalidArg;
        case _SC_XOPEN_UNIX:                    return PErrorCode::InvalidArg;
        case _SC_XOPEN_VERSION:                 return PErrorCode::InvalidArg;
        case _SC_2_CHAR_TERM:                   return PErrorCode::InvalidArg;
        case _SC_2_C_BIND:                      return PErrorCode::InvalidArg;
        case _SC_2_C_DEV:                       return PErrorCode::InvalidArg;
        case _SC_2_FORT_DEV:                    return PErrorCode::InvalidArg;
        case _SC_2_FORT_RUN:                    return PErrorCode::InvalidArg;
        case _SC_2_LOCALEDEF:                   return PErrorCode::InvalidArg;
        case _SC_2_PBS:                         return PErrorCode::InvalidArg;
        case _SC_2_PBS_ACCOUNTING:              return PErrorCode::InvalidArg;
        case _SC_2_PBS_CHECKPOINT:              return PErrorCode::InvalidArg;
        case _SC_2_PBS_LOCATE:                  return PErrorCode::InvalidArg;
        case _SC_2_PBS_MESSAGE:                 return PErrorCode::InvalidArg;
        case _SC_2_PBS_TRACK:                   return PErrorCode::InvalidArg;
        case _SC_2_SW_DEV:                      return PErrorCode::InvalidArg;
        case _SC_2_UPE:                         return PErrorCode::InvalidArg;
        case _SC_2_VERSION:                     return PErrorCode::InvalidArg;
        case _SC_THREAD_ROBUST_PRIO_INHERIT:    return PErrorCode::InvalidArg;
        case _SC_THREAD_ROBUST_PRIO_PROTECT:    return PErrorCode::InvalidArg;
        case _SC_XOPEN_UUCP:                    return PErrorCode::InvalidArg;
        case _SC_LEVEL1_ICACHE_SIZE:            return PErrorCode::InvalidArg;
        case _SC_LEVEL1_ICACHE_ASSOC:           return PErrorCode::InvalidArg;
        case _SC_LEVEL1_ICACHE_LINESIZE:        return PErrorCode::InvalidArg;
        case _SC_LEVEL1_DCACHE_SIZE:            return PErrorCode::InvalidArg;
        case _SC_LEVEL1_DCACHE_ASSOC:           return PErrorCode::InvalidArg;
        case _SC_LEVEL1_DCACHE_LINESIZE:        return PErrorCode::InvalidArg;
        case _SC_LEVEL2_CACHE_SIZE:             return PErrorCode::InvalidArg;
        case _SC_LEVEL2_CACHE_ASSOC:            return PErrorCode::InvalidArg;
        case _SC_LEVEL2_CACHE_LINESIZE:         return PErrorCode::InvalidArg;
        case _SC_LEVEL3_CACHE_SIZE:             return PErrorCode::InvalidArg;
        case _SC_LEVEL3_CACHE_ASSOC:            return PErrorCode::InvalidArg;
        case _SC_LEVEL3_CACHE_LINESIZE:         return PErrorCode::InvalidArg;
        case _SC_LEVEL4_CACHE_SIZE:             return PErrorCode::InvalidArg;
        case _SC_LEVEL4_CACHE_ASSOC:            return PErrorCode::InvalidArg;
        case _SC_LEVEL4_CACHE_LINESIZE:         return PErrorCode::InvalidArg;
        case _SC_POSIX_26_VERSION:              return PErrorCode::InvalidArg;

        default: return PErrorCode::InvalidArg;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_reboot(BootMode bootMode)
{
    kwrite_backup_register_trw(PBackupReg_BootMode, std::to_underlying(bootMode));
    NVIC_SystemReset();
}

} // extern "C"

} // namespace kernel
