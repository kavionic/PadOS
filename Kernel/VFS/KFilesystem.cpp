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
// Created: 23.02.2018 01:53:33

#include "System/Platform.h"

#include <sys/uio.h>
#include <sys/types.h>

#include "Kernel/VFS/KFilesystem.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KINode.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/VFS/FileIO.h"
#include "System/System.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::Probe(const char* devicePath, fs_info* fsInfo)
{
    set_last_error(ENOSYS);
    return -1;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KFilesystem::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    set_last_error(ENOSYS);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::Unmount(Ptr<KFSVolume> volume)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::Sync(Ptr<KFSVolume> volume)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::ReadFSStat(Ptr<KFSVolume> volume, fs_info* fsinfo)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::WriteFSStat(Ptr<KFSVolume> volume, const fs_info* fsinfo, uint32_t mask)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> KFilesystem::LocateInode(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* path, int pathLength)
{
    set_last_error(ENOSYS);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KFilesystem::ReleaseInode(KINode* inode)
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KFilesystemFileOps::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int openFlags)
{
    try
    {
        Ptr<KFileNode> file = ptr_new<KFileNode>(openFlags);
    	return file;
    }
    catch (const std::bad_alloc&)
    {
        set_last_error(ENOMEM);
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KFilesystem::CreateFile(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int openFlags, int permission)
{
    set_last_error(ENOSYS);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> KFilesystem::LoadInode(Ptr<KFSVolume> volume, ino_t inode)
{
    set_last_error(ENOSYS);
    return nullptr;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KDirectoryNode> KFilesystemFileOps::OpenDirectory(Ptr<KFSVolume> volume, Ptr<KINode> node)
{
    set_last_error(ENOSYS);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::CreateDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int permission)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::Rename(Ptr<KFSVolume> volume, Ptr<KINode> oldParent, const char* oldName, int oldNameLen, Ptr<KINode> newParent, const char* newName, int newNameLen, bool mustBeDir)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
int KFilesystem::Unlink(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystem::RemoveDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KFilesystemFileOps::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KFilesystemFileOps::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KFilesystemFileOps::Read(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position, ssize_t& outLength)
{
    ssize_t totalBytesRead = 0;
    for (size_t i = 0; i < segmentCount; ++i)
    {
        const iovec_t& segment = segments[i];

        ssize_t bytesRead = 0;
        const PErrorCode result = Read(file, segment.iov_base, segment.iov_len, position, bytesRead);
        if (result != PErrorCode::Success) {
            return result;
        }
        totalBytesRead += bytesRead;
        position += bytesRead;
        if (bytesRead != segment.iov_len) {
            break;
        }
    }
    outLength = totalBytesRead;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KFilesystemFileOps::Write(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position, ssize_t& outLength)
{
    ssize_t totalBytesWritten = 0;
    for (size_t i = 0; i < segmentCount; ++i)
    {
        const iovec_t& segment = segments[i];
        ssize_t bytesWritten = 0;
        const PErrorCode result = Write(file, segment.iov_base, segment.iov_len, position, bytesWritten);
        if (result != PErrorCode::Success) {
            return result;
        }
        totalBytesWritten += bytesWritten;
        position += bytesWritten;
        if (bytesWritten != segment.iov_len) {
            break;
        }
    }
    outLength = totalBytesWritten;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::ReadLink(Ptr<KFSVolume> volume, Ptr<KINode> node, char* buffer, size_t bufferSize)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> dirNode)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::CheckAccess(Ptr<KFSVolume> volume, Ptr<KINode> node, int mode)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* stat)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::WriteStat(Ptr<KFSVolume> volume, Ptr<KINode> node, const struct stat* stat, uint32_t mask)
{
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KFilesystemFileOps::Sync(Ptr<KFileNode> file)
{
    return 0;
}
