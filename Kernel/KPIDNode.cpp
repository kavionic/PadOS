// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.03.2026 18:00


#include <Kernel/KPIDNode.h>

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
            PERROR_THROW_CODE(PErrorCode::TryAgain);
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

Ptr<KThreadCB> kget_thread(pid_t threadID)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);
    return kget_thread_pl(threadID);
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
