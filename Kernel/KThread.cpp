// This file is part of PadOS.
//
// Copyright (C) 2025-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.10.2025 19:30

#include <string.h>

#include <System/System.h>
#include <System/ExceptionHandling.h>
#include <System/AppDefinition.h>
#include <Kernel/KProcess.h>
#include <Kernel/KThread.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>

using namespace os;


namespace kernel
{

thread_local KThread* KThread::st_CurrentThread = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThread::KThread(const PString& name) : m_Name(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThread::~KThread()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThread* KThread::GetCurrentThread()
{
    return st_CurrentThread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KThread::Start_trw(PThreadDetachState detachState, int priority, int stackSize)
{
    if (m_ThreadHandle == INVALID_HANDLE)
    {
        m_DetachState = detachState;
        PThreadAttribs attrs(m_Name.c_str(), priority, detachState, stackSize);
        m_ThreadHandle = kthread_spawn_trw(&attrs, /*tlsBlock*/ nullptr, /*privileged*/ true, ThreadEntry, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KThread::Join_trw(TimeValNanos deadline)
{
    if (m_DetachState != PThreadDetachState_Joinable) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    void* returnValue = kthread_join_trw(m_ThreadHandle);

    if (m_DeleteOnExit) {
        delete this;
    }
    return returnValue;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KThread::Run()
{
    if (!VFRun.Empty()) {
        return VFRun(this);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KThread::Exit(void* returnValue)
{
    if (m_DetachState == PThreadDetachState_Detached && m_DeleteOnExit) {
        delete this;
    }
    kthread_exit(returnValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KThread::ThreadEntry(void* data)
{
    KThread* self = static_cast<KThread*>(data);
    try
    {
        st_CurrentThread = self;
        self->Exit(self->Run());
    }
    catch (const std::exception& e)
    {
        kernel_log<PLogSeverity::FATAL>(LogCatKernel_Scheduler, "Uncaught exception in KThread {}: {}", self->GetName(), e.what());
        self->Exit(nullptr);
    }
    catch (...)
    {
        kernel_log<PLogSeverity::FATAL>(LogCatKernel_Scheduler, "Uncaught exception in KThread {}: unknown", self->GetName());
        self->Exit(nullptr);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_attribs_init(PThreadAttribs& outAttribs) noexcept
{
    outAttribs = PThreadAttribs(nullptr);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

thread_id kthread_spawn_trw(const PThreadAttribs* attribs, PThreadControlBlock* tlsBlock, bool privileged, ThreadEntryPoint_t entryPoint, void* arguments)
{
    Ptr<KThreadCB> thread;

    thread = ptr_new<KThreadCB>(attribs, tlsBlock, nullptr);
    thread->InitializeStack(entryPoint, privileged, /*skipEntryTrampoline*/ false, arguments);

    const thread_id handle = gk_ThreadTable.AllocHandle_trw();

    thread->SetHandle(handle);
    gk_ThreadTable.Set(handle, thread);

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        add_thread_to_ready_list(ptr_raw_pointer_cast(thread));
    } CRITICAL_END;

    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

__attribute__((noreturn)) void kthread_exit(void* returnValue)
{
    KThreadCB* thread = gk_CurrentThread;

    if (thread->m_UserspaceTLS != nullptr) {
        __app_definition.thread_terminated(thread->GetHandle(), thread->m_StackBuffer, thread->m_UserspaceTLS);
    }
    thread->m_State = ThreadState_Zombie;
    thread->m_ReturnValue = returnValue;

    _reclaim_reent(nullptr);

    KSWITCH_CONTEXT();

    panic("kthread_exit() survived a context switch!\n");
    for (;;);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_detach(thread_id handle)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);

    if (thread == nullptr) {
        return PErrorCode::InvalidArg;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->m_DetachState != PThreadDetachState_Joinable || thread->m_State == ThreadState_Deleted) {
            return PErrorCode::InvalidArg;
        }
        thread->m_DetachState = PThreadDetachState_Detached;
        if (thread->m_State == ThreadState_Zombie) {
            add_thread_to_zombie_list(ptr_raw_pointer_cast(thread));
        }
    } CRITICAL_END;

    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* kthread_join_trw(thread_id handle)
{
    KThreadCB* const thread = gk_CurrentThread;

    PErrorCode result = PErrorCode::Success;
    for (;;)
    {
        Ptr<KThreadCB> child = gk_ThreadTable.Get(handle);

        if (child == nullptr) {
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }

        KThreadWaitNode waitNode;
        waitNode.m_Thread = thread;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (child->m_State == ThreadState_Deleted)
            {
                result = PErrorCode::InvalidArg;
                break;
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

            if (child->m_State == ThreadState_Deleted)
            {
                result = PErrorCode::InvalidArg;
                break;
            }

            if (child->m_State != ThreadState_Zombie) { // We got interrupted
                continue;
            }
        } CRITICAL_END;
        void* returnValue = child->m_ReturnValue;
        gk_ThreadTable.FreeHandle_trw(handle);
        return returnValue;
    }
    PERROR_THROW_CODE(result);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

thread_id kget_thread_id() noexcept
{
    return gk_CurrentThread->GetHandle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kthread_set_priority_trw(thread_id handle, int priority)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);

    if (thread == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    PErrorCode result = PErrorCode::Success;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->m_State != ThreadState_Deleted)
        {
            const int prevPriorityLevel = thread->GetPriorityLevel();
            thread->SetPriority(priority);
            if (thread != gk_CurrentThread && thread->GetPriorityLevel() > prevPriorityLevel) {
                KSWITCH_CONTEXT();
            }
        }
        else
        {
            result = PErrorCode::InvalidArg;
        }
    } CRITICAL_END;

    PERROR_ERRORCODE_THROW_ON_FAIL(result);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kthread_get_priority_trw(thread_id handle)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);

    if (thread == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    PErrorCode result = PErrorCode::Success;
    int priority = 0;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->m_State != ThreadState_Deleted) {
            priority = thread->GetPriority();
        } else {
            result = PErrorCode::InvalidArg;
        }
    } CRITICAL_END;

    PERROR_ERRORCODE_THROW_ON_FAIL(result);
    return priority;
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

    info->ThreadID = thread->GetHandle();
    info->ProcessID = thread->GetProcessID();
    info->State = thread->m_State;
    info->Flags = 0; // thread->m_Flags;
    info->Priority = thread->GetPriority();
    info->DynamicPri = info->Priority;
    info->SysTimeNano = 0;   // We don't track system time yet (system calls are included in RunTime, IRQ's are not).
    info->RealTimeNano = thread->m_RunTime.AsNanoseconds();
    info->UserTimeNano = info->RealTimeNano - info->SysTimeNano;
    info->QuantumNano = TimeValNanos::TicksPerSecond / SYS_TICKS_PER_SEC;
    info->Stack = thread->GetStackTop();
    info->StackSize = thread->m_StackSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_thread_info(handle_id handle, ThreadInfo* info)
{
    Ptr<KThreadCB> thread;
    if (handle != INVALID_HANDLE)
    {
        thread = get_thread(handle);
        if (thread == nullptr) {
            return PErrorCode::InvalidArg;
        }
    }
    else
    {
        thread = gk_ThreadTable.GetNext(INVALID_HANDLE, [](Ptr<KThreadCB> thread) { return thread->m_State != ThreadState_Deleted; });
        if (thread == nullptr) {
            return PErrorCode::NoEntry;
        }
    }
    get_thread_info(thread, info);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kget_next_thread_info(ThreadInfo* info)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.GetNext(info->ThreadID, [](Ptr<KThreadCB> thread) { return thread->m_State != ThreadState_Deleted; });

    if (thread == nullptr) {
        return PErrorCode::NoEntry;
    }
    get_thread_info(thread, info);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksnooze_ns(bigtime_t delayNanos)
{
    return ksnooze_until_ns(kget_monotonic_time_ns() + delayNanos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksnooze_ms(bigtime_t millis)
{
    return ksnooze(TimeValNanos::FromMilliseconds(millis));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksnooze_until_ns(bigtime_t resumeTimeNanos)
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
        } CRITICAL_END;

        KSWITCH_CONTEXT();

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
        } CRITICAL_END;
        if (kget_monotonic_time() >= waitNode.m_ResumeTime)
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

PErrorCode ksnooze_until(TimeValNanos resumeTime)
{
    return ksnooze_until_ns(resumeTime.AsNanoseconds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksnooze(TimeValNanos delay)
{
    return ksnooze_ns(delay.AsNanoseconds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kyield()
{
    KSWITCH_CONTEXT();
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_kill(thread_id threadID, int sigNum)
{
    const Ptr<KThreadCB> thread = get_thread(threadID);

    if (thread == nullptr) {
        return PErrorCode::NoSuchProcess;
    }

    return ksend_signal_to_thread(*thread, sigNum);
}


} // namespace kernel
