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
// Created: 11.07.2020 13:20

#pragma once

#include "System/HandleObject.h"

namespace os
{
class KMutex;

class ObjectWaitGroup : public HandleObject
{
public:
	ObjectWaitGroup(const char* name = "") : HandleObject(create_object_wait_group(name)) {}

	bool AddObject(const HandleObject& object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return object_wait_group_add_object(m_Handle, object.GetHandle(), waitMode) >= 0; }
	bool RemoveObject(const HandleObject& object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return object_wait_group_remove_object(m_Handle, object.GetHandle(), waitMode) >= 0; }

    bool AddFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return object_wait_group_add_file(m_Handle, fileHandle, waitMode) >= 0; }
    bool RemoveFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return object_wait_group_remove_file(m_Handle, fileHandle, waitMode) >= 0; }

	void Clear() { object_wait_group_clear(m_Handle); }

	bool Wait(void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)								{ return object_wait_group_wait(m_Handle, INVALID_HANDLE, readyFlagsBuffer, readyFlagsSize) >= 0; }
    bool WaitTimeout(TimeValMicros timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return object_wait_group_wait_timeout(m_Handle, INVALID_HANDLE, timeout.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize) >= 0; }
	bool WaitDeadline(TimeValMicros deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)	{ return object_wait_group_wait_deadline(m_Handle, INVALID_HANDLE, deadline.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize) >= 0; }

	bool Wait(Mutex& lock, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)								{ return object_wait_group_wait(m_Handle, lock.GetHandle(), readyFlagsBuffer, readyFlagsSize) >= 0; }
	bool WaitTimeout(Mutex& lock, TimeValMicros timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)	{ return object_wait_group_wait_timeout(m_Handle, lock.GetHandle(), timeout.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize) >= 0; }
	bool WaitDeadline(Mutex& lock, TimeValMicros deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return object_wait_group_wait_deadline(m_Handle, lock.GetHandle(), deadline.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize) >= 0; }

private:

};


} // namespace
