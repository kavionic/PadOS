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

struct __posix_spawnattr
{
    short               sa_flags;
    pid_t               sa_pgroup;
    struct sched_param  sa_schedparam;
    int                 sa_schedpolicy;
    sigset_t            sa_sigdefault;
    sigset_t            sa_sigmask;
};

namespace kernel
{

extern "C"
{

void kposix_spawn_trw(pid_t* outPID, ThreadEntryTrampoline_t entryTrampoline, const char* path, const __posix_spawnattr* spawnAttr, PThreadUserData* threadUserData, char* const argv[], char* const envp[]);
PErrorCode kposix_spawnattr_init(__posix_spawnattr* attr) noexcept;
PErrorCode kposix_spawnattr_destroy(__posix_spawnattr* attr) noexcept;
void kposix_spawnattr_getflags(const posix_spawnattr_t* attr, short* flags) noexcept;
void kposix_spawnattr_setflags(posix_spawnattr_t* attr, short flags) noexcept;
void kposix_spawnattr_getpgroup(const posix_spawnattr_t* attr, pid_t* pgroup) noexcept;
void kposix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup) noexcept;
void kposix_spawnattr_getschedparam(const posix_spawnattr_t* attr, struct sched_param* sp) noexcept;
void kposix_spawnattr_setschedparam(posix_spawnattr_t* attr, const struct sched_param* sp) noexcept;
void kposix_spawnattr_getschedpolicy(const posix_spawnattr_t* attr, int* policy) noexcept;
void kposix_spawnattr_setschedpolicy(posix_spawnattr_t* attr, int policy) noexcept;
void kposix_spawnattr_getsigdefault(const posix_spawnattr_t* attr, sigset_t* sigdef) noexcept;
void kposix_spawnattr_setsigdefault(posix_spawnattr_t* attr, const sigset_t* sigdef) noexcept;
void kposix_spawnattr_getsigmask(const posix_spawnattr_t* attr, sigset_t* sigmask) noexcept;
void kposix_spawnattr_setsigmask(posix_spawnattr_t* attr, const sigset_t* sigmask) noexcept;


} // extern "C"

} // namespace kernel
