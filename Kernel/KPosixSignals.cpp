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
    const sigset_t  sigMask = sig_mkmask(sigNum);

    thread.m_PendingSignals |= sigMask;


    if (sigNum == SIGCONT || sigNum == SIGKILL) {
        wakeup_thread(thread.GetHandle(), true);
    }
    else if (sigNum == SIGCHLD || thread.GetUnblockedSignals(sigMask) != 0) {
        wakeup_thread(thread.GetHandle(), false);
    }
    return PErrorCode::Success;
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
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        if (gk_CurrentThread->GetUnblockedPendingSignals() == 0) {
            return;
        }
    } CRITICAL_END;

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
static void setup_signal_termination_exception_frame(T& frame, int sigNum)
{
    frame.ExceptionFrame.xPSR &= xPSR_T_Msk; // Clear everything but the thumb flag.
    frame.ExceptionFrame.R0 = sigNum;
    frame.ExceptionFrame.PC = uintptr_t(__app_definition.signal_terminate_thread);
    frame.ExceptionFrame.LR = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static uintptr_t add_signal_terminate_frame(uintptr_t prevStackPtr, int sigNum)
{
    KCtxSwitchKernelStackFrame* prevStackFrame = reinterpret_cast<KCtxSwitchKernelStackFrame*>(prevStackPtr);

    const bool   hasFPUFrame = (prevStackFrame->EXEC_RETURN & 0x10) == 0;
    const size_t frameSize = hasFPUFrame ? sizeof(KCtxSwitchStackFrameFPU) : sizeof(KCtxSwitchStackFrame);

    const uintptr_t newStackPtr = prevStackPtr - frameSize;

    memcpy(reinterpret_cast<void*>(newStackPtr), reinterpret_cast<const void*>(prevStackPtr), frameSize);

    if (hasFPUFrame) {
        setup_signal_termination_exception_frame(*reinterpret_cast<KCtxSwitchStackFrameFPU*>(newStackPtr), sigNum);
    }
    else {
        setup_signal_termination_exception_frame(*reinterpret_cast<KCtxSwitchStackFrame*>(newStackPtr), sigNum);
    }
    return newStackPtr;
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

static uintptr_t add_signal_handler_frame(uintptr_t prevStackPtr, KThreadCB& thread, bool userMode, int sigNum)
{
    const int    signalIndex = sigNum - 1;
    sigaction_t& sigAction   = thread.m_SignalHandlers[signalIndex];

    KCtxSwitchKernelStackFrame* prevStackFrame = reinterpret_cast<KCtxSwitchKernelStackFrame*>(prevStackPtr);

    const bool   hasFPUFrame = (prevStackFrame->EXEC_RETURN & 0x10) == 0;
    const size_t frameSize = hasFPUFrame ? sizeof(KCtxSwitchStackFrameFPU) : sizeof(KCtxSwitchStackFrame);

    const uintptr_t signalFramePtr = prevStackPtr - sizeof(KSignalStackFrame);
    const uintptr_t newStackPtr = signalFramePtr - frameSize;

    KSignalStackFrame* const signalFrame = reinterpret_cast<KSignalStackFrame*>(signalFramePtr);

    signalFrame->PreSignalPSPAndPrivilege = prevStackPtr;
    if (userMode) {
        signalFrame->PreSignalPSPAndPrivilege |= 0x01;
    }
    else {
        signalFrame->PreSignalPSPAndPrivilege &= ~0x01;
    }
    signalFrame->SignalMask = thread.m_BlockedSignals;

    signalFrame->SigInfo.si_signo = sigNum;
    signalFrame->SigInfo.si_code = SI_USER;
    signalFrame->SigInfo.si_value.sival_int = 0;

    thread.m_BlockedSignals |= sigAction.sa_mask;
    if ((sigAction.sa_flags & SA_NODEFER) == 0) {
        thread.m_BlockedSignals |= sig_mkmask(sigNum);
    }

    memcpy(reinterpret_cast<void*>(newStackPtr), reinterpret_cast<const void*>(prevStackPtr), frameSize);

    if (hasFPUFrame) {
        setup_signal_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrameFPU*>(newStackPtr), prevStackPtr, signalFrame, sigAction);
    }
    else {
        setup_signal_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrame*>(newStackPtr), prevStackPtr, signalFrame, sigAction);
    }
    return newStackPtr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static intptr_t process_signal(const uintptr_t prevStackPtr, KThreadCB& thread, bool userMode, int sigNum)
{
    static_assert(sizeof(KSignalStackFrame) % 8 == 0);
    static_assert(sizeof(KCtxSwitchStackFrame) % 8 == 0);
    static_assert(sizeof(KCtxSwitchStackFrameFPU) % 8 == 0);

    const sigset_t sigMask = sig_mkmask(sigNum);
    const int signalIndex = sigNum - 1;

    thread.m_PendingSignals &= ~sigMask;

    sigaction_t& sigAction = thread.m_SignalHandlers[signalIndex];

    if (sigAction.sa_handler == SIG_DFL || !sig_can_be_ignored(sigNum))
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
            return add_signal_terminate_frame(prevStackPtr, sigNum);
        }
        return prevStackPtr;
    }

    if (sigAction.sa_handler == SIG_IGN || sigAction.sa_handler == SIG_ERR) {
        return prevStackPtr;
    }

    if ((sigAction.sa_flags & SA_RESETHAND) && sig_can_auto_reset(sigNum)) {
        sigAction.sa_handler = SIG_DFL;
        sigAction.sa_flags &= ~(SA_SIGINFO | SA_RESETHAND);
    }

    if (prevStackPtr & 0x07) {
        kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_Scheduler, "{}: Unaligned SP: {:#08x}", __PRETTY_FUNCTION__, prevStackPtr);
    }
    return add_signal_handler_frame(prevStackPtr, thread, userMode, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" uintptr_t process_signals(intptr_t curStackPtr, KThreadCB* thread, bool userMode)
{
    sigset_t pendingSignals = thread->GetUnblockedPendingSignals();

    for (int sigNum : {SIGKILL, SIGSTOP})
    {
        const sigset_t sigMask = sig_mkmask(sigNum);
        if (pendingSignals & sigMask)
        {
            pendingSignals &= ~sigMask;
            const uintptr_t newStackPtr = process_signal(curStackPtr, *thread, userMode, sigNum);
            if (newStackPtr != curStackPtr) {
                return newStackPtr;
            }
        }
    }
    if (thread->m_PendingSignals & sig_mkmask(SIGCONT))
    {
        wakeup_thread(thread->GetHandle(), true);
        if (pendingSignals & sig_mkmask(SIGCONT))
        {
            pendingSignals &= ~sig_mkmask(SIGCONT);
            const uintptr_t newStackPtr = process_signal(curStackPtr, *thread, userMode, SIGCONT);
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
                const uintptr_t newStackPtr = process_signal(curStackPtr, *thread, userMode, i + 1);
                if (newStackPtr != curStackPtr) {
                    return newStackPtr;
                }
            }
        }
    }
    return curStackPtr;
}

} // namespace kernel
