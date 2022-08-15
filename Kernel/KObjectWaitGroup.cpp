// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 24.01.2020 14:00

#include <string.h>

#include <Kernel/KObjectWaitGroup.h>
#include <Kernel/KMutex.h>
#include <Kernel/Scheduler.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KINode.h>
#include <Kernel/VFS/KFileHandle.h>

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KObjectWaitGroup::KObjectWaitGroup(const char* name) : KNamedObject(name, KNamedObjectType::ObjectWaitGroup), m_Mutex(name), m_BlockedThreadCondition(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::AddObject(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return AddObjectInternal(object, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::SetObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    try {
        ClearInternal();
		m_Objects.reserve(objects.size());
		for (KWaitableObject* object : objects) {
            AddObjectInternal(object, waitMode);
		}
		return true;
	} catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::AppendObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    try {
		m_Objects.reserve(m_Objects.size() + objects.size());
        for (KWaitableObject* object : objects) {
            AddObjectInternal(object, waitMode);
        }
		return true;
	} catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::RemoveObject(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return RemoveObjectInternal(object, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::Clear()
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    ClearInternal();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::AddObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(m_Mutex.IsLocked());

    try {
        m_Objects.emplace_back(object, waitMode);
        object->m_WaitGroups.emplace_back(this, waitMode);
        return true;
    }
    catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::RemoveObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(m_Mutex.IsLocked());

    WaitForBlockedThread();

    auto i = std::find(m_Objects.begin(), m_Objects.end(), std::make_pair(object, waitMode));
    if (i != m_Objects.end())
    {
        auto j = std::find_if(i->first->m_WaitGroups.begin(), i->first->m_WaitGroups.end(), [this, waitMode](const std::pair<KObjectWaitGroup*, ObjectWaitMode>& node) {return node.first == this && node.second == waitMode; });
        if (j != i->first->m_WaitGroups.end()) {
            i->first->m_WaitGroups.erase(j);
        }
        m_Objects.erase(i);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::ClearInternal()
{
    kassert(m_Mutex.IsLocked());
    WaitForBlockedThread();

    while (!m_Objects.empty())
    {
        RemoveObjectInternal(m_Objects.back().first, m_Objects.back().second);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::WaitForBlockedThread(TimeValMicros deadline)
{
    while (m_BlockedThread != nullptr)
    {
        add_thread_to_ready_list(m_BlockedThread);

        ++m_ObjListModsPending;
        const bool result = m_BlockedThreadCondition.Wait(m_Mutex);
        --m_ObjListModsPending;
        m_BlockedThreadCondition.WakeupAll();
        if (!result)
        {
            if (get_last_error() != EAGAIN) {
                return false;
            }
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::Wait(KMutex* lock, TimeValMicros deadline, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    if (!WaitForBlockedThread(deadline)) {
        return false;
    }

    KThreadCB* thread = gk_CurrentThread;

    try {
        m_WaitNodes.resize(m_Objects.size());
    } catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }

    size_t maxFlagIndex = (readyFlagsBuffer != nullptr) ? std::min(m_WaitNodes.size(), readyFlagsSize * 8) : 0;

    memset(readyFlagsBuffer, 0, (maxFlagIndex + 7) & ~7);

    uint32_t* readyFlags = reinterpret_cast<uint32_t*>(readyFlagsBuffer);

    bool isReady = false;
    for (int i = 0; i < m_Objects.size(); ++i)
    {
        m_WaitNodes[i].m_Thread = thread;
        if (!m_Objects[i].first->AddListener(&m_WaitNodes[i], m_Objects[i].second))
        {
            isReady = true;
            if (i < maxFlagIndex)
            {
                readyFlags[i / 32] |= 1 << (i % 32);
            }
            else
            {
                for (int j = i - 1; j >= 0; --j) {
                    m_WaitNodes[j].Detatch();
                }
                return true;
            }
        }
    }
    if (isReady) {
        return true;
    }

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        for (const KThreadWaitNode& node : m_WaitNodes)
        {
            if (node.m_List == nullptr) {
                isReady = true;
                break;
            }
        }
        if (!isReady)
        {
            if (!deadline.IsInfinit())
            {
                thread->m_State = ThreadState::Sleeping;

                m_SleepNode.m_Thread = thread;
                m_SleepNode.m_ResumeTime = deadline;
                add_to_sleep_list(&m_SleepNode);
            }
            else
            {
                thread->m_State = ThreadState::Waiting;
            }
            thread->m_BlockingObject = this;
            m_BlockedThread = thread;
            if (lock != nullptr) lock->Unlock();
            KSWITCH_CONTEXT(); // Make sure we are suspended the moment we re-enable interrupts
        }
    } CRITICAL_END;

    if (!isReady && lock != nullptr) lock->Lock();

    m_BlockedThread = nullptr;
    m_BlockedThreadCondition.WakeupAll();

    bool didTimeout = false;
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        didTimeout = !m_SleepNode.Detatch() && !deadline.IsInfinit();
        for (int i = 0; i < m_Objects.size(); ++i)
        {
            KThreadWaitNode& waitNode = m_WaitNodes[i];

            // We have a strong-reference to each member of the group. So if any of them died, something has gone very wrong.
            kassure(!waitNode.m_TargetDeleted, "ERROR: KObjectWaitGroup::Wait(%s) member of the group deleted while waiting.\n", GetName());
            if (!waitNode.Detatch())
            {
                isReady = true;
				if (i < maxFlagIndex) {
					readyFlags[i / 32] |= 1 << (i % 32);
				}
            }
        }
        thread->m_BlockingObject = nullptr;
    } CRITICAL_END;

    if (!didTimeout && m_ObjListModsPending != 0)
    {
        // Give the threads attempting to add/remove objects a chance to work before locking them out again.
        if (!m_BlockedThreadCondition.WaitDeadline(m_Mutex, deadline)) {
            return false;
        }
    }

    if (!isReady) {
        set_last_error((didTimeout) ? ETIME : EAGAIN);
    }
    return isReady;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::AddFile(int fileHandle, ObjectWaitMode waitMode)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = FileIO::GetFile(fileHandle, inode);
    if (file == nullptr) {
        return false;
    }
    return AddObject(inode, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KObjectWaitGroup::RemoveFile(int fileHandle, ObjectWaitMode waitMode)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = FileIO::GetFile(fileHandle, inode);
    if (file == nullptr) {
        return false;
    }
    return RemoveObject(inode, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

handle_id create_object_wait_group(const char* name)
{
    try {
        return KNamedObject::RegisterObject(ptr_new<KObjectWaitGroup>(name));
    }
    catch (const std::bad_alloc& error) {
        set_last_error(ENOMEM);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t  object_wait_group_add_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    Ptr<KNamedObject> object = KNamedObject::GetAnyObject(objectHandle);
    if (object == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    return KNamedObject::ForwardToHandleBoolToInt<KObjectWaitGroup>(handle, static_cast<bool(KObjectWaitGroup::*)(KWaitableObject*, ObjectWaitMode)>(&KObjectWaitGroup::AddObject), ptr_raw_pointer_cast(object), waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t  object_wait_group_remove_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    Ptr<KNamedObject> object = KNamedObject::GetAnyObject(objectHandle);
    if (object == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    return KNamedObject::ForwardToHandleBoolToInt<KObjectWaitGroup>(handle, static_cast<bool(KObjectWaitGroup::*)(KWaitableObject*, ObjectWaitMode)>(&KObjectWaitGroup::RemoveObject), ptr_raw_pointer_cast(object), waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t  object_wait_group_clear(handle_id handle)
{
    return KNamedObject::ForwardToHandleVoid<KObjectWaitGroup>(handle, &KObjectWaitGroup::Clear);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t object_wait_group_wait(handle_id handle, handle_id mutexHandle, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    Ptr<KMutex> mutex;
    if (mutexHandle != INVALID_HANDLE)
    {
        mutex = KNamedObject::GetObject<KMutex>(mutexHandle);
        if (mutex == nullptr) {
            set_last_error(EINVAL);
            return -1;
        }
    }
    if (mutex != nullptr) {
        return KNamedObject::ForwardToHandleBoolToInt<KObjectWaitGroup>(handle, static_cast<bool(KObjectWaitGroup::*)(KMutex&, void*, size_t)>(&KObjectWaitGroup::Wait), *mutex, readyFlagsBuffer, readyFlagsSize);
    } else {
        return KNamedObject::ForwardToHandleBoolToInt<KObjectWaitGroup>(handle, static_cast<bool(KObjectWaitGroup::*)(void*, size_t)>(&KObjectWaitGroup::Wait), readyFlagsBuffer, readyFlagsSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t object_wait_group_wait_timeout(handle_id handle, handle_id mutexHandle, bigtime_t timeout, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    return object_wait_group_wait_deadline(handle, mutexHandle, (timeout != TimeValMicros::infinit.AsMicroSeconds()) ? (get_system_time().AsMicroSeconds() + timeout) : TimeValMicros::infinit.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t object_wait_group_wait_deadline(handle_id handle, handle_id mutexHandle, bigtime_t deadline, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    Ptr<KMutex> mutex;
    if (mutexHandle != INVALID_HANDLE)
    {
        mutex = KNamedObject::GetObject<KMutex>(mutexHandle);
        if (mutex == nullptr) {
            set_last_error(EINVAL);
            return -1;
        }
    }
    if (mutex != nullptr) {
        return KNamedObject::ForwardToHandleBoolToInt<KObjectWaitGroup>(handle, static_cast<bool(KObjectWaitGroup::*)(KMutex&, TimeValMicros, void*, size_t)>(&KObjectWaitGroup::WaitDeadline), *mutex, TimeValMicros::FromMicroseconds(deadline), readyFlagsBuffer, readyFlagsSize);
    } else {
        return KNamedObject::ForwardToHandleBoolToInt<KObjectWaitGroup>(handle, static_cast<bool(KObjectWaitGroup::*)(TimeValMicros, void*, size_t)>(&KObjectWaitGroup::WaitDeadline), TimeValMicros::FromMicroseconds(deadline), readyFlagsBuffer, readyFlagsSize);
    }
}
