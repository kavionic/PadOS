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

#pragma once

#include <concepts>
#include <limits>
#include <vector>
#include <map>
#include <optional>
#include <sys/types.h>

#include <Ptr/Ptr.h>
#include <Kernel/KMutex.h>
#include <Kernel/KThreadCB.h>

namespace kernel
{

class KThreadCB;
class KProcess;
class KProcessGroup;
class KProcessSession;

struct KPIDNode;

extern std::optional<KMutex>                           g_PIDMapMutexOpt;
extern std::optional<std::map<pid_t, Ptr<KPIDNode>>>   g_PIDMapOpt;

#define g_PIDMapMutex   (*g_PIDMapMutexOpt)
#define g_PIDMap        (*g_PIDMapOpt)

static constexpr pid_t KTHREAD_ID_IDLE = 0;
static constexpr pid_t KTHREAD_ID_INIT = 1;
static constexpr pid_t KTHREAD_ID_FIRST_DYNAMIC = 2;


struct KPIDNode : public PtrTarget
{
    KPIDNode(pid_t pid);

    bool IsEmpty() const noexcept { return Thread == nullptr && Process == nullptr && Group == nullptr && Session == nullptr; }

    pid_t PID;
    Ptr<KThreadCB>          Thread;
    Ptr<KProcess>           Process;
    Ptr<KProcessGroup>      Group;
    Ptr<KProcessSession>    Session;
};

Ptr<KPIDNode>   kallocate_pid_trw_pl();
void            kerase_pid_node(pid_t pid) noexcept;
void            kerase_pid_node_pl(pid_t pid) noexcept;
void            kerase_pid_node_if_empty(pid_t pid) noexcept;
void            kerase_pid_node_if_empty_pl(pid_t pid) noexcept;

Ptr<KPIDNode>   kget_pid_node(pid_t pid) noexcept;
Ptr<KPIDNode>   kget_pid_node_pl(pid_t pid) noexcept;

///////////////////////////////////////////////////////////////////////////////
/// Returns the next valid PID that is recognized by the delegate.
/// The delegate is called with the PID-map mutex held, and therefore must
/// not recurse into other PID related functions. And it is not allowed to
/// throw any exceptions.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TDelegate>
requires requires(TDelegate&& fn, const KPIDNode& node)
{
    { fn(node) } noexcept -> std::convertible_to<bool>;
}
pid_t kget_next_pid(pid_t currentPID, TDelegate&& delegate) noexcept
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    for (;;)
    {
        auto it = (currentPID != -1) ? g_PIDMap.upper_bound(currentPID) : g_PIDMap.begin();
        if (it == g_PIDMap.end()) {
            return -1;
        }
        if (delegate(*it->second)) {
            return it->first;
        }
        currentPID = it->first;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Returns the next valid PID that is recognized by the delegate.
/// The delegate is called with the PID-map mutex held, and therefore must
/// not recurse into other PID related functions. And it is not allowed to
/// throw any exceptions.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TDelegate>
requires requires(TDelegate&& fn, const KPIDNode& node)
{
    { fn(node) } noexcept -> std::convertible_to<bool>;
}
Ptr<KPIDNode> kget_next_pid_node_pl(pid_t currentPID, TDelegate&& delegate) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    for (;;)
    {
        auto it = (currentPID != -1) ? g_PIDMap.upper_bound(currentPID) : g_PIDMap.begin();
        if (it == g_PIDMap.end()) {
            return nullptr;
        }
        if (delegate(*it->second)) {
            return it->second;
        }
        currentPID = it->first;
    }
}

Ptr<KThreadCB> kget_thread_trw(pid_t threadID);
Ptr<KThreadCB> kget_thread(pid_t threadID);

Ptr<KThreadCB> kget_thread_trw_pl(pid_t threadID);
Ptr<KThreadCB> kget_thread_pl(pid_t threadID);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline Ptr<KThreadCB> kget_next_thread(pid_t currentTID) noexcept
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    Ptr<KPIDNode> pidNode = kget_next_pid_node_pl(currentTID, [](const KPIDNode& node) noexcept { return node.Thread != nullptr && !node.Thread->IsZombie(); });
    return (pidNode != nullptr) ? pidNode->Thread : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline pid_t kget_next_thread_id(pid_t currentTID) noexcept
{
    return kget_next_pid(currentTID, [](const KPIDNode& node) noexcept { return node.Thread != nullptr && !node.Thread->IsZombie(); });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline Ptr<KThreadCB> kget_first_thread() noexcept
{
    return kget_next_thread(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline pid_t kget_first_thread_id() noexcept
{
    return kget_next_thread_id(-1);
}


} // namespace kernel
