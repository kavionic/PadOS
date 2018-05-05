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

#include "KMutex.h"
#include "KHandleArray.h"
#include "Scheduler.h"
#include "System/System.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KMutex::KMutex(const char* name, bool recursive) : KNamedObject(name, KNamedObjectType::Mutex)
{
    m_Count = 0;
    m_Recursive = recursive;
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

bool KMutex::Lock()
{
    KThreadCB* thread = gk_CurrentThread;
    for (;;)
    {
        KThreadWaitNode waitNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count == 0 || (m_Recursive && m_Holder == thread->GetHandle()))
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
            
            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }
            
            if (m_Count == 0)
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

bool KMutex::LockDeadline(bigtime_t deadline)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count == 0 || (m_Recursive && m_Holder == thread->GetHandle()))
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
            
            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }            
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMutex::LockTimeout(bigtime_t timeout)
{
    return LockDeadline((timeout != INFINIT_TIMEOUT) ? (get_system_time() + timeout) : INFINIT_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMutex::TryLock()
{
    KThreadCB* thread = gk_CurrentThread;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (m_Count == 0 || (m_Recursive && m_Holder == thread->GetHandle()))
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

void KMutex::Unlock()
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        m_Count++;

        if (m_Count == 0) {
            m_Holder = -1;
            if (wakeup_wait_queue(&m_WaitQueue, 0, 1)) KSWITCH_CONTEXT();
        }
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMutex::LockShared()
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

            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }
            
            if (m_Count >= 0)
            {
                m_Count++;
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

bool KMutex::LockSharedDeadline(bigtime_t deadline)
{
    KThreadCB* thread = gk_CurrentThread;
    
    for (bool first = true; ; first = false)
    {
        KThreadWaitNode waitNode;
        KThreadWaitNode sleepNode;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (m_Count >= 0)
            {
                m_Count++;
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

            if (waitNode.m_TargetDeleted) {
                set_last_error(EINVAL);
                return false;
            }
            
            if (wakeup_wait_queue(&m_WaitQueue, 0, 0)) KSWITCH_CONTEXT();
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMutex::LockSharedTimeout(bigtime_t timeout)
{
    return LockSharedDeadline((timeout != INFINIT_TIMEOUT) ? (get_system_time() + timeout) : INFINIT_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMutex::TryLockShared()
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (m_Count >= 0)
        {
            m_Count++;
            return true;
        }
    } CRITICAL_END;
    set_last_error(EWOULDBLOCK);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KMutex::UnlockShared()
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        m_Count--;

        if (m_Count == 0) {
            if (wakeup_wait_queue(&m_WaitQueue, 0, 1)) KSWITCH_CONTEXT();
        }
    } CRITICAL_END;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KMutex::IsLocked() const
{
    return m_Count <= 0 && m_Holder == gk_CurrentThread->GetHandle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

sem_id create_mutex(const char* name, bool recursive)
{
    try {
        return KNamedObject::RegisterObject(ptr_new<KMutex>(name, recursive));
    } catch(const std::bad_alloc& error) {
        set_last_error(ENOMEM);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

sem_id duplicate_mutex(sem_id handle)
{
    Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(handle, KMutex::ObjectType));;
    if (mutex != nullptr) {
        return KNamedObject::RegisterObject(mutex);
    }
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t delete_mutex(sem_id handle)
{
    if (KNamedObject::FreeHandle(handle, KMutex::ObjectType)) {
        return 0;
    }
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t lock_mutex(sem_id handle)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::Lock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t lock_mutex_timeout(sem_id handle, bigtime_t timeout)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::LockTimeout, timeout);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t lock_mutex_deadline(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::LockDeadline, deadline);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t try_lock_mutex(sem_id handle)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::TryLock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t unlock_mutex(sem_id handle)
{
    return KNamedObject::ForwardToHandleVoid<KMutex>(handle, &KMutex::Unlock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t lock_mutex_shared(sem_id handle)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::LockShared);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t lock_mutex_shared_timeout(sem_id handle, bigtime_t timeout)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::LockSharedTimeout, timeout);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t lock_mutex_shared_deadline(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::LockSharedDeadline, deadline);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t try_lock_mutex_shared(sem_id handle)
{
    return KNamedObject::ForwardToHandleBoolToInt<KMutex>(handle, &KMutex::TryLockShared);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t unlock_mutex_shared(sem_id handle)
{
    return KNamedObject::ForwardToHandleVoid<KMutex>(handle, &KMutex::UnlockShared);
}
