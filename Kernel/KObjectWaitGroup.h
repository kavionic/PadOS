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

namespace kernel
{
class KMutex;

class KObjectWaitGroup : public KNamedObject
{
public:
	static constexpr KNamedObjectType ObjectType = KNamedObjectType::ObjectWaitGroup;

	KObjectWaitGroup(const char* name);

	bool AddObject(Ptr<KNamedObject> object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	bool SetObjects(const std::vector<Ptr<KNamedObject>>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	bool AppendObjects(const std::vector<Ptr<KNamedObject>>& objects, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	bool RemoveObject(Ptr<KNamedObject> object, ObjectWaitMode waitMode = ObjectWaitMode::Read);
	void Clear();

	bool Wait(void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, INFINIT_TIMEOUT, readyFlagsBuffer, readyFlagsSize); }
	bool WaitTimeout(bigtime_t timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, (timeout != INFINIT_TIMEOUT) ? (get_system_time() + timeout) : INFINIT_TIMEOUT, readyFlagsBuffer, readyFlagsSize); }
	bool WaitDeadline(bigtime_t deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(nullptr, deadline, readyFlagsBuffer, readyFlagsSize); }

	bool Wait(KMutex& lock, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, 0, readyFlagsBuffer, readyFlagsSize); }
	bool WaitTimeout(KMutex& lock, bigtime_t timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, (timeout != INFINIT_TIMEOUT) ? (get_system_time() + timeout) : INFINIT_TIMEOUT, readyFlagsBuffer, readyFlagsSize); }
	bool WaitDeadline(KMutex& lock, bigtime_t deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return Wait(&lock, deadline, readyFlagsBuffer, readyFlagsSize); }

private:
	bool Wait(KMutex* lock, bigtime_t deadline, void* readyFlagsBuffer, size_t readyFlagsSize);

	KMutex m_Mutex;

	std::vector<std::pair<Ptr<KNamedObject>, ObjectWaitMode>> m_Objects;
	std::vector<KThreadWaitNode>   m_WaitNodes;
	KThreadWaitNode				   m_SleepNode;
};


} // namespace
