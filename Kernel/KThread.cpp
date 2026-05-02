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
#include <Kernel/KPIDNode.h>
#include <Kernel/KPosixSpawn.h>
#include <Kernel/KProcess.h>
#include <Kernel/KThread.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Threads/ThreadUserspaceState.h>


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

void KThread::Start_trw(KSpawnThreadFlags flags, PThreadDetachState detachState, int priority, int stackSize)
{
    if (m_ThreadHandle == INVALID_HANDLE)
    {
        m_DetachState = detachState;
        PThreadAttribs attrs(m_Name.c_str(), priority, detachState, stackSize);
        m_ThreadHandle = kthread_spawn_trw(&attrs, nullptr, /*tlsBlock*/ nullptr, flags | KSpawnThreadFlag::Privileged, nullptr, ThreadEntry, this);
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

thread_id kthread_spawn_trw(const PThreadAttribs* threadAttr, const PPosixSpawnAttribs* spawnAttr, PThreadUserData* threadUserData, KSpawnThreadFlags flags, ThreadEntryTrampoline_t entryTrampoline, ThreadEntryPoint_t entryPoint, void* arguments)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    const Ptr<KPIDNode> pidNode = kallocate_pid_trw_pl();

    PScopeFail scopeFail([&pidNode]() { kerase_pid_node_pl(pidNode->PID); });

    Ptr<KProcess> process;
    if (flags.Has(KSpawnThreadFlag::SpawnProcess)) {
        process = ptr_new<KProcess>(*pidNode, ptr_tmp_cast(&kget_current_process()), spawnAttr, (threadAttr != nullptr && threadAttr->Name != nullptr) ? threadAttr->Name : "");
    } else {
        process = ptr_tmp_cast(&kget_current_process());
    }

    if (threadUserData != nullptr) {
        threadUserData->ThreadID = pidNode->PID;
    }

    Ptr<KThreadCB> thread;

    thread = ptr_new<KThreadCB>(pidNode->PID, process, threadAttr, flags.Has(KSpawnThreadFlag::Privileged), threadUserData, nullptr);
    thread->InitializeStack(entryTrampoline, entryPoint, /*skipEntryTrampoline*/ false, arguments);

#ifdef PADOS_MODULE_POSIX_SIGNALS
    const KThreadCB& currentThread = kget_current_thread();
    if (flags.Has(KSpawnThreadFlag::SpawnProcess)) {
        thread->m_BlockedSignals = currentThread.m_BlockedSignals;
    }
#ifdef PADOS_MODULE_POSIX_SPAWN
    if (spawnAttr != nullptr && (spawnAttr->sa_flags & POSIX_SPAWN_SETSIGMASK)) {
        thread->m_BlockedSignals = spawnAttr->sa_sigmask & KBLOCKABLE_SIGNALS_MASK;
    }
#endif // PADOS_MODULE_POSIX_SPAWN
#endif // PADOS_MODULE_POSIX_SIGNALS

    pidNode->Thread = thread;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        add_thread_to_ready_list(ptr_raw_pointer_cast(thread));
    } CRITICAL_END;

    return pidNode->PID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

__attribute__((noreturn)) void kthread_exit(void* returnValue)
{
    KThreadCB& thread = kget_current_thread();

    if (thread.IsMainThread() && !thread.m_Process->IsExitStatusSet()) {
        thread.m_Process->SetExitStatus(CLD_EXITED, (int)returnValue);
    }
    thread.m_Process->RemoveThread(&thread);

    thread.m_ReturnValue = returnValue;

    _reclaim_reent(nullptr);

    thread.SetState(ThreadState_Zombie);

    KSWITCH_CONTEXT();

    panic("kthread_exit() survived a context switch!\n");
    for (;;);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#ifdef PADOS_MODULE_POSIX_SIGNALS

static void kinitiate_thread_cancellation(KThreadCB& thread) noexcept
{
    thread.m_ThreadUserData->IsCanceled = true;
    if (thread.m_CancelType == THREAD_CANCEL_ASYNCHRONOUS)
    {
        thread.m_ThreadUserData->CancellationPending = true; // Make SIGKILL hard-kill.
        ksend_signal_to_thread(thread, SIGKILL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kthread_cancel_trw(pid_t threadID)
{
    Ptr<KThreadCB> thread = kget_thread_trw(threadID);

    if (thread->m_ThreadUserData == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    kthread_cancel_pl(*thread);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kthread_cancel_pl(KThreadCB& thread) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    kassert(thread.m_ThreadUserData != nullptr);

    if (!thread.m_ThreadUserData->CancellationPending)
    {
        thread.m_ThreadUserData->CancellationPending = true;
        if (thread.m_CancelState == THREAD_CANCEL_ENABLE)
        {
            kinitiate_thread_cancellation(thread);
        }
        wakeup_thread(thread, true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_cancel(pid_t threadID) noexcept
{
    try
    {
        kthread_cancel_trw(threadID);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_setcancelstate(PThreadCancelState state, PThreadCancelState* outOldState)
{
    KThreadCB& thread = kget_current_thread();

    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    const PThreadCancelState oldState = thread.m_CancelState;

    if (state != oldState)
    {
        PThreadUserData* userData = thread.m_ThreadUserData;
        if (userData == nullptr) {
            return PErrorCode::InvalidArg;
        }

        thread.m_CancelState = state;

        if (state == THREAD_CANCEL_DISABLE)
        {
            userData->IsCanceled = false;
        }
        else if (userData->CancellationPending)
        {
            kinitiate_thread_cancellation(thread);
        }
    }
    if (outOldState != nullptr) {
        *outOldState = oldState;
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_setcanceltype(PThreadCancelType type, PThreadCancelType* outOldType)
{
    KThreadCB& thread = kget_current_thread();

    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    const PThreadCancelType oldType = thread.m_CancelType;

    if (type != oldType)
    {
        thread.m_CancelType = type;
    }
    if (outOldType != nullptr) {
        *outOldType = oldType;
    }
    return PErrorCode::Success;
}

#endif // PADOS_MODULE_POSIX_SIGNALS

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_detach(thread_id handle)
{
    Ptr<KThreadCB> thread = kget_thread(handle);

    if (thread == nullptr) {
        return PErrorCode::InvalidArg;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->m_DetachState != PThreadDetachState_Joinable || thread->GetState() == ThreadState_Deleted) {
            return PErrorCode::InvalidArg;
        }
        thread->m_DetachState = PThreadDetachState_Detached;
        if (thread->GetState() == ThreadState_Zombie) {
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
        const Ptr<KPIDNode> pidNode = kget_pid_node(handle);
        if (pidNode == nullptr) {
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }

        const Ptr<KThreadCB> child = pidNode->Thread;
        if (child == nullptr) {
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }

        KThreadWaitNode waitNode;
        waitNode.m_Thread = thread;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (child->GetState() == ThreadState_Deleted)
            {
                result = PErrorCode::InvalidArg;
                break;
            }
            if (child->GetState() != ThreadState_Zombie)
            {
                thread->SetState(ThreadState_Waiting);
                child->GetWaitQueue().Append(&waitNode);

                KSWITCH_CONTEXT();
            }
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();

            if (child->GetState() == ThreadState_Deleted)
            {
                result = PErrorCode::InvalidArg;
                break;
            }

            if (child->GetState() != ThreadState_Zombie) { // We got interrupted
                continue;
            }
        } CRITICAL_END;
        void* returnValue = child->m_ReturnValue;

        p_thread_reaper_schedule_cleanup(child->m_ThreadUserData);

        kassert(!g_PIDMapMutex.IsLocked());
        KScopedLock lock(g_PIDMapMutex);

        pidNode->Thread = nullptr;
        if (pidNode->IsEmpty()) {
            kerase_pid_node_pl(handle);
        }
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
    Ptr<KThreadCB> thread = kget_thread(handle);

    if (thread == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    PErrorCode result = PErrorCode::Success;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->GetState() != ThreadState_Deleted)
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
    Ptr<KThreadCB> thread = kget_thread(handle);

    if (thread == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    PErrorCode result = PErrorCode::Success;
    int priority = 0;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (thread->GetState() != ThreadState_Deleted) {
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
        strncpy(info->ProcessName, thread->m_Process->GetName(), OS_NAME_LENGTH - 1);
        strncpy(info->ThreadName, thread->GetName(), OS_NAME_LENGTH - 1);

        info->ThreadName[OS_NAME_LENGTH - 1] = '\0';
        info->ProcessName[OS_NAME_LENGTH - 1] = '\0';

        const KNamedObject* blockingObject = thread->GetBlockingObject();
        info->BlockingObject = (blockingObject != nullptr) ? blockingObject->GetHandle() : INVALID_HANDLE;
    } CRITICAL_END;

    info->ThreadID = thread->GetHandle();
    info->ProcessID = thread->m_Process->GetPID();
    info->State = thread->GetState();
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
        thread = kget_thread(handle);
        if (thread == nullptr) {
            return PErrorCode::InvalidArg;
        }
    }
    else
    {
        thread = kget_first_thread();
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
    Ptr<KThreadCB> thread = kget_next_thread(info->ThreadID);

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
#ifdef PADOS_MODULE_POSIX_SIGNALS
            if (kis_thread_canceled()) {
                return PErrorCode::Interrupted;
            }
#endif // PADOS_MODULE_POSIX_SIGNALS
            add_to_sleep_list(&waitNode);
            thread->SetState(ThreadState_Sleeping);
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



} // namespace kernel
