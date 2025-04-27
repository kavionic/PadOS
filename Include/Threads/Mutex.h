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
#include "Threads/Threads.h"
#include "System/HandleObject.h"

namespace os
{


class Mutex : public HandleObject
{
public:
    enum class NoInit {};

    explicit Mutex(NoInit) : HandleObject(INVALID_HANDLE) {}

    Mutex(const char* name, EMutexRecursionMode recursionMode) : HandleObject(create_mutex(name, recursionMode)) {}

    bool Lock() { return lock_mutex(m_Handle) >= 0; }
    bool LockTimeout(bigtime_t timeout) { return lock_mutex_timeout(m_Handle, timeout) >= 0; }
    bool LockDeadline(bigtime_t deadline) { return lock_mutex_deadline(m_Handle, deadline) >= 0; }
    bool TryLock() { return try_lock_mutex(m_Handle) >= 0; }
    bool Unlock() { return unlock_mutex(m_Handle) >= 0; }
    bool IsLocked() const { return islocked_mutex(m_Handle) >= 0; }

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
