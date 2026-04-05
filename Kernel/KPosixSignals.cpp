// This file is part of PadOS.
//
// Copyright (C) 2025-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 27.12.2025 21:00

#include <System/AppDefinition.h>
#include <Kernel/KThread.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KProcess.h>
#include <Kernel/KProcessGroup.h>
#include <Kernel/KStackFrames.h>
#include <Kernel/KPosixSignals.h>
#include <Kernel/KCapabilities.h>
#include <Threads/ThreadUserspaceState.h>

namespace kernel
{

KSignalQueueNode* gk_FirstFreeSignalQueueNode = nullptr;
size_t            gk_FreeSignalQueueNodeCount = 0;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool kcheck_kill_permission(const KProcess& target, int sigNum)
{
    const KProcess& sender = kget_current_process();

    if (&sender == &target) {
        return true;
    }
    if (sigNum == SIGCONT && sender.GetSession() == target.GetSession()) {
        return true;
    }
    if (sender.CheckUIDMatch(target)) {
        return true;
    }
    if (kcheck_capability(KCapability::CAP_KILL)) {
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSignalMode kget_signal_mode(int sigNum)
{
    return kget_signal_mode(kget_current_thread(), sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSignalMode kget_signal_mode(const KThreadCB& thread, int sigNum) 
{
    if (sigNum < 1 || sigNum >= KTOTAL_SIG_COUNT) {
        return KSignalMode::Invalid;
    }

    if (thread.IsSignalBlocked(sigNum)) {
        return KSignalMode::Blocked;
    }

    const sigaction_t& handler = thread.m_Process->GetSignalHandler(sigNum - 1);

    if (handler.sa_handler == SIG_DFL) {
        return KSignalMode::Default;
    } else  if (handler.sa_handler == SIG_IGN) {
        return KSignalMode::Ignored;
    } else {
        return KSignalMode::Handled;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool khas_pending_signals()
{
    return kget_current_thread().HasUnblockedPendingSignals();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool kis_thread_canceled()
{
    const KThreadCB& thread = kget_current_thread();

    return thread.m_ThreadUserData != nullptr && thread.m_ThreadUserData->IsCanceled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode  ksend_signal_to_thread(KThreadCB& thread, int sigNum)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    return ksend_signal_to_thread_pl(thread, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksend_signal_to_thread_pl(KThreadCB& thread, int sigNum) noexcept
{
    kassert(g_PIDMapMutex.IsLocked());

    if (sigNum < 0 || sigNum >= KTOTAL_SIG_COUNT) {
        return PErrorCode::InvalidArg;
    }
    if (thread.IsZombie()) {
        return PErrorCode::NoSuchProcess;
    }
    if (thread.m_KernelThread) {
        return PErrorCode::NoAccess;
    }
    if (sigNum == 0) { // Sending signal 0 succeed if a signal can be delivered and fail if not, but does not affect the target thread.
        return PErrorCode::Success;
    }

    // Signals to 'init' will only wake it up. We don't change the signal mask
    if (thread.GetHandle() == 1)
    {
        wakeup_thread(thread, false);
        return PErrorCode::Success;
    }

    thread.SetPendingSignal(sigNum);

    if (sigNum == SIGCONT || sigNum == SIGKILL) {
        wakeup_thread(thread, true);
    } else if (sigNum == SIGCHLD || !thread.IsSignalBlocked(sigNum)) {
        wakeup_thread(thread, false);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kqueue_signal_to_thread(KThreadCB& thread, int sigNum, sigval_t value)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    return kqueue_signal_to_thread_pl(thread, sigNum, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kqueue_signal_to_thread_pl(KThreadCB& thread, int sigNum, sigval_t value)
{
    kassert(g_PIDMapMutex.IsLocked());

    if (thread.IsZombie()) {
        return PErrorCode::NoSuchProcess;
    }
    Ptr<KProcess> process = thread.m_Process;

    if (!kcheck_kill_permission(*process, sigNum)) {
        PERROR_THROW_CODE(PErrorCode::NoAccess);
    }
    if (sigNum == 0) {
        return PErrorCode::Success; // Sending signal 0 succeed if a signal can be delivered and fail if not, but does not affect the target(s).
    }
    if (sigNum == SIGKILL || sigNum == SIGSTOP || sigNum == SIGCONT)
    {
        for (KThreadCB* curThread : process->GetThreads()) {
            ksend_signal_to_thread_pl(*curThread, sigNum);
        }
        return PErrorCode::Success;
    }

    if (sigNum == 0) { // Sending signal 0 succeed if a signal can be delivered and fail if not, but does not affect the target thread.
        return PErrorCode::Success;
    }
    // Signals to 'init' will only wake it up. We don't change the signal mask
    if (thread.GetHandle() == 1)
    {
        wakeup_thread(thread, false);
        return PErrorCode::Success;
    }

    KSignalQueueNode* node = kalloc_signal_queue_node();
    if (node == nullptr)
    {
        if (sigNum < SIGRTMIN)
        {
            thread.SetPendingSignal(sigNum);
            return PErrorCode::Success;
        }
        else
        {
            return PErrorCode::NoMemory;
        }
    }
    node->Next   = nullptr;
    node->SigNum = sigNum;
    node->SigInfo = {};
    node->SigInfo.si_signo = sigNum;
    node->SigInfo.si_code  = SI_QUEUE;
    node->SigInfo.si_value = value;
    {
        KSchedulerLock slock;

        if (sigNum < SIGRTMIN)
        {
            for (KSignalQueueNode** i = &thread.m_FirstQueuedSignal; ; i = &(*i)->Next)
            {
                if (*i == nullptr || (*i)->SigNum >= sigNum)
                {
                    if (*i == nullptr)
                    {
                        node->Next = nullptr;
                        *i = node;
                        thread.m_QueuedSignalCount++;
                    }
                    else if ((*i)->SigNum == sigNum)
                    {
                        node->Next = (*i)->Next;    // Only keep the last node for non-realtime signals.
                        kfree_signal_queue_node(*i);
                        *i = node;
                    }
                    else
                    {
                        node->Next = *i;
                        *i = node;
                        thread.m_QueuedSignalCount++;
                    }
                    break;
                }
            }
        }
        else
        {
            for (KSignalQueueNode** i = &thread.m_FirstQueuedSignal; ; i = &(*i)->Next)
            {
                if (*i == nullptr || (*i)->SigNum > sigNum)
                {
                    node->Next = *i;
                    *i = node;
                    thread.m_QueuedSignalCount++;
                    break;
                }
            }
        }
        thread.SetPendingSignal(sigNum);
    }
    if (!thread.IsSignalBlocked(sigNum)) {
        wakeup_thread(thread, false);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksigaction_trw(int sigNum, const struct sigaction* action, struct sigaction* outPrevAction)
{
    PERROR_ERRORCODE_THROW_ON_FAIL(ksigaction(sigNum, action, outPrevAction));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksigaction(int sigNum, const struct sigaction* action, struct sigaction* outPrevAction)
{
    const int sigIndex = sigNum - 1;

    if (sigIndex < 0 || sigIndex >= KTOTAL_SIG_COUNT) {
        return PErrorCode::InvalidArg;
    }
    KProcess& process = kget_current_process();

    if (outPrevAction != nullptr) {
        *outPrevAction = process.GetSignalHandler(sigIndex);
    }
    if (action != nullptr) {
        process.SetSignalHandler(sigIndex, *action);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

_sig_func_ptr ksignal_trw(int sigNum, _sig_func_ptr handler)
{
    sigaction_t newAction = {};
    sigaction_t prevAction;

    newAction.sa_handler = handler;

    ksigaction_trw(sigNum, &newAction, &prevAction);

    return prevAction.sa_handler;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_sigmask(int how, const sigset_t* newSet, sigset_t* outOldSet)
{
    KThreadCB& thread = kget_current_thread();

    if (outOldSet != nullptr) {
        *outOldSet = thread.m_BlockedSignals;
    }
    if (newSet != nullptr)
    {
        sigset_t newMask;
        switch (how)
        {
            case SIG_BLOCK:
                newMask = thread.m_BlockedSignals | *newSet;
                break;
            case SIG_UNBLOCK:
                newMask = thread.m_BlockedSignals & ~(*newSet);
                break;
            case SIG_SETMASK:
                newMask = *newSet;
                break;
            default:
                return PErrorCode::InvalidArg;
        }
        thread.m_BlockedSignals = newMask & KBLOCKABLE_SIGNALS_MASK;
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_kill(thread_id threadID, int sigNum)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    const Ptr<KThreadCB> thread = kget_thread(threadID);

    if (thread == nullptr) {
        return PErrorCode::NoSuchProcess;
    }
    Ptr<KProcess> process = thread->m_Process;
    if (process == nullptr) {
        return PErrorCode::NoSuchProcess;
    }

    if (!kcheck_kill_permission(*process, sigNum)) {
        PERROR_THROW_CODE(PErrorCode::NoAccess);
    }
    if (sigNum == 0) {
        return PErrorCode::Success; // Sending signal 0 succeed if a signal can be delivered and fail if not, but does not affect the target(s).
    }
    if ((sigNum == SIGKILL || sigNum == SIGSTOP || sigNum == SIGCONT) && thread->m_Process != nullptr)
    {
        for (KThreadCB* curThread : process->GetThreads()) {
            ksend_signal_to_thread_pl(*curThread, sigNum);
        }
        return PErrorCode::Success;
    }
    return ksend_signal_to_thread_pl(*thread, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TDelegate>
static void kkill_if_trw(TDelegate&& delegate, int sigNum)
{
    kassert(g_PIDMapMutex.IsLocked());

    bool deliveredAny = false;
    PErrorCode error = PErrorCode::Success;

    pid_t prevPID = -1;
    for (;;)
    {
        const Ptr<KPIDNode> pidNode = kget_next_pid_node_pl(prevPID, [](const KPIDNode& node) noexcept { return node.Process != nullptr; });

        if (pidNode == nullptr) {
            break;
        }
        prevPID = pidNode->PID;

        if (delegate(*pidNode->Process))
        {
            const PErrorCode result = kkill_pl(pidNode->PID, sigNum);
            if (result == PErrorCode::Success) {
                deliveredAny = true;
            } else if (result == PErrorCode::NoSuchProcess) {
                // Ignore processes that die before we get to kill them.
            } else if (result == PErrorCode::NoAccess || error != PErrorCode::NoAccess) {
                error = result;
            }
        }
    }
    if (!deliveredAny) {
        PERROR_ERRORCODE_THROW_ON_FAIL(error);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kkill_trw(pid_t pid, int sigNum)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    kkill_trw_pl(pid, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kkill_trw_pl(pid_t pid, int sigNum)
{
    kassert(g_PIDMapMutex.IsLocked());

    if (ksystem_log_is_category_active(LogCatKernel_Signals, PLogSeverity::INFO_HIGH_VOL))
    {
        PString targetName;
        if (pid >= 0)
        {
            Ptr<KProcess> process = KProcess::GetProcess_pl(pid);
            if (process != nullptr) {
                targetName = process->GetName();
            } else {
                targetName = "*unknown*";
            }
        }
        else
        {
            targetName = "group";
        }
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCatKernel_Signals, "kill({}({}), {}", targetName.c_str(), pid, strsignal(sigNum));
    }
    if (sigNum < 0 || sigNum >= KTOTAL_SIG_COUNT) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    if (pid == -1)
    {
        kkill_if_trw([](const KProcess& process) { return process.GetPID() > 1; }, sigNum);
    }
    else if (pid < 0)
    {
        kkillpg_trw_pl(-pid, sigNum);
    }
    else if (pid == 0)
    {
        KProcess& process = kget_current_process();
        Ptr<KProcessGroup> group = process.GetGroup();
        if (group != nullptr && group->GetID() > 1) {
            kkillpg_trw_pl(*group, sigNum);
        } else {
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }
    }
    else
    {
        Ptr<KProcess> targetProcess = KProcess::GetProcess_pl(pid);
        if (targetProcess == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
        }
        const std::vector<KThreadCB*>& threads = targetProcess->GetThreads();

        if (threads.empty()) {
            PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
        }

        if (!kcheck_kill_permission(*targetProcess, sigNum)) {
            PERROR_THROW_CODE(PErrorCode::NoAccess);
        }
        if (sigNum == 0) {
            return; // Sending signal 0 succeed if a signal can be delivered and fail if not, but does not affect the target(s).
        }
        if (sigNum == SIGKILL || sigNum == SIGSTOP || sigNum == SIGCONT)
        {
            for (KThreadCB* thread : threads) {
                ksend_signal_to_thread_pl(*thread, sigNum);
            }
            return;
        }
        else
        {
            for (KThreadCB* thread : threads)
            {
                if (!thread->IsSignalBlocked(sigNum))
                {
                    if (ksend_signal_to_thread_pl(*thread, sigNum) == PErrorCode::Success) {
                        return;
                    }
                }
            }
        }
        // If all threads block the signal, leave it pending on the main thread.
        PERROR_ERRORCODE_THROW_ON_FAIL(ksend_signal_to_thread_pl(*threads[0], sigNum));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kkill(pid_t pid, int sigNum)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    return kkill_pl(pid, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kkill_pl(pid_t pid, int sigNum)
{
    try
    {
        kkill_trw_pl(pid, sigNum);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kkillpg_trw(pid_t pgroup, int sigNum)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    kkillpg_trw_pl(pgroup, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kkillpg_trw_pl(pid_t pgroup, int sigNum)
{
    kassert(g_PIDMapMutex.IsLocked());

    if (pgroup < 1) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    Ptr<KPIDNode> pidNode = kget_pid_node_pl(pgroup);

    if (pidNode == nullptr || pidNode->Group == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
    }

    kkillpg_trw_pl(*pidNode->Group, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kkillpg_trw_pl(const KProcessGroup& group, int sigNum)
{
    kassert(g_PIDMapMutex.IsLocked());

    bool deliveredAny = false;
    PErrorCode error = PErrorCode::Success;

    for (const KProcess* process : group.GetProcessList())
    {
        const PErrorCode result = kkill_pl(process->GetPID(), sigNum);
        if (result == PErrorCode::Success) {
            deliveredAny = true;
        } else if (result == PErrorCode::NoSuchProcess) {
            // Ignore processes that die before we get to kill them.
        } else if (result == PErrorCode::NoAccess || error != PErrorCode::NoAccess) {
            error = result;
        }
    }
    if (!deliveredAny) {
        PERROR_ERRORCODE_THROW_ON_FAIL(error);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kkillpg(pid_t pgroup, int sigNum)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    return kkillpg_pl(pgroup, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kkillpg(const KProcessGroup& group, int sigNum)
{
    kassert(!g_PIDMapMutex.IsLocked());
    KScopedLock lock(g_PIDMapMutex);

    return kkillpg_pl(group, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kkillpg_pl(pid_t pgroup, int sigNum)
{
    try
    {
        kkillpg_trw_pl(pgroup, sigNum);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kkillpg_pl(const KProcessGroup& group, int sigNum)
{
    try
    {
        kkillpg_trw_pl(group, sigNum);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KSignalQueueNode* kalloc_signal_queue_node()
{
    {
        KSchedulerLock slock;

        if (gk_FirstFreeSignalQueueNode != nullptr)
        {
            KSignalQueueNode* node = gk_FirstFreeSignalQueueNode;
            gk_FirstFreeSignalQueueNode = node->Next;
            gk_FreeSignalQueueNodeCount--;
            node->Next = nullptr;
            return node;
        }
    }
    try {
        return new KSignalQueueNode;
    } catch(std::exception& exc) {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kfree_signal_queue_node(KSignalQueueNode* node)
{
    KSchedulerLock slock;

    node->Next = gk_FirstFreeSignalQueueNode;
    gk_FirstFreeSignalQueueNode = node;
    gk_FreeSignalQueueNodeCount++;
}

///////////////////////////////////////////////////////////////////////////////
/// Force synchronous signal processing from privileged code.
/// 
/// Used to handle automatic syscall restarts
/// without fully returning from the syscall.
/// 
/// Might never return, so any locks or other resources must be
/// released before calling this.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kforce_process_signals()
{
    if (!kget_current_thread().HasUnblockedPendingSignals()) {
        return;
    }
    __asm volatile (
        "   ldr     r12, =%0\n"
        "   svc     0\n"
        :: "i"(SYS_process_signals)
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
static void setup_signal_handler_exception_frame(T& frame, uintptr_t prevStackPtr, int sigNum, const KSignalStackFrame* signalFrame, const sigaction_t& sigAction)
{
    frame.ExceptionFrame.xPSR &= xPSR_T_Msk; // Clear everything but the thumb flag.
    frame.ExceptionFrame.R0 = sigNum;
    frame.ExceptionFrame.R1 = uintptr_t(&signalFrame->SigInfo);
    frame.ExceptionFrame.R2 = prevStackPtr;
    frame.ExceptionFrame.PC = uintptr_t(sigAction.sa_sigaction);
    frame.ExceptionFrame.LR = uintptr_t(__app_definition.signal_trampoline);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
static void setup_exit_handler_exception_frame(T& frame, uintptr_t prevStackPtr, void* returnValue, PThreadUserData* threadUserData)
{
    frame.ExceptionFrame.xPSR &= xPSR_T_Msk; // Clear everything but the thumb flag.
    frame.ExceptionFrame.R0 = uintptr_t(returnValue);
    frame.ExceptionFrame.R1 = uintptr_t(threadUserData);
    frame.ExceptionFrame.PC = uintptr_t(__app_definition.thread_terminated);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static uintptr_t add_signal_handler_frame(uintptr_t prevStackPtr, KThreadCB& thread, bool userMode, const sigaction_t& sigAction, int sigNum, const siginfo_t& sigInfo)
{
    KCtxSwitchKernelStackFrame* prevStackFrame = reinterpret_cast<KCtxSwitchKernelStackFrame*>(prevStackPtr);

    const bool   hasFPUFrame = exception_has_fpu_frame(prevStackFrame->EXEC_RETURN);
    const size_t frameSize = hasFPUFrame ? sizeof(KCtxSwitchStackFrameFPU) : sizeof(KCtxSwitchStackFrame);

    const uintptr_t signalFramePtr = prevStackPtr - sizeof(KSignalStackFrame);
    const uintptr_t newStackPtr = signalFramePtr - frameSize;

    KSignalStackFrame* const signalFrame = reinterpret_cast<KSignalStackFrame*>(signalFramePtr);

    signalFrame->PreSignalPSPAndPrivilege = prevStackPtr;
    if (userMode) {
        signalFrame->PreSignalPSPAndPrivilege |= 0x01;
    } else {
        signalFrame->PreSignalPSPAndPrivilege &= ~0x01;
    }
    signalFrame->SignalMask = thread.m_BlockedSignals;

    signalFrame->SigInfo = sigInfo;

    thread.m_BlockedSignals |= sigAction.sa_mask;
    if ((sigAction.sa_flags & SA_NODEFER) == 0) {
        thread.m_BlockedSignals |= sig_mkmask(sigInfo.si_signo);
    }
    thread.m_BlockedSignals &= KBLOCKABLE_SIGNALS_MASK;

    memcpy(reinterpret_cast<void*>(newStackPtr), reinterpret_cast<const void*>(prevStackPtr), frameSize);

    if (hasFPUFrame) {
        setup_signal_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrameFPU*>(newStackPtr), prevStackPtr, sigNum, signalFrame, sigAction);
    } else {
        setup_signal_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrame*>(newStackPtr), prevStackPtr, sigNum, signalFrame, sigAction);
    }
    return newStackPtr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uintptr_t kprocess_signal(int sigNum, const uintptr_t prevStackPtr, bool userMode, bool fromFault, const siginfo_t* extSigInfo)
{
    static_assert(sizeof(KSignalStackFrame) % 8 == 0);
    static_assert(sizeof(KCtxSwitchStackFrame) % 8 == 0);
    static_assert(sizeof(KCtxSwitchStackFrameFPU) % 8 == 0);

    KThreadCB&  thread  = kget_current_thread();
    KProcess&   process = kget_current_process();

    const int signalIndex = sigNum - 1;

    siginfo_t sigInfo;

    if (extSigInfo == nullptr)
    {
        KSignalQueueNode* queuNode = nullptr;
        {
            KSchedulerLock slock;

            for (KSignalQueueNode** i = &thread.m_FirstQueuedSignal; *i != nullptr && (*i)->SigNum <= sigNum; i = &(*i)->Next)
            {
                if ((*i)->SigNum == sigNum)
                {
                    queuNode = *i;
                    *i = queuNode->Next;

                    thread.m_QueuedSignalCount--;

                    if (queuNode->Next == nullptr || queuNode->Next->SigNum != sigNum) {
                        thread.ClearPendingSignal(sigNum);
                    }

                    break;
                }
            }
        }
        if (queuNode != nullptr)
        {
            sigInfo = queuNode->SigInfo;
            kfree_signal_queue_node(queuNode);
        }
        else
        {
            sigInfo = {};
            sigInfo.si_signo = sigNum;
            sigInfo.si_code  = SI_USER;

            thread.ClearPendingSignal(sigNum);
        }
    }
    else
    {
        sigInfo = *extSigInfo;

        thread.ClearPendingSignal(sigNum);
    }
    sigInfo.si_signo = sigNum;

    sigaction_t sigAction = process.GetSignalHandler(signalIndex);

    if (sigAction.sa_handler == SIG_DFL || (sigAction.sa_handler == SIG_IGN && fromFault) || !sig_can_be_ignored(sigNum))
    {
        const PESignalDefaultAction action = sig_get_default_action(sigNum);
        if (action == PESignalDefaultAction::Stop)
        {
            Ptr<KProcess> parent = process.GetParent_pl();
            bool notifyParent = false;
            if (parent != nullptr)
            {
                const sigaction_t& parentSigaction = parent->GetSignalHandler(SIGCHLD - 1);
                notifyParent = (parentSigaction.sa_flags & SA_NOCLDSTOP) == 0;
            }
            stop_thread(notifyParent);
        }
        else if (action == PESignalDefaultAction::Terminate || action == PESignalDefaultAction::TerminateCoreDump)
        {
            if (thread.GetState() == ThreadState_Stopped) {
                wakeup_thread(thread, true);
            }
            sigaction_t terminateAction = {};
            terminateAction.sa_sigaction = __app_definition.signal_terminate_thread;
            return add_signal_handler_frame(prevStackPtr, thread, userMode, terminateAction, fromFault ? sigNum : -sigNum, sigInfo);
        }
        return prevStackPtr;
    }

    if (sigAction.sa_handler == SIG_IGN || sigAction.sa_handler == SIG_ERR) {
        return prevStackPtr;
    }

    if (prevStackPtr & 0x07) {
        kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_Scheduler, "{}: Unaligned SP: {:#08x}", __PRETTY_FUNCTION__, prevStackPtr);
    }
    const uintptr_t newStackPtr = add_signal_handler_frame(prevStackPtr, thread, userMode, sigAction, sigNum, sigInfo);

    if ((sigAction.sa_flags & SA_RESETHAND) && sig_can_auto_reset(sigNum))
    {
        sigAction.sa_handler = SIG_DFL;
        sigAction.sa_flags &= ~(SA_SIGINFO | SA_RESETHAND);
        process.SetSignalHandler(signalIndex, sigAction);
    }
    return newStackPtr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" uintptr_t kprocess_pending_signals(uintptr_t curStackPtr, bool userMode)
{
    KThreadCB& thread = kget_current_thread();

    sigset_t pendingSignals = thread.GetUnblockedPendingSignals();

    if (pendingSignals == 0) {
        return curStackPtr;
    }

    for (int sigNum : {SIGKILL, SIGSTOP, SIGCONT})
    {
        const sigset_t sigMask = sig_mkmask(sigNum);
        if (pendingSignals & sigMask)
        {
            pendingSignals &= ~sigMask;
            const uintptr_t newStackPtr = kprocess_signal(sigNum, curStackPtr, userMode, /*fromFault*/ false, /*sigInfo*/ nullptr);
            if (newStackPtr != curStackPtr) {
                return newStackPtr;
            }
        }
    }
    if (pendingSignals)
    {
        const int beginSignal = std::countr_zero(pendingSignals);
        const int endSignal   = sizeof(pendingSignals) * 8 - std::countl_zero(pendingSignals);
        sigset_t curMask = sig_mkmask(beginSignal + 1);
        for (int i = beginSignal; i < endSignal; ++i, curMask <<= 1)
        {
            if (pendingSignals & curMask)
            {
                const uintptr_t newStackPtr = kprocess_signal(i + 1, curStackPtr, userMode, /*fromFault*/ false, /*sigInfo*/ nullptr);
                if (newStackPtr != curStackPtr) {
                    return newStackPtr;
                }
            }
        }
    }
    return curStackPtr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" uintptr_t ksigreturn(uintptr_t curStackPtr)
{
    KThreadCB& thread = kget_current_thread();

    const KSignalStackFrame* signalStackFrame = reinterpret_cast<KSignalStackFrame*>(curStackPtr);

    const uint32_t control = __get_CONTROL();
    __set_CONTROL((control & ~0x01) | (signalStackFrame->PreSignalPSPAndPrivilege & 0x01)); // Restore nPRIV

    thread.m_BlockedSignals = signalStackFrame->SignalMask & KBLOCKABLE_SIGNALS_MASK;

    return kprocess_pending_signals(signalStackFrame->PreSignalPSPAndPrivilege & ~0x01, (signalStackFrame->PreSignalPSPAndPrivilege & 0x01) != 0);
}

extern "C" uintptr_t kprocess_thread_exit(uintptr_t prevStackPtr, void* returnValue)
{
    KCtxSwitchKernelStackFrame* prevStackFrame = reinterpret_cast<KCtxSwitchKernelStackFrame*>(prevStackPtr);

    const bool   hasFPUFrame = exception_has_fpu_frame(prevStackFrame->EXEC_RETURN);
    const size_t frameSize = hasFPUFrame ? sizeof(KCtxSwitchStackFrameFPU) : sizeof(KCtxSwitchStackFrame);

    const uintptr_t newStackPtr = prevStackPtr - frameSize;

    memcpy(reinterpret_cast<void*>(newStackPtr), reinterpret_cast<const void*>(prevStackPtr), frameSize);

    KThreadCB& thread = kget_current_thread();

    if (hasFPUFrame) {
        setup_exit_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrameFPU*>(newStackPtr), prevStackPtr, returnValue, thread.m_ThreadUserData);
    } else {
        setup_exit_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrame*>(newStackPtr), prevStackPtr, returnValue, thread.m_ThreadUserData);
    }
    return newStackPtr;
}

} // namespace kernel
