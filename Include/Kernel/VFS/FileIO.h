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
// Created: 08.05.2018 20:01:29

#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/pados_types.h>

#include <vector>
#include <map>

#include <Ptr/Ptr.h>
#include <Utils/String.h>
#include <Kernel/KMutex.h>

typedef struct iovec iovec_t;

namespace kernel
{

class KFilesystem;
class KFSVolume;
class KINode;
class KFileTableNode;
class KFileNode;
class KDirectoryNode;
class KRootFilesystem;
class Kernel;

void                            ksetup_rootfs_trw();
Ptr<KRootFilesystem>    kget_rootfs_trw();
Ptr<KRootFilesystem>    kget_rootfs() noexcept;

void                        kregister_filesystem_trw(const char* name, Ptr<KFilesystem> filesystem);
Ptr<KFilesystem>    kfind_filesystem_trw(const char* name);

void                        kmount_trw(const char* devicePath, const char* directoryPath, const char* filesystemName, uint32_t flags, const char* args, size_t argLength);

Ptr<KFileTableNode> kget_file_table_node_trw(int handle, bool forKernel = false);
Ptr<KFileNode>      kget_file_node_trw(int handle);
Ptr<KFileNode>      kget_file_node_trw(int handle, Ptr<KINode>& outInode);
Ptr<KDirectoryNode> kget_directory_node_trw(int handle);

int         kopen_trw(const char* path, int openFlags, int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
int         kopen_trw(int baseFolderFD, const char* path, int openFlags, int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
int         kreopen_file_trw(int oldHandle, int openFlags);
int         kdupe_trw(int oldHandle, int newHandle = -1);
PErrorCode  kclose(int handle) noexcept;

int         kget_file_flags_trw(int handle);
int         kset_file_flags_trw(int handle, int flags);


size_t kread_trw(int handle, void* buffer, size_t length);
size_t kpread_trw(int handle, void* buffer, size_t length, off_t position);
size_t kreadv_trw(int handle, const iovec_t* segments, size_t segmentCount);
size_t kpreadv_trw(int handle, const iovec_t* segments, size_t segmentCount, off_t position);

PErrorCode kread(int handle, void* buffer, size_t length);
PErrorCode kpread(int handle, void* buffer, size_t length, off_t position);

PErrorCode kread(int handle, void* buffer, size_t length, size_t& outLength);
PErrorCode kpread(int handle, void* buffer, size_t length, off_t position, size_t& outLength);

size_t kwrite_trw(int handle, const void* buffer, size_t length);
size_t kpwrite_trw(int handle, const void* buffer, size_t length, off_t position);
size_t kwritev_trw(int handle, const iovec_t* segments, size_t segmentCount);
size_t kpwritev_trw(int handle, const iovec_t* segments, size_t segmentCount, off_t position);

PErrorCode kwrite(int handle, const void* buffer, size_t length);
PErrorCode kpwrite(int handle, const void* buffer, size_t length, off_t position);

PErrorCode kwrite(int handle, const void* buffer, size_t length, size_t& outLength);
PErrorCode kpwrite(int handle, const void* buffer, size_t length, off_t position, size_t& outLength);


off_t klseek_trw(int handle, off_t offset, int mode);

void    kfsync_trw(int handle);

void    kdevice_control_trw(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

size_t  kread_directory_trw(int handle, dirent_t* entry, size_t bufSize);
void    krewind_directory_trw(int handle);

void    kcreate_directory_trw(const char* name, int permission = S_IRWXU);
void    kcreate_directory_trw(int baseFolderFD, const char* name, int permission = S_IRWXU);

void    ksymlink_trw(const char* target, const char* linkPath);
void    ksymlink_trw(const char* target, int baseFolderFD, const char* linkPath);

size_t  kreadlink_trw(int dirfd, const char* path, char* buffer, size_t bufferSize);

void    kread_stat_trw(Ptr<KINode> inode, struct stat* outStats);
void    kread_stat_trw(int handle, struct stat* outStats);
void    kwrite_stat_trw(int handle, const struct stat& value, uint32_t mask);

void    krename_trw(const char* oldPath, const char* newPath);
void    kunlink_trw(const char* path);
void    kunlink_trw(int baseFolderFD, const char* path);
void    kremove_directory_trw(const char* path);
void    kremove_directory_trw(int baseFolderFD, const char* path);

void    kget_directory_path_trw(int handle, char* buffer, size_t bufferSize);


Ptr<KINode> klocate_inode_by_name_trw(Ptr<KINode> parent, const char* name, int nameLength, bool crossMount);
Ptr<KINode> klocate_inode_by_path_trw(Ptr<KINode> parent, const char* path, int pathLength);
Ptr<KINode> klocate_parent_inode_trw(Ptr<KINode> parent, const char* path, int pathLength, const char** outName, size_t* outNameLength);
void                kget_directory_name_trw(Ptr<KINode> inode, char* path, size_t bufferSize);
int                 kallocate_filehandle_trw();
void                kfree_filehandle(int handle) noexcept;
int                 kopen_from_inode_trw(bool kernelFile, Ptr<KINode> inode, int openFlags);
void                kset_filehandle(int handle, Ptr<KFileTableNode> file) noexcept;

off_t       klseek(int handle, off_t offset, int mode) noexcept;

int         kfsync(int handle) noexcept;

PErrorCode  kdevice_control(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) noexcept;

ssize_t     kread_directory(int handle, dirent_t* entry, size_t bufSize) noexcept;
PErrorCode  krewind_directory(int handle) noexcept;

int         kcreate_directory(const char* name, int permission = S_IRWXU) noexcept;
int         kcreate_directory(int baseFolderFD, const char* name, int permission = S_IRWXU) noexcept;

PErrorCode  ksymlink(const char* target, const char* linkPath) noexcept;
PErrorCode  ksymlink(const char* target, int baseFolderFD, const char* linkPath) noexcept;

PErrorCode  kreadlink(int dirfd, const char* path, char* buffer, size_t bufferSize, size_t* outResultLength) noexcept;

PErrorCode  kread_stat(int handle, struct stat* outStats) noexcept;
PErrorCode  kwrite_stat(int handle, const struct stat& value, uint32_t mask) noexcept;

int         krename(const char* oldPath, const char* newPath) noexcept;
int         kunlink(const char* path) noexcept;
int         kunlink(int baseFolderFD, const char* path) noexcept;

int         kremove_directory(const char* path) noexcept;
int         kremove_directory(int baseFolderFD, const char* path) noexcept;

void        kfchdir_trw(int handle);
void        kchdir_trw(const char* path);
void        kgetcwd_trw(char* pathBuffer, size_t bufferSize);

} // namespace kernel
