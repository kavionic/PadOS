// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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

#include <atomic>
#include <map>
#include <mutex>
#include <shared_mutex>

#include <sys/types.h>
#include <sys/pados_mutex.h>

#include <System/ExceptionHandling.h>
#include <Kernel/Kernel.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/Scheduler.h>
#include <Threads/Threads.h>

namespace kernel
{

class KMutex : public KNamedObject
{
public:
    static const KNamedObjectType ObjectType = KNamedObjectType::Mutex;

    KMutex(const char* name, PEMutexRecursionMode recursionMode, clockid_t clockID = CLOCK_MONOTONIC);
    ~KMutex();

    PErrorCode Lock(bool interruptible = false);
    PErrorCode LockTimeout(TimeValNanos timeout, bool interruptible = false);
    PErrorCode LockDeadline(TimeValNanos deadline, bool interruptible = false);
    PErrorCode LockClock(clockid_t clockID, TimeValNanos deadline, bool interruptible = false);
    PErrorCode TryLock();
    PErrorCode Unlock();
    
    PErrorCode LockShared(bool interruptible = false);
    PErrorCode LockSharedTimeout(TimeValNanos timeout, bool interruptible = false);
    PErrorCode LockSharedDeadline(TimeValNanos deadline, bool interruptible = false);
    PErrorCode LockSharedClock(clockid_t clockID, TimeValNanos deadline, bool interruptible = false);
    PErrorCode TryLockShared();
    
    bool IsLocked() const;
private:
    int                  m_Count = 0;
    PEMutexRecursionMode m_RecursionMode = PEMutexRecursionMode_Recurse;
    clockid_t            m_ClockID = CLOCK_MONOTONIC;
    thread_id            m_Holder = INVALID_HANDLE; // Thread currently holding the mutex.

    KMutex(const KMutex &) = delete;
    KMutex& operator=(const KMutex &) = delete;


    // For STL compatibility:
public:
    void lock()     { PERROR_ERRORCODE_THROW_ON_FAIL(Lock()); }
    bool try_lock() { return ConvertTryResult(TryLock()); }
    void unlock()   { PERROR_ERRORCODE_THROW_ON_FAIL(Unlock()); }

    template<typename Rep, typename Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& relTime)
    {
        return ConvertTryResult(LockTimeout(ToTimeVal(relTime)));
    }

    template<typename Clock, typename Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& absTime)
    {
        if constexpr (std::is_same_v<Clock, std::chrono::steady_clock>) {
            return ConvertTryResult(LockClock(CLOCK_MONOTONIC, ToTimeVal(absTime.time_since_epoch())));
        }
        else if constexpr (std::is_same_v<Clock, std::chrono::system_clock>) {
            return ConvertTryResult(LockClock(CLOCK_REALTIME, ToTimeVal(absTime.time_since_epoch())));
        }
        else {
            return try_lock_for(absTime - Clock::now());
        }
    }

    void lock_shared()      { PERROR_ERRORCODE_THROW_ON_FAIL(LockShared()); }
    bool try_lock_shared()  { return ConvertTryResult(TryLockShared()); }
    void unlock_shared()    { PERROR_ERRORCODE_THROW_ON_FAIL(Unlock()); }

    template<typename Rep, typename Period>
    bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& relTime)
    {
        return ConvertTryResult(LockSharedTimeout(ToTimeVal(relTime)));
    }

    template<typename Clock, typename Duration>
    bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& absTime)
    {
        if constexpr (std::is_same_v<Clock, std::chrono::steady_clock>) {
            return ConvertTryResult(LockSharedClock(CLOCK_MONOTONIC, ToTimeVal(absTime.time_since_epoch())));
        } else if constexpr (std::is_same_v<Clock, std::chrono::system_clock>) {
            return ConvertTryResult(LockSharedClock(CLOCK_REALTIME, ToTimeVal(absTime.time_since_epoch())));
        } else {
            return try_lock_shared_for(absTime - Clock::now());
        }
    }

private:
    static bool ConvertTryResult(PErrorCode result)
    {
        if (result == PErrorCode::Success) {
            return true;
        }
        if (result == PErrorCode::Busy || result == PErrorCode::Timeout) {
            return false;
        }
        PERROR_ERRORCODE_THROW_ON_FAIL(result);
    }

    template<typename Rep, typename Period>
    static TimeValNanos ToTimeVal(const std::chrono::duration<Rep, Period>& duration)
    {
        return TimeValNanos::FromNanoseconds(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
    }
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
    ~KMutexSharedGuard() { if (m_IsLocked && m_Mutex != nullptr) m_Mutex->Unlock(); }

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

    KMutexSharedGuard(KMutexSharedGuard&& other) : m_Mutex(other.m_Mutex) { m_Mutex = nullptr; }

private:
    KMutex* m_Mutex;
    bool    m_IsLocked;
    KMutexSharedGuard(KMutexSharedGuard& other)  = delete;
};

inline KMutexGuard    critical_create_guard(Ptr<KMutex> sema, bool doLock = true) { return KMutexGuard(sema, doLock); }
inline KMutexGuardRaw critical_create_guard(KMutex& sema, bool doLock = true) { return KMutexGuardRaw(sema, doLock); }

inline KMutexSharedGuard critical_create_shared_guard(KMutex& sema, bool doLock = true) { return KMutexSharedGuard(sema, doLock); }

using KScopedLock = std::scoped_lock<KMutex>;
using KUniqueLock = std::unique_lock<KMutex>;
using KSharedLock = std::shared_lock<KMutex>;


} // namespace
