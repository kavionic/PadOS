// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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

    CRITICAL_SCOPE(m_Mutex);

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
