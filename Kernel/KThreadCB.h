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
// Created: 04.03.2018 19:30:41

#pragma once


#include <sys/reent.h>

#include "KNamedObject.h"
#include "Utils/IntrusiveList.h"
#include "Threads/Threads.h"
#include "System/SysTime.h"

namespace os
{
    class DebugCallTracker;
}
namespace kernel
{

static const int32_t THREAD_STACK_PADDING      = 0; // 512;
static const int32_t THREAD_DEFAULT_STACK_SIZE = 1024*8;
static const int32_t THREAD_MAX_TLS_SLOTS      = 64;
class KThreadCB;

static const int KTHREAD_PRIORITY_MIN = -16;
static const int KTHREAD_PRIORITY_MAX = 15;
static const int KTHREAD_PRIORITY_LEVELS = KTHREAD_PRIORITY_MAX - KTHREAD_PRIORITY_MIN + 1;

enum class KThreadState
{
    Running,
    Ready,
    Sleeping,
    Waiting,
    Zombie,
    Deleted
};

class KThreadCB : public KNamedObject
{
public:
    static const KNamedObjectType ObjectType = KNamedObjectType::Thread;

    KThreadCB(const char* name, int priority, bool joinable, int stackSize);
    ~KThreadCB();

    void InitializeStack(ThreadEntryPoint_t entryPoint, void* arguments);

    uint8_t* GetStackBottom() const { return m_StackBuffer + THREAD_STACK_PADDING + THREAD_MAX_TLS_SLOTS * sizeof(void*); }

    static int PriToLevel(int priority);
    static int LevelToPri(int level);

    uint32_t*                 m_CurrentStack;
    KThreadState              m_State;
    int                       m_PriorityLevel;
    TimeValNanos              m_StartTime;
    TimeValNanos              m_RunTime;
    _reent                    m_NewLibreent;
    bool                      m_IsJoinable;
    bool                      m_RestartSyscalls = true;
    uint8_t*                  m_StackBuffer;
    int                       m_StackSize;

    KThreadCB*                m_Prev = nullptr;
    KThreadCB*                m_Next = nullptr;
    IntrusiveList<KThreadCB>* m_List = nullptr;
    KNamedObject*             m_BlockingObject = nullptr;
    os::DebugCallTracker*     m_FirstDebugCallTracker = nullptr;
};

typedef IntrusiveList<KThreadCB>       KThreadList;

} // namespace
