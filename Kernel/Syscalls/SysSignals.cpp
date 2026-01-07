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

#include <Kernel/KThreadCB.h>

namespace kernel
{


extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_kill(thread_id threadID, int sigNum)
{
    return kthread_kill(threadID, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_kill(thread_id threadID, int sigNum)
{
    const Ptr<KThreadCB> thread = get_thread(threadID);

    if (thread == nullptr) {
        return PErrorCode::NoSuchProcess;
    }

    return ksend_signal_to_thread(*thread, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_raise(int sigNum)
{
    KThreadCB& thread = *gk_CurrentThread;
    return ksend_signal_to_thread(thread, sigNum);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_sigqueue(thread_id threadID, int signo, union sigval value)
{
    const Ptr<KThreadCB> thread = get_thread(threadID);

    if (thread == nullptr) {
        return PErrorCode::NoSuchProcess;
    }

    return kqueue_signal_to_thread(*thread, signo, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_sigaction(int sigNum, const struct sigaction* action, struct sigaction* outPrevAction)
{
    const int sigIndex = sigNum - 1;

    KThreadCB& thread = *gk_CurrentThread;

    if (sigIndex < 0 || sigIndex >= ARRAY_COUNT(KThreadCB::m_SignalHandlers)) {
        return PErrorCode::InvalidArg;
    }
    if (outPrevAction != nullptr) {
        *outPrevAction = thread.m_SignalHandlers[sigIndex];
    }
    if (action != nullptr) {
        thread.m_SignalHandlers[sigIndex] = *action;
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_signal(int sigNum, _sig_func_ptr handler)
{
    sigaction_t newAction = {};
    sigaction_t prevAction;

    newAction.sa_handler = handler;

    const PErrorCode result = sys_sigaction(sigNum, &newAction, &prevAction);

    if (result == PErrorCode::Success) {
        return PMakeSysRetSuccess(intptr_t(prevAction.sa_handler));
    } else {
        return PMakeSysRetFail(result);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_sigsuspend(const sigset_t* sigmask)
{
    try
    {
        validate_user_read_pointer_trw(sigmask);
        KThreadCB& thread = *gk_CurrentThread;

        const sigset_t prevMask = thread.m_BlockedSignals;
        thread.m_BlockedSignals = *sigmask & KBLOCKABLE_SIGNALS_MASK;

        thread.m_State = ThreadState_Waiting;
        KSWITCH_CONTEXT();

        thread.m_BlockedSignals = prevMask;

        return PErrorCode::Interrupted;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_sigmask(int how, const sigset_t* newSet, sigset_t* outOldSet)
{
    try
    {
        if (newSet != nullptr)      validate_user_read_pointer_trw(newSet);
        if (outOldSet != nullptr)   validate_user_write_pointer_trw(outOldSet);

        return kthread_sigmask(how, newSet, outOldSet);
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_sigpending(sigset_t* outSet)
{
    try
    {
        validate_user_write_pointer_trw(outSet);

        const KThreadCB& thread = *gk_CurrentThread;

        *outSet = thread.GetPendingSignals() & thread.m_BlockedSignals;

        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

} // extern "C"

} // namespace kernel
