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
#include <Kernel/KStackFrames.h>
#include <Kernel/KPosixSignals.h>

namespace kernel
{

KSignalQueueNode* gk_FirstFreeSignalQueueNode = nullptr;
size_t            gk_FreeSignalQueueNodeCount = 0;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksend_signal_to_thread(KThreadCB& thread, int sigNum)
{
    if (thread.m_State == ThreadState_Zombie) {
        return PErrorCode::NoSuchProcess;
    }
    if (sigNum == 0) { // Sending signal 0 succeed if a signal can be delivered and fail if not, but does not affect the target thread.
        return PErrorCode::Success;
    }

    // Signals to 'init' will only wake it up. We don't change the signal mask
    if (thread.GetHandle() == 1)
    {
        wakeup_thread(thread.GetHandle(), false);
        return PErrorCode::Success;
    }

    thread.SetPendingSignal(sigNum);

    if (sigNum == SIGCONT || sigNum == SIGKILL) {
        wakeup_thread(thread.GetHandle(), true);
    }
    else if (sigNum == SIGCHLD || !thread.IsSignalBlocked(sigNum)) {
        wakeup_thread(thread.GetHandle(), false);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kqueue_signal_to_thread(KThreadCB& thread, int sigNum, sigval_t value)
{
    if (thread.m_State == ThreadState_Zombie) {
        return PErrorCode::NoSuchProcess;
    }
    if (sigNum == 0) { // Sending signal 0 succeed if a signal can be delivered and fail if not, but does not affect the target thread.
        return PErrorCode::Success;
    }
    // Signals to 'init' will only wake it up. We don't change the signal mask
    if (thread.GetHandle() == 1)
    {
        wakeup_thread(thread.GetHandle(), false);
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
        wakeup_thread(thread.GetHandle(), false);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kthread_sigmask(int how, const sigset_t* newSet, sigset_t* outOldSet)
{
    KThreadCB& thread = *gk_CurrentThread;

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
    if (!gk_CurrentThread->HasUnblockedPendingSignals()) {
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
static void setup_signal_handler_exception_frame(T& frame, uintptr_t prevStackPtr, const KSignalStackFrame* signalFrame, const sigaction_t& sigAction)
{
    frame.ExceptionFrame.xPSR &= xPSR_T_Msk; // Clear everything but the thumb flag.
    frame.ExceptionFrame.R0 = signalFrame->SigInfo.si_signo;
    frame.ExceptionFrame.R1 = uintptr_t(&signalFrame->SigInfo);
    frame.ExceptionFrame.R2 = prevStackPtr;
    frame.ExceptionFrame.PC = uintptr_t(sigAction.sa_sigaction);
    frame.ExceptionFrame.LR = uintptr_t(__app_definition.signal_trampoline);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static uintptr_t add_signal_handler_frame(uintptr_t prevStackPtr, KThreadCB& thread, bool userMode, const sigaction_t& sigAction, const siginfo_t& sigInfo)
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
        setup_signal_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrameFPU*>(newStackPtr), prevStackPtr, signalFrame, sigAction);
    } else {
        setup_signal_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrame*>(newStackPtr), prevStackPtr, signalFrame, sigAction);
    }
    return newStackPtr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

intptr_t kprocess_signal(int sigNum, const uintptr_t prevStackPtr, bool userMode, bool fromFault, const siginfo_t* extSigInfo)
{
    static_assert(sizeof(KSignalStackFrame) % 8 == 0);
    static_assert(sizeof(KCtxSwitchStackFrame) % 8 == 0);
    static_assert(sizeof(KCtxSwitchStackFrameFPU) % 8 == 0);

    KThreadCB& thread = *gk_CurrentThread;

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

    sigaction_t& sigAction = thread.m_SignalHandlers[signalIndex];

    if (sigAction.sa_handler == SIG_DFL || (sigAction.sa_handler == SIG_IGN && fromFault) || !sig_can_be_ignored(sigNum))
    {
        const PESignalDefaultAction action = sig_get_default_action(sigNum);
        if (action == PESignalDefaultAction::Stop)
        {
            stop_thread(false);
        }
        else if (action == PESignalDefaultAction::Terminate || action == PESignalDefaultAction::TerminateCoreDump)
        {
            if (thread.m_State == ThreadState_Stopped) {
                wakeup_thread(thread.GetHandle(), true);
            }
            sigaction_t terminateAction = {};
            terminateAction.sa_sigaction = __app_definition.signal_terminate_thread;
            return add_signal_handler_frame(prevStackPtr, thread, userMode, terminateAction, sigInfo);
        }
        return prevStackPtr;
    }

    if (sigAction.sa_handler == SIG_IGN || sigAction.sa_handler == SIG_ERR) {
        return prevStackPtr;
    }

    if ((sigAction.sa_flags & SA_RESETHAND) && sig_can_auto_reset(sigNum))
    {
        sigAction.sa_handler = SIG_DFL;
        sigAction.sa_flags &= ~(SA_SIGINFO | SA_RESETHAND);
    }

    if (prevStackPtr & 0x07) {
        kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_Scheduler, "{}: Unaligned SP: {:#08x}", __PRETTY_FUNCTION__, prevStackPtr);
    }
    return add_signal_handler_frame(prevStackPtr, thread, userMode, sigAction, sigInfo);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" uintptr_t kprocess_pending_signals(intptr_t curStackPtr, bool userMode)
{
    KThreadCB& thread = *gk_CurrentThread;

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
    KThreadCB& thread = *gk_CurrentThread;

    const KSignalStackFrame* signalStackFrame = reinterpret_cast<KSignalStackFrame*>(curStackPtr);

    const uint32_t control = __get_CONTROL();
    __set_CONTROL((control & ~0x01) | (signalStackFrame->PreSignalPSPAndPrivilege & 0x01)); // Restore nPRIV

    thread.m_BlockedSignals = signalStackFrame->SignalMask & KBLOCKABLE_SIGNALS_MASK;

    return kprocess_pending_signals(signalStackFrame->PreSignalPSPAndPrivilege & ~0x01, (signalStackFrame->PreSignalPSPAndPrivilege & 0x01) != 0);
}

} // namespace kernel
