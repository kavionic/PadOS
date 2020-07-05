// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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


#include "KConditionVariable.h"
#include "Scheduler.h"
#include "KMutex.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KConditionVariable::KConditionVariable(const char* name) : KNamedObject(name, KNamedObjectType::ConditionVariable)
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

bool KConditionVariable::Wait(KMutex& lock)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (;;)
    {
        KThreadWaitNode waitNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.m_Thread = thread;
            thread->m_State = KThreadState::Waiting;
            m_WaitQueue.Append(&waitNode);
            lock.Unlock();
            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        lock.Lock();
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }
            if (!waitNode.Detatch())
            {
                return true;
            }                
            else if (thread->m_RestartSyscalls)
            {
                continue;
            }
            set_last_error(EINTR);
            return false;
        } CRITICAL_END;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KConditionVariable::WaitDeadline(KMutex& lock, bigtime_t deadline)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (deadline == INFINIT_TIMEOUT || get_system_time() < deadline)
            {
                if (!first) {
                    set_last_error(EINTR);
                    return false;
                }
                waitNode.m_Thread      = thread;
                sleepNode.m_Thread     = thread;
                sleepNode.m_ResumeTime = deadline;

                thread->m_State = KThreadState::Sleeping;
                m_WaitQueue.Append(&waitNode);
                add_to_sleep_list(&sleepNode);
                thread->m_BlockingObject = this;
            }
            else
            {
                set_last_error(ETIME);
                return false;
            }
            lock.Unlock();
            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        lock.Lock();
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
        	thread->m_BlockingObject = nullptr;
            sleepNode.Detatch();
            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }
            waitNode.Detatch();
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KConditionVariable::WaitTimeout(KMutex& lock, bigtime_t timeout)
{
    return WaitDeadline(lock, (timeout != INFINIT_TIMEOUT) ? (get_system_time() + timeout) : INFINIT_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KConditionVariable::IRQWait()
{
    IRQEnableState irqState = get_interrupt_enabled_state();
    
    if (irqState == IRQEnableState::Enabled)
    {
        printf("ERROR: KConditionVariable::IRQWait() called with interrupts enabled!\n");
        set_last_error(EINVAL);
        return false;
    }
    
    KThreadCB* thread = gk_CurrentThread;
    
    for (;;)
    {
        KThreadWaitNode waitNode;

        waitNode.m_Thread = thread;
        thread->m_State = KThreadState::Waiting;
        m_WaitQueue.Append(&waitNode);

        KSWITCH_CONTEXT();
        set_interrupt_enabled_state(IRQEnableState::Enabled); // Enable interrupts and allow the scheduled context switch to happen.
        set_interrupt_enabled_state(irqState); // Disable interrupts again when we wake up.

        if (waitNode.m_TargetDeleted) {
            set_last_error(EINVAL);
            return false;
        }
        if (!waitNode.Detatch())
        {
            return true;
        }                
        else if (thread->m_RestartSyscalls)
        {
            continue;
        }
        set_last_error(EINTR);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KConditionVariable::IRQWaitDeadline(bigtime_t deadline)
{
    IRQEnableState irqState = get_interrupt_enabled_state();
    
    if (irqState == IRQEnableState::Enabled)
    {
        printf("ERROR: KConditionVariable::IRQWaitDeadline() called with interrupts enabled!\n");
        set_last_error(EINVAL);
        return false;
    }
    
    KThreadCB* thread = gk_CurrentThread;
    
    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        if (deadline == INFINIT_TIMEOUT || get_system_time() < deadline)
        {
            if (!first) {
                set_last_error(EINTR);
                return false;
            }
            waitNode.m_Thread      = thread;
            sleepNode.m_Thread     = thread;
            sleepNode.m_ResumeTime = deadline;

            thread->m_State = KThreadState::Sleeping;
            m_WaitQueue.Append(&waitNode);
            add_to_sleep_list(&sleepNode);
        }
        else
        {
            set_last_error(ETIME);
            return false;
        }
        
        KSWITCH_CONTEXT();
        set_interrupt_enabled_state(IRQEnableState::Enabled); // Enable interrupts and allow the scheduled context switch to happen.
        set_interrupt_enabled_state(irqState); // Disable interrupts again when we wake up.
        
        sleepNode.Detatch();
        if (waitNode.m_TargetDeleted) {
            set_last_error(EINVAL);
            return false;
        }
        if (!waitNode.Detatch())
        {
            return true;
        }                
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KConditionVariable::IRQWaitTimeout(bigtime_t timeout)
{
    return IRQWaitDeadline((timeout != INFINIT_TIMEOUT) ? (get_system_time() + timeout) : INFINIT_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
/// Wakeup 1 or more threads waiting for the condition. If threadCount is 0 all
/// waiting threads will be woken up, if threadCount is > 0, up to threadCount
/// number of threads are woken up.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KConditionVariable::Wakeup(int threadCount)
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (wakeup_wait_queue(&m_WaitQueue, 0, threadCount)) KSWITCH_CONTEXT();
    } CRITICAL_END;    
}