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
// along with PadOS. If not, see < http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 07.03.2018 22:43:58

#pragma once

#include <sys/pados_types.h>
#include <sys/pados_mutex.h>
#include <sys/pados_syscalls.h>
#include "System/SysTime.h"
#include "Kernel/KNamedObject.h"


static constexpr uint8_t THREAD_STACK_FILLER = 0x5f;
extern "C"
{
status_t  wakeup_thread(thread_id handle);

status_t snooze_until(TimeValMicros resumeTime);
status_t snooze(TimeValMicros delay);

status_t snooze_us(bigtime_t micros);
status_t snooze_ms(bigtime_t millis);
status_t snooze_s(bigtime_t seconds);

PErrorCode create_object_wait_group(handle_id& outHandle, const char* name);

PErrorCode  object_wait_group_add_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
PErrorCode  object_wait_group_remove_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
PErrorCode  object_wait_group_add_file(handle_id handle, int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
PErrorCode  object_wait_group_remove_file(handle_id handle, int fileHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
PErrorCode  object_wait_group_clear(handle_id handle);

PErrorCode  object_wait_group_wait(handle_id handle, handle_id mutexHandle, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);
PErrorCode  object_wait_group_wait_timeout(handle_id handle, handle_id mutexHandle, bigtime_t timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);
PErrorCode  object_wait_group_wait_deadline(handle_id handle, handle_id mutexHandle, bigtime_t deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);

PErrorCode  duplicate_handle(handle_id& outNewHandle, sem_id handle);
status_t    delete_handle(sem_id handle);

}; // extern "C"

class SemaphoreGuard
{
public:
    SemaphoreGuard(sem_id sema) : m_Semaphore(sema) { sys_semaphore_acquire(m_Semaphore); }
    ~SemaphoreGuard() { if (m_Semaphore != -1) sys_semaphore_release(m_Semaphore); }

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
