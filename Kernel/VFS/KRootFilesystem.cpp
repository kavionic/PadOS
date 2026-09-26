// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    m_Volume = ptr_static_cast<KVirtualFSVolume>(KVirtualFilesystemBase::Mount(volumeID, devicePath, flags, args, argLength));

    const Ptr<KVirtualFSBaseInode> rootNode = ptr_dynamic_cast<KVirtualFSBaseInode>(m_Volume->m_RootNode);
    const Ptr<KVirtualFSBaseInode> devRoot  = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), m_Volume, ptr_raw_pointer_cast(rootNode), this, S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        
    rootNode->m_Children["dev"] = devRoot;

    m_DevRoot = devRoot;

    return m_Volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::RegisterDevice(const char* path, Ptr<KInode> deviceNode)
{
    CRITICAL_SCOPE(m_Volume->m_Mutex);
    int pathLength = strlen(path);

    int nameStart = 0;
    Ptr<KVirtualFSBaseInode> parent = LocateParentInode(m_Volume, m_DevRoot, path, pathLength, true, &nameStart);

    int nameLength = pathLength - nameStart;
    if (nameLength == 0)
    {
        PERROR_THROW_CODE(PErrorCode::INVAL);
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
    CRITICAL_SCOPE(m_Volume->m_Mutex);
    
    Ptr<KVirtualFSBaseInode> prevParent;
    Ptr<KInode> node = FindInode(m_Volume, m_DevRoot, handle, true, &prevParent);

    int pathLength = strlen(newPath);

    int nameStart = 0;
    Ptr<KVirtualFSBaseInode> newParent = LocateParentInode(m_Volume, m_DevRoot, newPath, pathLength, true, &nameStart);

    int nameLength = pathLength - nameStart;
    if (nameLength == 0)
    {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    newParent->m_Children[newPath + nameStart] = node;
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "Rename device {} at '/dev/{}'.", handle, newPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KRootFilesystem::RemoveDevice(int handle)
{
    CRITICAL_SCOPE(m_Volume->m_Mutex);

    Ptr<KVirtualFSBaseInode> prevParent;
    Ptr<KInode> node = FindInode(m_Volume, m_DevRoot, handle, true, &prevParent);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "Remove device {}.", handle);
    // Remove empty folders
    while(prevParent != m_DevRoot && prevParent->m_Children.empty())
    {
        FindInode(m_Volume, m_DevRoot, prevParent->m_InodeID, true, &prevParent);
    }
}


} // namespace kernel
