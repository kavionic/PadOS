// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 29.01.2026 23:00

#include <string.h>
#include <fcntl.h>
#include <atomic>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/FSDrivers/BinFS/BinFS.h>
#include <System/System.h>
#include <System/ExceptionHandling.h>
#include <System/AppDefinition.h>
#include <Utils/String.h>


namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KBinFilesystem::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    Ptr<KFSVolume> volume = KVirtualFilesystemBase::Mount(volumeID, devicePath, flags, args, argLength);

    const Ptr<KVirtualFSBaseInode> rootNode = ptr_dynamic_cast<KVirtualFSBaseInode>(volume->m_RootNode);

    const std::vector<const PAppDefinition*> apps = PAppDefinition::GetApplicationList();

    for (const PAppDefinition* app : apps)
    {
        Ptr<KVirtualFSBaseInode> fileInode = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(rootNode), this, S_IFREG | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

        const size_t nameLen = strlen(app->Name);
        fileInode->m_FileData.insert(fileInode->m_FileData.begin(), app->Name, app->Name + nameLen);

        rootNode->m_Children[app->Name] = fileInode;
    }


    return volume;
}


} // namespace kernel
