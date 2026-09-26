// This file is part of PadOS.
//
// Copyright (c) 2018-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 09.03.2018 14:53:24

#pragma once
#include "Threads/Threads.h"

class PSemaphore
{
public:
    enum class NoInit {};

    explicit PSemaphore(NoInit) : m_Handle(INVALID_HANDLE) {}

    PSemaphore(const char* name, int count = 1)
    {
        if (semaphore_create(&m_Handle, name, CLOCK_MONOTONIC_COARSE, count) != PErrorCode::Success) {
            m_Handle = INVALID_HANDLE;
        }
    }
    ~PSemaphore() {
        if (m_Handle != INVALID_HANDLE) semaphore_delete(m_Handle);
    }

    bool Acquire()                              { return ParseResult(semaphore_acquire(m_Handle)); }
    bool AcquireTimeout(TimeValNanos timeout)   { return ParseResult(semaphore_acquire_timeout_ns(m_Handle, timeout.AsNanoseconds())); }
    bool AcquireDeadline(TimeValNanos deadline) { return ParseResult(semaphore_acquire_deadline_ns(m_Handle, deadline.AsNanoseconds())); }
    bool TryAcquire()                           { return ParseResult(semaphore_try_acquire(m_Handle)); }
    bool Release()                              { return ParseResult(semaphore_release(m_Handle)); }

    PSemaphore(PSemaphore&& other) : m_Handle(other.m_Handle) { other.m_Handle = INVALID_HANDLE; }

    PSemaphore(const PSemaphore& other) { m_Handle = INVALID_HANDLE; semaphore_duplicate(&m_Handle, other.m_Handle); }
    PSemaphore& operator=(const PSemaphore& other) { m_Handle = INVALID_HANDLE; semaphore_duplicate(&m_Handle, other.m_Handle); return *this; }

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
    SemaphoreObjGuard(PSemaphore& sema) : m_Semaphore(&sema) { m_Semaphore->Acquire(); }
    ~SemaphoreObjGuard() { if (m_Semaphore != nullptr) m_Semaphore->Release(); }

    SemaphoreObjGuard(SemaphoreObjGuard&& other) : m_Semaphore(other.m_Semaphore) { m_Semaphore = nullptr; }

private:
    PSemaphore* m_Semaphore;

    SemaphoreObjGuard(SemaphoreObjGuard& other)  = delete;
};

inline SemaphoreObjGuard    critical_create_guard(PSemaphore& sema) { return SemaphoreObjGuard(sema); }
