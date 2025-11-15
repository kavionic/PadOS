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
// Created: 27.04.2018 22:30:54


#include <PadOS/Time.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KMutex.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KConditionVariable::KConditionVariable(const char* name, clockid_t clockID) : KNamedObject(name, KNamedObjectType::ConditionVariable), m_ClockID(clockID)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KConditionVariable::~KConditionVariable()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::WaitInternal(KMutex* lock)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (;;)
    {
        KThreadWaitNode waitNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.m_Thread = thread;
            thread->m_State = ThreadState_Waiting;
            m_WaitQueue.Append(&waitNode);
            if (lock != nullptr) lock->Unlock();
            thread->SetBlockingObject(this);

            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        thread->SetBlockingObject(nullptr);
        if (lock != nullptr) lock->Lock();
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (waitNode.m_TargetDeleted) {
                return PErrorCode::InvalidArg;
            }
            if (!waitNode.Detatch())
            {
                return PErrorCode::Success;
            }                
        } CRITICAL_END;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::WaitDeadlineInternal(KMutex* lock, clockid_t clockID, TimeValNanos clockDeadline)
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
            if (lock != nullptr) lock->Unlock();

            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        if (lock != nullptr) lock->Lock();
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
        	thread->SetBlockingObject(nullptr);
            sleepNode.Detatch();
            if (waitNode.m_TargetDeleted) {
                return PErrorCode::InvalidArg;
            }
            if (!waitNode.Detatch())
            {
                return PErrorCode::Success;
            }
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::WaitTimeoutInternal(KMutex* lock, TimeValNanos timeout)
{
    return WaitDeadlineInternal(lock, CLOCK_MONOTONIC_COARSE, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::IRQWait()
{
    IRQEnableState irqState = get_interrupt_enabled_state();
    
    if (irqState == IRQEnableState::Enabled)
    {
        p_system_log<PLogSeverity::ERROR>(LogCatKernel_General, "KConditionVariable::IRQWait() called with interrupts enabled!");
        return PErrorCode::InvalidArg;
    }
    
    KThreadCB* thread = gk_CurrentThread;
    
    for (;;)
    {
        KThreadWaitNode waitNode;

        waitNode.m_Thread = thread;
        thread->m_State = ThreadState_Waiting;
        m_WaitQueue.Append(&waitNode);

        thread->SetBlockingObject(this);

        KSWITCH_CONTEXT();
        set_interrupt_enabled_state(IRQEnableState::Enabled); // Enable interrupts and allow the scheduled context switch to happen.
        set_interrupt_enabled_state(irqState); // Disable interrupts again when we wake up.

        thread->SetBlockingObject(nullptr);

        if (waitNode.m_TargetDeleted) {
            return PErrorCode::InvalidArg;
        }
        if (!waitNode.Detatch())
        {
            return PErrorCode::Success;
        }                
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::IRQWaitDeadline(TimeValNanos deadline)
{
    return IRQWaitClock(m_ClockID, deadline);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::IRQWaitClock(clockid_t clockID, TimeValNanos clockDeadline)
{
    IRQEnableState irqState = get_interrupt_enabled_state();
    
    if (irqState == IRQEnableState::Enabled)
    {
        p_system_log<PLogSeverity::ERROR>(LogCatKernel_General, "KConditionVariable::IRQWaitDeadline() called with interrupts enabled!");
        return PErrorCode::InvalidArg;
    }
    
    TimeValNanos deadline;
    const PErrorCode result = kconvert_clock_to_monotonic(clockID, clockDeadline, deadline);
    if (result != PErrorCode::Success) {
        return result;
    }

    KThreadCB* thread = gk_CurrentThread;
    
    for (;;)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        if (deadline.IsInfinit() || get_monotonic_time() < deadline)
        {
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
        
        thread->SetBlockingObject(this);

        KSWITCH_CONTEXT();
        set_interrupt_enabled_state(IRQEnableState::Enabled); // Enable interrupts and allow the scheduled context switch to happen.
        set_interrupt_enabled_state(irqState); // Disable interrupts again when we wake up.

        thread->SetBlockingObject(nullptr);

        sleepNode.Detatch();
        if (waitNode.m_TargetDeleted) {
            return PErrorCode::InvalidArg;
        }
        if (!waitNode.Detatch())
        {
            return PErrorCode::Success;
        }                
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::IRQWaitTimeout(TimeValNanos timeout)
{
    return IRQWaitClock(CLOCK_MONOTONIC_COARSE, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit);
}

///////////////////////////////////////////////////////////////////////////////
/// Wakeup 1 or more threads waiting for the condition. If threadCount is 0 all
/// waiting threads will be woken up, if threadCount is > 0, up to threadCount
/// number of threads are woken up.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KConditionVariable::Wakeup(int threadCount)
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (wakeup_wait_queue(&m_WaitQueue, 0, threadCount)) KSWITCH_CONTEXT();
    } CRITICAL_END;    
    return PErrorCode::Success;
}

} // namespace kernel
