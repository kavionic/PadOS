// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 08.05.2018 20:01:27


#include <string.h>
#include <fcntl.h>
#include <sys/uio.h>

#include <Kernel/KProcess.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KINode.h>
#include <Kernel/VFS/KRootFilesystem.h>
#include <Kernel/VFS/KVFSManager.h>
#include <System/ExceptionHandling.h>


namespace kernel
{

static KMutex                               kg_TableMutex("vfs_tables", PEMutexRecursionMode_RaiseError);
static std::map<PString, Ptr<KFilesystem>>  kg_FilesystemDrivers;
static Ptr<KFileTableNode>                  kg_PlaceholderFile;
static Ptr<KRootFilesystem>                 kg_RootFilesystem;
static Ptr<KFSVolume>                       kg_RootVolume;
static std::vector<Ptr<KFileTableNode>>     kg_FileTable;

///////////////////////////////////////////////////////////////////////////////
/// Prepends a new name in front of a path.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int PrependNameToPath(char* buffer, size_t currentPathLength, const char* name, size_t nameLength)
{
    if (currentPathLength > 0) {
        memmove(buffer + nameLength + 1, buffer, currentPathLength);
    }
    buffer[0] = '/';
    memcpy(buffer + 1, name, nameLength);
    return currentPathLength + nameLength + 1;
}

///////////////////////////////////////////////////////////////////////////////
/// Some operations need to remove trailing slashes for POSIX.1 conformance.
/// For rename we also need to change the behavior depending on whether we
/// had a trailing slash or not.. (we cannot rename normal files with
/// trailing slashes, only directories)
///    
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool RemoveTrailingSlashes(PString* name)
{
    bool slashesRemoved = false;
    while (name->size() > 1 && (*name)[name->size() - 1] == '/')
    {
        name->resize(name->size() - 1);
        slashesRemoved = true;
    }
    return slashesRemoved;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksetup_rootfs_trw()
{
    kg_PlaceholderFile = ptr_new<KFileTableNode>(false, 0);
    kg_RootFilesystem = ptr_new<KRootFilesystem>();
    kg_RootVolume = kg_RootFilesystem->Mount(VOLID_ROOT, "", 0, nullptr, 0);
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
    PERROR_THROW_CODE(PErrorCode::NoEntry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KIOContext* kget_io_context()
{
    return gk_CurrentProcess->GetIOContext();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kmount_trw(const char* devicePath, const char* directoryPath, const char* filesystemName, uint32_t flags, const char* args, size_t argLength)
{
    Ptr<KINode> mountPoint = klocate_inode_by_path_trw(nullptr, directoryPath, strlen(directoryPath));
    Ptr<KFilesystem> filesystem = kfind_filesystem_trw(filesystemName);
    static fs_id nextFSID = VOLID_FIRST_NORMAL;
    Ptr<KFSVolume> volume = filesystem->Mount(nextFSID++, devicePath, flags, args, argLength);
    KVFSManager::RegisterVolume_trw(volume);
    volume->m_Filesystem = filesystem;
    volume->m_MountPoint = mountPoint;
    mountPoint->m_MountRoot = volume->m_RootNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileTableNode> kget_file_table_node_trw(int handle, bool forKernel)
{
    CRITICAL_SCOPE(kg_TableMutex);
    if (handle >= 0 && handle < int(kg_FileTable.size()) && kg_FileTable[handle] != nullptr && kg_FileTable[handle]->GetINode() != nullptr) {
        return kg_FileTable[handle];
    }
    PERROR_THROW_CODE(PErrorCode::BadFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> kget_file_node_trw(int handle)
{
    Ptr<KFileTableNode> node = kget_file_table_node_trw(handle);
    if (node->IsDirectory()) {
        PERROR_THROW_CODE(PErrorCode::IsDirectory);
    }
    return ptr_static_cast<KFileNode>(node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> kget_file_node_trw(int handle, Ptr<KINode>& outInode)
{
    Ptr<KFileNode> file = kget_file_node_trw(handle);
    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_FileOps == nullptr)
    {
        PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    outInode = inode;
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KDirectoryNode> kget_directory_node_trw(int handle)
{
    Ptr<KFileTableNode> node = kget_file_table_node_trw(handle);
    if (!node->IsDirectory()) {
        PERROR_THROW_CODE(PErrorCode::NotDirectory);
    }
    return ptr_static_cast<KDirectoryNode>(node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kopen_trw(int baseFolderFD, const char* path, int openFlags, int permissions)
{
    int handle = kallocate_filehandle_trw();

    PScopeFail handleGuard([handle]() { kfree_filehandle(handle); });

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = kget_directory_node_trw(baseFolderFD);
        baseInode = baseFolderFile->GetINode();
    }

    size_t      pathLength = strlen(path);
    const char* name;
    size_t      nameLength;
    Ptr<KINode> parent = klocate_parent_inode_trw(baseInode, path, pathLength, &name, &nameLength);
        
    Ptr<KINode> inode;
    try
    {
        inode = klocate_inode_by_name_trw(parent, name, nameLength, true);
    }
    PERROR_CATCH_RET(([handle, &parent, name, nameLength, openFlags, permissions](const std::exception& exc, PErrorCode error)
        {
            if (error == PErrorCode::NoEntry && (openFlags & O_CREAT))
            {
                const Ptr<KFileTableNode> file = parent->m_Filesystem->CreateFile(parent->m_Volume, parent, name, nameLength, openFlags, permissions);
                kset_filehandle(handle, file);
                return handle;
            }
            else
            {
                throw;
            }
        }
    ));
    if (inode->m_FileOps == nullptr)
    {
        PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    Ptr<KFileTableNode> file;
    if (inode->IsDirectory()) {
        file = inode->m_FileOps->OpenDirectory(inode->m_Volume, inode);
    } else {
        file = inode->m_FileOps->OpenFile(inode->m_Volume, inode, (openFlags & ~O_CREAT));
    }
    file->SetINode(inode);

    kset_filehandle(handle, file);
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kopen_trw(const char* path, int openFlags, int permissions)
{
    return kopen_trw(AT_FDCWD, path, openFlags, permissions);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kreopen_file_trw(int oldHandle, int openFlags)
{
    Ptr<KFileTableNode> fileNode = kget_file_table_node_trw(oldHandle);
    return kopen_from_inode_trw(false, fileNode->GetINode(), openFlags);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kdupe_trw(int oldHandle, int newHandle)
{
    if (oldHandle == newHandle)
    {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
        set_last_error(EINVAL);
        return -1;
    }
    Ptr<KFileNode> file = kget_file_node_trw(oldHandle);
    if (newHandle == -1)
    {
        newHandle = kallocate_filehandle_trw();
    }
    else
    {
        kclose(newHandle);
        CRITICAL_SCOPE(kg_TableMutex);
        if (newHandle >= int(kg_FileTable.size())) {
            kg_FileTable.resize(newHandle + 1);
        }
    }
    kset_filehandle(newHandle, file);
    return newHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kclose(int handle) noexcept
{
    if (handle >= 0 && handle <=2)
    {
        return PErrorCode::BadFile;
    }
    try
    {
        Ptr<KFileTableNode> file = kget_file_table_node_trw(handle);
        kfree_filehandle(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kget_file_flags_trw(int handle)
{
    Ptr<KFileNode> file = kget_file_node_trw(handle);
    return file->GetOpenFlags();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kset_file_flags_trw(int handle, int flags)
{
    Ptr<KFileNode> file = kget_file_node_trw(handle);
    file->SetOpenFlags(flags);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kread_trw(int handle, void* buffer, size_t length)
{
    iovec_t segment;
    segment.iov_base = buffer;
    segment.iov_len = length;
    return kreadv_trw(handle, &segment, 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kpread_trw(int handle, void* buffer, size_t length, off_t position)
{
    iovec_t segment;
    segment.iov_base = buffer;
    segment.iov_len = length;
    return kpreadv_trw(handle, &segment, 1, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kreadv_trw(int handle, const iovec_t* segments, size_t segmentCount)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(handle, inode);
    const size_t bytesRead = inode->m_FileOps->Read(file, segments, segmentCount, file->m_Position);
    file->m_Position += bytesRead;
    return bytesRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kpreadv_trw(int handle, const iovec_t* segments, size_t segmentCount, off_t position)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(handle, inode);
    return inode->m_FileOps->Read(file, segments, segmentCount, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kread(int handle, void* buffer, size_t length)
{
    size_t bytesRead = 0;
    const PErrorCode result = kread(handle, buffer, length, bytesRead);
    if (result != PErrorCode::Success) return result;
    return (bytesRead == length) ? PErrorCode::Success : PErrorCode::IOError;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kpread(int handle, void* buffer, size_t length, off_t position)
{
    size_t bytesRead = 0;
    const PErrorCode result = kpread(handle, buffer, length, position, bytesRead);
    if (result != PErrorCode::Success) return result;
    return (bytesRead == length) ? PErrorCode::Success : PErrorCode::IOError;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kread(int handle, void* buffer, size_t length, size_t& outLength)
{
    try
    {
        outLength = kread_trw(handle, buffer, length);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kpread(int handle, void* buffer, size_t length, off_t position, size_t& outLength)
{
    try
    {
        outLength = kpread_trw(handle, buffer, length, position);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kwrite_trw(int handle, const void* buffer, size_t length)
{
    iovec_t segment;
    segment.iov_base = const_cast<void*>(buffer);
    segment.iov_len = length;
    return kwritev_trw(handle, &segment, 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kpwrite_trw(int handle, const void* buffer, size_t length, off_t position)
{
    iovec_t segment;
    segment.iov_base = const_cast<void*>(buffer);
    segment.iov_len = length;
    return kpwritev_trw(handle, &segment, 1, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kwritev_trw(int handle, const iovec_t* segments, size_t segmentCount)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(handle, inode);
    ssize_t bytesWritten = inode->m_FileOps->Write(file, segments, segmentCount, file->m_Position);
    file->m_Position += bytesWritten;
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kpwritev_trw(int handle, const iovec_t* segments, size_t segmentCount, off_t position)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(handle, inode);
    return inode->m_FileOps->Write(file, segments, segmentCount, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kwrite(int handle, const void* buffer, size_t length)
{
    size_t bytesWritten = 0;
    const PErrorCode result = kwrite(handle, buffer, length, bytesWritten);
    if (result != PErrorCode::Success) return result;
    return (bytesWritten == length) ? PErrorCode::Success : PErrorCode::IOError;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kpwrite(int handle, const void* buffer, size_t length, off_t position)
{
    size_t bytesWritten = 0;
    const PErrorCode result = kpwrite(handle, buffer, length, position, bytesWritten);
    if (result != PErrorCode::Success) return result;
    return (bytesWritten == length) ? PErrorCode::Success : PErrorCode::IOError;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kwrite(int handle, const void* buffer, size_t length, size_t& outLength)
{
    try
    {
        outLength = kwrite_trw(handle, buffer, length);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kpwrite(int handle, const void* buffer, size_t length, off_t position, size_t& outLength)
{
    try
    {
        outLength = kpwrite_trw(handle, buffer, length, position);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off_t klseek_trw(int handle, off_t offset, int mode)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(handle, inode);
    switch (mode)
    {
        case SEEK_SET:
            if (offset < 0) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            file->m_Position = offset;
            return file->m_Position;
        case SEEK_CUR:
            if (file->m_Position + offset < 0) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            file->m_Position += offset;
            return file->m_Position;
        case SEEK_END:
        {
            struct stat fileStats;
            inode->m_FileOps->ReadStat(inode->m_Volume, inode, &fileStats);
            off_t size = fileStats.st_size;
            if (size + offset < 0) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            file->m_Position = size + offset;
            return file->m_Position;
        }
        default:
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kfsync_trw(int handle)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(handle, inode);
    inode->m_FileOps->Sync(file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kdevice_control_trw(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file = kget_file_node_trw(handle, inode);
    inode->m_FileOps->DeviceControl(file, request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kread_directory_trw(int handle, dirent_t* entry, size_t bufSize)
{
    Ptr<KDirectoryNode> dir = kget_directory_node_trw(handle);
    return dir->ReadDirectory(entry, bufSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void krewind_directory_trw(int handle)
{
    Ptr<KDirectoryNode> dir = kget_directory_node_trw(handle);
    dir->RewindDirectory();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kcreate_directory_trw(int baseFolderFD, const char* path, int permission)
{
    int         pathLength = strlen(path);
    const char* name;
    size_t      nameLength;

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = kget_directory_node_trw(baseFolderFD);
        baseInode = baseFolderFile->GetINode();
    }

    Ptr<KINode> parent = klocate_parent_inode_trw(baseInode, path, pathLength, &name, &nameLength);
    parent->m_Filesystem->CreateDirectory(parent->m_Volume, parent, name, nameLength, permission);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kcreate_directory_trw(const char* name, int permission)
{
    return kcreate_directory_trw(AT_FDCWD, name, permission);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksymlink_trw(const char* target, int baseFolderFD, const char* linkPath)
{
    PERROR_THROW_CODE(PErrorCode::NotImplemented);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksymlink_trw(const char* target, const char* linkPath)
{
    ksymlink_trw(target, AT_FDCWD, linkPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t kreadlink_trw(int baseFolderFD, const char* path, char* buffer, size_t bufferSize)
{
    PERROR_THROW_CODE(PErrorCode::NotImplemented);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kread_stat_trw(Ptr<KINode> inode, struct stat* outStats)
{
    if (inode == nullptr || inode->m_FileOps == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    inode->m_FileOps->ReadStat(inode->m_Volume, inode, outStats);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kread_stat_trw(int handle, struct stat* outStats)
{
    Ptr<KFileTableNode> node = kget_file_table_node_trw(handle);
    Ptr<KINode> inode = node->GetINode();
    if (inode == nullptr || inode->m_FileOps == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    kread_stat_trw(inode, outStats);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kwrite_stat_trw(int handle, const struct stat& value, uint32_t mask)
{
    Ptr<KFileTableNode> node = kget_file_table_node_trw(handle);
    Ptr<KINode> inode = node->GetINode();
    if (inode == nullptr || inode->m_FileOps == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    inode->m_FileOps->WriteStat(inode->m_Volume, inode, &value, mask);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void krename_trw(const char* inOldPath, const char* inNewPath)
{
    if (inOldPath == nullptr || inNewPath == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    PString oldPath(inOldPath);
    PString newPath(inNewPath);

    bool mustBeDir = RemoveTrailingSlashes(&oldPath);
    mustBeDir = RemoveTrailingSlashes(&newPath) || mustBeDir;

    const char* oldName;
    size_t      oldNameLength;
    Ptr<KINode> oldParent = klocate_parent_inode_trw(nullptr, oldPath.c_str(), oldPath.size(), &oldName, &oldNameLength);

    const char* newName;
    size_t      newNameLength;
    Ptr<KINode> newParent = klocate_parent_inode_trw(nullptr, newPath.c_str(), newPath.size(), &newName, &newNameLength);

    if (oldParent->m_Volume->m_VolumeID != newParent->m_Volume->m_VolumeID) {
        PERROR_THROW_CODE(PErrorCode::CrossDeviceLink);
    }
    oldParent->m_Filesystem->Rename(oldParent->m_Volume, oldParent, oldName, oldNameLength, newParent, newName, newNameLength, mustBeDir);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kunlink_trw(int baseFolderFD, const char* inPath)
{
    if (inPath == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    PString path(inPath);

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = kget_directory_node_trw(baseFolderFD);
        baseInode = baseFolderFile->GetINode();
    }

    const char* name;
    size_t      nameLength;

    Ptr<KINode> parent = klocate_parent_inode_trw(baseInode, path.c_str(), path.size(), &name, &nameLength);
    parent->m_Filesystem->Unlink(parent->m_Volume, parent, name, nameLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kunlink_trw(const char* path)
{
    kunlink_trw(AT_FDCWD, path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kremove_directory_trw(int baseFolderFD, const char* inPath)
{
    if (inPath == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    PString path(inPath);

    const char* name;
    size_t      nameLength;

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = kget_directory_node_trw(baseFolderFD);
        baseInode = baseFolderFile->GetINode();
    }

    Ptr<KINode> parent = klocate_parent_inode_trw(baseInode, path.c_str(), path.size(), &name, &nameLength);
    parent->m_Filesystem->RemoveDirectory(parent->m_Volume, parent, name, nameLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kremove_directory_trw(const char* path)
{
    kremove_directory_trw(AT_FDCWD, path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off_t klseek(int handle, off_t offset, int mode) noexcept
{
    try
    {
        return klseek_trw(handle, offset, mode);
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kfsync(int handle) noexcept
{
    try
    {
        kfsync_trw(handle);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kdevice_control(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) noexcept
{
    try
    {
        kdevice_control_trw(handle, request, inData, inDataLength, outData, outDataLength);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t kread_directory(int handle, dirent_t* entry, size_t bufSize) noexcept
{
    try
    {
        return kread_directory_trw(handle, entry, bufSize);
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode krewind_directory(int handle) noexcept
{
    try
    {
        krewind_directory_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kcreate_directory(const char* name, int permission) noexcept
{
    try
    {
        kcreate_directory_trw(name, permission);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kcreate_directory(int baseFolderFD, const char* name, int permission) noexcept
{
    try
    {
        kcreate_directory_trw(baseFolderFD, name, permission);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksymlink(const char* target, const char* linkPath) noexcept
{
    try
    {
        ksymlink_trw(target, linkPath);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksymlink(const char* target, int baseFolderFD, const char* linkPath) noexcept
{
    try
    {
        ksymlink_trw(target, baseFolderFD, linkPath);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kreadLink(int dirfd, const char* path, char* buffer, size_t bufferSize, size_t* outResultLength) noexcept
{
    try
    {
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kread_stat(int handle, struct stat* outStats) noexcept
{
    try
    {
        kread_stat_trw(handle, outStats);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kwrite_stat(int handle, const struct stat& value, uint32_t mask) noexcept
{
    try
    {
        kwrite_stat_trw(handle, value, mask);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int krename(const char* inOldPath, const char* inNewPath) noexcept
{
    try
    {
        krename_trw(inOldPath, inNewPath);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kunlink(int baseFolderFD, const char* inPath) noexcept
{
    try
    {
        kunlink_trw(baseFolderFD, inPath);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kunlink(const char* path) noexcept
{
    try
    {
        kunlink_trw(path);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kremove_directory(int baseFolderFD, const char* inPath) noexcept
{
    try
    {
        kremove_directory_trw(baseFolderFD, inPath);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kremove_directory(const char* inPath) noexcept
{
    try
    {
        kremove_directory_trw(inPath);
        return 0;
    }
    PERROR_CATCH_SET_ERRNO(-1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kget_directory_path_trw(int handle, char* buffer, size_t bufferSize)
{
    Ptr<KDirectoryNode> directory = kget_directory_node_trw(handle);
    Ptr<KINode> inode = directory->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    kget_directory_name_trw(inode, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KRootFilesystem> kget_rootfs_trw()
{
    Ptr<KRootFilesystem> filesystem = kget_rootfs();
    if (filesystem == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    return filesystem;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KRootFilesystem> kget_rootfs() noexcept
{
    return kg_RootFilesystem;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> klocate_inode_by_name_trw(Ptr<KINode> parent, const char* name, int nameLength, bool crossMount)
{
    if (nameLength == 0) {
        return parent;
    }
    if (nameLength == 2 && name[0] == '.' && name[1] == '.' && parent == parent->m_Volume->m_RootNode)
    {
        if (parent != kg_RootVolume->m_RootNode) {
            parent = parent->m_Volume->m_MountPoint;
        } else {
            return parent;
        }
    }
    Ptr<KINode> inode = parent->m_Filesystem->LocateInode(parent->m_Volume, parent, name, nameLength);
    if (crossMount && inode->m_MountRoot != nullptr) {
        inode = inode->m_MountRoot;
    }
    return inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> klocate_inode_by_path_trw(Ptr<KINode> parent, const char* path, int pathLength)
{
    const char* name;
    size_t      nameLength;
    parent = klocate_parent_inode_trw(parent, path, pathLength, &name, &nameLength);
    return klocate_inode_by_name_trw(parent, name, nameLength, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> klocate_parent_inode_trw(Ptr<KINode> parent, const char* path, int pathLength, const char** outName, size_t* outNameLength)
{
    const KIOContext* const ioContext = kget_io_context();
    Ptr<KINode> current = parent;

    int i = 0;
    if (path[0] == '/')
    {
        current = kg_RootVolume->m_RootNode;
        ++i;
    }
    else if (current == nullptr)
    {
        current = ioContext->GetCurrentDirectory();
    }
    if (current == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }
    int nameStart = i;

    for (;; ++i)
    {
        if (i == pathLength)
        {
            *outName = path + nameStart;
            *outNameLength = pathLength - nameStart;
            return current;
        }
        if (path[i] == '/')
        {
            if (i == nameStart) {
                nameStart = i + 1;
                continue;
            }
            current = klocate_inode_by_name_trw(current, path + nameStart, i - nameStart, true);
            nameStart = i + 1;
        }
    }
    PERROR_THROW_CODE(PErrorCode::NoEntry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kget_directory_name_trw(Ptr<KINode> inode, char* path, size_t bufferSize)
{
    int         pathLength = 0;

    if (inode == kg_RootVolume->m_RootNode)
    {
        if (bufferSize < 2) {
            PERROR_THROW_CODE(PErrorCode::NameTooLong);
        }
        path[0] = '/';
        path[1] = '\0';
        return;
    }

    for (;;)
    {
        dirent_t    dirEntry;
        int         directoryHandle;

        Ptr<KINode> parent = klocate_inode_by_name_trw(inode, "..", 2, true);
        directoryHandle = kopen_from_inode_trw(true, parent, O_RDONLY);
        PScopeExit handleGuard([directoryHandle]() { kclose(directoryHandle); });

        bool isMountPoint = (inode->m_Volume != parent->m_Volume);
        bool foundInParent = false;
        while (kread_directory(directoryHandle, &dirEntry, sizeof(dirEntry)) == sizeof(dirEntry))
        {
            if (strcmp(dirEntry.d_name, ".") == 0 || strcmp(dirEntry.d_name, "..") == 0) {
                continue;
            }
            if (isMountPoint)
            {
                Ptr<KINode> entryInode = klocate_inode_by_name_trw(parent, dirEntry.d_name, dirEntry.d_namlen, false);
                if (entryInode->m_MountRoot == inode)
                {
                    if (pathLength + dirEntry.d_namlen + 1 > bufferSize) {
                        PERROR_THROW_CODE(PErrorCode::NameTooLong);
                    }
                    pathLength = PrependNameToPath(path, pathLength, dirEntry.d_name, dirEntry.d_namlen);
                    foundInParent = true;
                    break;
                }
            }
            else if (dirEntry.d_ino == inode->m_INodeID)
            {
                if (pathLength + dirEntry.d_namlen + 1 > bufferSize) {
                    PERROR_THROW_CODE(PErrorCode::NameTooLong);
                }
                pathLength = PrependNameToPath(path, pathLength, dirEntry.d_name, dirEntry.d_namlen);
                foundInParent = true;
                break;
            }
        }
        if (!foundInParent) {
            PERROR_THROW_CODE(PErrorCode::NoEntry);
        }
        inode = parent;
        if (inode == kg_RootVolume->m_RootNode)
        {
            if (pathLength >= bufferSize) {
                PERROR_THROW_CODE(PErrorCode::NameTooLong);
            }
            path[pathLength] = '\0';
            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kallocate_filehandle_trw()
{
    CRITICAL_SCOPE(kg_TableMutex);
    auto i = std::find(kg_FileTable.begin(), kg_FileTable.end(), nullptr);
    if (i != kg_FileTable.end())
    {
        int file = i - kg_FileTable.begin();
        kg_FileTable[file] = kg_PlaceholderFile;
        return file;
    }
    else
    {
        int file = kg_FileTable.size();
        kg_FileTable.push_back(kg_PlaceholderFile);
        return file;
        //set_last_error(EMFILE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kfree_filehandle(int handle) noexcept
{
    CRITICAL_SCOPE(kg_TableMutex);
    if (handle >= 0 && handle < int(kg_FileTable.size())) {
        kg_FileTable[handle] = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kopen_from_inode_trw(bool kernelFile, Ptr<KINode> inode, int openFlags)
{
    int handle = kallocate_filehandle_trw();

    PScopeFail handleGuard([handle]() { kfree_filehandle(handle); });

    Ptr<KFileTableNode> file;
    if (inode->m_FileOps == nullptr)
    {
        PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    if (inode->IsDirectory()) {
        file = inode->m_FileOps->OpenDirectory(inode->m_Volume, inode);
    } else {
        file = inode->m_FileOps->OpenFile(inode->m_Volume, inode, (openFlags & ~O_CREAT));
    }
    file->SetINode(inode);

    kset_filehandle(handle, file);
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kset_filehandle(int handle, Ptr<KFileTableNode> file) noexcept
{
    CRITICAL_SCOPE(kg_TableMutex);
    if (handle >= 0 && handle < int(kg_FileTable.size()))
    {
        kg_FileTable[handle] = file;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kfchdir_trw(int handle)
{
    KIOContext* const ioContext = kget_io_context();

    const Ptr<KDirectoryNode> dirNode = kget_directory_node_trw(handle);
    const Ptr<KINode> inode = dirNode->GetINode();

    struct stat stats;
    kread_stat_trw(inode, &stats);

    if (!S_ISDIR(stats.st_mode)) {
        PERROR_THROW_CODE(PErrorCode::NotDirectory);
    }

    ioContext->SetCurrentDirectory(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kchdir_trw(const char* path)
{
    int fd = kopen_trw(path, O_RDONLY | O_DIRECTORY);

    PScopeExit cleanup([fd]() { kclose(fd); });

    kfchdir_trw(fd);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kgetcwd_trw(char* pathBuffer, size_t bufferSize)
{
    const KIOContext* const ioContext = kget_io_context();

    const Ptr<KINode> inode = ioContext->GetCurrentDirectory();

    if (inode == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }

    kget_directory_name_trw(inode, pathBuffer, bufferSize);
}

} // namespace kernel
