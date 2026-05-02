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
#include <Kernel/KPIDNode.h>
#include <Kernel/KAddressValidation.h>

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
    return kkill(threadID, sigNum);
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
    const Ptr<KThreadCB> thread = kget_thread(threadID);

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
    try
    {
        if (action != nullptr) {
            validate_user_read_pointer_trw(action);
        }
        if (outPrevAction != nullptr) {
            validate_user_write_pointer_trw(outPrevAction);
        }
        ksigaction_trw(sigNum, action, outPrevAction);
        
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_signal(int sigNum, _sig_func_ptr handler)
{
    try
    {
        return PMakeSysRetSuccess(intptr_t(ksignal_trw(sigNum, handler)));
    }
    PERROR_CATCH_RET_SYSRET;
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

        if (!thread.HasUnblockedPendingSignals())
        {
            thread.SetState(ThreadState_Waiting);
            KSWITCH_CONTEXT();
        }

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

        *outSet = thread.GetBlockedPendingSignals();

        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_sigprocmask(int how, const sigset_t* set, sigset_t* oset)
{
    try
    {
        if (set != nullptr)   validate_user_read_pointer_trw(set);
        if (oset != nullptr)  validate_user_write_pointer_trw(oset);

        return kthread_sigmask(how, set, oset);
    }
    PERROR_CATCH_RET_CODE;
}

} // extern "C"

} // namespace kernel
