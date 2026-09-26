// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 07.03.2018 22:43:58

#pragma once

#include <sys/pados_types.h>
#include <sys/pados_mutex.h>
#include <sys/pados_syscalls.h>
#include <System/TimeValue.h>
#include <Kernel/KNamedObject.h>


static constexpr uint8_t THREAD_STACK_FILLER = 0x5f;
extern "C"
{
inline PErrorCode snooze_until(TimeValNanos resumeTime) { return snooze_until_ns(resumeTime.AsNanoseconds()); }
inline PErrorCode snooze(TimeValNanos delay) { return snooze_ns(delay.AsNanoseconds()); }

inline PErrorCode snooze_ms(bigtime_t millis) { return snooze(TimeValNanos::FromMilliseconds(millis)); }


}; // extern "C"

class SemaphoreGuard
{
public:
    SemaphoreGuard(sem_id sema) : m_Semaphore(sema) { semaphore_acquire(m_Semaphore); }
    ~SemaphoreGuard() { if (m_Semaphore != -1) semaphore_release(m_Semaphore); }

    SemaphoreGuard(SemaphoreGuard&& other) : m_Semaphore(other.m_Semaphore) { m_Semaphore = -1; }

private:
    sem_id m_Semaphore;

    SemaphoreGuard(SemaphoreGuard& other)  = delete;
};

inline SemaphoreGuard     critical_create_guard(sem_id sema) { return SemaphoreGuard(sema); }

#define GET_CRITICAL_GUARD_NAME_CONCAT(label, linenr) label##linenr
#define GET_CRITICAL_GUARD_NAME(label, linenr) GET_CRITICAL_GUARD_NAME_CONCAT(label, linenr)
#define CRITICAL_SCOPE(lock, ...) auto GET_CRITICAL_GUARD_NAME(CRITICAL_SCOPE_guard,__LINE__) = critical_create_guard(lock, ## __VA_ARGS__)
#define CRITICAL_BEGIN(lock, ...) { auto GET_CRITICAL_GUARD_NAME(CRITICAL_SCOPE_guard,__LINE__) = critical_create_guard(lock, ## __VA_ARGS__);
#define CRITICAL_END }

#define CRITICAL_SHARED_SCOPE(lock, ...) auto GET_CRITICAL_GUARD_NAME(CRITICAL_SCOPE_guard,__LINE__) = critical_create_shared_guard(lock, ## __VA_ARGS__)
#define CRITICAL_SHARED_BEGIN(lock, ...) { auto GET_CRITICAL_GUARD_NAME(CRITICAL_SCOPE_guard,__LINE__) = critical_create_shared_guard(lock, ## __VA_ARGS__);
#define CRITICAL_SHARED_END }
