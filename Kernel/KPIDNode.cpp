// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.03.2026 18:00


#include <Kernel/KPIDNode.h>
#include <Kernel/KProcess.h>
#include <Kernel/KProcessGroup.h>
#include <Kernel/KProcessSession.h>

namespace kernel
{

std::optional<KMutex>                           g_PIDMapMutexOpt;
std::optional<std::map<pid_t, Ptr<KPIDNode>>>   g_PIDMapOpt;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPIDNode::KPIDNode(pid_t pid) : PID(pid)
{
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KPIDNode> kallocate_pid_trw_pl()
{
    kassert(g_PIDMapMutex.IsLocked());

    static pid_t nextPID = KTHREAD_ID_FIRST_DYNAMIC;

    const pid_t startPID = nextPID;

    for (;;)
    {
        const pid_t pid = nextPID;
        if (nextPID == std::numeric_limits<pid_t>::max()) {
            nextPID = KTHREAD_ID_FIRST_DYNAMIC;
        } else {
            ++nextPID;
        }
        if (g_PIDMap.find(pid) == g_PIDMap.end())
        {
            Ptr<KPIDNode> node = ptr_new<KPIDNode>(pid);
            
            g_PIDMap[pid] = node;

            return node;
        }
        if (nextPID == startPID) {
            PERROR_THROW_CODE(PErrorCode::AGAIN);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KPIDNode> kget_pid_node(pid_t pid) noexcept
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);
    
    return kget_pid_node_pl(pid);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KPIDNode> kget_pid_node_pl(pid_t pid) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    auto it = g_PIDMap.find(pid);
    return (it != g_PIDMap.end()) ? it->second : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KThreadCB> kget_thread_trw(pid_t threadID)
{
    Ptr<KThreadCB> thread = kget_thread(threadID);
    if (thread == nullptr) {
        PERROR_THROW_CODE(PErrorCode::SRCH);
    }
    return thread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KThreadCB> kget_thread(pid_t threadID)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);
    return kget_thread_pl(threadID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KThreadCB> kget_thread_trw_pl(pid_t threadID)
{
    Ptr<KThreadCB> thread = kget_thread_pl(threadID);
    if (thread == nullptr) {
        PERROR_THROW_CODE(PErrorCode::SRCH);
    }
    return thread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KThreadCB> kget_thread_pl(pid_t threadID)
{
    kassert(g_PIDMapMutex.IsLocked());
    Ptr<KPIDNode> pidNode = kget_pid_node_pl(threadID);
    return (pidNode != nullptr) ? pidNode->Thread : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kerase_pid_node(pid_t pid) noexcept
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);
    
    kerase_pid_node_pl(pid);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kerase_pid_node_pl(pid_t pid) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    auto it = g_PIDMap.find(pid);
    if (kensure(it != g_PIDMap.end())) {
        g_PIDMap.erase(it);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kerase_pid_node_if_empty(pid_t pid) noexcept
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    kerase_pid_node_if_empty_pl(pid);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kerase_pid_node_if_empty_pl(pid_t pid) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    auto it = g_PIDMap.find(pid);
    if (kensure(it != g_PIDMap.end()) && it->second->IsEmpty()) {
        g_PIDMap.erase(it);
    }
}

} // namespace kernel
