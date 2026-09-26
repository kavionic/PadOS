// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 30.08.2025 15:00

#include <sys/pados_syscalls.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KTime.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/KMutex.h>
#include <Kernel/Syscalls.h>


namespace kernel
{

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
        return PErrorCode::INVAL;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::INVAL, static_cast<PErrorCode(KConditionVariable::*)(KMutex&)>(&KConditionVariable::WaitCancelable), *mutex);
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
        return PErrorCode::INVAL;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::INVAL, static_cast<PErrorCode(KConditionVariable::*)(KMutex&, TimeValNanos)>(&KConditionVariable::WaitDeadlineCancelable), *mutex, TimeValNanos::FromNanoseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wait_clock_ns(handle_id handle, handle_id mutexHandle, clockid_t clockID, bigtime_t deadline)
{
    Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
    if (mutex == nullptr) {
        return PErrorCode::INVAL;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::INVAL, static_cast<PErrorCode(KConditionVariable::*)(KMutex&, clockid_t, TimeValNanos)>(&KConditionVariable::WaitClockCancelable), *mutex, clockID, TimeValNanos::FromNanoseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wakeup(handle_id handle, int threadCount)
{
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::INVAL, &KConditionVariable::Wakeup, threadCount);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wakeup_all(handle_id handle)
{
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::INVAL, &KConditionVariable::WakeupAll);
}

} // extern "C"

} // namespace kernel
