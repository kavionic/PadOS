// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 25.04.2018 20:52:53

#pragma once

#include <sys/pados_syscalls.h>

#include <Threads/Threads.h>
#include <System/HandleObject.h>

namespace os
{


class Mutex : public HandleObject
{
public:
    enum class NoInit {};

    explicit Mutex(NoInit) : HandleObject(INVALID_HANDLE) {}

    Mutex(const char* name, PEMutexRecursionMode recursionMode, clockid_t clockID = CLOCK_MONOTONIC)
    {
        handle_id handle;
        if (mutex_create(&handle, name, recursionMode, clockID) == PErrorCode::Success) {
            SetHandle(handle);
        }
    }
    ~Mutex() { mutex_delete(m_Handle); SetHandle(INVALID_HANDLE); }
    bool Lock()
    {
        return ParseResult(mutex_lock(m_Handle));
    }
    bool LockTimeout(TimeValNanos timeout)
    {
        return ParseResult(mutex_lock_timeout_ns(m_Handle, timeout.AsNanoseconds()));
    }
    bool LockDeadline(TimeValNanos deadline) { return ParseResult(mutex_lock_deadline_ns(m_Handle, deadline.AsNanoseconds())); }
    bool TryLock() { return ParseResult(mutex_try_lock(m_Handle)); }
    bool Unlock() { return ParseResult(mutex_unlock(m_Handle)); }
    bool IsLocked() const
    {
        const PErrorCode result = mutex_islocked(m_Handle);
        if (result == PErrorCode::Busy)
        {
            return true;
        }
        else if (result == PErrorCode::Success)
        {
            return false;
        }
        else
        {
            set_last_error(result);
            return false;
        }
    }

    Mutex(Mutex&& other) = default;
    Mutex(const Mutex& other) = default;
    Mutex& operator=(const Mutex& other) = default;

private:
};

class MutexObjGuard
{
    public:
    MutexObjGuard(Mutex& sema) : m_Mutex(&sema) { m_Mutex->Lock(); }
    ~MutexObjGuard() { if (m_Mutex != nullptr) m_Mutex->Unlock(); }

    MutexObjGuard(MutexObjGuard&& other) : m_Mutex(other.m_Mutex) { m_Mutex = nullptr; }

    private:
    Mutex* m_Mutex;

    MutexObjGuard(MutexObjGuard& other)  = delete;
};

inline MutexObjGuard    critical_create_guard(Mutex& sema) { return MutexObjGuard(sema); }

} // namespace

using PMutex = os::Mutex;
