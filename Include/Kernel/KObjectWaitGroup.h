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
#include <Kernel/KConditionVariable.h>

namespace kernel
{
class KMutex;

class KObjectWaitGroup : public KNamedObject
{
public:
	static constexpr KNamedObjectType ObjectType = KNamedObjectType::ObjectWaitGroup;

    IFLASHC KObjectWaitGroup(const char* name);

    IFLASHC bool AddObject(KWaitableObject* object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    template<typename T> bool AddObject(Ptr<T> object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return AddObject(ptr_raw_pointer_cast(object), waitMode); }

	IFLASHC bool SetObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	IFLASHC bool AppendObjects(const std::vector<KWaitableObject*>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	IFLASHC bool RemoveObject(KWaitableObject* object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    template<typename T> bool RemoveObject(Ptr<T> object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return RemoveObject(ptr_raw_pointer_cast(object), waitMode); }

    IFLASHC bool AddFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
    IFLASHC bool RemoveFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);

    IFLASHC void Clear();

	bool Wait(void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, TimeValMicros::infinit, readyFlagsBuffer, readyFlagsSize); }
	bool WaitTimeout(TimeValMicros timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, (!timeout.IsInfinit()) ? (get_system_time() + timeout) : TimeValMicros::infinit, readyFlagsBuffer, readyFlagsSize); }
	bool WaitDeadline(TimeValMicros deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, deadline, readyFlagsBuffer, readyFlagsSize); }

	bool Wait(KMutex& lock, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, TimeValMicros::infinit, readyFlagsBuffer, readyFlagsSize); }
	bool WaitTimeout(KMutex& lock, TimeValMicros timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, (!timeout.IsInfinit()) ? (get_system_time() + timeout) : TimeValMicros::infinit, readyFlagsBuffer, readyFlagsSize); }
	bool WaitDeadline(KMutex& lock, TimeValMicros deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, deadline, readyFlagsBuffer, readyFlagsSize); }

private:
    IFLASHC bool AddObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode);
    IFLASHC bool RemoveObjectInternal(KWaitableObject* object, ObjectWaitMode waitMode);
    IFLASHC void ClearInternal();
    IFLASHC bool WaitForBlockedThread(TimeValMicros deadline = TimeValMicros::infinit);
    IFLASHC bool Wait(KMutex* lock, TimeValMicros deadline, void* readyFlagsBuffer, size_t readyFlagsSize);

	KMutex m_Mutex;

	std::vector<std::pair<KWaitableObject*, ObjectWaitMode>> m_Objects;
	std::vector<KThreadWaitNode>    m_WaitNodes;
	KThreadWaitNode				    m_SleepNode;
    KConditionVariable              m_BlockedThreadCondition;
    KThreadCB* volatile             m_BlockedThread = nullptr;
    std::atomic_int32_t             m_ObjListModsPending = 0;
};


} // namespace
