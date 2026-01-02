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
// Created: 30.08.2025 15:00

#include <sys/pados_syscalls.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/KMutex.h>

using namespace os;

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
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::Lock, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_timeout_ns(sem_id handle, bigtime_t timeoutns)
{
    const TimeValNanos timeout = TimeValNanos::FromNanoseconds(timeoutns);
    const TimeValNanos deadline = (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit;
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::LockDeadline, deadline, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_deadline_ns(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::LockDeadline, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_clock_ns(sem_id handle, clockid_t clockID, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::LockClock, clockID, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_try_lock(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::TryLock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_unlock(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::Unlock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared(sem_id handle)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::LockShared, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared_timeout_ns(sem_id handle, bigtime_t timeoutns)
{
    const TimeValNanos timeout = TimeValNanos::FromNanoseconds(timeoutns);
    const TimeValNanos deadline = (!timeout.IsInfinit()) ? (kget_monotonic_time() + timeout) : TimeValNanos::infinit;
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::LockSharedDeadline, deadline, /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared_deadline_ns(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::LockSharedDeadline, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_lock_shared_clock_ns(sem_id handle, clockid_t clockID, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandleRestartable<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::LockSharedClock, clockID, TimeValNanos::FromNanoseconds(deadline), /*interruptible*/ true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_try_lock_shared(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KMutex>(handle, PErrorCode::InvalidArg, &KMutex::TryLockShared);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mutex_islocked(sem_id handle)
{
    return KNamedObject::ForwardToHandleBool<KMutex>(handle, PErrorCode::Success, PErrorCode::Busy, &KMutex::IsLocked);
}

} // extern "C"

} // namespace kernel
