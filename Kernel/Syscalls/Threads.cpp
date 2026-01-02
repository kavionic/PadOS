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
// Created: 31.08.2025 17:00

#include <string.h>
#include <sys/pados_error_codes.h>

#include <System/ErrorCodes.h>
#include <System/ExceptionHandling.h>
#include <Kernel/KTime.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KThread.h>
#include <Kernel/KProcess.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/Syscalls.h>

using namespace os;

namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_attribs_init(PThreadAttribs* attribs)
{
    return kthread_attribs_init(*attribs);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_spawn(thread_id* outThreadHandle, const PThreadAttribs* attribs, PThreadControlBlock* tlsBlock, ThreadEntryPoint_t entryPoint, void* arguments)
{
    Ptr<KThreadCB> thread;

    try
    {
        const thread_id handle = kthread_spawn_trw(attribs, tlsBlock, /*privileged*/ false, entryPoint, arguments);
        if (outThreadHandle != nullptr) {
            *outThreadHandle = handle;
        }
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void sys_thread_exit(void* returnValue)
{
    kthread_exit(returnValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_detach(thread_id handle)
{
    return kthread_detach(handle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_join(thread_id handle, void** outReturnValue)
{
    try
    {
        if (outReturnValue != nullptr) {
            *outReturnValue = kthread_join_trw(handle);
        } else {
            kthread_join_trw(handle);
        }
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

thread_id sys_get_thread_id()
{
    return kget_thread_id();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_set_priority(thread_id handle, int priority)
{
    try
    {
        kthread_set_priority_trw(handle, priority);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_thread_get_priority(thread_id handle, int* outPriority)
{
    try
    {
        *outPriority = kthread_get_priority_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_get_thread_info(handle_id handle, ThreadInfo* info)
{
    return kget_thread_info(handle, info);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_get_next_thread_info(ThreadInfo* info)
{
    return kget_next_thread_info(info);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_snooze_until_ns(bigtime_t resumeTimeNanos)
{
    return ksnooze_until_ns(resumeTimeNanos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_snooze_ns(bigtime_t delayNanos)
{
    return ksnooze_ns(delayNanos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_yield()
{
    return kyield();
}

} // extern "C"

} // namespace kernel
