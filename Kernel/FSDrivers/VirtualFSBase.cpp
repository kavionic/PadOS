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
#include <Kernel/FSDrivers/VirtualFSBase.h>
#include <System/System.h>
#include <System/ExceptionHandling.h>
#include <Utils/String.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KVirtualFSBaseInode::KVirtualFSBaseInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KVirtualFSBaseInode* parent, KFilesystemFileOps* fileOps, mode_t fileMode)
    : KInode(filesystem, volume, fileOps, fileMode)
    , m_Parent(parent)
{
    m_InodeID = KVirtualFilesystemBase::AllocInodeNumber();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KVirtualFilesystemBase::KVirtualFilesystemBase() : m_Mutex("virtual_fs_mutex", PEMutexRecursionMode_RaiseError)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KVirtualFilesystemBase::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KFSVolume>   volume   = ptr_new<KFSVolume>(volumeID, devicePath);
    Ptr<KVirtualFSBaseInode> rootNode = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, nullptr, this, S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);

    volume->m_RootNode = rootNode;
        
    m_Volume = volume;

    return m_Volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVirtualFilesystemBase::FindInode(Ptr<KVirtualFSBaseInode> parent, ino_t inodeNum, bool remove, Ptr<KVirtualFSBaseInode>* parentNode)
{
    kassert(m_Mutex.IsLocked());
    if (parent->m_InodeID == inodeNum) {
        return parent;
    }
    for (auto i = parent->m_Children.begin(); i != parent->m_Children.end(); ++i)
    {
        Ptr<KInode> child = i->second;
        if (child->m_InodeID == inodeNum)
        {
            if (parentNode != nullptr) {
                *parentNode = parent;
            }            
            if (remove) parent->m_Children.erase(i);
            return child;
        }
        if (child->IsDirectory())
        {
            return FindInode(ptr_static_cast<KVirtualFSBaseInode>(child), inodeNum, remove, parentNode);
        }
    }
    PERROR_THROW_CODE(PErrorCode::IOError);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KVirtualFSBaseInode> KVirtualFilesystemBase::LocateParentInode(Ptr<KVirtualFSBaseInode> parent, const char* path, int pathLength, bool createParents, int* outNameStart)
{
    kassert(m_Mutex.IsLocked());
    
    Ptr<KVirtualFSBaseInode> current = parent;

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
            PString name(path + nameStart, i - nameStart);
            auto nodeIterator = current->m_Children.find(name);
            if (nodeIterator != current->m_Children.end() && nodeIterator->second->IsDirectory())
            {
                current = ptr_static_cast<KVirtualFSBaseInode>(nodeIterator->second);
            }
            else
            {
                if (createParents)
                {
                    Ptr<KVirtualFSBaseInode> folder = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), m_Volume, ptr_raw_pointer_cast(current), this, S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);

                    current->m_Children[name] = folder;
                    current = folder;
                }
                else
                {
                    break;
                }
            }
            nameStart = i + 1;
        }
    }
    PERROR_THROW_CODE(PErrorCode::NoEntry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVirtualFilesystemBase::LocateInode(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength)
{
    CRITICAL_SCOPE(m_Mutex);
    Ptr<KVirtualFSBaseInode> current = ptr_static_cast<KVirtualFSBaseInode>(parent);

    if (current == nullptr || nameLength == 0) {
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }

    if (name[0] == '.')
    {
        if (nameLength == 1)
        {
            return current;
        }
        else if (nameLength == 2 && name[1] == '.')
        {
            if (current->m_Parent == nullptr) {
                PERROR_THROW_CODE(PErrorCode::NoEntry);
            }
            return ptr_tmp_cast(current->m_Parent);
        }
    }
    auto nodeIterator = current->m_Children.find(PString(name, nameLength));
    if (nodeIterator != current->m_Children.end()) {
        return nodeIterator->second;
    } else {
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KDirectoryNode> KVirtualFilesystemBase::OpenDirectory(Ptr<KFSVolume> volume, Ptr<KInode> node)
{
    Ptr<KVirtualFSBaseDirectoryNode> dirNode = ptr_new<KVirtualFSBaseDirectoryNode>(O_RDONLY);
    return dirNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KVirtualFilesystemBase::ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KVirtualFSBaseDirectoryNode> dirNode  = ptr_static_cast<KVirtualFSBaseDirectoryNode>(directory);
    Ptr<KVirtualFSBaseInode>         dirInode = ptr_static_cast<KVirtualFSBaseInode>(directory->GetInode());

    const bool haveParent = dirInode != volume->m_RootNode || volume->m_MountPoint != nullptr;

    Ptr<const KInode> inode;

    if (dirNode->m_CurrentIndex == 0)
    {
        entry->d_name[0] = '.';
        entry->d_name[1] = '\0';
        entry->d_namlen = 1;

        inode = dirInode;
    }
    else if (haveParent && dirNode->m_CurrentIndex == 1)
    {
        entry->d_name[0] = '.';
        entry->d_name[1] = '.';
        entry->d_name[2] = '\0';
        entry->d_namlen = 2;

        if (dirInode != volume->m_RootNode) {
            inode = ptr_tmp_cast(dirInode->m_Parent);
        } else if (volume->m_MountPoint != nullptr) {
            inode = volume->m_MountPoint;
        }
    }
    else
    {
        const size_t indexInDir = dirNode->m_CurrentIndex - (haveParent ? 2 : 1);
        if (indexInDir < dirInode->m_Children.size())
        {
            int index = 0;
            for (auto i : dirInode->m_Children)
            {
                if (index++ == indexInDir)
                {
                    const PString& name = i.first;
                    entry->d_namlen = static_cast<decltype(entry->d_namlen)>(name.size());
                    name.copy(entry->d_name, name.size());
                    entry->d_name[name.size()] = '\0';
                    inode = i.second;
                    break;
                }
            }
        }
    }
    if (inode != nullptr)
    {
        entry->d_volumeid = inode->m_Volume->m_VolumeID;
        entry->d_reclen   = sizeof(*entry);

        entry->d_ino    = inode->m_InodeID;

        if (S_ISBLK(inode->m_FileMode)) {
            entry->d_type = DT_BLK;
        } else if (S_ISCHR(inode->m_FileMode)) {
            entry->d_type = DT_CHR;
        } else if (S_ISDIR(inode->m_FileMode)) {
            entry->d_type = DT_DIR;
        } else if (S_ISFIFO(inode->m_FileMode)) {
            entry->d_type = DT_FIFO;
        } else if (S_ISREG(inode->m_FileMode)) {
            entry->d_type = DT_REG;
        } else if (S_ISLNK(inode->m_FileMode)) {
            entry->d_type = DT_LNK;
        } else if (S_ISSOCK(inode->m_FileMode)) {
            entry->d_type = DT_SOCK;
        } else {
            entry->d_type = DT_UNKNOWN;
        }

        dirNode->m_CurrentIndex++;

        return sizeof(dirent_t);
    }
    else
    {
        return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KVirtualFSBaseDirectoryNode> dirNode = ptr_static_cast<KVirtualFSBaseDirectoryNode>(directory);
    dirNode->m_CurrentIndex = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KVirtualFilesystemBase::CreateFile(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int flags, int permission)
{
    PERROR_THROW_CODE(PErrorCode::NotImplemented);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVirtualFilesystemBase::LoadInode(Ptr<KFSVolume> volume, ino_t inode)
{
    CRITICAL_SCOPE(m_Mutex);
    if (inode == m_Volume->m_RootNode->m_InodeID) {
        return m_Volume->m_RootNode;
    } else {
        return FindInode(ptr_static_cast<KVirtualFSBaseInode>(m_Volume->m_RootNode), inode, false, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::CreateDirectory(Ptr<KFSVolume> volume, Ptr<KInode> parentBase, const char* name, int nameLength, int permission)
{
    CRITICAL_SCOPE(m_Mutex);

    Ptr<KVirtualFSBaseInode> parent = ptr_static_cast<KVirtualFSBaseInode>(parentBase);
    Ptr<KVirtualFSBaseInode> dir    = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(parent), this, S_IFDIR | (permission & ~S_IFMT));

    PString nodeName(name, nameLength);
    if (parent->m_Children.find(nodeName) != parent->m_Children.end())
    {
        PERROR_THROW_CODE(PErrorCode::Exist);
    }
    parent->m_Children[nodeName] = dir;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KVirtualFilesystemBase::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position)
{
    Ptr<KVirtualFSBaseInode> inode = ptr_static_cast<KVirtualFSBaseInode>(file->GetInode());

    const size_t bytesToRead = std::max(0, ssize_t(inode->m_FileData.size()) - ssize_t(position));
    memcpy(buffer, inode->m_FileData.data() + position, bytesToRead);
    
    return bytesToRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    CRITICAL_SCOPE(m_Mutex);

    KFilesystemFileOps::ReadStat(volume, inode, statBuf);

    if (!inode->IsDirectory())
    {
        Ptr<KVirtualFSBaseInode> fsInode = ptr_static_cast<KVirtualFSBaseInode>(inode);
        statBuf->st_size = fsInode->m_FileData.size();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::WriteStat(Ptr<KFSVolume> volume, Ptr<KInode> node, const struct stat* stats, uint32_t mask)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KVirtualFilesystemBase::AllocInodeNumber()
{
    static std::atomic_int32_t nextID = 1;
    return nextID++;
}

} // namespace kernel
