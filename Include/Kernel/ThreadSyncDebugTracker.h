// This file is part of PadOS.
//
// Copyright (C) 2023 Kurt Skauen <http://kavionic.com/>
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

    void AddThread(const KThreadCB* thread, const KNamedObject* waitObject);
    void RemoveThread(const KThreadCB* thread);

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
