// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <string.h>
#include <fcntl.h>
#include <sys/uio.h>

#include "Kernel/VFS/FileIO.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KINode.h"
#include "Kernel/VFS/KRootFilesystem.h"
#include "Kernel/VFS/KVFSManager.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// Prepends a new name in front of a path.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int PrependNameToPath(char* buffer, int currentPathLength, char* name, int nameLength)
{
    if (currentPathLength > 0) {
        memmove(buffer + nameLength + 1, buffer, currentPathLength);
    }
    buffer[0] = '/';
    memcpy(buffer + 1, name, nameLength);
    return(currentPathLength + nameLength + 1);
}

///////////////////////////////////////////////////////////////////////////////
/// Some operations need to remove trailing slashes for POSIX.1 conformance.
/// For rename we also need to change the behavior depending on whether we
/// had a trailing slash or not.. (we cannot rename normal files with
/// trailing slashes, only directories)
///    
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool RemoveTrailingSlashes(String* name)
{
    bool slashesRemoved = false;
    while (name->size() > 1 && (*name)[name->size() - 1] == '/')
    {
        name->resize(name->size() - 1);
        slashesRemoved = true;
    }
    return slashesRemoved;
}


KMutex                                         FileIO::s_TableMutex("vfs_tables", PEMutexRecursionMode_RaiseError);
std::map<os::String, Ptr<kernel::KFilesystem>> FileIO::s_FilesystemDrivers;
Ptr<KFileTableNode>                            FileIO::s_PlaceholderFile;
Ptr<KRootFilesystem>                           FileIO::s_RootFilesystem;
Ptr<KFSVolume>                                 FileIO::s_RootVolume;
std::vector<Ptr<KFileTableNode>>               FileIO::s_FileTable;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FileIO::Initialze()
{
    s_PlaceholderFile = ptr_new<KFileTableNode>(false, 0);
    s_RootFilesystem = ptr_new<KRootFilesystem>();
    s_RootVolume = s_RootFilesystem->Mount(VOLID_ROOT, "", 0, nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileIO::RegisterFilesystem(const char* name, Ptr<KFilesystem> filesystem)
{
    try {
        CRITICAL_SCOPE(s_TableMutex);
        s_FilesystemDrivers[name] = filesystem;
        return true;
    }
    catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFilesystem> FileIO::FindFilesystem(const char* name)
{
    CRITICAL_SCOPE(s_TableMutex);
    auto i = s_FilesystemDrivers.find(name);
    if (i != s_FilesystemDrivers.end()) {
        return i->second;
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Mount(const char* devicePath, const char* directoryPath, const char* filesystemName, uint32_t flags, const char* args, size_t argLength)
{
    Ptr<KINode> mountPoint = LocateInodeByPath(nullptr, directoryPath, strlen(directoryPath));
    if (mountPoint == nullptr) {
        return -1;
    }
    Ptr<KFilesystem> filesystem = FindFilesystem(filesystemName);
    if (filesystem == nullptr) {
        return -1;
    }
    static fs_id nextFSID = VOLID_FIRST_NORMAL;
    Ptr<KFSVolume> volume = filesystem->Mount(nextFSID++, devicePath, flags, args, argLength);
    if (volume != nullptr) {
        KVFSManager::RegisterVolume(volume);
        volume->m_Filesystem = filesystem;
        volume->m_MountPoint = mountPoint;
        mountPoint->m_MountRoot = volume->m_RootNode;
        return 0;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileTableNode> FileIO::GetFileNode(int handle, bool forKernel)
{
    CRITICAL_SCOPE(s_TableMutex);
    if (handle >= 0 && handle < int(s_FileTable.size()) && s_FileTable[handle] != nullptr && s_FileTable[handle]->GetINode() != nullptr) {
        return s_FileTable[handle];
    }
    set_last_error(EBADF);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::GetFile(int handle, Ptr<KFileNode>& outFileNode)
{
    Ptr<KFileTableNode> node = GetFileNode(handle);
    if (node != nullptr)
    {
        if (!node->IsDirectory()) {
            outFileNode = ptr_static_cast<KFileNode>(node);
            return PErrorCode::Success;
        } else {
            return PErrorCode::IsDirectory;
        }
    }
    return PErrorCode::BadFile;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::GetFile(int handle, Ptr<KFileNode>& outFileNode, Ptr<KINode>& outInode)
{
    Ptr<KFileNode> file;
    
    PErrorCode result = GetFile(handle, file);
    if (result != PErrorCode::Success) {
        return result;
    }
    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_FileOps == nullptr)
    {
        return PErrorCode::NotImplemented;
    }
    outFileNode = file;
    outInode = inode;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KDirectoryNode> FileIO::GetDirectory(int handle)
{
    Ptr<KFileTableNode> node = GetFileNode(handle);
    if (node != nullptr && node->IsDirectory()) {
        return ptr_static_cast<KDirectoryNode>(node);
    }
    set_last_error(EBADF);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Open(int baseFolderFD, const char* path, int openFlags, int permissions)
{
    int handle = AllocateFileHandle();
    if (handle < 0) {
        return handle;
    }
    struct HandleGuard {
        HandleGuard(int handle) : m_Handle(handle) {}
        ~HandleGuard() { if (m_Handle != -1) FileIO::FreeFileHandle(m_Handle); }
        void Detach() { m_Handle = -1; }
        int m_Handle;
    } handleGuard(handle);

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = GetDirectory(baseFolderFD);
        if (baseFolderFile == nullptr)
        {
            return -1;
        }
        baseInode = baseFolderFile->GetINode();
    }

    size_t      pathLength = strlen(path);
    const char* name;
    size_t      nameLength;
    Ptr<KINode> parent = LocateParentInode(baseInode, path, pathLength, &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }

    Ptr<KINode> inode = LocateInodeByName(parent, name, nameLength, true);

    Ptr<KFileTableNode> file;
    if (inode == nullptr)
    {
        if (get_last_error() == ENOENT && (openFlags & O_CREAT))
        {
            file = parent->m_Filesystem->CreateFile(parent->m_Volume, parent, name, nameLength, openFlags, permissions);
            if (file == nullptr) {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    if (file == nullptr)
    {
        if (inode->m_FileOps == nullptr)
        {
            set_last_error(ENOSYS);
            return -1;
        }
        if (inode->IsDirectory()) {
            file = inode->m_FileOps->OpenDirectory(inode->m_Volume, inode);
        }
        else {
            file = inode->m_FileOps->OpenFile(inode->m_Volume, inode, (openFlags & ~O_CREAT));
        }
        if (file == nullptr) {
            return -1;
        }
        file->SetINode(inode);
    }
    handleGuard.Detach();
    SetFile(handle, file);
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Open(const char* path, int openFlags, int permissions)
{
    return Open(AT_FDCWD, path, openFlags, permissions);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::CopyFD(int oldHandle)
{
    Ptr<KFileTableNode> fileNode = GetFileNode(oldHandle);

    if (fileNode == nullptr) {
        set_last_error(EBADF);
        return -1;
    }
    return OpenInode(false, fileNode->GetINode(), fileNode->GetOpenFlags());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Dupe(int oldHandle, int newHandle)
{
    if (oldHandle == newHandle)
    {
        set_last_error(EINVAL);
        return -1;
    }
    Ptr<KFileNode> file;
    PErrorCode result = GetFile(oldHandle, file);
    if (result != PErrorCode::Success)
    {
        set_last_error(result);
        return -1;
    }
    if (newHandle == -1)
    {
        newHandle = AllocateFileHandle();
    }
    else
    {
        Close(newHandle);
        CRITICAL_SCOPE(s_TableMutex);
        if (newHandle >= int(s_FileTable.size())) {
            s_FileTable.resize(newHandle + 1);
        }
    }
    if (newHandle >= 0) {
        SetFile(newHandle, file);
    } else {
        set_last_error(EMFILE);
    }
    return newHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Close(int handle)
{
    if (handle >= 0 && handle <=2)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KFileTableNode> file = GetFileNode(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    FreeFileHandle(handle);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::GetFileFlags(int handle)
{
    Ptr<KFileNode> file;
    PErrorCode result = GetFile(handle, file);
    if (result != PErrorCode::Success) {
        return -1;
    }
    return file->GetOpenFlags();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::SetFileFlags(int handle, int flags)
{
    Ptr<KFileNode> file;
    PErrorCode result = GetFile(handle, file);
    if (result != PErrorCode::Success) {
        return -1;
    }
    file->SetOpenFlags(flags);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Read(int handle, void* buffer, size_t length, ssize_t& outLength)
{
    iovec_t segment;
    segment.iov_base = buffer;
    segment.iov_len = length;
    return Read(handle, &segment, 1, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Read(int handle, void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    iovec_t segment;
    segment.iov_base = buffer;
    segment.iov_len = length;
    return Read(handle, &segment, 1, position, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Read(int handle, const struct iovec* segments, size_t segmentCount, ssize_t& outLength)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file;
    PErrorCode result = GetFile(handle, file, inode);
    if (result != PErrorCode::Success) {
        return result;
    }
    ssize_t bytesRead;
    result = inode->m_FileOps->Read(file, segments, segmentCount, file->m_Position, bytesRead);
    if (result != PErrorCode::Success) {
        return result;
    }
    file->m_Position += bytesRead;
    outLength = bytesRead;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Read(int handle, const iovec_t* segments, size_t segmentCount, off64_t position, ssize_t& outLength)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file;
    PErrorCode result = GetFile(handle, file, inode);
    if (result != PErrorCode::Success) {
        return result;
    }
    return inode->m_FileOps->Read(file, segments, segmentCount, position, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t FileIO::Read(int handle, void* buffer, size_t length)
{
    ssize_t bytesRead;
    const PErrorCode result = Read(handle, buffer, length, bytesRead);
    if (result != PErrorCode::Success)
    {
        set_last_error(result);
        return -1;
    }
    return bytesRead;
}

ssize_t FileIO::Read(int handle, void* buffer, size_t length, off64_t position)
{
    ssize_t bytesRead;
    const PErrorCode result = Read(handle, buffer, length, position, bytesRead);
    if (result != PErrorCode::Success) {
        set_last_error(result);
        return -1;
    }
    return bytesRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Write(int handle, const void* buffer, size_t length, ssize_t& outLength)
{
    iovec_t segment;
    segment.iov_base = const_cast<void*>(buffer);
    segment.iov_len = length;
    return Write(handle, &segment, 1, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Write(int handle, const void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    iovec_t segment;
    segment.iov_base = const_cast<void*>(buffer);
    segment.iov_len = length;
    return Write(handle, &segment, 1, position, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Write(int handle, const iovec_t* segments, size_t segmentCount, ssize_t& outLength)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file;
    PErrorCode result = GetFile(handle, file, inode);
    if (result != PErrorCode::Success) {
        return result;
    }
    ssize_t bytesWritten;
    result = inode->m_FileOps->Write(file, segments, segmentCount, file->m_Position, bytesWritten);
    if (result != PErrorCode::Success) {
        return result;
    }
    file->m_Position += bytesWritten;
    outLength = bytesWritten;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Write(int handle, const iovec_t* segments, size_t segmentCount, off64_t position, ssize_t& outLength)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file;
    PErrorCode result = GetFile(handle, file, inode);
    if (result != PErrorCode::Success) {
        return result;
    }
    return inode->m_FileOps->Write(file, segments, segmentCount, position, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t FileIO::Write(int handle, const void* buffer, size_t length)
{
    iovec_t segment;
    segment.iov_base = const_cast<void*>(buffer);
    segment.iov_len = length;
    ssize_t bytesWritten;
    const PErrorCode result = Write(handle, &segment, 1, bytesWritten);
    if (result != PErrorCode::Success)
    {
        set_last_error(result);
        return -1;
    }
    return bytesWritten;
}


ssize_t FileIO::Write(int handle, const void* buffer, size_t length, off64_t position)
{
    ssize_t bytesWritten;
    const PErrorCode result = Write(handle, buffer, length, position, bytesWritten);
    if (result != PErrorCode::Success)
    {
        set_last_error(result);
        return -1;
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off64_t FileIO::Seek(int handle, off64_t offset, int mode)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file;
    const PErrorCode result = GetFile(handle, file, inode);
    if (result != PErrorCode::Success) {
        set_last_error(result);
        return -1;
    }
    switch (mode)
    {
        case SEEK_SET:
            if (offset < 0) {
                errno = EINVAL;
                return -1;
            }
            file->m_Position = offset;
            return file->m_Position;
        case SEEK_CUR:
            if (file->m_Position + offset < 0) {
                errno = EINVAL;
                return -1;
            }
            file->m_Position += offset;
            return file->m_Position;
        case SEEK_END:
        {
            struct stat fileStats;
            if (inode->m_FileOps->ReadStat(inode->m_Volume, inode, &fileStats) < 0) {
                return -1;
            }
            off64_t size = fileStats.st_size;
            if (size + offset < 0) {
                errno = EINVAL;
                return -1;
            }
            file->m_Position = size + offset;
            return file->m_Position;
        }
        default:
            errno = EINVAL;
            return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::FSync(int handle)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file;
    const PErrorCode result = GetFile(handle, file, inode);
    if (result != PErrorCode::Success) {
        set_last_error(result);
        return -1;
    }
    return inode->m_FileOps->Sync(file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::DeviceControl(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<KINode> inode;
    Ptr<KFileNode> file;
    const PErrorCode result = GetFile(handle, file, inode);
    if (result != PErrorCode::Success) {
        set_last_error(result);
        return -1;
    }
    return inode->m_FileOps->DeviceControl(file, request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::ReadDirectory(int handle, dirent_t* entry, size_t bufSize)
{
    Ptr<KDirectoryNode> dir = GetDirectory(handle);
    if (dir == nullptr) {
        return -1;
    }
    return dir->ReadDirectory(entry, bufSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::RewindDirectory(int handle)
{
    Ptr<KDirectoryNode> dir = GetDirectory(handle);
    if (dir == nullptr) {
        return -1;
    }
    return dir->RewindDirectory();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::CreateDirectory(int baseFolderFD, const char* path, int permission)
{
    int         pathLength = strlen(path);
    const char* name;
    size_t      nameLength;

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = GetDirectory(baseFolderFD);
        if (baseFolderFile == nullptr)
        {
            return -1;
        }
        baseInode = baseFolderFile->GetINode();
    }

    Ptr<KINode> parent = LocateParentInode(baseInode, path, pathLength, &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }
    return parent->m_Filesystem->CreateDirectory(parent->m_Volume, parent, name, nameLength, permission);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::CreateDirectory(const char* name, int permission)
{
    return CreateDirectory(AT_FDCWD, name, permission);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Symlink(const char* target, int baseFolderFD, const char* linkPath)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::Symlink(const char* target, const char* linkPath)
{
    return Symlink(target, AT_FDCWD, linkPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FileIO::ReadLink(int baseFolderFD, const char* path, char* buffer, size_t bufferSize, size_t* outResultLength)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::ReadStats(int handle, struct stat* outStats)
{
    Ptr<KFileTableNode> node = GetFileNode(handle);
    if (node == nullptr) {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = node->GetINode();
    if (inode == nullptr || inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }
    return inode->m_FileOps->ReadStat(inode->m_Volume, inode, outStats);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::WriteStats(int handle, const struct stat& value, uint32_t mask)
{
    Ptr<KFileTableNode> node = GetFileNode(handle);
    if (node == nullptr) {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = node->GetINode();
    if (inode == nullptr || inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }
    return inode->m_FileOps->WriteStat(inode->m_Volume, inode, &value, mask);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Rename(const char* inOldPath, const char* inNewPath)
{
    if (inOldPath == nullptr || inNewPath == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    try
    {
        String oldPath(inOldPath);
        String newPath(inNewPath);

        bool mustBeDir = RemoveTrailingSlashes(&oldPath);
        mustBeDir = RemoveTrailingSlashes(&newPath) || mustBeDir;

        const char* oldName;
        size_t      oldNameLength;
        Ptr<KINode> oldParent = LocateParentInode(nullptr, oldPath.c_str(), oldPath.size(), &oldName, &oldNameLength);

        if (oldParent == nullptr) {
            return -1;
        }

        const char* newName;
        size_t      newNameLength;
        Ptr<KINode> newParent = LocateParentInode(nullptr, newPath.c_str(), newPath.size(), &newName, &newNameLength);

        if (newParent == nullptr) {
            return -1;
        }
        if (oldParent->m_Volume->m_VolumeID != newParent->m_Volume->m_VolumeID) {
            set_last_error(EXDEV);
            return -1;
        }
        return oldParent->m_Filesystem->Rename(oldParent->m_Volume, oldParent, oldName, oldNameLength, newParent, newName, newNameLength, mustBeDir);
    }
    catch (const std::bad_alloc&)
    {
        set_last_error(ENOMEM);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Unlink(int baseFolderFD, const char* inPath)
{
    if (inPath == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    String path(inPath);

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = GetDirectory(baseFolderFD);
        if (baseFolderFile == nullptr)
        {
            return -1;
        }
        baseInode = baseFolderFile->GetINode();
    }

    const char* name;
    size_t      nameLength;

    Ptr<KINode> parent = LocateParentInode(baseInode, path.c_str(), path.size(), &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }
    return parent->m_Filesystem->Unlink(parent->m_Volume, parent, name, nameLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Unlink(const char* path)
{
    return Unlink(AT_FDCWD, path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::RemoveDirectory(int baseFolderFD, const char* inPath)
{
    if (inPath == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    String path(inPath);

    const char* name;
    size_t      nameLength;

    Ptr<KINode> baseInode;
    if (baseFolderFD != AT_FDCWD)
    {
        Ptr<KFileTableNode> baseFolderFile = GetDirectory(baseFolderFD);
        if (baseFolderFile == nullptr)
        {
            return -1;
        }
        baseInode = baseFolderFile->GetINode();
    }

    Ptr<KINode> parent = LocateParentInode(baseInode, path.c_str(), path.size(), &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }
    return parent->m_Filesystem->RemoveDirectory(parent->m_Volume, parent, name, nameLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::RemoveDirectory(const char* path)
{
    return RemoveDirectory(AT_FDCWD, path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::GetDirectoryPath(int handle, char* buffer, size_t bufferSize)
{
    Ptr<KDirectoryNode> directory = GetDirectory(handle);
    if (directory == nullptr)
    {
        return -1;
    }
    Ptr<KINode> inode = directory->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    return GetDirectoryName(inode, buffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> FileIO::LocateInodeByName(Ptr<KINode> parent, const char* name, int nameLength, bool crossMount)
{
    if (nameLength == 0) {
        return parent;
    }
    if (nameLength == 2 && name[0] == '.' && name[1] == '.' && parent == parent->m_Volume->m_RootNode)
    {
        if (parent != s_RootVolume->m_RootNode) {
            parent = parent->m_Volume->m_MountPoint;
        } else {
            return parent;
        }
    }
    Ptr<KINode> inode = parent->m_Filesystem->LocateInode(parent->m_Volume, parent, name, nameLength);
    if (inode != nullptr && crossMount && inode->m_MountRoot != nullptr) {
        inode = inode->m_MountRoot;
    }
    return inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> FileIO::LocateInodeByPath(Ptr<KINode> parent, const char* path, int pathLength)
{
    const char* name;
    size_t      nameLength;
    parent = LocateParentInode(parent, path, pathLength, &name, &nameLength);
    if (parent != nullptr) {
        return LocateInodeByName(parent, name, nameLength, true);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> FileIO::LocateParentInode(Ptr<KINode> parent, const char* path, int pathLength, const char** outName, size_t* outNameLength)
{
    Ptr<KINode> current = parent;

    int i = 0;
    if (path[0] == '/')
    {
        current = s_RootVolume->m_RootNode;
        ++i;
    }
    else if (current == nullptr)
    {
        current = s_RootVolume->m_RootNode; // FIXME: Implement working directory
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
            current = LocateInodeByName(current, path + nameStart, i - nameStart, true);
            if (current == nullptr) {
                return nullptr;
            }
            nameStart = i + 1;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::GetDirectoryName(Ptr<KINode> inode, char* path, size_t bufferSize)
{
    status_t    error = 0;
    int         pathLength = 0;

    while (error == 0)
    {
        dirent_t    dirEntry;
        int         directoryHandle;

        Ptr<KINode> parent = LocateInodeByName(inode, "..", 2, true);
        if (parent == nullptr) {
            break;
        }
        directoryHandle = OpenInode(true, parent, O_RDONLY);

        if (directoryHandle < 0) {
            return -1;
        }
        bool isMountPoint = (inode->m_Volume != parent->m_Volume);
        error = ENOENT;
        while (ReadDirectory(directoryHandle, &dirEntry, sizeof(dirEntry)) == 1)
        {
            if (strcmp(dirEntry.d_name, ".") == 0 || strcmp(dirEntry.d_name, "..") == 0) {
                continue;
            }
            if (isMountPoint)
            {
                Ptr<KINode> entryInode = LocateInodeByName(parent, dirEntry.d_name, dirEntry.d_namlen, false);
                if (entryInode == nullptr)
                {
                    error = get_last_error();
                    if (error == 0) error = EINVAL;
                    break;
                }
                if (entryInode->m_MountRoot == inode)
                {
                    if (pathLength + dirEntry.d_namlen + 1 > bufferSize) {
                        error = ENAMETOOLONG;
                        break;
                    }
                    pathLength = PrependNameToPath(path, pathLength, dirEntry.d_name, dirEntry.d_namlen);
                    error = 0;
                    break;
                }
            }
            else if (dirEntry.d_ino == inode->m_INodeID)
            {
                if (pathLength + dirEntry.d_namlen + 1 > bufferSize) {
                    error = ENAMETOOLONG;
                    break;
                }
                pathLength = PrependNameToPath(path, pathLength, dirEntry.d_name, dirEntry.d_namlen);
                error = 0;
                break;
            }
        }
        Close(directoryHandle);
        inode = parent;
        if (error == 0 && inode == s_RootVolume->m_RootNode)
        {
            if (pathLength < bufferSize) {
                path[pathLength] = '\0';
            }
            break;
        }
    }
    if (error == 0) {
        return pathLength;
    } else {
        set_last_error(error);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::AllocateFileHandle()
{
    CRITICAL_SCOPE(s_TableMutex);
    auto i = std::find(s_FileTable.begin(), s_FileTable.end(), nullptr);
    if (i != s_FileTable.end())
    {
        int file = i - s_FileTable.begin();
        s_FileTable[file] = s_PlaceholderFile;
        return file;
    }
    else
    {
        int file = s_FileTable.size();
        s_FileTable.push_back(s_PlaceholderFile);
        return file;
        //set_last_error(EMFILE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FileIO::FreeFileHandle(int handle)
{
    CRITICAL_SCOPE(s_TableMutex);
    if (handle >= 0 && handle < int(s_FileTable.size())) {
        s_FileTable[handle] = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::OpenInode(bool kernelFile, Ptr<KINode> inode, int openFlags)
{
    int handle = AllocateFileHandle();
    if (handle < 0) {
        return handle;
    }
    struct HandleGuard {
        HandleGuard(int handle) : m_Handle(handle) {}
        ~HandleGuard() { if (m_Handle != -1) FileIO::FreeFileHandle(m_Handle); }
        void Detach() { m_Handle = -1; }
        int m_Handle;
    } handleGuard(handle);

    Ptr<KFileTableNode> file;
    if (inode->m_FileOps == nullptr)
    {
        set_last_error(ENOSYS);
        return -1;
    }
    if (inode->IsDirectory()) {
        file = inode->m_FileOps->OpenDirectory(inode->m_Volume, inode);
    }
    else {
        file = inode->m_FileOps->OpenFile(inode->m_Volume, inode, (openFlags & ~O_CREAT));
    }
    if (file == nullptr) {
        return -1;
    }
    file->SetINode(inode);

    handleGuard.Detach();
    SetFile(handle, file);
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FileIO::SetFile(int handle, Ptr<KFileTableNode> file)
{
    CRITICAL_SCOPE(s_TableMutex);
    if (handle >= 0 && handle < int(s_FileTable.size()))
    {
        s_FileTable[handle] = file;
    }
}
