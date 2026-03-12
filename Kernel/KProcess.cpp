// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 11.03.2018 01:23:12

#include <System/Platform.h>

#include <string.h>
#include <sys/errno.h>

#include <Kernel/KHandleArray.h>
#include <Kernel/KProcess.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/VFS/Kpty.h>
#include <Threads/Threads.h>
#include <System/AppDefinition.h>
#include <Utils/Utils.h>


namespace kernel
{

KProcess* gk_KernelProcess;
KMutex KProcess::s_ProcessMutex("proctable", PEMutexRecursionMode_RaiseError);
std::map<pid_t, KProcess*> KProcess::s_ProcessMap;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::KProcess(const char* name)
{
    strncpy(m_Name, name, OS_NAME_LENGTH);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::KProcess(const KProcess& parentProcess, const char* name) : m_ParentPID(parentProcess.m_PID)
{
    strncpy(m_Name, name, OS_NAME_LENGTH);
    m_IOContext.Clone(parentProcess.m_IOContext);
    
    
    m_RUID      = parentProcess.m_RUID;
    m_EUID      = parentProcess.m_EUID;
    m_SUID      = parentProcess.m_SUID;
    m_RGID      = parentProcess.m_RGID;
    m_EGID      = parentProcess.m_EGID;
    m_SGID      = parentProcess.m_SGID;
    m_NumGroups = parentProcess.m_NumGroups;
    m_PGroupID  = parentProcess.m_PGroupID;
    m_Session   = parentProcess.m_Session;

    memcpy(m_Groups, parentProcess.m_Groups, sizeof(m_Groups));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::~KProcess()
{
    if (m_PID != -1)
    {
        kassert(!s_ProcessMutex.IsLocked());
        CRITICAL_SCOPE(s_ProcessMutex);

        SetPID(-1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::AddThread(KThreadCB* thread)
{
    kassert(!s_ProcessMutex.IsLocked());
    CRITICAL_SCOPE(s_ProcessMutex);

    kassert(thread->m_Process == nullptr);

    if (m_PID == -1) {
        SetPID(thread->GetHandle());
    }
    m_Threads.push_back(thread);
    thread->m_Process = ptr_tmp_cast(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::RemoveThread(KThreadCB* thread) noexcept
{
    {
        kassert(!s_ProcessMutex.IsLocked());
        CRITICAL_SCOPE(s_ProcessMutex);

        kassert(thread->m_Process == this);

        auto i = std::find(m_Threads.begin(), m_Threads.end(), thread);
        kassert(i != m_Threads.end());

        if (i != m_Threads.end()) {
            m_Threads.erase(i);
        }
    }
    if (m_Threads.empty())
    {
        if (IsGroupLeader())
        {
            try {
                kdisassociate_controlling_tty_trw(false);
            }
            catch (const std::exception& exc) {}
        }
        CRITICAL_SCOPE(s_ProcessMutex);
        SetPID(-1);
    }
    thread->m_Process = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<Ptr<KThreadCB>> KProcess::GetThreads() const
{
    kassert(!s_ProcessMutex.IsLocked());
    CRITICAL_SCOPE(s_ProcessMutex);

    std::vector<Ptr<KThreadCB>> result;
    result.reserve(m_Threads.size());

    for (KThreadCB* thread : m_Threads) {
        result.push_back(ptr_tmp_cast(thread));
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KProcess::CheckUIDMatch(uid_t uid) const noexcept
{
    return uid == m_RUID || uid == m_EUID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KProcess::CheckUIDMatch(const KProcess& target) const noexcept
{
    return CheckUIDMatch(target.m_RUID) || CheckUIDMatch(target.m_SUID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KProcess> KProcess::GetProcess(pid_t pid) noexcept
{
    kassert(!s_ProcessMutex.IsLocked());
    CRITICAL_SCOPE(s_ProcessMutex);

    auto it = s_ProcessMap.find(pid);
    return (it != s_ProcessMap.end()) ? ptr_tmp_cast(it->second) : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::SetPID(pid_t pid)
{
    kassert(s_ProcessMutex.IsLocked());

    if (pid != m_PID)
    {
        if (m_PID != -1)
        {
            auto i = s_ProcessMap.find(m_PID);
            kassert(i != s_ProcessMap.end());

            if (i != s_ProcessMap.end()) {
                s_ProcessMap.erase(i);
            }
            m_PID = -1;
        }
        if (pid != -1)
        {
            s_ProcessMap[pid] = this;
            m_PID = pid;
        }
    }
}


} // namespace kernel
