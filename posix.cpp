// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>

#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"
#include "Kernel/KMutex.h"
#include "Kernel/VFS/FileIO.h"

extern unsigned char* _sheap;
extern unsigned char* _eheap;

#define HEAP_START ((uint8_t*)&_sheap)
#define HEAP_END   ((uint8_t*)&_eheap)

using namespace kernel;
using namespace os;

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

int _gettimeofday(struct timeval* time, void* unused)
{
    bigtime_t timeUs = get_real_time().AsMicroSeconds();
    time->tv_sec  = time_t(timeUs / 1000000);
    time->tv_usec = suseconds_t(timeUs % 1000000);
    return 0;
}

int _open_r(_reent* reent, const char* path, int flags)
{
    return FileIO::Open(path, flags);
}

int _dup_r(_reent* reent, int oldFile)
{
    return FileIO::Dupe(oldFile, -1);
}

//int dup2(int oldFile, int newFile) { return FileIO::DupeFile(oldFile, newFile); }
int _close_r(_reent* reent, int file)
{
    return FileIO::Close(file);
}

int _fstat(int file, struct stat *buf)
{
    return FileIO::ReadStats(file, buf);
}

int _stat(const char* path, struct stat *buf)
{
    int file = FileIO::Open(path, O_RDONLY);
    if (file != -1)
    {
	int result = FileIO::ReadStats(file, buf);
	FileIO::Close(file);
	return result;
    }
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
    return FileIO::Read(file, buffer, length);
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
    return FileIO::Write(file, buffer, length);
}

uint32_t g_HeapSize = 0;

caddr_t _sbrk_r(_reent* reent, ptrdiff_t size)
{
    static uint8_t* heap = nullptr;
    uint8_t* prev_heap;
 
    
    if(heap == nullptr) {
        heap = HEAP_START;
    }
    prev_heap = heap;
    
    if((heap + size) > HEAP_END)
    {
        reent->_errno = ENOMEM;
        return caddr_t(-1);
    }
	g_HeapSize += size;
    heap += size;
    if (size > 0) {
        memset(prev_heap, 0, size);
    }        
    return caddr_t(prev_heap);
}

char* getcwd(char* path, size_t bufferSize)
{
    if (bufferSize < 2) {
        errno = ERANGE;
        return nullptr;
    }
    strncpy(path, "/", bufferSize);
    return path;
}

}

size_t get_heap_size()
{
    return g_HeapSize;
}

size_t get_max_heap_size()
{
    return HEAP_END - HEAP_START;
}
