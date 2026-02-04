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
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFileHandle.h>
#include <System/ExceptionHandling.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KObjectWaitGroup::KObjectWaitGroup(const char* name) : KNamedObject(name, KNamedObjectType::ObjectWaitGroup), m_Mutex(name, PEMutexRecursionMode_RaiseError), m_BlockedThreadCondition(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::AddObject_trw(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    AddObjectInternal_trw(object, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::SetObjects_trw(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    ClearInternal_trw();
    m_Objects.reserve(objects.size());
    for (KWaitableObject* object : objects) {
        AddObjectInternal_trw(object, waitMode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::AppendObjects_trw(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    m_Objects.reserve(m_Objects.size() + objects.size());
    for (KWaitableObject* object : objects) {
        AddObjectInternal_trw(object, waitMode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::RemoveObject_trw(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    RemoveObjectInternal_trw(object, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::Clear_trw()
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    ClearInternal_trw();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::AddObjectInternal_trw(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(m_Mutex.IsLocked());

    m_Objects.emplace_back(object, waitMode);
    object->m_WaitGroups.emplace_back(this, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::RemoveObjectInternal_trw(KWaitableObject* object, ObjectWaitMode waitMode)
{
    kassert(m_Mutex.IsLocked());

    WaitForBlockedThread_trw();

    auto i = std::find(m_Objects.begin(), m_Objects.end(), std::make_pair(object, waitMode));
    if (i == m_Objects.end()) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    auto j = std::find_if(i->first->m_WaitGroups.begin(), i->first->m_WaitGroups.end(), [this, waitMode](const std::pair<KObjectWaitGroup*, ObjectWaitMode>& node) {return node.first == this && node.second == waitMode; });
    if (j != i->first->m_WaitGroups.end()) {
        i->first->m_WaitGroups.erase(j);
    }
    m_Objects.erase(i);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::ClearInternal_trw()
{
    kassert(m_Mutex.IsLocked());
    WaitForBlockedThread_trw();

    while (!m_Objects.empty())
    {
        RemoveObjectInternal_trw(m_Objects.back().first, m_Objects.back().second);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::WaitForBlockedThread_trw(TimeValNanos deadline)
{
    while (m_BlockedThread != nullptr)
    {
        add_thread_to_ready_list(m_BlockedThread);

        ++m_ObjListModsPending;
        const PErrorCode result = m_BlockedThreadCondition.Wait(m_Mutex);
        --m_ObjListModsPending;
        m_BlockedThreadCondition.WakeupAll();
        if (result != PErrorCode::Success && result != PErrorCode::Interrupted) {
            PERROR_THROW_CODE(result);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::Wait_trw(KMutex* lock, TimeValNanos deadline, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    WaitForBlockedThread_trw(deadline);

    KThreadCB* thread = gk_CurrentThread;

    m_WaitNodes.resize(m_Objects.size());

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
                return;
            }
        }
    }
    if (isReady) {
        return;
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
            PERROR_THROW_CODE(result);
        }
    }

    if (!isReady) {
        PERROR_THROW_CODE(didTimeout ? PErrorCode::Timeout : PErrorCode::Interrupted);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::AddFile_trw(int fileHandle, ObjectWaitMode waitMode)
{
    Ptr<KInode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(fileHandle, inode);
    AddObject_trw(inode, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KObjectWaitGroup::RemoveFile_trw(int fileHandle, ObjectWaitMode waitMode)
{
    Ptr<KInode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(fileHandle, inode);
    RemoveObject_trw(inode, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

handle_id kobject_wait_group_create_trw(const char* name)
{
    return KNamedObject::RegisterObject_trw(ptr_new<KObjectWaitGroup>(name));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_delete_trw(handle_id handle)
{
    KNamedObject::FreeHandle_trw(handle, KObjectWaitGroup::ObjectType);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_add_object_trw(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    Ptr<KNamedObject> object = KNamedObject::GetAnyObject_trw(objectHandle);
    KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, static_cast<void(KObjectWaitGroup::*)(KWaitableObject*, ObjectWaitMode)>(&KObjectWaitGroup::AddObject_trw), ptr_raw_pointer_cast(object), waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_remove_object_trw(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode)
{
    Ptr<KNamedObject> object = KNamedObject::GetAnyObject_trw(objectHandle);
    KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, static_cast<void(KObjectWaitGroup::*)(KWaitableObject*, ObjectWaitMode)>(&KObjectWaitGroup::RemoveObject_trw), ptr_raw_pointer_cast(object), waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_add_file_trw(handle_id handle, int fileHandle, ObjectWaitMode waitMode /*= ObjectWaitMode::Read*/)
{
    KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, &KObjectWaitGroup::AddFile_trw, fileHandle, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_remove_file_trw(handle_id handle, int fileHandle, ObjectWaitMode waitMode /*= ObjectWaitMode::Read*/)
{
    KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, &KObjectWaitGroup::RemoveFile_trw, fileHandle, waitMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_clear_trw(handle_id handle)
{
    KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, &KObjectWaitGroup::Clear_trw);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_wait_trw(handle_id handle, handle_id mutexHandle, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    Ptr<KMutex> mutex;
    if (mutexHandle != INVALID_HANDLE) {
        mutex = KNamedObject::GetObject_trw<KMutex>(mutexHandle);
    }
    if (mutex != nullptr) {
        KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, static_cast<void(KObjectWaitGroup::*)(KMutex&, void*, size_t)>(&KObjectWaitGroup::Wait_trw), *mutex, readyFlagsBuffer, readyFlagsSize);
    } else {
        KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, static_cast<void(KObjectWaitGroup::*)(void*, size_t)>(&KObjectWaitGroup::Wait_trw), readyFlagsBuffer, readyFlagsSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_wait_timeout_ns_trw(handle_id handle, handle_id mutexHandle, bigtime_t timeout, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    kobject_wait_group_wait_deadline_ns_trw(handle, mutexHandle, (timeout != TimeValNanos::infinit.AsNanoseconds()) ? (kget_monotonic_time_ns() + timeout) : TimeValNanos::infinit.AsNanoseconds(), readyFlagsBuffer, readyFlagsSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kobject_wait_group_wait_deadline_ns_trw(handle_id handle, handle_id mutexHandle, bigtime_t deadline, void* readyFlagsBuffer, size_t readyFlagsSize)
{
    Ptr<KMutex> mutex;
    if (mutexHandle != INVALID_HANDLE)
    {
        mutex = KNamedObject::GetObject_trw<KMutex>(mutexHandle);
    }
    if (mutex != nullptr) {
        KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, static_cast<void(KObjectWaitGroup::*)(KMutex&, TimeValNanos, void*, size_t)>(&KObjectWaitGroup::WaitDeadline_trw), *mutex, TimeValNanos::FromNanoseconds(deadline), readyFlagsBuffer, readyFlagsSize);
    } else {
        KNamedObject::ForwardToHandle_trw<KObjectWaitGroup>(handle, static_cast<void(KObjectWaitGroup::*)(TimeValNanos, void*, size_t)>(&KObjectWaitGroup::WaitDeadline_trw), TimeValNanos::FromNanoseconds(deadline), readyFlagsBuffer, readyFlagsSize);
    }
}

} // namespace kernel
