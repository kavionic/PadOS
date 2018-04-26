// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
#include "System/Threads.h"

namespace os
{


class Mutex
{
    public:
    enum class NoInit {};

    explicit Mutex(NoInit) : m_Handle(-1) {}

    Mutex(const char* name, bool recursive = true) {
        m_Handle = create_mutex(name, recursive);
    }
    ~Mutex() {
        if (m_Handle != -1) delete_mutex(m_Handle);
    }

    bool Lock()                           { return lock_mutex(m_Handle) >= 0; }
    bool LockTimeout(bigtime_t timeout)   { return lock_mutex_timeout(m_Handle, timeout) >= 0; }
    bool LockDeadline(bigtime_t deadline) { return lock_mutex_deadline(m_Handle, deadline) >= 0; }
    bool TryLock()                        { return try_lock_mutex(m_Handle) >= 0; }
    bool Unlock()                         { return unlock_mutex(m_Handle) >= 0; }

    Mutex(Mutex&& other) : m_Handle(other.m_Handle) { other.m_Handle = -1; }

    Mutex(const Mutex& other) { m_Handle = duplicate_mutex(other.m_Handle); }
    Mutex& operator=(const Mutex& other) { m_Handle = duplicate_mutex(other.m_Handle); return *this; }

    private:
    sem_id m_Handle;
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
