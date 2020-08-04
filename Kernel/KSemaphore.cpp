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
// Created: 04.03.2018 22:38:38

#include <string.h>

#include "Kernel/KSemaphore.h"
#include "Kernel/KHandleArray.h"
#include "Kernel/Scheduler.h"
#include "System/System.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSemaphore::KSemaphore(const char* name, int count) : KNamedObject(name, KNamedObjectType::Semaphore)
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

Ptr<KSemaphore> KGetSemaphore(sem_id handle)
{
    return ptr_static_cast<KSemaphore>(KNamedObject::GetObject(handle, KNamedObjectType::Semaphore));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KSemaphore::Acquire()
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
                return true;
            }
            waitNode.m_Thread = thread;
            thread->m_State = ThreadState::Waiting;
            m_WaitQueue.Append(&waitNode);
            thread->m_BlockingObject = this;
            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.        
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
        	thread->m_BlockingObject = nullptr;
            waitNode.Detatch();
            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }
            if (m_Count > 0)
            {
                m_Count--;
                m_Holder = thread->GetHandle();
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

bool KSemaphore::AcquireDeadline(TimeValMicros deadline)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count > 0)
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return true;
            }
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
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
            sleepNode.Detatch();
            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }
        } CRITICAL_END;
    }
}

bool KSemaphore::AcquireTimeout(TimeValMicros timeout)
{
    return AcquireDeadline((!timeout.IsInfinit()) ? (get_system_time() + timeout) : TimeValMicros::infinit);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KSemaphore::TryAcquire()
{
    KThreadCB* thread = gk_CurrentThread;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (m_Count > 0)
        {
            m_Count--;
            m_Holder = thread->GetHandle();
            return true;
        }
    } CRITICAL_END;
    set_last_error(EWOULDBLOCK);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KSemaphore::Release()
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        m_Count++;

        if (m_Count > 0) {
            m_Holder = -1;
            if (wakeup_wait_queue(&m_WaitQueue, 0, m_Count)) KSWITCH_CONTEXT();
        }
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

sem_id create_semaphore(const char* name, int count)
{
    try {
        return KNamedObject::RegisterObject(ptr_new<KSemaphore>(name, count));
    } catch(const std::bad_alloc& error) {
        set_last_error(ENOMEM);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

sem_id duplicate_semaphore(sem_id handle)
{
    Ptr<KSemaphore> sema = KGetSemaphore(handle);
    if (sema != nullptr) {
        return KNamedObject::RegisterObject(sema);
    }
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t delete_semaphore(sem_id handle)
{
    if (KNamedObject::FreeHandle(handle, KNamedObjectType::Semaphore)) {
        return 0;
    }
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t acquire_semaphore(sem_id handle)
{
    return KNamedObject::ForwardToHandleBoolToInt<KSemaphore>(handle, &KSemaphore::Acquire);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t acquire_semaphore_timeout(sem_id handle, bigtime_t timeout)
{
    return KNamedObject::ForwardToHandleBoolToInt<KSemaphore>(handle, &KSemaphore::AcquireTimeout, TimeValMicros::FromMicroseconds(timeout));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t acquire_semaphore_deadline(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleBoolToInt<KSemaphore>(handle, &KSemaphore::AcquireDeadline, TimeValMicros::FromMicroseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t try_acquire_semaphore(sem_id handle)
{
    return KNamedObject::ForwardToHandleBoolToInt<KSemaphore>(handle, &KSemaphore::TryAcquire);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t release_semaphore(sem_id handle)
{
    return KNamedObject::ForwardToHandleVoid<KSemaphore>(handle, &KSemaphore::Release);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

sem_id duplicate_handle(handle_id handle)
{
    Ptr<KNamedObject> object = KNamedObject::GetAnyObject(handle);
    if (object != nullptr) {
        return KNamedObject::RegisterObject(object);
    }
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t delete_handle(handle_id handle)
{
	if (KNamedObject::FreeHandle(handle)) {
		return 0;
	}
	set_last_error(EINVAL);
	return -1;
}
