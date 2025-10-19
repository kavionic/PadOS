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

    PErrorCode AddObject(KWaitableObject* object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    template<typename T> PErrorCode AddObject(Ptr<T> object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return AddObject(ptr_raw_pointer_cast(object), waitMode); }

	PErrorCode SetObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	PErrorCode AppendObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	PErrorCode RemoveObject(KWaitableObject* object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    template<typename T> PErrorCode RemoveObject(Ptr<T> object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return RemoveObject(ptr_raw_pointer_cast(object), waitMode); }

    PErrorCode AddFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    PErrorCode RemoveFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);

    PErrorCode Clear();

	PErrorCode Wait(void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
	PErrorCode WaitTimeout(TimeValNanos timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
	PErrorCode WaitDeadline(TimeValNanos deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, deadline, readyFlagsBuffer, readyFlagsSize); }

	PErrorCode Wait(KMutex& lock, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
	PErrorCode WaitTimeout(KMutex& lock, TimeValNanos timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit, readyFlagsBuffer, readyFlagsSize); }
	PErrorCode WaitDeadline(KMutex& lock, TimeValNanos deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, deadline, readyFlagsBuffer, readyFlagsSize); }

private:
    PErrorCode  AddObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode);
    PErrorCode  RemoveObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode);
    void        ClearInternal();
    PErrorCode  WaitForBlockedThread(TimeValNanos deadline = TimeValNanos::infinit);
    PErrorCode  Wait(KMutex* lock, TimeValNanos deadline, void* readyFlagsBuffer, size_t readyFlagsSize);

	KMutex m_Mutex;

	std::vector<std::pair<KWaitableObject*, ObjectWaitMode>> m_Objects;
	std::vector<KThreadWaitNode>    m_WaitNodes;
	KThreadWaitNode				    m_SleepNode;
    KConditionVariable              m_BlockedThreadCondition;
    KThreadCB* volatile             m_BlockedThread = nullptr;
    std::atomic_int32_t             m_ObjListModsPending = 0;
};


} // namespace
