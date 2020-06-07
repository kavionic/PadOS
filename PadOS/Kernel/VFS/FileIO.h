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

#include <vector>
#include <map>

#include "System/Types.h"
#include "Ptr/Ptr.h"
#include "Utils/String.h"


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
    struct dir_entry;
}

namespace os
{

    
class FileIO
{
public:
    static void    Initialze();
    
    static bool    RegisterFilesystem(const char* name, Ptr<kernel::KFilesystem> filesystem);
    static Ptr<kernel::KFilesystem> FindFilesystem(const char* name);
    
    static int     Mount(const char* devicePath, const char* directoryPath, const char* filesystemName, uint32_t flags, const char* args, size_t argLength);
    
    static int     Open(const char* path, int flags, int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    static int     Dupe(int oldHandle, int newHandle);
    static int     Close(int handle);
    
    
    static ssize_t Read(int handle, void* buffer, size_t length);
    static ssize_t Write(int handle, const void* buffer, size_t length);
    static ssize_t Write(int handle, off64_t position, const void* buffer, size_t length);
    static ssize_t Read(int handle, off64_t position, void* buffer, size_t length);
    static int     DeviceControl(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

    static int     ReadDirectory(int handle, kernel::dir_entry* entry, size_t bufSize);
    static int     RewindDirectory(int handle);

    static int     CreateDirectory(const char* name, int permission);

    static int     Rename(const char* oldPath, const char* newPath);
    static int     Unlink(const char* path);
    static int     RemoveDirectory(const char* path);
    

//    static int ReadAsync(int handle, off64_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback);
//    static int WriteAsync(int handle, off64_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback);
//    static int CancelAsyncRequest(int handle, int requestHandle);
private:
    friend class kernel::Kernel;

    static Ptr<kernel::KINode>         LocateInodeByName(Ptr<kernel::KINode> parent, const char* name, int nameLength, bool crossMount);
    static Ptr<kernel::KINode>         LocateInodeByPath(Ptr<kernel::KINode> parent, const char* path, int pathLength);
    static Ptr<kernel::KINode>         LocateParentInode(Ptr<kernel::KINode> parent, const char* path, int pathLength, const char** outName, size_t* outNameLength);
    static int                         AllocateFileHandle();
    static void                        FreeFileHandle(int handle);
    static Ptr<kernel::KFileTableNode> GetFileNode(int handle, bool forKernel = false);
    static Ptr<kernel::KFileNode>    GetFile(int handle);
    static Ptr<kernel::KDirectoryNode> GetDirectory(int handle);
    static void                        SetFile(int handle, Ptr<kernel::KFileTableNode> file);

    static std::map<os::String, Ptr<kernel::KFilesystem>> s_FilesystemDrivers;

    static Ptr<kernel::KFileTableNode>     s_PlaceholderFile;
    static Ptr<kernel::KRootFilesystem> s_RootFilesystem;
    static Ptr<kernel::KFSVolume>       s_RootVolume;
    static std::vector<Ptr<kernel::KFileTableNode>> s_FileTable;

};

} // namespace
