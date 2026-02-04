// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.02.2018 01:49:14

#include <System/Platform.h>

#include <string.h>
#include <fcntl.h>
#include <atomic>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/VFS/KRootFilesystem.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <System/System.h>
#include <System/ExceptionHandling.h>
#include <Utils/String.h>


namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KRootFilesystem::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    Ptr<KFSVolume> volume = KVirtualFilesystemBase::Mount(volumeID, devicePath, flags, args, argLength);

    CRITICAL_SCOPE(m_Mutex);

    const Ptr<KVirtualFSBaseInode> rootNode = ptr_dynamic_cast<KVirtualFSBaseInode>(volume->m_RootNode);
    const Ptr<KVirtualFSBaseInode> devRoot  = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(rootNode), this, S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
    rootNode->m_Children["dev"] = devRoot;

    m_DevRoot = devRoot;

    return volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::RegisterDevice(const char* path, Ptr<KInode> deviceNode)
{
    CRITICAL_SCOPE(m_Mutex);
    int pathLength = strlen(path);

    int nameStart = 0;
    Ptr<KVirtualFSBaseInode> parent = LocateParentInode(m_DevRoot, path, pathLength, true, &nameStart);

    int nameLength = pathLength - nameStart;
    if (nameLength == 0)
    {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    int32_t handle = AllocInodeNumber();
    deviceNode->m_InodeID   = handle;
    deviceNode->m_Filesystem = ptr_tmp_cast(this);
    deviceNode->m_Volume     = m_Volume;
    
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "Register device {} at '/dev/{}'.", handle, path);
    parent->m_Children[path + nameStart] = deviceNode;
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KRootFilesystem::RenameDevice(int handle, const char* newPath)
{
    CRITICAL_SCOPE(m_Mutex);
    
    Ptr<KVirtualFSBaseInode> prevParent;
    Ptr<KInode> node = FindInode(m_DevRoot, handle, true, &prevParent);

    int pathLength = strlen(newPath);

    int nameStart = 0;
    Ptr<KVirtualFSBaseInode> newParent = LocateParentInode(m_DevRoot, newPath, pathLength, true, &nameStart);

    int nameLength = pathLength - nameStart;
    if (nameLength == 0)
    {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    newParent->m_Children[newPath + nameStart] = node;
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "Rename device {} at '/dev/{}'.", handle, newPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KRootFilesystem::RemoveDevice(int handle)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KVirtualFSBaseInode> prevParent;
    Ptr<KInode> node = FindInode(m_DevRoot, handle, true, &prevParent);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "Remove device {}.", handle);
    // Remove empty folders
    while(prevParent != m_DevRoot && prevParent->m_Children.empty())
    {
        FindInode(m_DevRoot, prevParent->m_InodeID, true, &prevParent);
    }
}


} // namespace kernel
