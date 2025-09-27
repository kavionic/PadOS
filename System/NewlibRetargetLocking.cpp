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
// Created: 06.09.2025 15:30


#include <utility>
#include <Threads/Mutex.h>
#include <Kernel/KMutex.h>

using namespace os;
using namespace kernel;

struct __lock : public KMutex
{
    __lock(PEMutexRecursionMode recursionMode) : KMutex("newlib", recursionMode) {}
};

extern "C"
{


struct __lock __lock___sfp_recursive_mutex(PEMutexRecursionMode_Recurse);
struct __lock __lock___atexit_recursive_mutex(PEMutexRecursionMode_Recurse);
struct __lock __lock___malloc_recursive_mutex(PEMutexRecursionMode_Recurse);
struct __lock __lock___env_recursive_mutex(PEMutexRecursionMode_Recurse);
struct __lock __lock___tz_mutex(PEMutexRecursionMode_RaiseError);
struct __lock __lock___dd_hash_mutex(PEMutexRecursionMode_RaiseError);
struct __lock __lock___arc4random_mutex(PEMutexRecursionMode_RaiseError);


IFLASHC void __retarget_lock_init(_LOCK_T* lock)
{
    try {
        *lock = new __lock(PEMutexRecursionMode_RaiseError);
    }
    catch (const std::bad_alloc& error) {
        *lock = nullptr;
    }
}

IFLASHC void __retarget_lock_init_recursive(_LOCK_T* lock)
{
    try {
        *lock = new __lock(PEMutexRecursionMode_Recurse);
    }
    catch (const std::bad_alloc& error) {
        *lock = nullptr;
    }
}

IFLASHC void __retarget_lock_close(_LOCK_T lock)
{
    delete lock;
}

IFLASHC void __retarget_lock_close_recursive(_LOCK_T lock)
{
    delete lock;
}

IFLASHC void __retarget_lock_acquire(_LOCK_T lock)
{
    lock->Lock();
}

IFLASHC void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    lock->Lock();
}

IFLASHC int __retarget_lock_try_acquire(_LOCK_T lock)
{
    return std::to_underlying(lock->TryLock());
}

IFLASHC int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    return std::to_underlying(lock->TryLock());
}

IFLASHC void __retarget_lock_release(_LOCK_T lock)
{
    lock->Unlock();
}

IFLASHC void __retarget_lock_release_recursive(_LOCK_T lock)
{
    lock->Unlock();
}

} // extern "C"
