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

struct IOSegment
{
    IOSegment(void* inBuffer = nullptr, size_t inLength = 0) : Buffer(inBuffer), Length(inLength) {}
    void* Buffer;
    size_t  Length;
};

class FileIO
{
public:
    static IFLASHC void    Initialze();
    
    static IFLASHC bool    RegisterFilesystem(const char* name, Ptr<kernel::KFilesystem> filesystem);
    static IFLASHC Ptr<kernel::KFilesystem> FindFilesystem(const char* name);
    
    static IFLASHC int     Mount(const char* devicePath, const char* directoryPath, const char* filesystemName, uint32_t flags, const char* args, size_t argLength);
    
    static IFLASHC int     Open(const char* path, int openFlags, int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) { return Open(-1, path, openFlags, permissions); }
    static IFLASHC int     Open(int baseFolderFD, const char* path, int openFlags, int permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    static IFLASHC int     CopyFD(int oldHandle);
    static IFLASHC int     Dupe(int oldHandle, int newHandle = -1);
    static IFLASHC int     Close(int handle);
    
    static IFLASHC ssize_t Read(int handle, void* buffer, size_t length);
    static IFLASHC ssize_t Write(int handle, const void* buffer, size_t length);

    static IFLASHC ssize_t Read(int handle, const IOSegment* segments, size_t segmentCount);
    static IFLASHC ssize_t Write(int handle, const IOSegment* segments, size_t segmentCount);

    static IFLASHC ssize_t Read(int handle, off64_t position, void* buffer, size_t length);
    static IFLASHC ssize_t Write(int handle, off64_t position, const void* buffer, size_t length);

    static IFLASHC ssize_t Read(int handle, off64_t position, const IOSegment* segments, size_t segmentCount);
    static IFLASHC ssize_t Write(int handle, off64_t position, const IOSegment* segments, size_t segmentCount);

    static IFLASHC int     DeviceControl(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

    static IFLASHC int     ReadDirectory(int handle, kernel::dir_entry* entry, size_t bufSize);
    static IFLASHC int     RewindDirectory(int handle);

    static IFLASHC int     CreateDirectory(const char* name, int permission = S_IRWXU) { return CreateDirectory(-1, name, permission); }
    static IFLASHC int     CreateDirectory(int baseFolderFD, const char* name, int permission = S_IRWXU);

    static IFLASHC int     Symlink(const char* target, const char* linkPath) { return Symlink(-1, target, linkPath); }
    static IFLASHC int     Symlink(int baseFolderFD, const char* target, const char* linkPath);

    static IFLASHC int	   ReadStats(int handle, struct stat* outStats);
    static IFLASHC int	   WriteStats(int handle, const struct stat& value, uint32_t mask);

    static IFLASHC int     Rename(const char* oldPath, const char* newPath);
    static IFLASHC int     Unlink(const char* path) { return Unlink(-1, path); }
    static IFLASHC int     Unlink(int baseFolderFD, const char* path);
    static IFLASHC int     RemoveDirectory(const char* path) { return RemoveDirectory(-1, path); }
    static IFLASHC int     RemoveDirectory(int baseFolderFD, const char* path);

    static IFLASHC int     GetDirectoryPath(int handle, char* buffer, size_t bufferSize);

private:
    friend class kernel::Kernel;

    static IFLASHC Ptr<kernel::KINode>          LocateInodeByName(Ptr<kernel::KINode> parent, const char* name, int nameLength, bool crossMount);
    static IFLASHC Ptr<kernel::KINode>          LocateInodeByPath(Ptr<kernel::KINode> parent, const char* path, int pathLength);
    static IFLASHC Ptr<kernel::KINode>          LocateParentInode(Ptr<kernel::KINode> parent, const char* path, int pathLength, const char** outName, size_t* outNameLength);
    static IFLASHC int                          GetDirectoryName(Ptr<kernel::KINode> inode, char* path, size_t bufferSize);
    static IFLASHC int                          AllocateFileHandle();
    static IFLASHC void                         FreeFileHandle(int handle);
    static IFLASHC int                          OpenInode(bool kernelFile, Ptr<kernel::KINode> inode, int openFlags);
    static IFLASHC Ptr<kernel::KFileTableNode>  GetFileNode(int handle, bool forKernel = false);
    static IFLASHC Ptr<kernel::KFileNode>       GetFile(int handle);
    static IFLASHC Ptr<kernel::KDirectoryNode>  GetDirectory(int handle);
    static IFLASHC void                         SetFile(int handle, Ptr<kernel::KFileTableNode> file);

    static std::map<os::String, Ptr<kernel::KFilesystem>> s_FilesystemDrivers;

    static Ptr<kernel::KFileTableNode>              s_PlaceholderFile;
    static Ptr<kernel::KRootFilesystem>             s_RootFilesystem;
    static Ptr<kernel::KFSVolume>                   s_RootVolume;
    static std::vector<Ptr<kernel::KFileTableNode>> s_FileTable;
};

} // namespace
