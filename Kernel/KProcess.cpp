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
#include <sys/wait.h>
#include <sys/errno.h>

#include <Kernel/Kernel.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/KLogging.h>
#include <Kernel/KPIDNode.h>
#include <Kernel/KPosixSpawn.h>
#include <Kernel/KProcess.h>
#include <Kernel/KProcessGroup.h>
#include <Kernel/KProcessSession.h>
#include <Kernel/KThread.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/Scheduler.h>
#include <Kernel/VFS/Kpty.h>
#include <System/AppDefinition.h>
#include <Threads/Threads.h>
#include <Utils/Utils.h>


namespace kernel
{

KProcess* gk_KernelProcess;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::KProcess(KPIDNode& pidNode, const char* name)
    : m_Mutex("process", PEMutexRecursionMode_RaiseError)
    , m_ChildrenCondition("process")
    , m_PID(pidNode.PID)
{
    kassert(g_PIDMapMutex.IsLocked());

    strncpy(m_Name, name, OS_NAME_LENGTH);

    m_ExitInfo.si_signo = SIGCHLD;
    m_ExitInfo.si_pid   = pidNode.PID;
    m_ExitInfo.si_uid   = m_RUID;

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

KProcess::KProcess(KPIDNode& pidNode, Ptr<KProcess> parentProcess, const PPosixSpawnAttribs* spawnAttr, const char* name)
    : m_Mutex("process", PEMutexRecursionMode_RaiseError)
    , m_ChildrenCondition("process")
    , m_PID(pidNode.PID)
    , m_Parent(parentProcess)
{
    kassert(g_PIDMapMutex.IsLocked());

    strncpy(m_Name, name, OS_NAME_LENGTH);
    m_IOContext.Clone(parentProcess->m_IOContext);

#ifdef PADOS_MODULE_POSIX_SPAWN
    const bool resetIDs = spawnAttr != nullptr && (spawnAttr->sa_flags & POSIX_SPAWN_RESETIDS);
#else // PADOS_MODULE_POSIX_SPAWN
    const bool resetIDs = false;
    (void)spawnAttr;
#endif // PADOS_MODULE_POSIX_SPAWN

    m_RUID      = parentProcess->m_RUID;
    m_EUID      = resetIDs ? parentProcess->m_RUID : parentProcess->m_EUID;
    m_SUID      = parentProcess->m_SUID;
    m_RGID      = parentProcess->m_RGID;
    m_EGID      = resetIDs ? parentProcess->m_RGID : parentProcess->m_EGID;
    m_SGID      = parentProcess->m_SGID;
    m_NumGroups = parentProcess->m_NumGroups;
    m_Session   = parentProcess->m_Session;

    m_ExitInfo.si_signo = SIGCHLD;
    m_ExitInfo.si_pid   = pidNode.PID;
    m_ExitInfo.si_uid   = m_RUID;

    parentProcess->m_Group->AddProcess(this);
#ifdef PADOS_MODULE_POSIX_SIGNALS
    std::copy(std::begin(parentProcess->m_SignalHandlers), std::end(parentProcess->m_SignalHandlers), std::begin(m_SignalHandlers));
#endif // PADOS_MODULE_POSIX_SIGNALS

#ifdef PADOS_MODULE_POSIX_SPAWN
    if (spawnAttr != nullptr)
    {
        if (spawnAttr->sa_flags & POSIX_SPAWN_SETSID) {
            CreateSession();
        } else if (spawnAttr->sa_flags & POSIX_SPAWN_SETPGROUP) {
            SetPGroupID(ptr_tmp_cast(this), (spawnAttr->sa_pgroup != 0) ? spawnAttr->sa_pgroup : m_PID);
        }

#ifdef PADOS_MODULE_POSIX_SIGNALS
        if (spawnAttr->sa_flags & POSIX_SPAWN_SETSIGDEF)
        {
            for (int sigNum = 1; sigNum <= KTOTAL_SIG_COUNT; ++sigNum)
            {
                if (sigismember(&spawnAttr->sa_sigdefault, sigNum))
                {
                    m_SignalHandlers[sigNum - 1] = {};
                }
            }
        }
#endif // PADOS_MODULE_POSIX_SIGNALS
    }
#endif // PADOS_MODULE_POSIX_SPAWN
    memcpy(m_Groups, parentProcess->m_Groups, sizeof(m_Groups));

    parentProcess->m_Children.push_back(this);

    pidNode.Process = ptr_tmp_cast(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess::~KProcess()
{
    kassert(m_Threads.IsEmpty());
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

    KSchedulerLock lock;
    m_Threads.Append(thread);
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

    m_TotalCPUTime += thread->m_RunTime;

    bool lastThread;
    {
        KSchedulerLock lock;
        m_Threads.Remove(thread);
        lastThread = m_Threads.IsEmpty();
    }
    if (lastThread) {
        HandleExit();
    }
    thread->m_Process = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#ifdef PADOS_MODULE_POSIX_SIGNALS
void KProcess::ThreadStopped()
{
    kassert(KSchedulerLock::IsLocked());

    if (++m_StoppedThreadCount == m_ThreadsToStop)
    {
        if (m_State == KProcessState::Stopping)
        {
            m_State = KProcessState::Stopped;
            if (m_Parent != nullptr)
            {
                m_Parent->m_ChildrenCondition.Wakeup(1);

                const sigaction_t& parentSigaction = m_Parent->GetSignalHandler(SIGCHLD - 1);
                if ((parentSigaction.sa_flags & SA_NOCLDSTOP) == 0) {
                    m_Parent->Kill(SIGCHLD);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::ThreadContinued()
{
    kassert(KSchedulerLock::IsLocked());

    if (--m_StoppedThreadCount == m_ThreadsToStop)
    {
        if (m_State == KProcessState::Continuing)
        {
            m_State = KProcessState::Continued;
            if (m_Parent != nullptr)
            {
                m_Parent->m_ChildrenCondition.Wakeup(1);
                const sigaction_t& parentSigaction = m_Parent->GetSignalHandler(SIGCHLD - 1);
                if ((parentSigaction.sa_flags & SA_NOCLDSTOP) == 0) {
                    m_Parent->Kill(SIGCHLD);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::StopProcess(int sigNum)
{
    kassert(g_PIDMapMutex.IsLocked());

    if (m_State == KProcessState::Stopping || m_State == KProcessState::Stopped) {
        return;
    }

    SetExitStatus(CLD_STOPPED, sigNum);

    m_State = KProcessState::Stopping;

    m_ThreadsToStop = static_cast<int>(m_Threads.GetCount());

    for (KThreadCB* curThread : m_Threads) {
        ksend_signal_to_thread(*curThread, sigNum);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::ContinueProcess(int sigNum)
{
    kassert(g_PIDMapMutex.IsLocked());

    if (m_State != KProcessState::Stopping && m_State != KProcessState::Stopped) {
        return;
    }

    SetExitStatus(CLD_CONTINUED, sigNum);

    m_State = KProcessState::Continuing;
    m_ThreadsToStop = 0;

    for (KThreadCB* curThread : m_Threads) {
        ksend_signal_to_thread(*curThread, SIGCONT);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::Kill(int sigNum)
{
    KSchedulerLock lock;

    if (sigNum == SIGKILL)
    {
        for (KThreadCB* thread : m_Threads) {
            ksend_signal_to_thread(*thread, sigNum);
        }
        return;
    }
    else
    {
        for (KThreadCB* thread : m_Threads)
        {
            if (!thread->IsSignalBlocked(sigNum))
            {
                if (ksend_signal_to_thread(*thread, sigNum) == PErrorCode::Success) {
                    return;
                }
            }
        }
    }
    // If all threads block the signal, leave it pending on the main thread.
    if (!m_Threads.IsEmpty()) {
        ksend_signal_to_thread(*m_Threads.GetFirst(), sigNum);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::CancelThreads(const KThreadCB* threadToIgnore)
{
    kassert(g_PIDMapMutex.IsLocked());

    for (KThreadCB* thread : m_Threads)
    {
        if (thread != threadToIgnore) {
            kthread_cancel_pl(*thread);
        }
    }
}

#endif // PADOS_MODULE_POSIX_SIGNALS

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::RemoveChild(KProcess* child)
{
    kassert(g_PIDMapMutex.IsLocked());

    if (kensure(child->m_Parent == this))
    {
        auto it = std::find(m_Children.begin(), m_Children.end(), child);
        if (kensure(it != m_Children.end())) {
            m_Children.erase(it);
        }
        child->m_Parent = nullptr;
    }
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

    if (pidNode->Group == nullptr)
    {
        const Ptr<KProcessGroup> group = ptr_new<KProcessGroup>(pgroup, m_Session);

        group->ReserveSpace(); // Make sure AddProcess() don't throw after we modified the old group.

        pidNode->Group = group;
    }
    else
    {
        if (pidNode->Group->GetSession() != m_Session) {
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }
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

void KProcess::SetExitStatus(int exitCode, int exitStatus) noexcept
{
    const int cur = m_ExitInfo.si_code;
    if (cur != CLD_EXITED && cur != CLD_KILLED && cur != CLD_DUMPED)
    {
        m_ExitInfo.si_code = exitCode;
        m_ExitInfo.si_status = exitStatus;
    }
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

bool KProcess::IsSessionLeader() const noexcept
{
    kassert(g_PIDMapMutex.IsLocked());
    return m_Session->GetID() == m_PID;
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

siginfo_t KProcess::GetChildInfo(Ptr<KPIDNode> pidNode, int options)
{
    KProcess& child = *pidNode->Process;

    if (child.m_State == KProcessState::Zombie)
    {
        siginfo_t info = child.m_ExitInfo;
        info.si_utime = (clock_t)child.m_TotalCPUTime.AsMilliseconds();

        if ((options & WNOWAIT) == 0)
        {
            RemoveChild(&child);

            pidNode->Process = nullptr;
            kerase_pid_node_if_empty_pl(pidNode->PID);
        }
        return info;
    }

    const siginfo_t info = child.m_ExitInfo;

    if ((options & WNOWAIT) == 0)
    {
        child.m_ExitInfo.si_code = 0;

        if (child.m_State == KProcessState::Continued) {
            child.m_State = KProcessState::Running;
        }
    }
    return info;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

siginfo_t KProcess::WaitPID(pid_t pid, int options)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    for (;;)
    {
        Ptr<KPIDNode> pidNode = kget_pid_node_pl(pid);

        if (pidNode == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
        }

        Ptr<KProcess> child = pidNode->Process;

        if (child == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
        }
        if (child->m_Parent != this) {
            PERROR_THROW_CODE(PErrorCode::CHILD);
        }

        const bool isReady =
            ((options & WEXITED)    && child->m_State == KProcessState::Zombie) ||
            ((options & WSTOPPED)   && child->m_State == KProcessState::Stopped && child->m_ExitInfo.si_code == CLD_STOPPED) ||
            ((options & WCONTINUED) && child->m_State == KProcessState::Continued && child->m_ExitInfo.si_code == CLD_CONTINUED);

        if (isReady) {
            return GetChildInfo(pidNode, options);
        }
        if (options & WNOHANG) {
            return siginfo_t{};  // Child exists but no reportable state change yet.
        }
        m_ChildrenCondition.WaitCancelable(g_PIDMapMutex);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

siginfo_t KProcess::WaitGID(pid_t gid, int options)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    for (;;)
    {
        const std::vector<KProcess*>* processList = nullptr;

        if (gid != -1)
        {
            if (gid != 0)
            {
                Ptr<KPIDNode> pidNodeGroup = kget_pid_node_pl(gid);

                if (pidNodeGroup == nullptr) {
                    PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
                }

                Ptr<KProcessGroup> group = pidNodeGroup->Group;

                if (group == nullptr) {
                    PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
                }
                processList = &group->GetProcessList();
            }
            else
            {
                processList = &m_Group->GetProcessList();
            }
        }
        else
        {
            processList = &m_Children;
        }

        bool foundChild = false;

        for (KProcess* child : *processList)
        {
            if (child->m_Parent == this)
            {
                foundChild = true;

                const bool isReady =
                    ((options & WEXITED) &&    child->m_State == KProcessState::Zombie) ||
                    ((options & WSTOPPED) &&   child->m_State == KProcessState::Stopped) ||
                    ((options & WCONTINUED) && child->m_State == KProcessState::Continued);

                if (isReady)
                {
                    Ptr<KPIDNode> pidNodeChild = kget_pid_node_pl(child->GetPID());
                    if (kensure(pidNodeChild != nullptr && pidNodeChild->Process != nullptr)) {
                        return GetChildInfo(pidNodeChild, options);
                    }
                }
            }
        }

        if (!foundChild) {
            PERROR_THROW_CODE(PErrorCode::CHILD);
        }
        if (options & WNOHANG) {
            return siginfo_t{};  // Children exist but none have a reportable state change yet.
        }
        m_ChildrenCondition.WaitCancelable(g_PIDMapMutex);
    }
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KProcess::HandleExit()
{
    if (IsSessionLeader())
    {
        try {
            kdisassociate_controlling_tty_trw(false);
        }
        catch (const std::exception& exc) {}
    }

    m_Group->RemoveProcess(this);
    m_Session = nullptr;

    m_IOContext.Reset();

    // Re-parent children to init.
    if (!m_Children.empty())
    {
        for (KProcess* child : m_Children)
        {
            child->m_Parent = ptr_tmp_cast(gk_KernelProcess);
            gk_KernelProcess->m_Children.push_back(child);
        }
        m_Children.clear();

        {
            KSchedulerLock lock;
            kwakeup_init_thread();
        }
    }

    if (!kensure(m_ExitInfo.si_code != 0)) {
        m_ExitInfo.si_code   = CLD_KILLED;
    }

    m_State = KProcessState::Zombie;

    if (m_Parent != nullptr)
    {
        m_Parent->m_ChildrenCondition.Wakeup(1);
#ifdef PADOS_MODULE_POSIX_SIGNALS
        kkill_pl(m_Parent->m_PID, SIGCHLD);
#endif
    }
    else
    {
        Ptr<KPIDNode> pidNode = kget_pid_node_pl(m_PID);
        if (kensure(pidNode != nullptr))
        {
            pidNode->Process = nullptr;
            kerase_pid_node_if_empty_pl(m_PID);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kexit(int exitCode)
{
    KThreadCB& thread = kget_current_thread();
    {
        kassert(!g_PIDMapMutex.IsLocked());
        KScopedLock lock(g_PIDMapMutex);

        KProcess& process = *thread.m_Process;

        process.SetExitStatus(CLD_EXITED, exitCode);

#ifdef PADOS_MODULE_POSIX_SIGNALS
        process.CancelThreads(&thread);
#endif // PADOS_MODULE_POSIX_SIGNALS
    }
    kthread_exit((void*)(thread.IsMainThread() ? exitCode : -1));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int siginfo_to_status(const siginfo_t& info)
{
    switch (info.si_code)
    {
        case CLD_EXITED:    return info.si_status << 8;
        case CLD_KILLED:    return info.si_status;
        case CLD_DUMPED:    return info.si_status | 0x80;
        case CLD_STOPPED:   return (info.si_status << 8) | 0x7f;
        case CLD_CONTINUED: return 0xffff;
        default:            return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kwaitid_trw(idtype_t idtype, id_t id, siginfo_t* infop, int options)
{
    KProcess& process = kget_current_process();

    switch(idtype)
    {
        case P_PID:
            *infop = process.WaitPID(id, options);
            break;
        case P_PGID:
            *infop = process.WaitGID(id, options);
            break;
        case P_ALL:
            *infop = process.WaitGID(-1, options);
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kwaitid(idtype_t idtype, id_t id, siginfo_t* infop, int options) noexcept
{
    try
    {
        kwaitid_trw(idtype, id, infop, options);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

pid_t kwait_trw(int* status)
{
    siginfo_t info = {};
    kwaitid_trw(P_ALL, 0, &info, WEXITED);
    if (status != nullptr) {
        *status = siginfo_to_status(info);
    }
    return info.si_pid;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kwait(int* status, pid_t* outPID) noexcept
{
    try
    {
        *outPID = kwait_trw(status);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

pid_t kwaitpid_trw(pid_t pid, int* status, int options)
{
    idtype_t idtype;
    id_t     id;

    if (pid == -1)
    {
        idtype = P_ALL;
        id     = 0;
    }
    else if (pid > 0)
    {
        idtype = P_PID;
        id     = (id_t)pid;
    }
    else if (pid == 0)
    {
        idtype = P_PGID;
        id     = 0;
    }
    else // pid < -1
    {
        idtype = P_PGID;
        id     = (id_t)(-pid);
    }

    siginfo_t info = {};
    kwaitid_trw(idtype, id, &info, options | WEXITED);
    if (status != nullptr) {
        *status = siginfo_to_status(info);
    }
    return info.si_pid;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kwaitpid(pid_t pid, int* status, int options, pid_t* outPID) noexcept
{
    try
    {
        *outPID = kwaitpid_trw(pid, status, options);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uid_t kgetuid() noexcept
{
    KScopedLock lock(g_PIDMapMutex);
    return kget_current_process().GetRUID();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

gid_t kgetgid() noexcept
{
    KScopedLock lock(g_PIDMapMutex);
    return kget_current_process().GetRGID();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kseteuid(uid_t uid) noexcept
{
    KScopedLock lock(g_PIDMapMutex);
    KProcess& process = kget_current_process();
    if (uid != process.GetRUID() && uid != process.GetEUID() && uid != process.GetSUID()) {
        return PErrorCode::NoPermission;
    }
    process.SetEUID(uid);
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksetegid(gid_t gid) noexcept
{
    KScopedLock lock(g_PIDMapMutex);
    KProcess& process = kget_current_process();
    if (gid != process.GetRGID() && gid != process.GetEGID() && gid != process.GetSGID()) {
        return PErrorCode::NoPermission;
    }
    process.SetEGID(gid);
    return PErrorCode::Success;
}

} // namespace kernel
