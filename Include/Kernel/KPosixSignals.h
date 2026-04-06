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

#pragma once

#include <signal.h>

#include <Kernel/KStackFrames.h>

namespace kernel
{

class KProcess;
class KThreadCB;
class KProcessGroup;

static constexpr int KTOTAL_SIG_COUNT = NSIG + NRTSIG;
static_assert(KTOTAL_SIG_COUNT <= sizeof(sigset_t) * 8);

struct KSignalQueueNode
{
    KSignalQueueNode*   Next;
    int                 SigNum;
    siginfo_t           SigInfo;
};

inline constexpr sigset_t sig_mkmask(int sigNum) { return sigset_t(1) << (sigNum - 1); }

static constexpr sigset_t KBLOCKABLE_SIGNALS_MASK = ~(sig_mkmask(SIGKILL) | sig_mkmask(SIGSTOP));

enum class PESignalDefaultAction
{
    Ignore,
    Stop,
    Continue,
    Terminate,
    TerminateCoreDump
};

enum class KSignalMode
{
    Invalid,
    Handled,
    Ignored,
    Default,
    Blocked
};

KSignalMode kget_signal_mode(int sigNum);
KSignalMode kget_signal_mode(const KThreadCB& thread, int sigNum);

bool        khas_pending_signals();
bool        kis_thread_canceled();

PErrorCode  ksend_signal_to_thread(KThreadCB& thread, int sigNum) noexcept;

PErrorCode  kqueue_signal_to_thread(KThreadCB& thread, int signo, sigval_t value);
PErrorCode  kqueue_signal_to_thread_pl(KThreadCB& thread, int signo, sigval_t value);

void ksigaction_trw(int sigNum, const struct sigaction* action, struct sigaction* outPrevAction);
PErrorCode ksigaction(int sigNum, const struct sigaction* action, struct sigaction* outPrevAction);

_sig_func_ptr ksignal_trw(int sigNum, _sig_func_ptr handler);

PErrorCode  kthread_sigmask(int how, const sigset_t* newSet, sigset_t* outOldSet);

PErrorCode  kthread_kill(pid_t pid, int sig);

void        kkill_trw(pid_t pid, int sigNum);
void        kkill_trw_pl(pid_t pid, int sigNum);

PErrorCode  kkill(pid_t pid, int sigNum);
PErrorCode  kkill_pl(pid_t pid, int sigNum);

void        kkillpid_trw_pl(pid_t pid, int sigNum);
void        kkillpid_trw_pl(KProcess& targetProcess, int sigNum);

void        kkillpg_trw(pid_t pgroup, int sigNum);
void        kkillpg_trw_pl(pid_t pgroup, int sigNum);
void        kkillpg_trw_pl(const KProcessGroup& group, int sigNum);

PErrorCode  kkillpg(pid_t pgroup, int sigNum);
PErrorCode  kkillpg(const KProcessGroup& group, int sigNum);

PErrorCode  kkillpg_pl(pid_t pgroup, int sigNum);
PErrorCode  kkillpg_pl(const KProcessGroup& group, int sigNum);

KSignalQueueNode* kalloc_signal_queue_node();
void kfree_signal_queue_node(KSignalQueueNode* node);

void kforce_process_signals();

uintptr_t kprocess_signal(int sigNum, const uintptr_t prevStackPtr, bool userMode, bool fromFault, const siginfo_t* extSigInfo);
extern "C" uintptr_t kprocess_pending_signals(uintptr_t curStackPtr, bool userMode);
extern "C" uintptr_t kprocess_thread_exit(uintptr_t prevStackPtr, void* returnValue);

static inline bool exception_has_fpu_frame(uint32_t execReturn)
{
    return (execReturn & 0x10) == 0;
}

static inline uint32_t get_ctxswitch_frame_pc(const void* ctxBase)
{
    if (exception_has_fpu_frame(static_cast<const KCtxSwitchKernelStackFrame*>(ctxBase)->EXEC_RETURN)) {
        return static_cast<const KCtxSwitchStackFrameFPU*>(ctxBase)->ExceptionFrame.PC;
    } else {
        return static_cast<const KCtxSwitchStackFrame*>(ctxBase)->ExceptionFrame.PC;
    }
}

static inline PESignalDefaultAction sig_get_default_action(int sigNum)
{
    switch (sigNum)
    {
        case SIGCHLD:   // 20
        case SIGURG:    // 16
        case SIGWINCH:  // 28
            return PESignalDefaultAction::Ignore;

        case SIGSTOP:   // 17
        case SIGTSTP:   // 18
        case SIGTTIN:   // 21
        case SIGTTOU:   // 22
            return PESignalDefaultAction::Stop;

        case SIGCONT:   // 19
            return PESignalDefaultAction::Continue;

        case SIGHUP:    // 1
        case SIGINT:    // 2
        case SIGALRM:   // 14
        case SIGPIPE:   // 13
        case SIGTERM:   // 15
        case SIGIO:     // 23
        case SIGPROF:   // 27
        case SIGPWR:    // 29
        case SIGUSR1:   // 30
        case SIGUSR2:   // 31
            return PESignalDefaultAction::Terminate;

        case SIGQUIT:   // 3
        case SIGILL:    // 4
        case SIGTRAP:   // 5
        case SIGABRT:   // 6
        case SIGEMT:    // 7
        case SIGFPE:    // 8
        case SIGBUS:    // 10
        case SIGSEGV:   // 11
        case SIGSYS:    // 12
            return PESignalDefaultAction::TerminateCoreDump;

        default:
            return PESignalDefaultAction::Terminate;
    }
}

static inline bool sig_can_be_ignored(int sigNum)
{
    switch (sigNum)
    {
        case SIGKILL: // 9
        case SIGSTOP: // 17
            return false;
        default:
            return true;
    }
}

static inline bool sig_can_auto_reset(int sigNum)
{
    switch (sigNum)
    {
        case SIGILL:  // 4
        case SIGTRAP: // 5
            return false;
        default:
            return true;
    }
}

} // namespace kernel
