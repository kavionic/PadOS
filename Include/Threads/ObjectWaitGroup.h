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
	ObjectWaitGroup(const char* name = "")
    {
        handle_id handle;
        if (create_object_wait_group(handle, name) == PErrorCode::Success) {
            SetHandle(handle);
        }
    }

    bool AddObject(const HandleObject& object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return ParseResult(object_wait_group_add_object(m_Handle, object.GetHandle(), waitMode)); }
	bool RemoveObject(const HandleObject& object, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return ParseResult(object_wait_group_remove_object(m_Handle, object.GetHandle(), waitMode)); }

    bool AddFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return ParseResult(object_wait_group_add_file(m_Handle, fileHandle, waitMode)); }
    bool RemoveFile(int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read) { return ParseResult(object_wait_group_remove_file(m_Handle, fileHandle, waitMode)); }

	void Clear() { object_wait_group_clear(m_Handle); }

	bool Wait(void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)								    { return ParseResult(object_wait_group_wait(m_Handle, INVALID_HANDLE, readyFlagsBuffer, readyFlagsSize)); }
    bool WaitTimeout(TimeValMicros timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)    { return ParseResult(object_wait_group_wait_timeout(m_Handle, INVALID_HANDLE, timeout.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize)); }
	bool WaitDeadline(TimeValMicros deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)	{ return ParseResult(object_wait_group_wait_deadline(m_Handle, INVALID_HANDLE, deadline.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize)); }

	bool Wait(Mutex& lock, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)								    { return ParseResult(object_wait_group_wait(m_Handle, lock.GetHandle(), readyFlagsBuffer, readyFlagsSize)); }
	bool WaitTimeout(Mutex& lock, TimeValMicros timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0)	{ return ParseResult(object_wait_group_wait_timeout(m_Handle, lock.GetHandle(), timeout.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize)); }
	bool WaitDeadline(Mutex& lock, TimeValMicros deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0) { return ParseResult(object_wait_group_wait_deadline(m_Handle, lock.GetHandle(), deadline.AsMicroSeconds(), readyFlagsBuffer, readyFlagsSize)); }

private:
    bool ParseResult(PErrorCode result) const
    {
        if (result == PErrorCode::Success)
        {
            return true;
        }
        else
        {
            set_last_error(result);
            return false;
        }
    }
};


} // namespace
