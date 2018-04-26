// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 24.02.2018 22:50:42

#include "sam.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>

#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"
#include "Kernel/KMutex.h"

extern unsigned char* _sheap;
extern unsigned char* _eheap;

#define HEAP_START ((uint8_t*)&_sheap)
#define HEAP_END   ((uint8_t*)&_eheap)

using namespace kernel;

extern "C" {

void _exit(int status)
{
    for(;;);
}

struct __lock : public KMutex
{
    __lock() : KMutex("newlib", true) {}
};

struct __lock __lock___sinit_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___malloc_recursive_mutex;
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___tz_mutex;
struct __lock __lock___dd_hash_mutex;
struct __lock __lock___arc4random_mutex;

static bool g_MutexesInitialized = false;
void InitializeNewLibMutexes()
{
    g_MutexesInitialized = true;
/*    __lock___malloc_recursive_mutex.m_Mutex   = create_mutex("newlib_sinit", true);
    __lock___sinit_recursive_mutex.m_Mutex  = create_mutex("newlib_sinit", true);
    __lock___sfp_recursive_mutex.m_Mutex    = create_mutex("newlib_sinit", true);
    __lock___atexit_recursive_mutex.m_Mutex = create_mutex("newlib_sinit", true);
    __lock___at_quick_exit_mutex.m_Mutex    = create_mutex("newlib_sinit", true);
    __lock___env_recursive_mutex.m_Mutex    = create_mutex("newlib_sinit", true);
    __lock___tz_mutex.m_Mutex               = create_mutex("newlib_sinit", true);
    __lock___dd_hash_mutex.m_Mutex          = create_mutex("newlib_sinit", true);
    __lock___arc4random_mutex.m_Mutex       = create_mutex("newlib_sinit", true);*/
}

void __retarget_lock_init (_LOCK_T *lock)
{
    try {
        *lock = new __lock;
    } catch(const std::bad_alloc& error) {
        *lock = nullptr;
    }
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
    try {
        *lock = new __lock;
    } catch(const std::bad_alloc& error) {
        *lock = nullptr;
    }
}

void __retarget_lock_close(_LOCK_T lock)
{
    delete lock;
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
    delete lock;
}

void __retarget_lock_acquire (_LOCK_T lock)
{
    if (g_MutexesInitialized && lock != nullptr) lock->Lock();
}

void __retarget_lock_acquire_recursive (_LOCK_T lock)
{
    if (g_MutexesInitialized && lock != nullptr) lock->Lock();
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
    if (g_MutexesInitialized && lock != nullptr) return (lock->TryLock()) ? 0 : -1;
    return -1;
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    if (g_MutexesInitialized && lock != nullptr) return (lock->TryLock()) ? 0 : -1;
    return -1;
}

void __retarget_lock_release (_LOCK_T lock)
{
    if (g_MutexesInitialized && lock != nullptr) lock->Unlock();
}

void __retarget_lock_release_recursive (_LOCK_T lock)
{
    if (g_MutexesInitialized && lock != nullptr) lock->Unlock();
}

int _open_r(_reent* reent, const char* path, int flags)
{
    return Kernel::OpenFile(path, flags);
}

int _dup_r(_reent* reent, int oldFile)
{
    return Kernel::DupeFile(oldFile, -1);
}

//int dup2(int oldFile, int newFile) { return Kernel::DupeFile(oldFile, newFile); }
int _close_r(_reent* reent, int file)
{
    return Kernel::CloseFile(file);
}

int _fstat(int file, struct stat *buf)
{
    return -1;
}

int _isatty(int file)
{
    return -1;
}

off_t _lseek(int file, off_t offset, int whence)
{
    return 0;
}

ssize_t _read_r(_reent* reent, int file, void *buffer, size_t length)
{
    return Kernel::Read(file, buffer, length);
}

int _kill(pid_t pid, int sig)
{
    return -1;
}

pid_t _getpid(void)
{
    return get_thread_id();
}

ssize_t _write_r(_reent* reent, int file, const void *buffer, size_t length)
{
    return Kernel::Write(file, buffer, length);
}

uint32_t g_HeapSize = 0;

caddr_t _sbrk_r(_reent* reent, ptrdiff_t __incr)
{
    static uint8_t* heap = nullptr;
    uint8_t* prev_heap;
 
    g_HeapSize += __incr;
    
    if(heap == nullptr)
    {
        heap = HEAP_START;
    }
    prev_heap = heap;
    
    if((heap + __incr) > HEAP_END)
    {
        reent->_errno = ENOMEM;
//        assert(strerror(errno));
//        __BKPT(0);
        return (caddr_t) -1;
    }
    heap += __incr;
    if (__incr > 0) {
        memset(prev_heap, 0, __incr);
    } else {
        printf("sbrk(): heap reduced by %d bytes\n", __incr);
    }        
    return(caddr_t) prev_heap;
}

}
