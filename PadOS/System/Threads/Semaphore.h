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
// Created: 09.03.2018 14:53:24

#pragma once
#include "System/Threads.h"

class Semaphore
{
public:
    enum class NoInit {};

    explicit Semaphore(NoInit) : m_Handle(-1) {}

    Semaphore(const char* name, int count = 1) {
        m_Handle = create_semaphore(name, count);
    }
    ~Semaphore() {
        if (m_Handle != -1) delete_semaphore(m_Handle);
    }

    bool Acquire()                           { return acquire_semaphore(m_Handle) >= 0; }
    bool AcquireTimeout(bigtime_t timeout)   { return acquire_semaphore_timeout(m_Handle, timeout) >= 0; }
    bool AcquireDeadline(bigtime_t deadline) { return acquire_semaphore_deadline(m_Handle, deadline) >= 0; }
    bool TryAcquire()                        { return try_acquire_semaphore(m_Handle) >= 0; }
    bool Release()                          { return release_semaphore(m_Handle) >= 0; }

    Semaphore(Semaphore&& other) : m_Handle(other.m_Handle) { other.m_Handle = -1; }

    Semaphore(const Semaphore& other) { m_Handle = duplicate_semaphore(other.m_Handle); }
    Semaphore& operator=(const Semaphore& other) { m_Handle = duplicate_semaphore(other.m_Handle); return *this; }

private:
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
