// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 09.03.2018 14:53:24

#pragma once
#include "Threads/Threads.h"

class Semaphore
{
public:
    enum class NoInit {};

    explicit Semaphore(NoInit) : m_Handle(INVALID_HANDLE) {}

    Semaphore(const char* name, int count = 1)
    {
        if (__semaphore_create(&m_Handle, name, CLOCK_MONOTONIC_COARSE, count) != PErrorCode::Success) {
            m_Handle = INVALID_HANDLE;
        }
    }
    ~Semaphore() {
        if (m_Handle != INVALID_HANDLE) __semaphore_delete(m_Handle);
    }

    bool Acquire()                              { return ParseResult(__semaphore_acquire(m_Handle)); }
    bool AcquireTimeout(TimeValNanos timeout)   { return ParseResult(__semaphore_acquire_timeout_ns(m_Handle, timeout.AsNanoseconds())); }
    bool AcquireDeadline(TimeValNanos deadline) { return ParseResult(__semaphore_acquire_deadline_ns(m_Handle, deadline.AsNanoseconds())); }
    bool TryAcquire()                           { return ParseResult(__semaphore_try_acquire(m_Handle)); }
    bool Release()                              { return ParseResult(__semaphore_release(m_Handle)); }

    Semaphore(Semaphore&& other) : m_Handle(other.m_Handle) { other.m_Handle = INVALID_HANDLE; }

    Semaphore(const Semaphore& other) { m_Handle = INVALID_HANDLE; __semaphore_duplicate(&m_Handle, other.m_Handle); }
    Semaphore& operator=(const Semaphore& other) { m_Handle = INVALID_HANDLE; __semaphore_duplicate(&m_Handle, other.m_Handle); return *this; }

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

    sem_id m_Handle;
};

class SemaphoreObjGuard
{
public:
    SemaphoreObjGuard(Semaphore& sema) : m_Semaphore(&sema) { m_Semaphore->Acquire(); }
    ~SemaphoreObjGuard() { if (m_Semaphore != nullptr) m_Semaphore->Release(); }

    SemaphoreObjGuard(SemaphoreObjGuard&& other) : m_Semaphore(other.m_Semaphore) { m_Semaphore = nullptr; }

private:
    Semaphore* m_Semaphore;

    SemaphoreObjGuard(SemaphoreObjGuard& other)  = delete;
};

inline SemaphoreObjGuard    critical_create_guard(Semaphore& sema) { return SemaphoreObjGuard(sema); }
