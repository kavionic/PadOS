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
// Created: 31.08.2025 17:00

#include <string.h>
#include <sys/pados_error_codes.h>

#include <System/ErrorCodes.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KProcess.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/Syscalls.h>

using namespace os;
using namespace kernel;

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_thread_attribs_init(PThreadAttribs* attribs)
{
    *attribs = PThreadAttribs(nullptr);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_spawn(thread_id* outThreadHandle, const PThreadAttribs* attribs, ThreadEntryPoint_t entryPoint, void* arguments)
{
    Ptr<KThreadCB> thread;

    try
    {
        thread = ptr_new<KThreadCB>(attribs);
        thread->InitializeStack(entryPoint, arguments);

        thread_id handle;
        PErrorCode result = gk_ThreadTable.AllocHandle(handle);
        if (result == PErrorCode::Success)
        {
            thread->SetHandle(handle);
            gk_ThreadTable.Set(handle, thread);
            if (outThreadHandle != nullptr) {
                *outThreadHandle = handle;
            }
        }
        else
        {
            return result;
        }
    }
    catch (const std::bad_alloc& error)
    {
        return PErrorCode::NoMemory;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        add_thread_to_ready_list(ptr_raw_pointer_cast(thread));
    } CRITICAL_END;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void sys_thread_exit(void* returnValue)
{
    KProcess* process = gk_CurrentProcess;
    KThreadCB* thread = gk_CurrentThread;

    process->ThreadQuit(thread);

    thread->m_State = ThreadState_Zombie;
    thread->m_ReturnValue = returnValue;

//#define CLEAR_IF_STDF(f) if ((f) == 0 || (f) == 1 || (f) == 2) { (f) = -1; }
//    CLEAR_IF_STDF(stdin->_file);
//    CLEAR_IF_STDF(stdout->_file);
//    CLEAR_IF_STDF(stderr->_file);
//#undef CLEAR_IF_STDF
    _reclaim_reent(nullptr);

    KSWITCH_CONTEXT();

    panic("exit_thread() survived a context switch!\n");
    for (;;);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_thread_detach(thread_id handle)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);

    if (thread == nullptr) {
        return EINVAL;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->m_DetachState != PThreadDetachState_Joinable || thread->m_State == ThreadState_Deleted) {
            return EINVAL;
        }
        thread->m_DetachState = PThreadDetachState_Detached;
        if (thread->m_State == ThreadState_Zombie) {
            add_thread_to_zombie_list(ptr_raw_pointer_cast(thread));
        }
    } CRITICAL_END;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_thread_join(thread_id handle, void** outReturnValue)
{
    KThreadCB* const thread = gk_CurrentThread;

    for (;;)
    {
        Ptr<KThreadCB> child = gk_ThreadTable.Get(handle);

        if (child == nullptr) {
            return EINVAL;
        }

        KThreadWaitNode waitNode;
        waitNode.m_Thread = thread;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (child->m_State == ThreadState_Deleted)
            {
                return EINVAL;
            }
            if (child->m_State != ThreadState_Zombie)
            {
                thread->m_State = ThreadState_Waiting;
                child->GetWaitQueue().Append(&waitNode);

                KSWITCH_CONTEXT();
            }
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();

            if (child->m_State == ThreadState_Deleted) {
                return EINVAL;
            }

            if (child->m_State != ThreadState_Zombie) // We got interrupted
            {
                continue;
            }
        } CRITICAL_END;
        if (outReturnValue != nullptr) {
            *outReturnValue = child->m_ReturnValue;
        }
        gk_ThreadTable.FreeHandle(handle);
        return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

thread_id sys_get_thread_id()
{
    return gk_CurrentThread->GetHandle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_thread_set_priority(thread_id handle, int priority)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);

    if (thread == nullptr) {
        return EINVAL;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->m_State == ThreadState_Deleted) {
            return EINVAL;
        }
        const int prevPriorityLevel = thread->GetPriorityLevel();
        thread->SetPriority(priority);
        if (thread->GetPriorityLevel() > prevPriorityLevel)
        {
            KSWITCH_CONTEXT();
        }
    } CRITICAL_END;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_thread_get_priority(thread_id handle, int* outPriority)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);

    if (thread == nullptr) {
        return EINVAL;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->m_State == ThreadState_Deleted) {
            return EINVAL;
        }
        *outPriority = thread->GetPriority();
    } CRITICAL_END;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void get_thread_info(Ptr<KThreadCB> thread, ThreadInfo* info)
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        strncpy(info->ThreadName, thread->GetName(), OS_NAME_LENGTH);
        const KNamedObject* blockingObject = thread->GetBlockingObject();
        info->BlockingObject = (blockingObject != nullptr) ? blockingObject->GetHandle() : INVALID_HANDLE;
    } CRITICAL_END;
    info->ProcessName[0] = '\0';

    info->ThreadID      = thread->GetHandle();
    info->ProcessID     = thread->GetProcessID();
    info->State         = thread->m_State;
    info->Flags         = 0; // thread->m_Flags;
    info->Priority      = thread->GetPriority();
    info->DynamicPri    = info->Priority;
    info->SysTimeNano   = 0;   // We don't track system time yet (system calls are included in RunTime, IRQ's are not).
    info->RealTimeNano  = thread->m_RunTime.AsNanoseconds();
    info->UserTimeNano  = info->RealTimeNano - info->SysTimeNano;
    info->QuantumNano   = TimeValNanos::TicksPerSecond / SYS_TICKS_PER_SEC;
    info->Stack         = thread->GetStackTop();
    info->StackSize     = thread->m_StackSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_get_thread_info(handle_id handle, ThreadInfo* info)
{
    Ptr<KThreadCB> thread;
    if (handle != INVALID_HANDLE)
    {
        thread = get_thread(handle);
        if (thread == nullptr) {
            return EINVAL;
        }
    }
    else
    {
        thread = gk_ThreadTable.GetNext(INVALID_HANDLE, [](Ptr<KThreadCB> thread) { return thread->m_State != ThreadState_Deleted; });
        if (thread == nullptr) {
            return ENOENT;
        }
    }
    get_thread_info(thread, info);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_get_next_thread_info(ThreadInfo* info)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.GetNext(info->ThreadID, [](Ptr<KThreadCB> thread) { return thread->m_State != ThreadState_Deleted; });

    if (thread == nullptr) {
        return ENOENT;
    }
    get_thread_info(thread, info);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_snooze_until(bigtime_t resumeTimeNanos)
{
    const TimeValNanos resumeTime = TimeValNanos::FromNanoseconds(resumeTimeNanos);
    KThreadCB* thread = gk_CurrentThread;

    KThreadWaitNode waitNode;

    waitNode.m_Thread = thread;
    waitNode.m_ResumeTime = resumeTime + TimeValNanos::FromMilliseconds(1); // Add 1 tick-time to ensure we always round up.

    for (;;)
    {
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            add_to_sleep_list(&waitNode);
            thread->m_State = ThreadState_Sleeping;
//            ThreadSyncDebugTracker::GetInstance().AddThread(thread, nullptr);
        } CRITICAL_END;

        KSWITCH_CONTEXT();

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
//            ThreadSyncDebugTracker::GetInstance().RemoveThread(thread);
        } CRITICAL_END;
        if (kget_system_time() >= waitNode.m_ResumeTime)
        {
            return PErrorCode::Success;
        }
        else
        {
            return PErrorCode::Interrupted;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_snooze_ns(bigtime_t delayNanos)
{
    return sys_snooze_until(sys_get_system_time() + delayNanos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_kill(pid_t pid, int sig)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksnooze_until(TimeValNanos resumeTime)
{
    return sys_snooze_until(resumeTime.AsNanoseconds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksnooze(TimeValNanos delay)
{
    return sys_snooze_ns(delay.AsNanoseconds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int sys_yield()
{
    KSWITCH_CONTEXT();
    return 0;
}

} // extern "C"

