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

#include <spawn.h>
#include <fcntl.h>

#include <Kernel/KPosixSpawn.h>
#include <Kernel/KThread.h>
#include <Kernel/VFS/FileIO.h>
#include <System/AppDefinition.h>
#include <System/ErrorCodes.h>
#include <System/ExceptionHandling.h>
#include <Threads/ThreadUserspaceState.h>

namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawn_trw(pid_t* outPID, ThreadEntryTrampoline_t entryTrampoline, const char* path, const PPosixSpawnAttribs* spawnAttr, PThreadUserData* threadUserData, char* const argv[], char* const envp[])
{
    const PAppDefinition* app = nullptr;

    {
        const int file = kopen_trw(path, O_RDONLY);

        PScopeExit scopeCleanup([file]() { kclose(file); });

        struct stat fileStats;

        kread_stat_trw(file, &fileStats);

        if ((fileStats.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
            PERROR_THROW_CODE(PErrorCode::NoAccess);
        }
        if (fileStats.st_size > NAME_MAX) {
            PERROR_THROW_CODE(PErrorCode::InvalidFileType);
        }
        PString appName;
        appName.resize(size_t(fileStats.st_size));
        if (kread_trw(file, appName.data(), appName.size()) != appName.size()) {
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        app = PAppDefinition::FindApplication(appName.c_str());
    }

    if (app == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidFileType);
    }
    threadUserData->TLSData->Ptr1 = const_cast<void*>(static_cast<const void*>(app));
    threadUserData->TLSData->Ptr2 = const_cast<void*>(static_cast<const void*>(argv));

    int priority = 0;
    if (spawnAttr != nullptr && (spawnAttr->sa_flags & (POSIX_SPAWN_SETSCHEDPARAM | POSIX_SPAWN_SETSCHEDULER))) {
        priority = spawnAttr->sa_schedparam.sched_priority;
    }

    PThreadAttribs threadAttr(path, priority, PThreadDetachState_Joinable, threadUserData->StackSize);

    threadAttr.StackAddress  = threadUserData->StackBuffer;

    const thread_id mainThreadID = kthread_spawn_trw(&threadAttr, spawnAttr, threadUserData, KSpawnThreadFlag::SpawnProcess, entryTrampoline, nullptr, threadUserData->TLSData);
    if (outPID != nullptr) {
        *outPID = mainThreadID;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kposix_spawnattr_init(PPosixSpawnAttribs* attr) noexcept
{
    *attr = {};
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kposix_spawnattr_destroy(PPosixSpawnAttribs* attr) noexcept
{
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_getflags(const posix_spawnattr_t* attr, short* flags) noexcept
{
    *flags = (*attr)->sa_flags;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_setflags(posix_spawnattr_t* attr, short flags) noexcept
{
    (*attr)->sa_flags = flags;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_getpgroup(const posix_spawnattr_t* attr, pid_t* pgroup) noexcept
{
    *pgroup = (*attr)->sa_pgroup;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup) noexcept
{
    (*attr)->sa_pgroup = pgroup;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_getschedparam(const posix_spawnattr_t* attr, struct sched_param* sp) noexcept
{
    *sp = (*attr)->sa_schedparam;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_setschedparam(posix_spawnattr_t* attr, const struct sched_param* sp) noexcept
{
    (*attr)->sa_schedparam = *sp;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_getschedpolicy(const posix_spawnattr_t* attr, int* policy) noexcept
{
    *policy = (*attr)->sa_schedpolicy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_setschedpolicy(posix_spawnattr_t* attr, int policy) noexcept
{
    (*attr)->sa_schedpolicy = policy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_getsigdefault(const posix_spawnattr_t* attr, sigset_t* sigdef) noexcept
{
    *sigdef = (*attr)->sa_sigdefault;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_setsigdefault(posix_spawnattr_t* attr, const sigset_t* sigdef) noexcept
{
    (*attr)->sa_sigdefault = *sigdef;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_getsigmask(const posix_spawnattr_t* attr, sigset_t* sigmask) noexcept
{
    *sigmask = (*attr)->sa_sigmask;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kposix_spawnattr_setsigmask(posix_spawnattr_t* attr, const sigset_t* sigmask) noexcept
{
    (*attr)->sa_sigmask = *sigmask;
}


} // extern "C"

} // namespace kernel
