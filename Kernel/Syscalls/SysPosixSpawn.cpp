// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 09.04.2026 23:00

#include <spawn.h>
#include <sys/pados_syscalls.h>

#include <PadOS/SyscallReturns.h>

#include <Kernel/KAddressValidation.h>
#include <Kernel/KPosixSpawn.h>
#include <Kernel/Syscalls.h>
#include <System/ErrorCodes.h>
#include <Threads/ThreadUserspaceState.h>


namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawn(pid_t* outPID, ThreadEntryTrampoline_t entryTrampoline, const char* path, const PPosixSpawnAttribs* attr, PThreadUserData* threadUserData, char* const argv[], char* const envp[])
{
    try
    {
        if (outPID != nullptr) {
            validate_user_write_pointer_trw(outPID);
        }
        validate_user_read_string_trw(path, PATH_MAX);
        validate_user_read_pointer_trw(threadUserData);
        validate_user_write_pointer_trw(threadUserData->TLSData);
        if (attr != nullptr) {
            validate_user_read_pointer_trw(attr);
        }
        kposix_spawn_trw(outPID, entryTrampoline, path, attr, threadUserData, argv, envp);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_init(PPosixSpawnAttribs* attr, size_t attrSize)
{
    if (attrSize != sizeof(*attr)) {
        return PErrorCode::INVAL;
    }
    try
    {
        validate_user_write_pointer_trw(attr);
        return kposix_spawnattr_init(attr);
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_destroy(PPosixSpawnAttribs* attr)
{
    try
    {
        validate_user_write_pointer_trw(attr);
        return kposix_spawnattr_destroy(attr);
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_getflags(const posix_spawnattr_t* __restrict attr, short* __restrict flags)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_write_pointer_trw(flags);
        kposix_spawnattr_getflags(attr, flags);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_setflags(posix_spawnattr_t* attr, short flags)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        kposix_spawnattr_setflags(attr, flags);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_getpgroup(const posix_spawnattr_t* __restrict attr, pid_t* __restrict pgroup)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_write_pointer_trw(pgroup);
        kposix_spawnattr_getpgroup(attr, pgroup);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        kposix_spawnattr_setpgroup(attr, pgroup);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_getschedparam(const posix_spawnattr_t* __restrict attr, struct sched_param* __restrict schedparam)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_write_pointer_trw(schedparam);
        kposix_spawnattr_getschedparam(attr, schedparam);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_setschedparam(posix_spawnattr_t* __restrict attr, const struct sched_param* __restrict schedparam)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_read_pointer_trw(schedparam);
        kposix_spawnattr_setschedparam(attr, schedparam);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_getschedpolicy(const posix_spawnattr_t* __restrict attr, int* __restrict schedpolicy)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_write_pointer_trw(schedpolicy);
        kposix_spawnattr_getschedpolicy(attr, schedpolicy);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_setschedpolicy(posix_spawnattr_t* attr, int schedpolicy)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        kposix_spawnattr_setschedpolicy(attr, schedpolicy);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_getsigdefault(const posix_spawnattr_t* __restrict attr, sigset_t* __restrict sigdefault)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_write_pointer_trw(sigdefault);
        kposix_spawnattr_getsigdefault(attr, sigdefault);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_setsigdefault(posix_spawnattr_t* __restrict attr, const sigset_t* __restrict sigdefault)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_read_pointer_trw(sigdefault);
        kposix_spawnattr_setsigdefault(attr, sigdefault);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_getsigmask(const posix_spawnattr_t* __restrict attr, sigset_t* __restrict sigmask)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_write_pointer_trw(sigmask);
        kposix_spawnattr_getsigmask(attr, sigmask);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_posix_spawnattr_setsigmask(posix_spawnattr_t* __restrict attr, const sigset_t* __restrict sigmask)
{
    try
    {
        validate_user_read_pointer_trw(attr);
        validate_user_read_pointer_trw(*attr);
        validate_user_read_pointer_trw(sigmask);
        kposix_spawnattr_setsigmask(attr, sigmask);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

} // extern "C"

} // namespace kernel
