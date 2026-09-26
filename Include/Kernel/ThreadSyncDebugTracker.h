// This file is part of PadOS.
//
// Copyright (c) 2023 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 02.02.2023 22:00

#pragma once

#include <stddef.h>

namespace kernel
{

class KThreadCB;
class KNamedObject;
class KMutex;
class KSemaphore;
class KConditionVariable;

enum class KNamedObjectType;

class ThreadSyncDebugTracker
{
public:
    static constexpr size_t MAX_TRACKED_THREADS = 100;

    ThreadSyncDebugTracker();

    static ThreadSyncDebugTracker& GetInstance();

    void AddThread(const KThreadCB* thread, const KNamedObject* waitObject) noexcept;
    void RemoveThread(const KThreadCB* thread) noexcept;

private:
    static ThreadSyncDebugTracker s_Instance;

//    void DetectDeadlock(const KThreadCB* thread, const KNamedObject* waitObject);

    struct BlockedThread
    {
        const char*         ThreadName;
        const char*         WaitObjectName;
        KNamedObjectType    WaitObjectType;
        const KThreadCB*    Thread;
        union
        {
            const KNamedObject* WaitObject;
            const KMutex* Mutex;
            const KSemaphore* Semaphore;
            const KConditionVariable* Condition;
        };
    } m_BlockedThreads[MAX_TRACKED_THREADS];
    size_t m_ThreadCount = 0;
};


} // namespace kernel
