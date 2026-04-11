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
// Created: 11.03.2018 01:23:13

#pragma once

#include <functional>

#include <sys/wait.h>
#include <sys/param.h>
#include <sys/pados_syscalls.h>

#include <Ptr/PtrTarget.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/VFS/KIOContext.h>
#include <Threads/Threads.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

class KProcessGroup;
class KProcessSession;

struct KPIDNode;

extern KProcess* gk_KernelProcess;


enum KProcessState
{
    Running,
    Stopping,
    Stopped,
    Continuing,
    Continued,
    Terminating,
    Zombie
};

class KProcess : public PtrTarget
{
public:
    KProcess(KPIDNode& pidNode, const char* name);
    KProcess(KPIDNode& pidNode, Ptr<KProcess> parentProcess, const __posix_spawnattr* spawnAttr, const char* name);
    ~KProcess();

    void AddThread(KThreadCB* thread);
    void RemoveThread(KThreadCB* thread) noexcept;
    
    void ThreadStopped();
    void ThreadContinued();

    void RemoveChild(KProcess* child);

    const char*         GetName() const noexcept { return m_Name; }
    pid_t               GetPID() const noexcept { return m_PID; }

    void                SetPGroupID(Ptr<const KProcess> instigator, pid_t pgroup);
    pid_t               GetPGroupID() const noexcept;
    void                SetGroup(Ptr<KProcessGroup> group) noexcept;
    Ptr<KProcessGroup>  GetGroup() const noexcept;

    void SetExitStatus(int exitCode, int exitStatus) noexcept;
    bool IsExitStatusSet() const noexcept { return m_ExitInfo.si_code != 0; }

    const KIOContext&   GetIOContext() const noexcept { return m_IOContext; }
    KIOContext&         GetIOContext() noexcept { return m_IOContext; }

    Ptr<KProcess> GetParent_pl() const noexcept { return m_Parent; }

    int                     GetSessionID() const noexcept;
    Ptr<KProcessSession>    GetSession() const noexcept;
    pid_t    CreateSession();

    bool    IsSessionLeader() const noexcept;
    bool    IsGroupLeader() const noexcept;
    bool    HasExeced() const noexcept { return false; }

    void                SetSignalHandler(int sigNum, const sigaction_t& action) noexcept { kassert(sigNum >= 0 && sigNum < KTOTAL_SIG_COUNT); m_SignalHandlers[sigNum] = action; }
    const sigaction_t&  GetSignalHandler(int sigNum) const noexcept { kassert(sigNum >= 0 && sigNum < KTOTAL_SIG_COUNT); return m_SignalHandlers[sigNum]; }

    uid_t   GetRUID() const noexcept { return m_RUID; }
    uid_t   GetEUID() const noexcept { return m_EUID; }
    uid_t   GetSUID() const noexcept { return m_SUID; }
    void    SetEUID(uid_t euid) noexcept { m_EUID = euid; }

    gid_t   GetRGID() const noexcept { return m_RGID; }
    gid_t   GetEGID() const noexcept { return m_EGID; }
    gid_t   GetSGID() const noexcept { return m_SGID; }
    void    SetEGID(gid_t egid) noexcept { m_EGID = egid; }

    bool CheckUIDMatch(uid_t uid) const noexcept;
    bool CheckUIDMatch(const KProcess& target) const noexcept;

    void StopProcess(int sigNum);
    void ContinueProcess(int sigNum);
    void Kill(int sigNum);

    void CancelThreads(const KThreadCB* threadToIgnore);

    siginfo_t GetChildInfo(Ptr<KPIDNode> pidNode, int options);
    siginfo_t WaitPID(pid_t pid, int options);
    siginfo_t WaitGID(pid_t gid, int options);

    static Ptr<KProcess> GetProcess(pid_t pid) noexcept;
    static Ptr<KProcess> GetProcess_pl(pid_t pid) noexcept;

private:
    void HandleExit();

    mutable KMutex m_Mutex;

    char       m_Name[OS_NAME_LENGTH];
    KIOContext m_IOContext;

    KProcessState           m_State = KProcessState::Running;
    KConditionVariable      m_ChildrenCondition;
    KProcessThreadList      m_Threads;
    sigaction_t             m_SignalHandlers[KTOTAL_SIG_COUNT] = {};

    pid_t      m_PID = -1;  // Our process ID.

    uid_t      m_RUID = 0;  // Real User ID.
    uid_t      m_EUID = 0;  // Effective User ID.
    uid_t      m_SUID = 0;  // Set User ID.
    
    gid_t      m_RGID = 0;  // Real Group ID.
    gid_t      m_EGID = 0;  // Effective Group ID.
    gid_t      m_SGID = 0;  // Set Group ID.

    int        m_NumGroups = 0;
    gid_t      m_Groups[NGROUPS];

    Ptr<KProcess>           m_Parent;
    Ptr<KProcessSession>    m_Session;
    Ptr<KProcessGroup>      m_Group;

    std::vector<KProcess*> m_Children;

    std::atomic<int>    m_StoppedThreadCount = 0;
    std::atomic<int>    m_ThreadsToStop = 0;

    siginfo_t m_ExitInfo = {};
    TimeValNanos m_TotalCPUTime;

    KProcess(const KProcess &) = delete;
    KProcess& operator=(const KProcess &) = delete;
};

void kexit(int exitCode);
void kwaitid_trw(idtype_t idtype, id_t id, siginfo_t* infop, int options);

PErrorCode kwaitid(idtype_t idtype, id_t id, siginfo_t* infop, int options) noexcept;

uid_t      kgetuid() noexcept;
gid_t      kgetgid() noexcept;
PErrorCode kseteuid(uid_t uid) noexcept;
PErrorCode ksetegid(gid_t gid) noexcept;


} // namespace kernel
