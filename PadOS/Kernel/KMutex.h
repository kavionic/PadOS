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

#include "Platform.h"

#include <atomic>
#include <map>

#include <sys/types.h>

#include "Kernel.h"
#include "KNamedObject.h"
#include "Scheduler.h"
#include "System/Threads.h"

namespace kernel
{

class KMutex : public KNamedObject
{
public:
    static const KNamedObjectType ObjectType = KNamedObjectType::Mutex;

    KMutex(const char* name, bool recursive = true);
    ~KMutex();

    bool Lock();
    bool LockTimeout(bigtime_t timeout);
    bool LockDeadline(bigtime_t deadline);
    bool TryLock();
    void Unlock();
    
    bool LockShared();
    bool LockSharedTimeout(bigtime_t timeout);
    bool LockSharedDeadline(bigtime_t deadline);
    bool TryLockShared();
    void UnlockShared();
    
    bool IsLocked() const;
private:
    int             m_Count;
    bool            m_Recursive;
    thread_id       m_Holder = -1; // Thread currently holding the mutex.

    KMutex(const KMutex &) = delete;
    KMutex& operator=(const KMutex &) = delete;
};

class KMutexGuard
{
public:
    KMutexGuard(Ptr<KMutex> sema, bool doLock) : m_Mutex(sema), m_IsLocked(doLock) { if (doLock) m_Mutex->Lock(); }
    ~KMutexGuard() { if (m_IsLocked) m_Mutex->Unlock(); }

    void Lock()
    {
        if (!m_IsLocked)
        {
            m_IsLocked = true;
            if (m_Mutex != nullptr) {
                m_Mutex->Lock();
            }
        }
    }
    void Unlock()
    {
        if (m_IsLocked)
        {
            m_IsLocked = false;
            if (m_Mutex != nullptr) {
                m_Mutex->Unlock();
            }
        }
    }

    KMutexGuard(KMutexGuard&& other) : m_Mutex(other.m_Mutex) { m_Mutex = nullptr; }

private:
    Ptr<KMutex> m_Mutex;
    bool        m_IsLocked;

    KMutexGuard(KMutexGuard& other)  = delete;
};

class KMutexGuardRaw
{
public:
    KMutexGuardRaw(KMutex& mutex, bool doLock) : m_Mutex(&mutex), m_IsLocked(doLock) { if (doLock) m_Mutex->Lock(); }
    ~KMutexGuardRaw() { if (m_IsLocked && m_Mutex != nullptr) m_Mutex->Unlock(); }

    void Lock()
    {
        if (!m_IsLocked)
        {
            m_IsLocked = true;
            if (m_Mutex != nullptr) {
                m_Mutex->Lock();
            }                
        }
    }
    void Unlock()
    {
        if (m_IsLocked)
        {
            m_IsLocked = false;
            if (m_Mutex != nullptr) {
                m_Mutex->Unlock();
            }
        }
    }

    KMutexGuardRaw(KMutexGuardRaw&& other) : m_Mutex(other.m_Mutex) { m_Mutex = nullptr; }

private:
    KMutex* m_Mutex;
    bool    m_IsLocked;
    KMutexGuardRaw(KMutexGuardRaw& other)  = delete;
};

class KMutexSharedGuard
{
public:
    KMutexSharedGuard(KMutex& mutex, bool doLock) : m_Mutex(&mutex), m_IsLocked(doLock) { if (doLock) m_Mutex->LockShared(); }
    ~KMutexSharedGuard() { if (m_IsLocked && m_Mutex != nullptr) m_Mutex->UnlockShared(); }

    void LockShared()
    {
        if (!m_IsLocked)
        {
            m_IsLocked = true;
            if (m_Mutex != nullptr) {
                m_Mutex->LockShared();
            }                
        }
    }
    void UnlockShared()
    {
        if (m_IsLocked)
        {
            m_IsLocked = false;
            if (m_Mutex != nullptr) {
                m_Mutex->UnlockShared();
            }
        }
    }

    KMutexSharedGuard(KMutexSharedGuard&& other) : m_Mutex(other.m_Mutex) { m_Mutex = nullptr; }

private:
    KMutex* m_Mutex;
    bool    m_IsLocked;
    KMutexSharedGuard(KMutexSharedGuard& other)  = delete;
};

inline KMutexGuard    critical_create_guard(Ptr<KMutex> sema, bool doLock = true) { return KMutexGuard(sema, doLock); }
inline KMutexGuardRaw critical_create_guard(KMutex& sema, bool doLock = true) { return KMutexGuardRaw(sema, doLock); }

inline KMutexSharedGuard critical_create_shared_guard(KMutex& sema, bool doLock = true) { return KMutexSharedGuard(sema, doLock); }

} // namespace
