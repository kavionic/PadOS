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
#include <Kernel/KProcessSession.h>
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::KProcess(KPIDNode& pidNode, const char* name)
    : m_Mutex("process", PEMutexRecursionMode_RaiseError)
    , m_PID(pidNode.PID)
{
    kassert(g_PIDMapMutex.IsLocked());

    strncpy(m_Name, name, OS_NAME_LENGTH);

    m_Session = ptr_new<KProcessSession>(pidNode.PID);
    Ptr<KProcessGroup> group = ptr_new<KProcessGroup>(pidNode.PID, m_Session);

    group->AddProcess(this);

    pidNode.Session = m_Session;
    pidNode.Group   = m_Group;
    pidNode.Process = ptr_tmp_cast(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::KProcess(KPIDNode& pidNode, Ptr<KProcess> parentProcess, const char* name)
    : m_Mutex("process", PEMutexRecursionMode_RaiseError)
    , m_PID(pidNode.PID)
    , m_Parent(parentProcess)
{
    kassert(g_PIDMapMutex.IsLocked());

    strncpy(m_Name, name, OS_NAME_LENGTH);
    m_IOContext.Clone(parentProcess->m_IOContext);
    
    m_RUID      = parentProcess->m_RUID;
    m_EUID      = parentProcess->m_EUID;
    m_SUID      = parentProcess->m_SUID;
    m_RGID      = parentProcess->m_RGID;
    m_EGID      = parentProcess->m_EGID;
    m_SGID      = parentProcess->m_SGID;
    m_NumGroups = parentProcess->m_NumGroups;
    m_Session   = parentProcess->m_Session;

    parentProcess->m_Group->AddProcess(this);

    memcpy(m_Groups, parentProcess->m_Groups, sizeof(m_Groups));

    parentProcess->m_Children.push_back(this);

    pidNode.Process = ptr_tmp_cast(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::~KProcess()
{
    kassert(m_Threads.empty());
    kassert(m_Session == nullptr);
    kassert(m_Group == nullptr);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::AddThread(KThreadCB* thread)
{
    kassert(g_PIDMapMutex.IsLocked());

    kassert(thread->m_Process == nullptr);

    m_Threads.push_back(thread);
    thread->m_Process = ptr_tmp_cast(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::RemoveThread(KThreadCB* thread) noexcept
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    kassert(thread->m_Process == this);

    auto i = std::find(m_Threads.begin(), m_Threads.end(), thread);
    kassert(i != m_Threads.end());

    if (i != m_Threads.end()) {
        m_Threads.erase(i);
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

        m_Group->RemoveProcess(this);
        m_Session = nullptr;

        Ptr<KPIDNode> pidNode = kget_pid_node_pl(m_PID);

        if (kensure(pidNode != nullptr)) {
            pidNode->Process = nullptr;
        }
        if (m_Parent != nullptr)
        {
            auto it = std::find(m_Parent->m_Children.begin(), m_Parent->m_Children.end(), this);
            if (kensure(it != m_Parent->m_Children.end())) {
                m_Parent->m_Children.erase(it);
            }
            kkill_pl(m_Parent->m_PID, SIGCHLD);
            m_Parent = nullptr;
        }
        kerase_pid_node_if_empty_pl(m_PID);
    }
    thread->m_Process = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const std::vector<kernel::KThreadCB*>& KProcess::GetThreads() const
{
    kassert(g_PIDMapMutex.IsLocked());
    return m_Threads;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::SetPGroupID(Ptr<const KProcess> instigator, pid_t pgroup)
{
    kassert(g_PIDMapMutex.IsLocked());

    if (pgroup == m_Group->GetID()) {
        return;
    }

    if (instigator == m_Parent)
    {
        if (m_Session != instigator->GetSession()) {
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }
        if (HasExeced()) {
            PERROR_THROW_CODE(PErrorCode::NoAccess);
        }
    }
    else if (instigator != this)
    {
        kernel_log<PLogSeverity::NOTICE>(LogCatKernel_Processes, "{}: {} not same as {}\n", __PRETTY_FUNCTION__, m_PID, instigator->m_PID);
        PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
    }

    Ptr<KPIDNode> pidNode = kget_pid_node_pl(pgroup);

    if (pidNode == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    if (pidNode->Session != m_Session) {
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }

    if (pidNode->Group == nullptr)
    {
        const Ptr<KProcessGroup> group = ptr_new<KProcessGroup>(pgroup, m_Session);
        
        group->ReserveSpace(); // Make sure AddProcess() don't throw after we modified the old group.

        pidNode->Group = group;
    }
    else
    {
        pidNode->Group->ReserveSpace(); // Make sure AddProcess() don't throw after we modified the old group.
    }
    m_Group->RemoveProcess(this);
    pidNode->Group->AddProcess(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

pid_t KProcess::GetPGroupID() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    return (m_Group != nullptr) ? m_Group->GetID() : -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::SetGroup(Ptr<KProcessGroup> group) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    m_Group = group;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KProcessGroup> KProcess::GetGroup() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    return m_Group;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KProcess::GetSessionID() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    return (m_Session != nullptr) ? m_Session->GetID() : -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KProcessSession> KProcess::GetSession() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    return m_Session;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

pid_t KProcess::CreateSession()
{
    kassert(g_PIDMapMutex.IsLocked());

    if (IsGroupLeader()) {
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }

    const pid_t sessionID = m_PID;

    Ptr<KPIDNode> pidNode = kget_pid_node_pl(sessionID);
    if (pidNode == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    // Make sure the process group we are about to make don't already exist.
    if (pidNode->Session != nullptr || pidNode->Group != nullptr) {
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }
    
    Ptr<KProcessSession>    session = ptr_new<KProcessSession>(sessionID);
    Ptr<KProcessGroup>      group = ptr_new<KProcessGroup>(sessionID, session);

    group->ReserveSpace(); // Make sure AddProcess() don't throw after modifying the old group.

    m_Group->RemoveProcess(this);

    group->AddProcess(this);
    m_Session = session;

    pidNode->Session = session;
    pidNode->Group = group;

    return sessionID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KProcess::IsGroupLeader() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    return m_Group->GetID() == m_PID;
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
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    return GetProcess_pl(pid);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KProcess> KProcess::GetProcess_pl(pid_t pid) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    Ptr<KPIDNode> pidNode = kget_pid_node_pl(pid);

    return (pidNode != nullptr) ? pidNode->Process : nullptr;
}


} // namespace kernel
