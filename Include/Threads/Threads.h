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
// along with PadOS. If not, see < http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 07.03.2018 22:43:58

#pragma once

#include "System/Types.h"
#include "Kernel/KNamedObject.h"


typedef void (*ThreadEntryPoint_t)( void * );
typedef void (*TLSDestructor_t)(void *);

thread_id spawn_thread(const char* name, ThreadEntryPoint_t entryPoint, int priority, void* arguments = nullptr, bool joinable = false, int stackSize = 0 );
int       exit_thread(int returnCode);
int       wait_thread(thread_id handle);
status_t  wakeup_thread(thread_id handle);

int thread_yield();

thread_id get_thread_id();
status_t snooze_until(bigtime_t resumeTime);

status_t snooze_us(bigtime_t micros);
status_t snooze_ms(bigtime_t millis);
status_t snooze_s(bigtime_t seconds);

sem_id   create_semaphore(const char* name, int count);
sem_id   duplicate_semaphore(sem_id handle);
status_t delete_semaphore(sem_id handle);
status_t acquire_semaphore(sem_id handle);
status_t acquire_semaphore_timeout(sem_id handle, bigtime_t timeout);
status_t acquire_semaphore_deadline(sem_id handle, bigtime_t deadline);
status_t try_acquire_semaphore(sem_id handle);
status_t release_semaphore(sem_id handle);

sem_id   create_mutex(const char* name, bool recursive);
sem_id   duplicate_mutex(sem_id handle);
status_t delete_mutex(sem_id handle);
status_t lock_mutex(sem_id handle);
status_t lock_mutex_timeout(sem_id handle, bigtime_t timeout);
status_t lock_mutex_deadline(sem_id handle, bigtime_t deadline);
status_t try_lock_mutex(sem_id handle);
status_t unlock_mutex(sem_id handle);

status_t lock_mutex_shared(sem_id handle);
status_t lock_mutex_shared_timeout(sem_id handle, bigtime_t timeout);
status_t lock_mutex_shared_deadline(sem_id handle, bigtime_t deadline);
status_t try_lock_mutex_shared(sem_id handle);
status_t unlock_mutex_shared(sem_id handle);

handle_id create_condition_var(const char* name);
status_t  condition_var_wait(handle_id handle, handle_id mutexHandle);
status_t  condition_var_wait_timeout(handle_id handle, handle_id mutexHandle, bigtime_t timeout);
status_t  condition_var_wait_deadline(handle_id handle, handle_id mutexHandle, bigtime_t deadline);
status_t  condition_var_wakeup(handle_id handle, int threadCount);
status_t  condition_var_wakeup_all(handle_id handle);

handle_id create_object_wait_group(const char* name);

status_t  object_wait_group_add_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
status_t  object_wait_group_remove_object(handle_id handle, handle_id objectHandle, ObjectWaitMode waitMode = ObjectWaitMode::Read);
status_t  object_wait_group_clear(handle_id handle);

status_t  object_wait_group_wait(handle_id handle, handle_id mutexHandle, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);
status_t  object_wait_group_wait_timeout(handle_id handle, handle_id mutexHandle, bigtime_t timeout, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);
status_t  object_wait_group_wait_deadline(handle_id handle, handle_id mutexHandle, bigtime_t deadline, void* readyFlagsBuffer = nullptr, size_t readyFlagsSize = 0);

sem_id   duplicate_handle(sem_id handle);
status_t delete_handle(sem_id handle);

int   alloc_thread_local_storage(TLSDestructor_t destructor);
int   delete_thread_local_storage(int slot);
int   set_thread_local(int slot, void* value);
void* get_thread_local(int slot);

class SemaphoreGuard
{
public:
    SemaphoreGuard(sem_id sema) : m_Semaphore(sema) { acquire_semaphore(m_Semaphore); }
    ~SemaphoreGuard() { if (m_Semaphore != -1) release_semaphore(m_Semaphore); }

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
