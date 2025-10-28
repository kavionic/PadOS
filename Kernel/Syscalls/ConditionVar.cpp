// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 30.08.2025 15:00

#include <sys/pados_syscalls.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KTime.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/KMutex.h>
#include <Kernel/Syscalls.h>

using namespace os;
using namespace kernel;

extern "C"
{
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_create(handle_id* outHandle, const char* name, clockid_t clockID)
{
    try
    {
        *outHandle = KNamedObject::RegisterObject_trw(ptr_new<KConditionVariable>(name, clockID));
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_delete(handle_id handle)
{
    try
    {
        KNamedObject::FreeHandle_trw(handle, KConditionVariable::ObjectType);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wait(handle_id handle, handle_id mutexHandle)
{
    Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
    if (mutex == nullptr) {
        return PErrorCode::InvalidArg;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KConditionVariable::*)(KMutex&)>(&KConditionVariable::Wait), *mutex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wait_timeout_ns(handle_id handle, handle_id mutexHandle, bigtime_t timeout)
{
    return sys_condition_var_wait_deadline_ns(handle, mutexHandle, (timeout != TimeValNanos::infinit.AsNanoseconds()) ? (kget_monotonic_time_ns() + timeout) : TimeValNanos::infinit.AsNanoseconds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wait_deadline_ns(handle_id handle, handle_id mutexHandle, bigtime_t deadline)
{
    Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
    if (mutex == nullptr) {
        return PErrorCode::InvalidArg;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KConditionVariable::*)(KMutex&, TimeValNanos)>(&KConditionVariable::WaitDeadline), *mutex, TimeValNanos::FromNanoseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wait_clock_ns(handle_id handle, handle_id mutexHandle, clockid_t clockID, bigtime_t deadline)
{
    Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
    if (mutex == nullptr) {
        return PErrorCode::InvalidArg;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KConditionVariable::*)(KMutex&, clockid_t, TimeValNanos)>(&KConditionVariable::WaitClock), *mutex, clockID, TimeValNanos::FromNanoseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wakeup(handle_id handle, int threadCount)
{
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::InvalidArg, &KConditionVariable::Wakeup, threadCount);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wakeup_all(handle_id handle)
{
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::InvalidArg, &KConditionVariable::WakeupAll);
}

} // extern "C"
