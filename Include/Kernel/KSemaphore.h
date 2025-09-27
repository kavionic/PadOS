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
// Created: 04.03.2018 22:38:38

#pragma once

#include "System/Platform.h"

#include <atomic>
#include <map>

#include <sys/types.h>

#include "Kernel.h"
#include "KNamedObject.h"
#include "Scheduler.h"
#include "Threads/Threads.h"

namespace kernel
{

class KSemaphore : public KNamedObject
{
public:
    static const KNamedObjectType ObjectType = KNamedObjectType::Semaphore;

    KSemaphore(const char* name, clockid_t clockID, int count);
    ~KSemaphore();

    PErrorCode Acquire();
    PErrorCode AcquireTimeout(TimeValMicros timeout);
    PErrorCode AcquireDeadline(TimeValMicros deadline);
    PErrorCode AcquireClock(clockid_t clockID, TimeValMicros deadline);
    PErrorCode TryAcquire();
    PErrorCode Release();
    PErrorCode SetCount(int count) { m_Count = count; return PErrorCode::Success; }
    int  GetCount() const { return m_Count; }
private:
    int             m_Count;
    clockid_t       m_ClockID = CLOCK_MONOTONIC_COARSE;
    thread_id       m_Holder = -1; // Thread currently holding the semaphore.

    KSemaphore(const KSemaphore &) = delete;
    KSemaphore& operator=(const KSemaphore &) = delete;
};

class KSemaphoreGuard
{
public:
    KSemaphoreGuard(Ptr<KSemaphore> sema) : m_Semaphore(sema) { m_Semaphore->Acquire(); }
    ~KSemaphoreGuard() { if (m_Semaphore != nullptr) m_Semaphore->Release(); }

    KSemaphoreGuard(KSemaphoreGuard&& other) : m_Semaphore(other.m_Semaphore) { m_Semaphore = nullptr; }

private:
    Ptr<KSemaphore> m_Semaphore;

    KSemaphoreGuard(KSemaphoreGuard& other)  = delete;
};

class KSemaphoreGuardRaw
{
public:
    KSemaphoreGuardRaw(KSemaphore& sema) : m_Semaphore(&sema) { m_Semaphore->Acquire(); }
    ~KSemaphoreGuardRaw() { if (m_Semaphore != nullptr) m_Semaphore->Release(); }

    KSemaphoreGuardRaw(KSemaphoreGuardRaw&& other) : m_Semaphore(other.m_Semaphore) { m_Semaphore = nullptr; }

private:
    KSemaphore* m_Semaphore;

    KSemaphoreGuardRaw(KSemaphoreGuardRaw& other) = delete;
};

inline KSemaphoreGuard    critical_create_guard(Ptr<KSemaphore> sema) { return KSemaphoreGuard(sema); }
inline KSemaphoreGuardRaw critical_create_guard(KSemaphore& sema) { return KSemaphoreGuardRaw(sema); }

} // namespace
