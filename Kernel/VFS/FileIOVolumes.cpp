// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>
#include <map>

#include <Kernel/KLogging.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KVFSManager.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

static KMutex                               kg_TableMutex("vfs_tables", PEMutexRecursionMode_RaiseError);
static std::map<PString, Ptr<KFilesystem>>  kg_FilesystemDrivers;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void UnmountAfterMountFailureNoThrow(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume) noexcept
{
    try
    {
        filesystem->Unmount(volume);
    }
    catch (const std::exception& exception)
    {
        kernel_log<PLogSeverity::ERROR>(LogCatKernel_VFS, "Failed to clean up volume after a mount-registration error: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kregister_filesystem_trw(const char* name, Ptr<KFilesystem> filesystem)
{
    CRITICAL_SCOPE(kg_TableMutex);
    kg_FilesystemDrivers[name] = filesystem;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFilesystem> kfind_filesystem_trw(const char* name)
{
    CRITICAL_SCOPE(kg_TableMutex);
    auto i = kg_FilesystemDrivers.find(name);
    if (i != kg_FilesystemDrivers.end()) {
        return i->second;
    }
    PERROR_THROW_CODE(PErrorCode::NOENT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kmount_trw(const char* devicePath, const char* directoryPath, const char* filesystemName, uint32_t flags, const char* args, size_t argLength)
{
    Ptr<KInode> mountPoint = klocate_inode_by_path_trw(KLocateFlag::FollowSymlinks, nullptr, directoryPath, strlen(directoryPath));
    Ptr<KFilesystem> filesystem = kfind_filesystem_trw(filesystemName);
    static fs_id nextFSID = VOLID_FIRST_NORMAL;
    Ptr<KFSVolume> volume = filesystem->Mount(nextFSID++, devicePath, flags, args, argLength);
    PScopeFail volumeGuard([&filesystem, &volume]()
    {
        UnmountAfterMountFailureNoThrow(filesystem, volume);
    });
    volume->m_Filesystem = filesystem;
    volume->m_MountPoint = mountPoint;
    KVFSManager::RegisterVolume_trw(volume);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kunmount_trw(const char* directoryPath)
{
    Ptr<KInode> rootNode = klocate_inode_by_path_trw(
        KLocateFlag::FollowSymlinks,
        nullptr,
        directoryPath,
        strlen(directoryPath));
    Ptr<KFSVolume> volume = rootNode->m_Volume;

    if (rootNode != volume->m_RootNode || volume->m_MountPoint == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    Ptr<KFilesystem> filesystem = volume->m_Filesystem;
    if (filesystem == nullptr) {
        PERROR_THROW_CODE(PErrorCode(ENODEV));
    }
    filesystem->Unmount(volume);
}

} // namespace kernel
