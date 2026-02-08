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
// Created: 04.03.2018 19:30:41

#pragma once

#include <sys/signal.h>
#include <sys/pados_threads.h>
#include <PadOS/Threads.h>
#include <System/TimeValue.h>
#include <Utils/IntrusiveList.h>
#include <Threads/Threads.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/KSchedulerLock.h>


extern PThreadControlBlock* __kernel_thread_data;

static constexpr int32_t THREAD_MAX_TLS_SLOTS = 256;

namespace kernel
{
struct KSignalQueueNode;

static constexpr int32_t THREAD_DEFAULT_STACK_SIZE = 1024*32;

static const int KTHREAD_PRIORITY_MIN = -16;
static const int KTHREAD_PRIORITY_MAX = 15;
static const int KTHREAD_PRIORITY_LEVELS = KTHREAD_PRIORITY_MAX - KTHREAD_PRIORITY_MIN + 1;

class KThreadCB : public KNamedObject
{
public:
    static const KNamedObjectType ObjectType = KNamedObjectType::Thread;

    KThreadCB(const PThreadAttribs* attribs, PThreadControlBlock* tlsBlock, void* kernelTLSMemory);
    ~KThreadCB();

    virtual void SetHandle(int32_t handle) noexcept override;
    pid_t GetProcessID() const;
    void InitializeStack(ThreadEntryPoint_t entryPoint, bool privileged, bool skipEntryTrampoline, void* arguments);

    uint8_t* GetStackTop() const { return m_StackBuffer; }
    uint8_t* GetStackBottom() const { return m_StackBuffer + m_StackSize - 4; }

    int SetPriority(int priority) { m_PriorityLevel = PriToLevel(priority); return 0; }
    int GetPriority() const { return LevelToPri(m_PriorityLevel);  }
    int GetPriorityLevel() const { return m_PriorityLevel; }

    static int PriToLevel(int priority);
    static int LevelToPri(int level);

    sigset_t GetPendingSignals() const { KSchedulerLock slock; return GetUnblockedSignals(m_PendingSignals_); }
    sigset_t GetUnblockedSignals(sigset_t signalMask) const { return signalMask & ~m_BlockedSignals; }
    sigset_t GetUnblockedPendingSignals() const { return GetUnblockedSignals(GetPendingSignals()); }
    bool     HasUnblockedPendingSignals() const { return GetUnblockedPendingSignals() != 0; }
    bool     IsSignalBlocked(int sigNum) const { return (m_BlockedSignals & sig_mkmask(sigNum)) != 0; }
    void     ReplacePendingSignals(sigset_t newSet) { KSchedulerLock slock; m_PendingSignals_ = newSet; }
    void     MergePendingSignals(sigset_t set) { KSchedulerLock slock; m_PendingSignals_ |= set; }

    void     SetPendingSignal(int sigNum) { const sigset_t mask = sig_mkmask(sigNum); KSchedulerLock slock; m_PendingSignals_ |= mask; }
    void     ClearPendingSignal(int sigNum) { const sigset_t invMask = ~sig_mkmask(sigNum); KSchedulerLock slock; m_PendingSignals_ &= invMask; }

    void                SetBlockingObject(const KNamedObject* WaitObject);
    const KNamedObject* GetBlockingObject() const { return m_BlockingObject; }

    void SetupTLS(const PThreadAttribs* attribs, void* kernelTLSMemory);

    // For suspended threads, this holds upper 31 bits of current stack
    // frame address in bit 31:1 and current CONTROL.nPRIV value in bit 0.
    uint32_t                  m_CurrentStackAndPrivilege = 0;

    // For threads currently executing a syscall, this hold the return
    // address with the thumb flag replaced by CONTROL.nPRIV.
    uint32_t                  m_SyscallReturn = 0;

    ThreadState               m_State;
    int                       m_PriorityLevel;
    TimeValNanos              m_StartTime;
    TimeValNanos              m_RunTime;
    void*                     m_ReturnValue = nullptr;
    PThreadDetachState        m_DetachState = PThreadDetachState_Detached;
    bool                      m_FreeStackOnExit = true;
    bool                      m_FreeTLSOnExit = true;
    uint8_t*                  m_StackBuffer;
    int                       m_StackSize;

    PThreadControlBlock*      m_KernelTLS = nullptr;
    PThreadControlBlock*      m_UserspaceTLS = nullptr;

    KThreadCB*                m_Prev = nullptr;
    KThreadCB*                m_Next = nullptr;
    PIntrusiveList<KThreadCB>* m_List = nullptr;
    const KNamedObject*       m_BlockingObject = nullptr;

    int                       m_SymlinkDepth = 0;

    sigset_t                  m_PendingSignals_ = 0;
    sigset_t		          m_BlockedSignals = 0;
    sigaction_t               m_SignalHandlers[NSIG + NRTSIG];
    KSignalQueueNode*         m_FirstQueuedSignal = nullptr;
    size_t                    m_QueuedSignalCount = 0;
};

typedef PIntrusiveList<KThreadCB>       KThreadList;

} // namespace
