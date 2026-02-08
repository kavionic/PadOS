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
// Created: 05.09.2025 23:00

#include <string.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <PadOS/SyscallReturns.h>

#include <System/ExceptionHandling.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KBlockCache.h>
#include <Kernel/KAddressValidation.h>
#include <Kernel/Syscalls.h>


namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_open(const char* path, int flags, mode_t mode)
{
    try
    {
        validate_user_read_string_trw(path, PATH_MAX);
        return PMakeSysRetSuccess(kopen_trw(path, flags, mode));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_openat(int dirfd, const char* path, int flags, mode_t mode)
{
    try
    {
        validate_user_read_string_trw(path, PATH_MAX);
        return PMakeSysRetSuccess(kopen_trw(dirfd, path, flags, mode));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_reopen_file(int oldHandle, int openFlags)
{
    try
    {
        return PMakeSysRetSuccess(kreopen_file_trw(oldHandle, openFlags));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_fcntl(int file, int cmd, int arg, int* outResult)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_dup(int oldFile)
{
    try
    {
        return PMakeSysRetSuccess(kdupe_trw(oldFile, -1));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_dup2(int oldFile, int newFile)
{
    try
    {
        return PMakeSysRetSuccess(kdupe_trw(oldFile, newFile));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_close(int file)
{
    return kclose(file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_rename(const char* oldPath, const char* newPath)
{
    try
    {
        validate_user_read_string_trw(oldPath, PATH_MAX);
        validate_user_read_string_trw(newPath, PATH_MAX);
        krename_trw(oldPath, newPath);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_fstat(int file, struct stat* buf)
{
    try
    {
        kread_stat_trw(file, buf);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_stat(const char* path, struct stat* buf)
{
    try
    {
        validate_user_read_string_trw(path, PATH_MAX);
        validate_user_write_pointer_trw(buf);

        const int file = kopen_trw(path, O_RDONLY);

        PScopeExit cleanup([file]() { kclose(file); });

        kread_stat_trw(file, buf);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_write_stat(int file, const struct stat* value, uint32_t mask)
{
    try
    {
        validate_user_read_pointer_trw(value);
        kwrite_stat_trw(file, *value, mask);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_isatty(int file)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_seek(int file, off64_t* ioOffset, int whence)
{
    try
    {
        validate_user_write_pointer_trw(ioOffset);
        *ioOffset = klseek_trw(file, *ioOffset, whence);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_read(int file, void* buffer, size_t length)
{
    try
    {
        validate_user_write_pointer_trw(buffer, length);
        return PMakeSysRetSuccess(kread_trw(file, buffer, length));
    }
    PERROR_CATCH_RET_SYSRET;
}

PSysRetPair sys_read_pos(int file, void* buffer, size_t length, off_t position)
{
    try
    {
        validate_user_write_pointer_trw(buffer, length);
        return PMakeSysRetSuccess(kpread_trw(file, buffer, length, position));
    }
    PERROR_CATCH_RET_SYSRET;
}

PSysRetPair sys_readv(int file, const struct iovec* segments, size_t segmentCount)
{
    try
    {
        validate_user_read_pointer_trw(segments, sizeof(struct iovec) * segmentCount);
        for (size_t i = 0; i < segmentCount; ++i) {
            validate_user_write_pointer_trw(segments[i].iov_base, segments[i].iov_len);
        }
        return PMakeSysRetSuccess(kreadv_trw(file, segments, segmentCount));
    }
    PERROR_CATCH_RET_SYSRET;
}

PSysRetPair sys_readv_pos(int file, const struct iovec* segments, size_t segmentCount, off_t position)
{
    try
    {
        validate_user_read_pointer_trw(segments, sizeof(struct iovec) * segmentCount);
        for (size_t i = 0; i < segmentCount; ++i) {
            validate_user_write_pointer_trw(segments[i].iov_base, segments[i].iov_len);
        }
        return PMakeSysRetSuccess(kpreadv_trw(file, segments, segmentCount, position));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_write(int file, const void* buffer, size_t length)
{
    try
    {
        validate_user_read_pointer_trw(buffer, length);
        return PMakeSysRetSuccess(kwrite_trw(file, buffer, length));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_write_pos(int file, const void* buffer, size_t length, off_t position)
{
    try
    {
        validate_user_read_pointer_trw(buffer, length);
        return PMakeSysRetSuccess(kpwrite_trw(file, buffer, length, position));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_writev(int file, const struct iovec* segments, size_t segmentCount)
{
    try
    {
        validate_user_read_pointer_trw(segments, sizeof(struct iovec) * segmentCount);
        for (size_t i = 0; i < segmentCount; ++i) {
            validate_user_read_pointer_trw(segments[i].iov_base, segments[i].iov_len);
        }
        return PMakeSysRetSuccess(kwrite_trw(file, segments, segmentCount));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_writev_pos(int file, const struct iovec* segments, size_t segmentCount, off_t position)
{
    try
    {
        validate_user_read_pointer_trw(segments, sizeof(struct iovec) * segmentCount);
        for (size_t i = 0; i < segmentCount; ++i) {
            validate_user_read_pointer_trw(segments[i].iov_base, segments[i].iov_len);
        }
        return PMakeSysRetSuccess(kpwritev_trw(file, segments, segmentCount, position));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_device_control(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    try
    {
        validate_user_read_pointer_trw(inData, inDataLength);
        validate_user_write_pointer_trw(outData, outDataLength);
        kdevice_control_trw(handle, request, inData, inDataLength, outData, outDataLength);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_fsync(int file)
{
    try
    {
        kfsync_trw(file);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_create_directory(int dirfd, const char* name, mode_t permission)
{
    try
    {
        validate_user_read_string_trw(name, PATH_MAX);
        kcreate_directory_trw(dirfd, name, permission);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_read_directory(int handle, dirent_t* entry, size_t bufSize)
{
    try
    {
        return PMakeSysRetSuccess(kread_directory_trw(handle, entry, bufSize));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_rewind_directory(int handle)
{
    return krewind_directory(handle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_unlink_file(int dirfd, const char* path)
{
    try
    {
        validate_user_read_string_trw(path, PATH_MAX);
        kunlink_trw(dirfd, path);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_remove_directory(int dirfd, const char* path)
{
    try
    {
        validate_user_read_string_trw(path, PATH_MAX);
        kremove_directory_trw(dirfd, path);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSysRetPair sys_readlink(int dirfd, const char* path, char* buffer, size_t bufferSize)
{
    try
    {
        validate_user_read_string_trw(path, PATH_MAX);
        validate_user_write_pointer_trw(buffer, bufferSize);
        return PMakeSysRetSuccess(kreadlink_trw(dirfd, path, buffer, bufferSize));
    }
    PERROR_CATCH_RET_SYSRET;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_symlink(const char* targetPath, int dirfd, const char* symlinkPath)
{
    try
    {
        validate_user_read_string_trw(targetPath, PATH_MAX);
        validate_user_read_string_trw(symlinkPath, PATH_MAX);
        ksymlink_trw(targetPath, dirfd, symlinkPath);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_get_directory_path(int handle, char* buffer, size_t bufferSize)
{
    try
    {
        validate_user_write_pointer_trw(buffer, bufferSize);
        kget_directory_path_trw(handle, buffer, bufferSize);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_chdir(const char* path)
{
    try
    {
        validate_user_read_string_trw(path, PATH_MAX);

        kchdir_trw(path);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_getcwd(char* pathBuffer, size_t bufferSize)
{
    try
    {
        kgetcwd_trw(pathBuffer, bufferSize);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode sys_mount(const char* devicePath, const char* directoryPath, const char* filesystemName, uint32_t flags, const char* args, size_t argLength)
{
    try
    {
        validate_user_read_string_trw(devicePath, PATH_MAX);
        validate_user_read_string_trw(directoryPath, PATH_MAX);
        validate_user_read_pointer_trw(args, argLength);
        kmount_trw(devicePath, directoryPath, filesystemName, flags, args, argLength);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

PErrorCode sys_get_dirty_disk_cache_blocks(size_t* outBlocks)
{
    try
    {
        validate_user_read_pointer_trw(outBlocks);
        *outBlocks = kget_dirty_disk_cache_blocks();
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

} // extern "C"

} // namespace kernel
