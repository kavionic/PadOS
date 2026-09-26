// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 25.04.2018 20:52:53

#pragma once

#include <sys/pados_syscalls.h>

#include <Threads/Threads.h>
#include <System/HandleObject.h>

class PMutex : public PHandleObject
{
public:
    enum class NoInit {};

    explicit PMutex(NoInit) : PHandleObject(INVALID_HANDLE) {}

    PMutex(const char* name, PEMutexRecursionMode recursionMode, clockid_t clockID = CLOCK_MONOTONIC)
    {
        handle_id handle;
        if (mutex_create(&handle, name, recursionMode, clockID) == PErrorCode::Success) {
            SetHandle(handle);
        }
    }
    ~PMutex() { mutex_delete(m_Handle); SetHandle(INVALID_HANDLE); }
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
        if (result == PErrorCode::BUSY)
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

    PMutex(PMutex&& other) = default;
    PMutex(const PMutex& other) = default;
    PMutex& operator=(const PMutex& other) = default;

private:
};

class MutexObjGuard
{
    public:
    MutexObjGuard(PMutex& sema) : m_Mutex(&sema) { m_Mutex->Lock(); }
    ~MutexObjGuard() { if (m_Mutex != nullptr) m_Mutex->Unlock(); }

    MutexObjGuard(MutexObjGuard&& other) : m_Mutex(other.m_Mutex) { m_Mutex = nullptr; }

    private:
    PMutex* m_Mutex;

    MutexObjGuard(MutexObjGuard& other)  = delete;
};

inline MutexObjGuard    critical_create_guard(PMutex& sema) { return MutexObjGuard(sema); }
