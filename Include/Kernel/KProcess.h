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

#include <sys/param.h>
#include <sys/pados_syscalls.h>

#include <Ptr/PtrTarget.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KMutex.h>
#include <Kernel/VFS/KIOContext.h>
#include <Threads/Threads.h>
#include <System/ExceptionHandling.h>

namespace kernel
{
extern KProcess* gk_KernelProcess;

class KProcess : public PtrTarget
{
public:
    KProcess(const char* name);
    KProcess(const KProcess& parentProcess, const char* name);
    ~KProcess();

    static KMutex& GetProcessMutex() { return s_ProcessMutex; }
    static const std::map<pid_t, KProcess*>& GetProcessMap() { kassert(s_ProcessMutex.IsLocked()); return s_ProcessMap; }

    void AddThread(KThreadCB* thread);
    void RemoveThread(KThreadCB* thread) noexcept;

    const std::vector<KThreadCB*>& GetThreadListRef() const { kassert(s_ProcessMutex.IsLocked()); return m_Threads; }
    std::vector<Ptr<KThreadCB>> GetThreads() const;

    const char* GetName() const { return m_Name; }
    pid_t       GetPID() const noexcept { return m_PID; }
    pid_t       GetParentPID() const noexcept { return m_ParentPID; }
    void        SetPGroupID(pid_t pgroup) noexcept { m_PGroupID = pgroup; }
    pid_t       GetPGroupID() const noexcept { return m_PGroupID; }

    const KIOContext&   GetIOContext() const noexcept { return m_IOContext; }
    KIOContext&         GetIOContext() noexcept { return m_IOContext; }

    int     GetSession() const noexcept { return m_Session; }
    void    SetSession(int session) noexcept { m_Session = m_PGroupID = session; }
    bool    IsGroupLeader() const noexcept { return m_PGroupID == m_PID; }
    bool    HasExeced() const noexcept { return false; }

    uid_t   GetRUID() const noexcept { return m_RUID; }
    uid_t   GetEUID() const noexcept { return m_EUID; }
    uid_t   GetSUID() const noexcept { return m_SUID; }

    gid_t   GetRGID() const noexcept { return m_RGID; }
    gid_t   GetEGID() const noexcept { return m_EGID; }
    gid_t   GetSGID() const noexcept { return m_SGID; }

    bool CheckUIDMatch(uid_t uid) const noexcept;
    bool CheckUIDMatch(const KProcess& target) const noexcept;

    static Ptr<KProcess> GetProcess(pid_t pid) noexcept;

    template<typename Tcallback>
    static void ForEachProcess(Tcallback&& callback)
    {
        CRITICAL_SCOPE(s_ProcessMutex);

        pid_t currentPID = 0;

        for (auto it = s_ProcessMap.begin();
            it != s_ProcessMap.end();
            it = s_ProcessMap.upper_bound(currentPID))
        {
            currentPID = it->first;
            Ptr<KProcess> process = ptr_tmp_cast(it->second);

            s_ProcessMutex.Unlock();
            const PScopeExit scopeRelock([]() { s_ProcessMutex.Lock(); });

            if constexpr (std::is_void_v<std::invoke_result_t<Tcallback, KProcess&>>)
            {
                std::invoke(callback, *process);
            }
            else
            {
                if (!std::invoke(callback, *process)) {
                    break;
                }
            }
        }
    }
private:
    void SetPID(pid_t pid);

    static KMutex s_ProcessMutex;
    static std::map<pid_t, KProcess*> s_ProcessMap;
    char       m_Name[OS_NAME_LENGTH];
    KIOContext m_IOContext;

    std::vector<KThreadCB*> m_Threads;

    pid_t      m_ParentPID = -1;    // Parent process.
    pid_t      m_PID = -1;          // Our process ID.

    uid_t      m_RUID = 0;  // Real User ID.
    uid_t      m_EUID = 0;  // Effective User ID.
    uid_t      m_SUID = 0;  // Set User ID.
    
    gid_t      m_RGID = 0;  // Real Group ID.
    gid_t      m_EGID = 0;  // Effective Group ID.
    gid_t      m_SGID = 0;  // Set Group ID.

    int        m_NumGroups = 0;
    gid_t      m_Groups[NGROUPS];


    pid_t      m_PGroupID = -1;
    int        m_Session = -1;

    KProcess(const KProcess &) = delete;
    KProcess& operator=(const KProcess &) = delete;
};

class KProcessLock
{
public:
    KProcessLock() { Lock(); }
    ~KProcessLock() { Unlock(); }

    void Lock() { if (!m_IsLocked) { KProcess::GetProcessMutex().Lock(); m_IsLocked = true; } }
    void Unlock() { if (m_IsLocked) { KProcess::GetProcessMutex().Unlock(); m_IsLocked = false; } }

private:
    bool m_IsLocked = false;
};

} // namespace kernel
