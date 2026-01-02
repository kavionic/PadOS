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

namespace kernel
{

class KThreadCB;

inline constexpr sigset_t sig_mkmask(int sigNum) { return sigset_t(1) << (sigNum - 1); }

static constexpr sigset_t KBLOCKABLE_SIGNALS_MASK = ~(sig_mkmask(SIGKILL) | sig_mkmask(SIGSTOP));

#include <signal.h>

enum class PESignalDefaultAction
{
    Ignore,
    Stop,
    Continue,
    Terminate,
    TerminateCoreDump
};

PErrorCode ksend_signal_to_thread(KThreadCB& thread, int sigNum);

void kforce_process_signals();

extern "C" uintptr_t process_signals(intptr_t curStackPtr, KThreadCB* thread, bool userMode);


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
