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
// Created: 23.02.2018 01:49:14

#include "System/Platform.h"

#include <string.h>
#include <fcntl.h>
#include <atomic>

#include "Kernel/VFS/KRootFilesystem.h"
#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/VFS/KDeviceNode.h"
#include "System/System.h"
#include "Utils/String.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KRootFSINode::KRootFSINode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KRootFSINode* parent, KFilesystemFileOps* fileOps, bool isDirectory)
    : KINode(filesystem, volume, fileOps, isDirectory)
    , m_Parent(parent)
{
    m_INodeID = KRootFilesystem::AllocINodeNumber();
    m_CreateTime = get_system_time();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KRootFilesystem::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<KRootFSINode> rootNode;
    try {
        Ptr<KFSVolume>    volume   = ptr_new<KFSVolume>(volumeID, devicePath);
        Ptr<KRootFSINode> rootNode = ptr_new<KRootFSINode>(ptr_tmp_cast(this), volume, nullptr, this, true);
        Ptr<KRootFSINode> devRoot  = ptr_new<KRootFSINode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(rootNode), this, true);
        
        volume->m_RootNode = rootNode;
        rootNode->m_Children["dev"] = devRoot;
        
        m_Volume = volume;
        m_DevRoot = devRoot;
        return m_Volume;
    } catch (const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return nullptr;
    }        

    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> KRootFilesystem::FindINode(Ptr<KRootFSINode> parent, ino_t inodeNum, bool remove, Ptr<KRootFSINode>* parentNode)
{
    kassert(m_Mutex.IsLocked());
    if (parent->m_INodeID == inodeNum) {
        return parent;
    }
    for (auto i = parent->m_Children.begin(); i != parent->m_Children.end(); ++i)
    {
        Ptr<KINode> child = i->second;
        if (child->m_INodeID == inodeNum)
        {
            if (parentNode != nullptr) {
                *parentNode = parent;
            }            
            if (remove) parent->m_Children.erase(i);
            return child;
        }
        if (child->IsDirectory())
        {
            Ptr<KINode> node = FindINode(ptr_static_cast<KRootFSINode>(child), inodeNum, remove, parentNode);
            if (node != nullptr) {
                return node;
            }                
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KRootFSINode> KRootFilesystem::LocateParentInode(Ptr<KRootFSINode> parent, const char* path, int pathLength, bool createParents, int* outNameStart)
{
    kassert(m_Mutex.IsLocked());
    
    Ptr<KRootFSINode> current = parent;

    int nameStart = 0;
    for (int i = 0; i <= pathLength; ++i)
    {
        if (i == pathLength)
        {
            *outNameStart = nameStart;
            return current;
        }
        if (path[i] == '/')
        {
            if (i == nameStart) {
                nameStart = i + 1;
                continue;
            }
            String name(path + nameStart, i - nameStart);
            auto nodeIterator = current->m_Children.find(name);
            if (nodeIterator != current->m_Children.end() && nodeIterator->second->IsDirectory())
            {
                current = ptr_static_cast<KRootFSINode>(nodeIterator->second);
            }
            else
            {
                if (createParents)
                {
                    Ptr<KRootFSINode> folder = ptr_new<KRootFSINode>(ptr_tmp_cast(this), m_Volume, ptr_raw_pointer_cast(current), this, true);

                    if (folder == nullptr) return nullptr;
                    current->m_Children[name] = folder;
                    current = folder;
                }
                else
                {
                    return nullptr;
                }
            }
            nameStart = i + 1;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> KRootFilesystem::LocateInode(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<KRootFSINode> current = ptr_static_cast<KRootFSINode>(parent);

    if (current == nullptr || nameLength == 0)
    {
        set_last_error(ENOENT);
        return nullptr;
    }
//    int nameStart = 0;

    if (name[0] == '.')
    {
        if (nameLength == 1) {
            return current;
        } else if (nameLength == 2 && name[1] == '.') {
            return ptr_tmp_cast(current->m_Parent);
        }
    }
    auto nodeIterator = current->m_Children.find(String(name, nameLength));
    if (nodeIterator != current->m_Children.end())
    {
        return nodeIterator->second;
    }
    else
    {
        set_last_error(ENOENT);
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KDirectoryNode> KRootFilesystem::OpenDirectory(Ptr<KFSVolume> volume, Ptr<KINode> node)
{
    try
    {
        Ptr<KRootFSDirectoryNode> dirNode = ptr_new<KRootFSDirectoryNode>(O_RDONLY);
        return dirNode;
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

int KRootFilesystem::CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dir_entry* entry, size_t bufSize)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KRootFSDirectoryNode> dirNode  = ptr_static_cast<KRootFSDirectoryNode>(directory);
    Ptr<KRootFSINode>         dirInode = ptr_static_cast<KRootFSINode>(directory->GetINode());


    if (dirNode->m_CurrentIndex < dirInode->m_Children.size())
    {
        entry->d_volumeid = volume->m_VolumeID;
        entry->d_reclength = sizeof(*entry);

        int index = 0;
        for (auto i : dirInode->m_Children)
        {
            if (index++ == dirNode->m_CurrentIndex)
            {
                const String&   name  = i.first;
                Ptr<KINode>     inode = i.second;

                dirNode->m_CurrentIndex++;

                entry->d_inode = inode->m_INodeID;
                entry->d_type = (inode->IsDirectory()) ? dir_entry_type::DT_DIRECTORY : dir_entry_type::DT_FILE;
                entry->d_namelength = name.size();
                name.copy(entry->d_name, name.size());
                entry->d_name[name.size()] = '\0';
                return 1;
            }
        }
        return 0;
    }
    else
    {
        return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KRootFSDirectoryNode> dirNode = ptr_static_cast<KRootFSDirectoryNode>(directory);
    dirNode->m_CurrentIndex = 0;
    
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KRootFilesystem::CreateFile(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int flags, int permission)
{
    set_last_error(ENOSYS);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> KRootFilesystem::LoadInode(Ptr<KFSVolume> volume, ino_t inode)
{
    CRITICAL_SCOPE(m_Mutex);
    if (inode == m_Volume->m_RootNode->m_INodeID) {
        return m_Volume->m_RootNode;
    } else {
        return FindINode(ptr_static_cast<KRootFSINode>(m_Volume->m_RootNode), inode, false, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::CreateDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parentBase, const char* name, int nameLength, int permission)
{
    try
    {
        CRITICAL_SCOPE(m_Mutex);

        Ptr<KRootFSINode> parent = ptr_static_cast<KRootFSINode>(parentBase);
        Ptr<KRootFSINode> dir    = ptr_new<KRootFSINode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(parent), this, true);

        String nodeName(name, nameLength);
        if (parent->m_Children.find(nodeName) != parent->m_Children.end())
        {
            set_last_error(EEXIST);
            return -1;
        }
        parent->m_Children[nodeName] = dir;
        return 0;
    }
    catch (const std::bad_alloc&)
    {
        set_last_error(ENOMEM);
        return -1;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> inode, struct stat* outStats)
{
    Ptr<KRootFSINode>  node = ptr_static_cast<KRootFSINode>(inode);

    CRITICAL_SCOPE(m_Mutex);

    outStats->st_dev    = dev_t(volume->m_VolumeID);
    outStats->st_ino    = node->m_INodeID;
    outStats->st_mode   = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    if (node->IsDirectory()) {
        outStats->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    } else {
        outStats->st_mode |= S_IFREG;
    }
    if (volume->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
        outStats->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    }
    outStats->st_nlink      = 1;
    outStats->st_uid        = 0;
    outStats->st_gid        = 0;
    outStats->st_size       = 0;
    outStats->st_blksize    = 512;
    outStats->st_atime = outStats->st_mtime = outStats->st_ctime = node->m_CreateTime.AsSecondsI();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::WriteStat(Ptr<KFSVolume> volume, Ptr<KINode> node, const struct stat* stats, uint32_t mask)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::RegisterDevice(const char* path, Ptr<KINode> deviceNode)
{
    CRITICAL_SCOPE(m_Mutex);
    int pathLength = strlen(path);

    int nameStart = 0;
    Ptr<KRootFSINode> parent = LocateParentInode(m_DevRoot, path, pathLength, true, &nameStart);

    int nameLength = pathLength - nameStart;
    if (parent == nullptr || nameLength == 0)
    {
        return -1;
    }
//    Ptr<KRootFSINode> deviceInode = ptr_new<KRootFSINode>(ptr_tmp_cast(this), m_Volume, this, false);
//    deviceInode->m_DeviceNode = device;
    int32_t handle = AllocINodeNumber();
    deviceNode->m_INodeID   = handle;
    deviceNode->m_Filesystem = ptr_tmp_cast(this);
    deviceNode->m_Volume     = m_Volume;
    
    kprintf("Register device %ld at '/dev/%s'\n", handle, path);
    parent->m_Children[path + nameStart] = deviceNode;
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::RenameDevice(int handle, const char* newPath)
{
    CRITICAL_SCOPE(m_Mutex);
    
    Ptr<KRootFSINode> prevParent;
    Ptr<KINode> node = FindINode(m_DevRoot, handle, true, &prevParent);
    if (node == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }

    int pathLength = strlen(newPath);

    int nameStart = 0;
    Ptr<KRootFSINode> newParent = LocateParentInode(m_DevRoot, newPath, pathLength, true, &nameStart);

    int nameLength = pathLength - nameStart;
    if (newParent == nullptr || nameLength == 0)
    {
        return -1;
    }
    try {    
        newParent->m_Children[newPath + nameStart] = node;
        kprintf("Rename device %ld at '/dev/%s'\n", handle, newPath);
    } catch(const std::bad_alloc&) {
        set_last_error(ENOMEM);
        return -1;
    }        
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::RemoveDevice(int handle)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KRootFSINode> prevParent;
    Ptr<KINode> node = FindINode(m_DevRoot, handle, true, &prevParent);
    if (node == nullptr) {
        set_last_error(EINVAL);
        return -1;
    }
    kprintf("Remove device %ld\n", handle);
    // Remove empty folders
    while(prevParent != m_DevRoot && prevParent->m_Children.empty())
    {
        FindINode(m_DevRoot, prevParent->m_INodeID, true, &prevParent);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::AllocINodeNumber()
{
    static std::atomic_int32_t nextID = 1;
    return nextID++;
}
