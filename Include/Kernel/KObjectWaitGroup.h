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

#pragma once
#include <vector>

#include "KNamedObject.h"
#include "KMutex.h"
#include "Ptr/Ptr.h"
#include <Kernel/KTime.h>
#include <Kernel/KConditionVariable.h>

namespace kernel
{
class KMutex;

class KObjectWaitGroup : public KNamedObject
{
public:
    static constexpr KNamedObjectType ObjectType = KNamedObjectType::ObjectWaitGroup;

    KObjectWaitGroup(const char* name);

    void AddObject_trw(KWaitableObject* object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    template<typename T> void AddObject_trw(Ptr<T> object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { AddObject_trw(ptr_raw_pointer_cast(object), waitMode); }

    void        SetObjects_trw(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    void        AppendObjects_trw(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    void        RemoveObject_trw(KWaitableObject* object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    template<typename T> void RemoveObject_trw(Ptr<T> object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { RemoveObject_trw(ptr_raw_pointer_cast(object), waitMode); }

    void AddFile_trw(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    void RemoveFile_trw(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);

    void Clear_trw();

    void Wait_trw(void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { Wait_trw(nullptr, TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
    void WaitTimeout_trw(TimeValNanos timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { Wait_trw(nullptr, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
    void WaitDeadline_trw(TimeValNanos deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { Wait_trw(nullptr, deadline, readyFlagsBuffer, readyFlagsSize); }

    void Wait_trw(KMutex& lock, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { Wait_trw(&lock, TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
    void WaitTimeout_trw(KMutex& lock, TimeValNanos timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { Wait_trw(&lock, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
    void WaitDeadline_trw(KMutex& lock, TimeValNanos deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { Wait_trw(&lock, deadline, readyFlagsBuffer, readyFlagsSize); }

private:
    void AddObjectInternal_trw(KWaitableObject* object, ObjectWaitMode waitMode);
    void RemoveObjectInternal_trw(KWaitableObject* object, ObjectWaitMode waitMode);
    void ClearInternal_trw();
    void WaitForBlockedThread_trw(TimeValNanos deadline = TimeValNanos::infinit);
    void Wait_trw(KMutex* lock, TimeValNanos deadline, void* readyFlagsBuffer, size_t readyFlagsSize);

    KMutex m_Mutex;

    std::vector<std::pair<KWaitableObject*, ObjectWaitMode>> m_Objects;
    std::vector<KThreadWaitNode>    m_WaitNodes;
    KThreadWaitNode                 m_SleepNode;
    KConditionVariable              m_BlockedThreadCondition;
    KThreadCB* volatile             m_BlockedThread = nullptr;
    std::atomic_int32_t             m_ObjListModsPending = 0;
};

handle_id   kobject_wait_group_create_trw(const char* name);
void        kobject_wait_group_delete_trw(handle_id handle);
void        kobject_wait_group_add_object_trw(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
void        kobject_wait_group_remove_object_trw(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
void        kobject_wait_group_add_file_trw(handle_id handle, int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
void        kobject_wait_group_remove_file_trw(handle_id handle, int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
void        kobject_wait_group_clear_trw(handle_id handle);
void        kobject_wait_group_wait_trw(handle_id handle, handle_id mutexHandle, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);
void        kobject_wait_group_wait_timeout_ns_trw(handle_id handle, handle_id mutexHandle, bigtime_t timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);
void        kobject_wait_group_wait_deadline_ns_trw(handle_id handle, handle_id mutexHandle, bigtime_t deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);

} // namespace
