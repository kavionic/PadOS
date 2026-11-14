// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include <Kernel/KPIDNode.h>
#include <Kernel/KProcess.h>
#include <Kernel/KThread.h>
#include <Kernel/Scheduler.h>

namespace kernel
{

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
            return PErrorCode::INVAL;
        }
    }
    else
    {
        thread = kget_first_thread();
        if (thread == nullptr) {
            return PErrorCode::NOENT;
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
        return PErrorCode::NOENT;
    }
    get_thread_info(thread, info);
    return PErrorCode::Success;
}

} // namespace kernel
