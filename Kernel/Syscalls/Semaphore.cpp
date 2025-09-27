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
// Created: 24.09.2025 23:00

#include <string.h>
#include <sys/fcntl.h>

#include <Kernel/KSemaphore.h>
#include <Kernel/KMutex.h>

using namespace kernel;
using namespace os;

static KMutex gk_PublicSemaphoresMutex("global_sema_mutex", PEMutexRecursionMode_RaiseError);
static std::map<String, Ptr<KSemaphore>> gk_PublicSemaphores;

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_create(sem_id* outHandle, const char* name, clockid_t clockID, int count)
{
    try {
        return KNamedObject::RegisterObject(*outHandle, ptr_new<KSemaphore>(name, clockID, count));
    }
    catch (const std::bad_alloc& error) {
        return PErrorCode::NoMemory;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_duplicate(sem_id* outNewHandle, sem_id handle)
{
    Ptr<KSemaphore> sema = KNamedObject::GetObject<KSemaphore>(handle);
    if (sema != nullptr) {
        return KNamedObject::RegisterObject(*outNewHandle, sema);
    }
    return PErrorCode::InvalidArg;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_delete(sem_id handle)
{
    if (KNamedObject::FreeHandle(handle, KNamedObjectType::Semaphore)) {
        return PErrorCode::Success;
    }
    return PErrorCode::InvalidArg;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_create_public(sem_id* outHandle, const char* name, clockid_t clockID, int flags, mode_t mode, int count)
{
    CRITICAL_SCOPE(gk_PublicSemaphoresMutex);

    try
    {
        auto iter = gk_PublicSemaphores.find(name);
        if (iter != gk_PublicSemaphores.end())
        {
            if (flags & O_EXCL)
            {
                return PErrorCode::Exist;
            }
            return KNamedObject::RegisterObject(*outHandle, iter->second);
        }
        else
        {
            if ((flags & O_CREAT) == 0) {
                return PErrorCode::NoEntry;
            }
            const size_t nameLength = strlen(name);
            const size_t nameOffset = (nameLength > (OS_NAME_LENGTH - 1)) ? (nameLength - (OS_NAME_LENGTH - 1)) : 0;

            Ptr<KSemaphore> semaphoreObject = ptr_new<KSemaphore>(name + nameOffset, clockID, count);
            gk_PublicSemaphores[name] = semaphoreObject;

            sem_id handle = INVALID_HANDLE;
            const PErrorCode result = KNamedObject::RegisterObject(handle, semaphoreObject);
            if (result == PErrorCode::Success) {
                *outHandle = handle;
            } else {
                gk_PublicSemaphores.erase(gk_PublicSemaphores.find(name));
            }
            return result;
        }
    }
    catch (const std::bad_alloc& error)
    {
        return PErrorCode::NoMemory;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_unlink_public(const char* name)
{
    CRITICAL_SCOPE(gk_PublicSemaphoresMutex);

    auto iter = gk_PublicSemaphores.find(name);
    if (iter != gk_PublicSemaphores.end())
    {
        gk_PublicSemaphores.erase(iter);
        return PErrorCode::Success;
    }
    return PErrorCode::NoEntry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_acquire(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KSemaphore>(handle, PErrorCode::InvalidArg, &KSemaphore::Acquire);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_acquire_timeout(sem_id handle, bigtime_t timeout)
{
    return KNamedObject::ForwardToHandle<KSemaphore>(handle, PErrorCode::InvalidArg, &KSemaphore::AcquireTimeout, TimeValMicros::FromMicroseconds(timeout));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_acquire_deadline(sem_id handle, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandle<KSemaphore>(handle, PErrorCode::InvalidArg, &KSemaphore::AcquireDeadline, TimeValMicros::FromMicroseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_acquire_clock(sem_id handle, clockid_t clockID, bigtime_t deadline)
{
    return KNamedObject::ForwardToHandle<KSemaphore>(handle, PErrorCode::InvalidArg, &KSemaphore::AcquireClock, clockID, TimeValMicros::FromMicroseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_try_acquire(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KSemaphore>(handle, PErrorCode::InvalidArg, &KSemaphore::TryAcquire);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_release(sem_id handle)
{
    return KNamedObject::ForwardToHandle<KSemaphore>(handle, PErrorCode::InvalidArg, &KSemaphore::Release);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_semaphore_get_count(sem_id handle, int* outCount)
{
    const int count = KNamedObject::ForwardToHandle<KSemaphore>(handle, -1, &KSemaphore::GetCount);
    if (count == -1) {
        return PErrorCode::InvalidArg;
    }
    *outCount = count;
    return PErrorCode::Success;
}

} // extern "C"
