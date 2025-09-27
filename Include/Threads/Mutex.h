// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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
        if (sys_mutex_create(&handle, name, recursionMode, clockID) == PErrorCode::Success) {
            SetHandle(handle);
        }
    }
    ~Mutex() { sys_mutex_delete(m_Handle); }
    bool Lock()
    {
        return ParseResult(sys_mutex_lock(m_Handle));
    }
    bool LockTimeout(bigtime_t timeout)
    {
        return ParseResult(sys_mutex_lock_timeout(m_Handle, timeout));
    }
    bool LockDeadline(bigtime_t deadline) { return ParseResult(sys_mutex_lock_deadline(m_Handle, deadline)); }
    bool TryLock() { return ParseResult(sys_mutex_try_lock(m_Handle)); }
    bool Unlock() { return ParseResult(sys_mutex_unlock(m_Handle)); }
    bool IsLocked() const
    {
        const PErrorCode result = sys_mutex_islocked(m_Handle);
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
