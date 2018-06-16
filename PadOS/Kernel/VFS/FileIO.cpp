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
// Created: 08.05.2018 20:01:27

#include "sam.h"

#include <string.h>
#include <fcntl.h>

#include "FileIO.h"
#include "KFileHandle.h"
#include "KFSVolume.h"
#include "KINode.h"
#include "KRootFilesystem.h"
#include "KVFSManager.h"

using namespace kernel;
using namespace os;

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
    s_PlaceholderFile = ptr_new<KFileTableNode>(false);
    s_RootFilesystem = ptr_new<KRootFilesystem>();
    s_RootVolume =  s_RootFilesystem->Mount(VOLID_ROOT, "", 0, nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FileIO::RegisterFilesystem(const char* name, Ptr<KFilesystem> filesystem)
{
    try {
        s_FilesystemDrivers[name] = filesystem;
        return true;
    } catch(const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return false;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFilesystem> FileIO::FindFilesystem(const char* name)
{
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

int FileIO::Open(const char* path, int flags, int permissions)
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
    
    size_t      pathLength = strlen(path);
    const char* name;
    size_t      nameLength;
    Ptr<KINode> parent = LocateParentInode(nullptr, path, pathLength, &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }
    
    Ptr<KINode> inode = LocateInodeByName(parent, name, nameLength, true);

    Ptr<KFileTableNode> file;
    if (inode == nullptr)
    {
        if (get_last_error() == ENOENT && (flags & O_CREAT))
        {
            file = parent->m_Filesystem->CreateFile(parent->m_Volume, parent, name, nameLength, flags, permissions);
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
//    Ptr<KINode> inode = LocateInodeByPath(nullptr, path, strlen(path));
    
        if (inode->m_FileOps == nullptr)
        {
            set_last_error(ENOSYS);
            return -1;
        }
        if (inode->IsDirectory()) {
            file = inode->m_FileOps->OpenDirectory(inode->m_Volume, inode);
        } else {
            file = inode->m_FileOps->OpenFile(inode->m_Volume, inode, (flags & ~O_CREAT));
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

int FileIO::Dupe(int oldHandle, int newHandle)
{
    if (oldHandle == newHandle)
    {
        set_last_error(EINVAL);
        return -1;
    }
    Ptr<KFileTableNode> file = GetFile(oldHandle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    if (newHandle == -1) {
        newHandle = AllocateFileHandle();
    } else {
        Close(newHandle);
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

    Ptr<KFileTableNode> file = GetFileNode(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    FreeFileHandle(handle);

    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr && inode->m_FileOps !=  nullptr);

    int result = 0;
    if (inode->IsDirectory()) {
        result = inode->m_FileOps->CloseDirectory(inode->m_Volume, ptr_static_cast<KDirectoryNode>(file));
    } else {
        result = inode->m_FileOps->CloseFile(inode->m_Volume, ptr_static_cast<KFileNode>(file));
    }
//    inode->m_Filesystem->ReleaseInode(inode);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t FileIO::Read(int handle, void* buffer, size_t length)
{
    Ptr<KFileNode> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }

    ssize_t result = inode->m_FileOps->Read(file, file->m_Position, buffer, length);
    if (result < 0)
    {
        return result;
    }
    file->m_Position += result;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t FileIO::Write(int handle, const void* buffer, size_t length)
{
    Ptr<KFileNode> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }

    ssize_t result = inode->m_FileOps->Write(file, file->m_Position, buffer, length);
    if (result < 0)
    {
        return result;
    }
    file->m_Position += result;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t FileIO::Read(int handle, off64_t position, void* buffer, size_t length)
{
    Ptr<KFileNode> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    
    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }

    return inode->m_FileOps->Read(file, position, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t FileIO::Write(int handle, off64_t position, const void* buffer, size_t length)
{
    Ptr<KFileNode> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    
    if (inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }
    
    return inode->m_FileOps->Write(file, position, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::DeviceControl(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<KFileNode> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }
    
    return inode->m_FileOps->DeviceControl(file, request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::ReadDirectory(int handle, int position, dir_entry* entry, size_t bufSize)
{
    Ptr<KDirectoryNode> dir = GetDirectory(handle);
    if (dir == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    
    Ptr<KINode> inode = dir->GetINode();
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_FileOps == nullptr) {
        set_last_error(ENOSYS);
        return -1;
    }
    
    return inode->m_FileOps->ReadDirectory(inode->m_Volume, dir, position, entry, bufSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::CreateDirectory(const char* path, int permission)
{
    int         pathLength = strlen(path);
    const char* name;
    size_t      nameLength;
    Ptr<KINode> parent = LocateParentInode(nullptr, path, pathLength, &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }
    return parent->m_Filesystem->CreateDirectory(parent->m_Volume, parent, name, nameLength, permission);
}

///////////////////////////////////////////////////////////////////////////////
///    Some operations need to remove trailing slashes for POSIX.1 conformance.
///    For rename we also need to change the behavior depending on whether we
///    had a trailing slash or not.. (we cannot rename normal files with
///    trailing slashes, only dirs)
///    
///    "dummy" is used to make sure we don't do "/" -> "".
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool remove_trailing_slashes(String* name)
{
    bool slashesRemoved = false;
    while(name->size() > 1 && (*name)[name->size()-1] == '/') {
        name->resize(name->size() - 1);
    }
    return slashesRemoved;
/*    
    int   nError;
    char  nChar[1];
    char* pzRemove = nChar + 1;

    for (;;) {
	char c = *pzName;
	pzName++;
	if ( c == 0 ) {
	    break;
	}
	if ( c != '/' ) {
	    pzRemove = NULL;
	    continue;
	}
	if ( pzRemove != NULL ) {
	    continue;
	}
	pzRemove = pzName;
    }

    nError = 0;
    if ( pzRemove != NULL ) {
	pzRemove[-1] = 0;
	nError = 1;
    }

    return( nError );*/
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
        
        bool mustBeDir = remove_trailing_slashes(&oldPath);
        mustBeDir = remove_trailing_slashes(&newPath) | mustBeDir;
        
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
    catch(const std::bad_alloc&)
    {
        set_last_error(ENOMEM);
        return -1;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::Unlink(const char* inPath)
{
    if (inPath == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    String path(inPath);
    
    const char* name;
    size_t      nameLength;
    
    Ptr<KINode> parent = LocateParentInode(nullptr, path.c_str(), path.size(), &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }
    return parent->m_Filesystem->Unlink(parent->m_Volume, parent, name, nameLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int FileIO::RemoveDirectory(const char* inPath)
{
    if (inPath == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    String path(inPath);
    
    const char* name;
    size_t      nameLength;
    
    Ptr<KINode> parent = LocateParentInode(nullptr, path.c_str(), path.size(), &name, &nameLength);
    if (parent == nullptr) {
        return -1;
    }
    return parent->m_Filesystem->RemoveDirectory(parent->m_Volume, parent, name, nameLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> FileIO::LocateInodeByName(Ptr<KINode> parent, const char* name, int nameLength, bool crossMount)
{
    if (nameLength == 2 && name[0] == '.' && name[1] == '.' && parent == parent->m_Volume->m_RootNode)
    {
        if ( parent != s_RootVolume->m_RootNode ) {
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
    if (path[0] == '/') {
        current = s_RootVolume->m_RootNode;
        ++i;
    } else if (current == nullptr) {
        current = s_RootVolume->m_RootNode; // FIXME: Implement working directory
    }
    int nameStart = i;
    
    for (;; ++i)
    {
        if (i == pathLength)
        {
            *outName       = path + nameStart;
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

int FileIO::AllocateFileHandle()
{
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
    if (handle >= 0 && handle < int(s_FileTable.size())) {
        s_FileTable[handle] = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileTableNode> FileIO::GetFileNode(int handle)
{
    if (handle >= 0 && handle < int(s_FileTable.size()) && s_FileTable[handle] != nullptr && s_FileTable[handle]->GetINode() != nullptr)
    {
        return s_FileTable[handle];
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<kernel::KFileNode> FileIO::GetFile(int handle)
{
    Ptr<KFileTableNode> node = GetFileNode(handle);
    if (node != nullptr && !node->IsDirectory()) {
        return ptr_static_cast<KFileNode>(node);
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<kernel::KDirectoryNode> FileIO::GetDirectory(int handle)
{
    Ptr<KFileTableNode> node = GetFileNode(handle);
    if (node != nullptr && node->IsDirectory()) {
        return ptr_static_cast<KDirectoryNode>(node);
    } else {
        return nullptr;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FileIO::SetFile(int handle, Ptr<KFileTableNode> file)
{
    if (handle >= 0 && handle < int(s_FileTable.size()))
    {
        s_FileTable[handle] = file;
    }
}

