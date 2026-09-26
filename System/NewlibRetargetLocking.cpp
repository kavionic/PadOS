// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 06.09.2025 15:30


#include <utility>
#include <pthread.h>
#include <Kernel/KMutex.h>

struct __lock
{
    pthread_mutex_t Mutex;
};

extern "C"
{

struct __lock __lock___sfp_recursive_mutex      = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___atexit_recursive_mutex   = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___malloc_recursive_mutex   = { (pthread_mutex_t)INVALID_HANDLE };
struct __lock __lock___env_recursive_mutex      = { PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP };
struct __lock __lock___tz_mutex                 = { PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP };
struct __lock __lock___dd_hash_mutex            = { PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP };
struct __lock __lock___arc4random_mutex         = { PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP };


void newlib_retarget_locks_initialize()
{
    pthread_mutexattr_t attrs;
    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
    
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, &attrs);

    __lock___malloc_recursive_mutex.Mutex = mutex;
}


void __retarget_lock_init(_LOCK_T* lock)
{
    *lock = new __lock;
    (*lock)->Mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
}

void __retarget_lock_init_recursive(_LOCK_T* lock)
{
    *lock = new __lock;
    (*lock)->Mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
}

void __retarget_lock_close(_LOCK_T lock)
{
    pthread_mutex_destroy(&lock->Mutex);
    delete lock;
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
    pthread_mutex_destroy(&lock->Mutex);
    delete lock;
}

void __retarget_lock_acquire(_LOCK_T lock)
{
    pthread_mutex_lock(&lock->Mutex);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    pthread_mutex_lock(&lock->Mutex);
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
    return pthread_mutex_trylock(&lock->Mutex);
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    return pthread_mutex_trylock(&lock->Mutex);
}

void __retarget_lock_release(_LOCK_T lock)
{
    pthread_mutex_unlock(&lock->Mutex);
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
    pthread_mutex_unlock(&lock->Mutex);
}

} // extern "C"
