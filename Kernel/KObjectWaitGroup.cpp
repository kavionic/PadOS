// This file is part of PadOS.
//
// Copyright (C) 2020-2024 Kurt Skauen <http://kavionic.com/>
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
#include <Kernel/Syscalls.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KINode.h>
#include <Kernel/VFS/KFileHandle.h>
#include <System/ExceptionHandling.h>

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KObjectWaitGroup::KObjectWaitGroup(const char* name) : KNamedObject(name, KNamedObjectType::ObjectWaitGroup), m_Mutex(name, PEMutexRecursionMode_RaiseError), m_BlockedThreadCondition(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::AddObject(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return AddObjectInternal(object, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::SetObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    try {
        ClearInternal();
		m_Objects.reserve(objects.size());
		for (KWaitableObject* object : objects) {
            AddObjectInternal(object, waitMode);
		}
		return PErrorCode::Success;
	} catch (const std::bad_alloc&) {
        return PErrorCode::NoMemory;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::AppendObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    try {
		m_Objects.reserve(m_Objects.size() + objects.size());
        for (KWaitableObject* object : objects) {
            AddObjectInternal(object, waitMode);
        }
		return PErrorCode::Success;
	} catch (const std::bad_alloc&) {
        return PErrorCode::NoMemory;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::RemoveObject(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return RemoveObjectInternal(object, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::Clear()
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    ClearInternal();
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::AddObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(m_Mutex.IsLocked());

    try {
        m_Objects.emplace_back(object, waitMode);
        object->m_WaitGroups.emplace_back(this, waitMode);
        return PErrorCode::Success;
    }
    catch (const std::bad_alloc&) {
        return PErrorCode::NoMemory;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::RemoveObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode)
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
        return PErrorCode::Success;
    }
    return PErrorCode::InvalidArg;
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

PErrorCode KObjectWaitGroup::WaitForBlockedThread(TimeValNanos deadline)
{
    while (m_BlockedThread != nullptr)
    {
        add_thread_to_ready_list(m_BlockedThread);

        ++m_ObjListModsPending;
        const PErrorCode result = m_BlockedThreadCondition.Wait(m_Mutex);
        --m_ObjListModsPending;
        m_BlockedThreadCondition.WakeupAll();
        if (result != PErrorCode::Success && result != PErrorCode::Interrupted)
        {
            return result;
        }
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::Wait(KMutex* lock, TimeValNanos deadline, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    PErrorCode result = WaitForBlockedThread(deadline);
    if (result != PErrorCode::Success) {
        return result;
    }

    KThreadCB* thread = gk_CurrentThread;

    try {
        m_WaitNodes.resize(m_Objects.size());
    } catch (const std::bad_alloc&) {
        return PErrorCode::NoMemory;
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
                return PErrorCode::Success;
            }
        }
    }
    if (isReady) {
        return PErrorCode::Success;
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
                thread->m_State = ThreadState_Sleeping;

                m_SleepNode.m_Thread = thread;
                m_SleepNode.m_ResumeTime = deadline;
                add_to_sleep_list(&m_SleepNode);
            }
            else
            {
                thread->m_State = ThreadState_Waiting;
            }
            thread->SetBlockingObject(this);
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
        thread->SetBlockingObject(nullptr);
    } CRITICAL_END;

    if (!didTimeout && m_ObjListModsPending != 0)
    {
        // Give the threads attempting to add/remove objects a chance to work before locking them out again.
        const PErrorCode result = m_BlockedThreadCondition.WaitDeadline(m_Mutex, deadline);
        if (result != PErrorCode::Success) {
            return result;
        }
    }

    if (!isReady) {
        return didTimeout ? PErrorCode::Timeout : PErrorCode::Interrupted;
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::AddFile(int fileHandle, ObjectWaitMode waitMode)
{
    try
    {
        Ptr<KINode> inode;
        Ptr<KFileNode> file = kget_file_node_trw(fileHandle, inode);
        return AddObject(inode, waitMode);
    }
    catch (const std::system_error& error)
    {
        return PErrorCode(error.code().value());
    }
    catch (const std::bad_alloc& error)
    {
        return PErrorCode::NoMemory;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KObjectWaitGroup::RemoveFile(int fileHandle, ObjectWaitMode waitMode)
{
    try
    {
        Ptr<KINode> inode;
        Ptr<KFileNode> file = kget_file_node_trw(fileHandle, inode);
        return RemoveObject(inode, waitMode);
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode create_object_wait_group(handle_id& outHandle, const char* name)
{
    try {
        return KNamedObject::RegisterObject(outHandle, ptr_new<KObjectWaitGroup>(name));
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_add_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    Ptr<KNamedObject> object = KNamedObject::GetAnyObject(objectHandle);
    if (object == nullptr) {
        return PErrorCode::InvalidArg;
    }
    return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KObjectWaitGroup::*)(KWaitableObject*, ObjectWaitMode)>(&KObjectWaitGroup::AddObject), ptr_raw_pointer_cast(object), waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_remove_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    Ptr<KNamedObject> object = KNamedObject::GetAnyObject(objectHandle);
    if (object == nullptr) {
        return PErrorCode::InvalidArg;
    }
    return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KObjectWaitGroup::*)(KWaitableObject*, ObjectWaitMode)>(&KObjectWaitGroup::RemoveObject), ptr_raw_pointer_cast(object), waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_add_file(handle_id handle, int fileHandle, ObjectWaitMode waitMode /*= ObjectWaitMode::Read*/)
{
    return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, &KObjectWaitGroup::AddFile, fileHandle, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_remove_file(handle_id handle, int fileHandle, ObjectWaitMode waitMode /*= ObjectWaitMode::Read*/)
{
    return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, &KObjectWaitGroup::RemoveFile, fileHandle, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_clear(handle_id handle)
{
    return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, &KObjectWaitGroup::Clear);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_wait(handle_id handle, handle_id mutexHandle, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    Ptr<KMutex> mutex;
    if (mutexHandle != INVALID_HANDLE)
    {
        mutex = KNamedObject::GetObject<KMutex>(mutexHandle);
        if (mutex == nullptr) {
            return PErrorCode::InvalidArg;
        }
    }
    if (mutex != nullptr) {
        return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KObjectWaitGroup::*)(KMutex&, void*, size_t)>(&KObjectWaitGroup::Wait), *mutex, readyFlagsBuffer, readyFlagsSize);
    } else {
        return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KObjectWaitGroup::*)(void*, size_t)>(&KObjectWaitGroup::Wait), readyFlagsBuffer, readyFlagsSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_wait_timeout_ns(handle_id handle, handle_id mutexHandle, bigtime_t timeout, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    return object_wait_group_wait_deadline_ns(handle, mutexHandle, (timeout != TimeValNanos::infinit.AsNanoseconds()) ? (kget_monotonic_time_ns() + timeout) : TimeValNanos::infinit.AsNanoseconds(), readyFlagsBuffer, readyFlagsSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode object_wait_group_wait_deadline_ns(handle_id handle, handle_id mutexHandle, bigtime_t deadline, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    Ptr<KMutex> mutex;
    if (mutexHandle != INVALID_HANDLE)
    {
        mutex = KNamedObject::GetObject<KMutex>(mutexHandle);
        if (mutex == nullptr) {
            return PErrorCode::InvalidArg;
        }
    }
    if (mutex != nullptr) {
        return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KObjectWaitGroup::*)(KMutex&, TimeValNanos, void*, size_t)>(&KObjectWaitGroup::WaitDeadline), *mutex, TimeValNanos::FromNanoseconds(deadline), readyFlagsBuffer, readyFlagsSize);
    } else {
        return KNamedObject::ForwardToHandle<KObjectWaitGroup>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KObjectWaitGroup::*)(TimeValNanos, void*, size_t)>(&KObjectWaitGroup::WaitDeadline), TimeValNanos::FromNanoseconds(deadline), readyFlagsBuffer, readyFlagsSize);
    }
}
