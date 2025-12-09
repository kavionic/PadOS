// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.03.2018 22:38:38

#include <string.h>
#include <limits.h>
#include <sys/fcntl.h>

#include <Kernel/KSemaphore.h>
#include <Kernel/KMutex.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KTime.h>
#include <System/System.h>

using namespace os;

namespace kernel
{

static KMutex gk_PublicSemaphoresMutex("global_sema_mutex", PEMutexRecursionMode_RaiseError);
static std::map<String, Ptr<KSemaphore>> gk_PublicSemaphores;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSemaphore::KSemaphore(const char* name, clockid_t clockID, int count) : KNamedObject(name, KNamedObjectType::Semaphore), m_ClockID(clockID)
{
    m_Count = count;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSemaphore::~KSemaphore()
{
    if (m_WaitQueue.m_First != nullptr) {
        panic("KSemaphore destructed while threads waiting for it\n");
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KSemaphore::Acquire()
{
    KThreadCB* thread = gk_CurrentThread;
    for (;;)
    {
        KThreadWaitNode waitNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count > 0)
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return PErrorCode::Success;
            }
            waitNode.m_Thread = thread;
            thread->m_State = ThreadState_Waiting;
            m_WaitQueue.Append(&waitNode);
            thread->SetBlockingObject(this);

            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.        
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
        	thread->SetBlockingObject(nullptr);
            waitNode.Detatch();
            if (waitNode.m_TargetDeleted) {
                return PErrorCode::InvalidArg;
            }
            if (m_Count > 0)
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return PErrorCode::Success;
            }
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KSemaphore::AcquireDeadline(TimeValNanos deadline)
{
    return AcquireClock(m_ClockID, deadline);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KSemaphore::AcquireClock(clockid_t clockID, TimeValNanos clockDeadline)
{
    KThreadCB* thread = gk_CurrentThread;
    
    TimeValNanos deadline;
    const PErrorCode result = kconvert_clock_to_monotonic(clockID, clockDeadline, deadline);
    if (result != PErrorCode::Success) {
        return result;
    }
    for (;;)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count > 0)
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return PErrorCode::Success;
            }
            if (deadline.IsInfinit() || kget_monotonic_time() < deadline)
            {
                waitNode.m_Thread = thread;

                m_WaitQueue.Append(&waitNode);
                if (!deadline.IsInfinit())
                {
                    thread->m_State = ThreadState_Sleeping;
                    sleepNode.m_Thread = thread;
                    sleepNode.m_ResumeTime = deadline;
                    add_to_sleep_list(&sleepNode);
                }
                else
                {
                    thread->m_State = ThreadState_Waiting;
                }
                thread->SetBlockingObject(this);
            }
            else
            {
                return PErrorCode::Timeout;
            }

            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            thread->SetBlockingObject(nullptr);
            waitNode.Detatch();
            sleepNode.Detatch();
            if (waitNode.m_TargetDeleted) {
                return PErrorCode::InvalidArg;
            }
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KSemaphore::AcquireTimeout(TimeValNanos timeout)
{
    return AcquireClock(CLOCK_MONOTONIC_COARSE, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KSemaphore::TryAcquire()
{
    KThreadCB* thread = gk_CurrentThread;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (m_Count > 0)
        {
            m_Count--;
            m_Holder = thread->GetHandle();
            return PErrorCode::Success;
        }
    } CRITICAL_END;
    return PErrorCode::TryAgain;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KSemaphore::Release()
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (m_Count == SEM_VALUE_MAX) {
            return PErrorCode::Overflow;
        }
        m_Count++;

        if (m_Count > 0) {
            m_Holder = -1;
            if (wakeup_wait_queue(&m_WaitQueue, 0, m_Count)) KSWITCH_CONTEXT();
        }
    } CRITICAL_END;

    return PErrorCode::Success;
}

} // namespace kernel
