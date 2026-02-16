// This file is part of PadOS.
//
// Copyright (C) 2025-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.10.2025 19:30

#pragma once


#include <sys/pados_types.h>
#include <Signals/VFConnector.h>
#include <System/System.h>
#include <Utils/String.h>
#include <Utils/EnumBitmask.h>

namespace kernel
{

enum class KSpawnThreadFlag : uint32_t
{
    None            = 0,
    Privileged      = 0x01,
    SpawnProcess    = 0x02
};
using KSpawnThreadFlags = PEnumBitmask<KSpawnThreadFlag>;

class KThread
{
public:
    KThread(const PString& name);
    virtual ~KThread();

    static KThread* GetCurrentThread();

    const PString& GetName() const { return m_Name; }

    void SetDeleteOnExit(bool doDelete) { m_DeleteOnExit = doDelete; }
    bool GetDeleteOnExit() const { return m_DeleteOnExit; }

    void  Start_trw(KSpawnThreadFlags flags = KSpawnThreadFlag::None, PThreadDetachState detachState = PThreadDetachState_Detached, int priority = 0, int stackSize = 0);
    void* Join_trw(TimeValNanos deadline = TimeValNanos::infinit);

    bool IsRunning() const { return m_ThreadHandle != INVALID_HANDLE; }
    thread_id GetThreadID() const { return m_ThreadHandle; }

    virtual void* Run();

    void Exit(void* returnValue);

    VFConnector<void*, KThread*> VFRun;
private:
    static void* ThreadEntry(void* data);

    static thread_local KThread* st_CurrentThread;

    PString             m_Name;
    thread_id           m_ThreadHandle = INVALID_HANDLE;
    PThreadDetachState  m_DetachState = PThreadDetachState_Detached;
    bool                m_DeleteOnExit = true;

    KThread(const KThread&) = delete;
    KThread& operator=(const KThread&) = delete;
};

PErrorCode  kthread_attribs_init(PThreadAttribs& outAttribs) noexcept;
thread_id   kthread_spawn_trw(const PThreadAttribs* attribs, PThreadControlBlock* tlsBlock, KSpawnThreadFlags flags, ThreadEntryPoint_t entryPoint, void* arguments);
__attribute__((noreturn)) void kthread_exit(void* returnValue);
PErrorCode  kthread_detach(thread_id handle);
void*       kthread_join_trw(thread_id handle);
thread_id   kget_thread_id() noexcept;
void        kthread_set_priority_trw(thread_id handle, int priority);
int         kthread_get_priority_trw(thread_id handle);
PErrorCode  kget_thread_info(handle_id handle, ThreadInfo* info);
PErrorCode  kget_next_thread_info(ThreadInfo* info);

PErrorCode  ksnooze_ns(bigtime_t delayNanos);
PErrorCode  ksnooze_ms(bigtime_t millis);
PErrorCode  ksnooze(TimeValNanos delay);
PErrorCode  ksnooze_until_ns(bigtime_t resumeTimeNanos);
PErrorCode  ksnooze_until(TimeValNanos resumeTime);

PErrorCode  kyield();
PErrorCode  kthread_kill(pid_t pid, int sig);

} // namespace
