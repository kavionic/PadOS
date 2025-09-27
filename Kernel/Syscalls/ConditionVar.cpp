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

#include <Kernel/KConditionVariable.h>
#include <Kernel/KMutex.h>

using namespace os;
using namespace kernel;

extern "C"
{
///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_create(handle_id* outHandle, const char* name, clockid_t clockID)
{
    try {
        return KNamedObject::RegisterObject(*outHandle, ptr_new<KConditionVariable>(name, clockID));
    }
    catch (const std::bad_alloc& error) {
        return PErrorCode::NoMemory;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_delete(handle_id handle)
{
    if (KNamedObject::FreeHandle(handle, KConditionVariable::ObjectType)) {
        return PErrorCode::Success;
    }
    return PErrorCode::InvalidArg;
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

PErrorCode sys_condition_var_wait_timeout(handle_id handle, handle_id mutexHandle, bigtime_t timeout)
{
    return sys_condition_var_wait_deadline(handle, mutexHandle, (timeout != TimeValMicros::infinit.AsMicroSeconds()) ? (get_system_time().AsMicroSeconds() + timeout) : TimeValMicros::infinit.AsMicroSeconds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wait_deadline(handle_id handle, handle_id mutexHandle, bigtime_t deadline)
{
    Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
    if (mutex == nullptr) {
        return PErrorCode::InvalidArg;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KConditionVariable::*)(KMutex&, TimeValMicros)>(&KConditionVariable::WaitDeadline), *mutex, TimeValMicros::FromMicroseconds(deadline));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_condition_var_wait_clock(handle_id handle, handle_id mutexHandle, clockid_t clockID, bigtime_t deadline)
{
    Ptr<KMutex> mutex = ptr_static_cast<KMutex>(KNamedObject::GetObject(mutexHandle, KNamedObjectType::Mutex));
    if (mutex == nullptr) {
        return PErrorCode::InvalidArg;
    }
    return KNamedObject::ForwardToHandle<KConditionVariable>(handle, PErrorCode::InvalidArg, static_cast<PErrorCode(KConditionVariable::*)(KMutex&, clockid_t, TimeValMicros)>(&KConditionVariable::WaitClock), *mutex, clockID, TimeValMicros::FromMicroseconds(deadline));
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
