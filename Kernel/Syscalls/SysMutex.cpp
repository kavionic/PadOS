// This file is part of PadOS.
//
// Copyright (c) 2025-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 30.08.2025 15:00

#include <sys/pados_syscalls.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/KMutex.h>
#include <Kernel/KTime.h>


namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_create(sem_id* outHandle, const char* name, PEMutexRecursionMode recursionMode, clockid_t clockID)
{
    try
    {
        *outHandle = KNamedObject::RegisterObject_trw(ptr_new<KMutex>(name, recursionMode, clockID));
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_duplicate(sem_id* outNewHandle, sem_id handle)
{
    try
    {
        Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject_trw(handle, KMutex::ObjectType));;
        *outNewHandle = KNamedObject::RegisterObject_trw(mutex);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_delete(sem_id handle)
{
    try
    {
        KNamedObject::FreeHandle_trw(handle, KMutex::ObjectType);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock(sem_id handle)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::Lock, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_timeout_ns(sem_id handle, bigtime_t timeoutns)
{
    const TimeValNanos timeout = TimeValNanos::FromNanoseconds(timeoutns);
    const TimeValNanos deadline = (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit;
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::LockDeadline, deadline, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_deadline_ns(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::LockDeadline, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_clock_ns(sem_id handle, clockid_t clockID, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::LockClock, clockID, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_try_lock(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KMutex>(handle, PErrorCode::INVAL, &KMutex::TryLock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_unlock(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KMutex>(handle, PErrorCode::INVAL, &KMutex::Unlock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared(sem_id handle)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::LockShared, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared_timeout_ns(sem_id handle, bigtime_t timeoutns)
{
    const TimeValNanos timeout = TimeValNanos::FromNanoseconds(timeoutns);
    const TimeValNanos deadline = (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit;
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::LockSharedDeadline, deadline, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared_deadline_ns(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::LockSharedDeadline, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared_clock_ns(sem_id handle, clockid_t clockID, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::INVAL, &KMutex::LockSharedClock, clockID, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_try_lock_shared(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KMutex>(handle, PErrorCode::INVAL, &KMutex::TryLockShared);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_islocked(sem_id handle)
{
    return KNamedObject::ForwardToHandleBool<KMutex>(handle, PErrorCode::Success, PErrorCode::BUSY, &KMutex::IsLocked);
}

} // extern "C"

} // namespace kernel
