// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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


#include "Kernel/KConditionVariable.h"
#include "Kernel/Scheduler.h"
#include "Kernel/KMutex.h"

using namespace kernel;
using namespace os;

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
            thread->m_State = ThreadState::Waiting;
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

bool KConditionVariable::WaitDeadline(KMutex& lock, TimeValMicros deadline)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (deadline.IsInfinit() || get_system_time() < deadline)
            {
                if (!first) {
                    set_last_error(EINTR);
                    return false;
                }
                waitNode.m_Thread      = thread;

                m_WaitQueue.Append(&waitNode);
                if (!deadline.IsInfinit())
                {
                    thread->m_State = ThreadState::Sleeping;
                    sleepNode.m_Thread = thread;
                    sleepNode.m_ResumeTime = deadline;
                    add_to_sleep_list(&sleepNode);
                }
                else
                {
                    thread->m_State = ThreadState::Waiting;
                }
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

bool KConditionVariable::WaitTimeout(KMutex& lock, TimeValMicros timeout)
{
    return WaitDeadline(lock, (!timeout.IsInfinit()) ? (get_system_time() + timeout) : TimeValMicros::infinit);
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
        thread->m_State = ThreadState::Waiting;
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

bool KConditionVariable::IRQWaitDeadline(TimeValMicros deadline)
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

        if (deadline.IsInfinit() || get_system_time() < deadline)
        {
            if (!first) {
                set_last_error(EINTR);
                return false;
            }
            waitNode.m_Thread      = thread;

            m_WaitQueue.Append(&waitNode);
            if (!deadline.IsInfinit())
            {
                thread->m_State = ThreadState::Sleeping;
                sleepNode.m_Thread = thread;
                sleepNode.m_ResumeTime = deadline;
                add_to_sleep_list(&sleepNode);
            }
            else
            {
                thread->m_State = ThreadState::Waiting;
            }
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

bool KConditionVariable::IRQWaitTimeout(TimeValMicros timeout)
{
    return IRQWaitDeadline((!timeout.IsInfinit()) ? (get_system_time() + timeout) : TimeValMicros::infinit);
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

handle_id create_condition_var(const char* name)
{
  try {
    return KNamedObject::RegisterObject(ptr_new<KConditionVariable>(name));
  }
  catch (const std::bad_alloc& error) {
    set_last_error(ENOMEM);
    return -1;
  }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t  condition_var_wait(handle_id handle, handle_id mutexHandle)
{
  Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
  if (mutex == nullptr) {
    set_last_error(EINVAL);
    return -1;
  }
  return KNamedObject::ForwardToHandleBoolToInt<KConditionVariable>(handle, &KConditionVariable::Wait, *mutex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t  condition_var_wait_timeout(handle_id handle, handle_id mutexHandle, bigtime_t timeout)
{
    return condition_var_wait_deadline(handle, mutexHandle, (timeout != TimeValMicros::infinit.AsMicroSeconds()) ? (get_system_time().AsMicroSeconds() + timeout) : TimeValMicros::infinit.AsMicroSeconds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t  condition_var_wait_deadline(handle_id handle, handle_id mutexHandle, bigtime_t deadline)
{
  Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
  if (mutex == nullptr) {
    set_last_error(EINVAL);
    return -1;
  }
  return KNamedObject::ForwardToHandleBoolToInt<KConditionVariable>(handle, &KConditionVariable::WaitDeadline, *mutex, TimeValMicros::FromMicroseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t condition_var_wakeup(handle_id handle, int threadCount)
{
  return KNamedObject::ForwardToHandleVoid<KConditionVariable>(handle, &KConditionVariable::Wakeup, threadCount);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t condition_var_wakeup_all(handle_id handle)
{
  return KNamedObject::ForwardToHandleVoid<KConditionVariable>(handle, &KConditionVariable::WakeupAll);
}

