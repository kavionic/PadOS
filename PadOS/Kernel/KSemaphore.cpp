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
// Created: 04.03.2018 22:38:38

#include <string.h>

#include "KSemaphore.h"
#include "KHandleArray.h"
#include "Scheduler.h"
#include "System/System.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSemaphore::KSemaphore(const char* name, int count, bool recursive) : KNamedObject(name, KNamedObjectType::Semaphore)
{
    m_Count = count;
    m_Recursive = recursive;
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
            if (m_Count > 0 || (m_Recursive && m_Holder == thread->GetHandle()))
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return true;
            }
            waitNode.m_Thread = thread;
            thread->m_State = KThreadState::Waiting;
            m_WaitQueue.Append(&waitNode);
            KSWITCH_CONTEXT();
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.        
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
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

bool KSemaphore::AcquireDeadline(bigtime_t deadline)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count > 0 || (m_Recursive && m_Holder == thread->GetHandle()))
            {
                m_Count--;
                m_Holder = thread->GetHandle();
                return true;
            }
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
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
            sleepNode.Detatch();
        } CRITICAL_END;
    }
}

bool KSemaphore::AcquireTimeout(bigtime_t timeout)
{
    return AcquireDeadline((timeout != INFINIT_TIMEOUT) ? (get_system_time() + timeout) : INFINIT_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KSemaphore::TryAcquire()
{
    KThreadCB* thread = gk_CurrentThread;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (m_Count > 0 || (m_Recursive && m_Holder == thread->GetHandle()))
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

sem_id create_semaphore(const char* name, int count, bool recursive)
{
    try {
        return KNamedObject::RegisterObject(ptr_new<KSemaphore>(name, count, recursive));
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
    Ptr<KSemaphore> sema = KGetSemaphore(handle);
    if (sema != nullptr) {
        return sema->Acquire() ? 0 : -1;
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t acquire_semaphore_timeout(sem_id handle, bigtime_t timeout)
{
    Ptr<KSemaphore> sema = KGetSemaphore(handle);
    if (sema != nullptr) {
        return sema->AcquireTimeout(timeout) ? 0 : -1;
    } else {
        set_last_error(EINVAL);
        return -1;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t acquire_semaphore_deadline(sem_id handle, bigtime_t deadline)
{
    Ptr<KSemaphore> sema = KGetSemaphore(handle);
    if (sema != nullptr) {
        return sema->AcquireDeadline(deadline) ? 0 : -1;
    } else {
        set_last_error(EINVAL);
        return -1;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t try_acquire_semaphore(sem_id handle)
{
    Ptr<KSemaphore> sema = KGetSemaphore(handle);
    if (sema != nullptr) {
        return sema->TryAcquire() ? 0 : -1;
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t release_semaphore(sem_id handle)
{
    Ptr<KSemaphore> sema = KGetSemaphore(handle);
    if (sema != nullptr) {
        sema->Release();
        return 0;
    } else {
        set_last_error(EINVAL);
        return -1;
    }
}
