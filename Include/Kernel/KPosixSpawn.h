// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 09.04.2026 23:00

#pragma once

#include <sys/types.h>
#include <sys/sched.h>
#include <signal.h>
#include <Utils/IntrusiveList.h>

struct PPosixSpawnAttribs
{
    short               sa_flags;
    pid_t               sa_pgroup;
    struct sched_param  sa_schedparam;
    int                 sa_schedpolicy;
    sigset_t            sa_sigdefault;
    sigset_t            sa_sigmask;
};

enum class PSpawnFileActionType : int
{
    Close,
    Dup2,
    Open,
    Chdir,
    Fchdir,
};

struct PSpawnFileAction : public PIntrusiveListNode<PSpawnFileAction>
{
    struct Payload
    {
        PSpawnFileActionType    ActionType;
        int                     FD;     // target fd (close/dup2/open), dirfd (fchdir)
        union
        {
            struct { int NewFD; }               Dup2;
            struct { int OFlag; mode_t Mode; }  Open;
        };
    };

    Payload Data;

    // Path stored inline immediately after this struct (same allocation).
    // Non-path actions store a single null byte here; strlen(GetPath()) == 0.
    char*       GetPath() noexcept { return reinterpret_cast<char*>(this + 1); }
    const char* GetPath() const noexcept { return reinterpret_cast<const char*>(this + 1); }
};

struct PPosixSpawnFileActions
{
    PIntrusiveList<PSpawnFileAction> Actions;
};

namespace kernel
{

extern "C"
{

void kposix_spawn_trw(pid_t* outPID, ThreadEntryTrampoline_t entryTrampoline, const char* path, const PPosixSpawnAttribs* spawnAttr, PThreadUserData* threadUserData, char* const argv[], char* const envp[]);
PErrorCode kposix_spawnattr_init(PPosixSpawnAttribs* attr) noexcept;
PErrorCode kposix_spawnattr_destroy(PPosixSpawnAttribs* attr) noexcept;

void kposix_spawnattr_getflags(const posix_spawnattr_t* __restrict attr, short* __restrict flags) noexcept;
void kposix_spawnattr_setflags(posix_spawnattr_t* attr, short flags) noexcept;

void kposix_spawnattr_getpgroup(const posix_spawnattr_t* __restrict attr, pid_t* __restrict pgroup) noexcept;
void kposix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup) noexcept;

void kposix_spawnattr_getschedparam(const posix_spawnattr_t* __restrict attr, struct sched_param* __restrict sp) noexcept;
void kposix_spawnattr_setschedparam(posix_spawnattr_t* __restrict attr, const struct sched_param* __restrict sp) noexcept;

void kposix_spawnattr_getschedpolicy(const posix_spawnattr_t* __restrict attr, int* __restrict policy) noexcept;
void kposix_spawnattr_setschedpolicy(posix_spawnattr_t* attr, int policy) noexcept;

void kposix_spawnattr_getsigdefault(const posix_spawnattr_t* __restrict attr, sigset_t* __restrict sigdef) noexcept;
void kposix_spawnattr_setsigdefault(posix_spawnattr_t* __restrict attr, const sigset_t* __restrict sigdef) noexcept;

void kposix_spawnattr_getsigmask(const posix_spawnattr_t* __restrict attr, sigset_t* __restrict sigmask) noexcept;
void kposix_spawnattr_setsigmask(posix_spawnattr_t* __restrict attr, const sigset_t* __restrict sigmask) noexcept;


} // extern "C"

} // namespace kernel
