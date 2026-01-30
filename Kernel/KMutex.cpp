// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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

#include <Kernel/KTime.h>
#include <Kernel/KMutex.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KPosixSignals.h>
#include <System/System.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KMutex::KMutex(const char* name, PEMutexRecursionMode recursionMode, int clockID) : KNamedObject(name, KNamedObjectType::Mutex), m_RecursionMode(recursionMode), m_ClockID(clockID)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KMutex::~KMutex()
{
    if (m_WaitQueue.m_First != nullptr) {
        panic("KMutex destructed while threads waiting for it\n");
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::Lock(bool interruptible)
{
    KThreadCB* thread = gk_CurrentThread;
    for (;;)
    {
        KThreadWaitNode waitNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count == 0 || (m_RecursionMode == PEMutexRecursionMode_Recurse && m_Holder == thread->GetHandle()))
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return PErrorCode::Success;
            }
            else
            {
                kassert(!(m_RecursionMode == PEMutexRecursionMode_RaiseError && m_Holder == thread->GetHandle()));
                if (m_RecursionMode == PEMutexRecursionMode_RaiseError && m_Holder == thread->GetHandle()) {
                    return PErrorCode::Deadlock;
                }
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
            waitNode.Detatch();
            thread->SetBlockingObject(nullptr);
            
            if (waitNode.m_TargetDeleted) {
                return PErrorCode::InvalidArg;
            }
            
            if (m_Count == 0)
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return PErrorCode::Success;
            }
        } CRITICAL_END;
        if (interruptible) {
            return PErrorCode::RestartSyscall;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::LockDeadline(TimeValNanos deadline, bool interruptible)
{
    return LockClock(m_ClockID, deadline, interruptible);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::LockClock(int clockID, TimeValNanos clockDeadline, bool interruptible)
{
    KThreadCB* thread = gk_CurrentThread;
    
    TimeValNanos deadline;
    const PErrorCode result = kconvert_clock_to_monotonic(clockID, clockDeadline, deadline);
    if (result != PErrorCode::Success) {
        return result;
    }

    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count == 0 || (m_RecursionMode == PEMutexRecursionMode_Recurse && m_Holder == thread->GetHandle()))
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return PErrorCode::Success;
            }
            else
            {
                kassert(!(m_RecursionMode == PEMutexRecursionMode_RaiseError && m_Holder == thread->GetHandle()));
                if (m_RecursionMode == PEMutexRecursionMode_RaiseError && m_Holder == thread->GetHandle()) {
                    return PErrorCode::Deadlock;
                }
            }
            if (deadline.IsInfinit() || kget_monotonic_time() < deadline)
            {
                if (!first && interruptible) {
                    return PErrorCode::RestartSyscall;
                }
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

PErrorCode KMutex::LockTimeout(TimeValNanos timeout, bool interruptible)
{
    return LockClock(CLOCK_MONOTONIC_COARSE, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit, interruptible);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::TryLock()
{
    KThreadCB* thread = gk_CurrentThread;

    CRITICAL_SCOPE(CRITICAL_IRQ);

    if (m_Count == 0 || (m_RecursionMode == PEMutexRecursionMode_Recurse && m_Holder == thread->GetHandle()))
    {
        m_Count--;
        m_Holder = thread->GetHandle();
        return PErrorCode::Success;
    }

    return PErrorCode::Busy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::Unlock()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);

    if (m_Count < 0) {
        m_Count++;
    } else if (m_Count > 0) {
        m_Count--;
    } else {
        return PErrorCode::InvalidArg;
    }

    if (m_Count == 0) {
        m_Holder = -1;
        if (wakeup_wait_queue(&m_WaitQueue, 0, 0)) KSWITCH_CONTEXT();
    }

    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::LockShared(bool interruptible)
{
    KThreadCB* thread = gk_CurrentThread;
    for (;;)
    {
        KThreadWaitNode waitNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count >= 0)
            {
                m_Count++;
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
            waitNode.Detatch();
            thread->SetBlockingObject(nullptr);

            if (waitNode.m_TargetDeleted) {
                return PErrorCode::InvalidArg;
            }
            
            if (m_Count >= 0)
            {
                m_Count++;
                return PErrorCode::Success;
            }
        } CRITICAL_END;
        if (interruptible) {
            return PErrorCode::RestartSyscall;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::LockSharedDeadline(TimeValNanos deadline, bool interruptible)
{
    return LockSharedClock(m_ClockID, deadline, interruptible);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::LockSharedClock(clockid_t clockID, TimeValNanos clockDeadline, bool interruptible)
{
    KThreadCB* thread = gk_CurrentThread;
    
    TimeValNanos deadline;
    const PErrorCode result = kconvert_clock_to_monotonic(clockID, clockDeadline, deadline);
    if (result != PErrorCode::Success) {
        return result;
    }

    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count >= 0)
            {
                m_Count++;
                return PErrorCode::Success;
            }
            if (deadline.IsInfinit() || kget_monotonic_time() < deadline)
            {
                if (!first && interruptible) {
                    return PErrorCode::RestartSyscall;
                }
                waitNode.m_Thread      = thread;

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
            waitNode.Detatch();
            sleepNode.Detatch();

            if (waitNode.m_TargetDeleted) {
                return PErrorCode::InvalidArg;
            }
            
            if (wakeup_wait_queue(&m_WaitQueue, 0, 0)) KSWITCH_CONTEXT();
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::LockSharedTimeout(TimeValNanos timeout, bool interruptible)
{
    return LockSharedClock(CLOCK_MONOTONIC_COARSE, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit, interruptible);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KMutex::TryLockShared()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);

    if (m_Count >= 0)
    {
        m_Count++;
        return PErrorCode::Success;
    }

    return PErrorCode::Busy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMutex::IsLocked() const
{
    return !(m_Count == 0 || m_Holder != gk_CurrentThread->GetHandle());
}

} // namespace kernel
