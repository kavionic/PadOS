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

PErrorCode sys_sigaction(int sigNum, const struct sigaction* action, struct sigaction* outPrevAction)
{
    const int sigIndex = sigNum - 1;

    KThreadCB* const thread = gk_CurrentThread;

    if (sigIndex < 0 || sigIndex >= ARRAY_COUNT(KThreadCB::m_SignalHandlers)) {
        return PErrorCode::InvalidArg;
    }
    if (outPrevAction != nullptr) {
        *outPrevAction = thread->m_SignalHandlers[sigIndex];
    }
    if (action != nullptr) {
        thread->m_SignalHandlers[sigIndex] = *action;
    }
    return PErrorCode::Success;
}


} // extern "C"

} // namespace kernel
