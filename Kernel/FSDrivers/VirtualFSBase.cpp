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
#include <algorithm>
#include <atomic>
#include <iterator>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/FSDrivers/VirtualFSBase.h>
#include <System/System.h>
#include <System/ExceptionHandling.h>
#include <Storage/DirectoryEntry.h>
#include <Utils/String.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KVirtualFSVolume::KVirtualFSVolume(fs_id volumeID, const PString& devicePath)
    : KFSVolume(volumeID, devicePath)
    , m_Mutex("virtual_fs_mutex", PEMutexRecursionMode_RaiseError)
{

}


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

KVirtualFilesystemBase::KVirtualFilesystemBase() // : m_Mutex("virtual_fs_mutex", PEMutexRecursionMode_RaiseError)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KVirtualFilesystemBase::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    Ptr<KVirtualFSVolume>    volume = ptr_new<KVirtualFSVolume>(volumeID, devicePath);
    DoMount(volume, flags, args, argLength);
    return volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::DoMount(Ptr<KVirtualFSVolume> volume, uint32_t flags, const char* args, size_t argLength)
{
    Ptr<KVirtualFSBaseInode> rootNode = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, nullptr, this, S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);

    volume->m_RootNode = rootNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVirtualFilesystemBase::FindInode(Ptr<KFSVolume> volume, Ptr<KVirtualFSBaseInode> parent, ino_t inodeNum, bool remove, Ptr<KVirtualFSBaseInode>* parentNode)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);
    kassert(fsVolume->m_Mutex.IsLocked());
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
            try
            {
                return FindInode(volume, ptr_static_cast<KVirtualFSBaseInode>(child), inodeNum, remove, parentNode);
            }
            catch (const std::system_error& error)
            {
                if (PErrorCode(error.code().value()) != PErrorCode::IO) {
                    throw;
                }
            }
        }
    }
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KVirtualFSBaseInode> KVirtualFilesystemBase::LocateParentInode(Ptr<KFSVolume> volume, Ptr<KVirtualFSBaseInode> parent, const char* path, int pathLength, bool createParents, int* outNameStart)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    kassert(fsVolume->m_Mutex.IsLocked());
    
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
                    Ptr<KVirtualFSBaseInode> folder = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(current), this, S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);

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
    PERROR_THROW_CODE(PErrorCode::NOENT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVirtualFilesystemBase::LocateInode(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);
    kassert(!fsVolume->m_Mutex.IsLocked());
    CRITICAL_SCOPE(fsVolume->m_Mutex);
    Ptr<KInode> inode = LocateInodeInternal(volume, parent, name, nameLength);
    if (inode == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVirtualFilesystemBase::LocateInodeInternal(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength) noexcept
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    kassert(fsVolume->m_Mutex.IsLocked());

    Ptr<KVirtualFSBaseInode> current = ptr_static_cast<KVirtualFSBaseInode>(parent);

    if (current == nullptr || nameLength == 0) {
        return nullptr;
    }

    if (name[0] == '.')
    {
        if (nameLength == 1)
        {
            return current;
        }
        else if (nameLength == 2 && name[1] == '.')
        {
            return ptr_tmp_cast(current->m_Parent);
        }
    }
    auto nodeIterator = current->m_Children.find(PString(name, nameLength));
    if (nodeIterator != current->m_Children.end()) {
        return nodeIterator->second;
    } else {
        return nullptr;
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

size_t KVirtualFilesystemBase::ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, void* buffer, size_t bufferSize)
{
    PDirEntryWriter entryWriter(buffer, bufferSize);
    if (!entryWriter.IsValid() || bufferSize < PGetDirEntryRecordSize(0)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    Ptr<KVirtualFSBaseDirectoryNode> dirNode  = ptr_static_cast<KVirtualFSBaseDirectoryNode>(directory);
    Ptr<KVirtualFSBaseInode>         dirInode = ptr_static_cast<KVirtualFSBaseInode>(directory->GetInode());

    const bool haveParent = dirInode != volume->m_RootNode || volume->m_MountPoint != nullptr;

    const size_t syntheticEntryCount = haveParent ? 2 : 1;
    const size_t childIndex = (dirNode->m_CurrentIndex >= syntheticEntryCount)
        ? dirNode->m_CurrentIndex - syntheticEntryCount
        : 0;
    auto childIterator = dirInode->m_Children.begin();
    std::advance(childIterator, std::min(childIndex, dirInode->m_Children.size()));

    for (;;)
    {
        if (entryWriter.GetRemainingSize() < PGetDirEntryRecordSize(0)) {
            break;
        }

        const char* name = nullptr;
        size_t nameLength = 0;
        Ptr<const KInode> inode;
        bool isChildEntry = false;

        if (dirNode->m_CurrentIndex == 0)
        {
            name = ".";
            nameLength = 1;
            inode = dirInode;
        }
        else if (haveParent && dirNode->m_CurrentIndex == 1)
        {
            name = "..";
            nameLength = 2;
            if (dirInode != volume->m_RootNode) {
                inode = ptr_tmp_cast(dirInode->m_Parent);
            } else if (volume->m_MountPoint != nullptr) {
                inode = volume->m_MountPoint;
            }
        }
        else
        {
            if (childIterator == dirInode->m_Children.end()) {
                break;
            }
            name = childIterator->first.c_str();
            nameLength = childIterator->first.size();
            inode = childIterator->second;
            isChildEntry = true;
        }

        dirent_t* entry = entryWriter.AddEntry(name, nameLength);
        if (entry == nullptr)
        {
            if (entryWriter.GetBytesWritten() == 0) {
                PERROR_THROW_CODE(PErrorCode::NAMETOOLONG);
            }
            break;
        }

        entry->d_volumeid = inode->m_Volume->m_VolumeID;
        entry->d_ino = inode->m_InodeID;

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
        if (isChildEntry) {
            ++childIterator;
        }
    }
    return entryWriter.GetBytesWritten();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    Ptr<KVirtualFSBaseDirectoryNode> dirNode = ptr_static_cast<KVirtualFSBaseDirectoryNode>(directory);
    dirNode->m_CurrentIndex = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KVirtualFilesystemBase::CreateFile(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int flags, int permission)
{
    PERROR_THROW_CODE(PErrorCode::NOSYS);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::CreateSymlink(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, const char* targetPath)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    Ptr<KVirtualFSBaseInode> fsParent = ptr_static_cast<KVirtualFSBaseInode>(parent);

    PString nodeName(name, nameLength);
    if (fsParent->m_Children.find(nodeName) != fsParent->m_Children.end())
    {
        PERROR_THROW_CODE(PErrorCode::EXIST);
    }

    Ptr<KVirtualFSBaseInode> linkInode = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(fsParent), this, S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO);

    linkInode->m_FileData.insert(linkInode->m_FileData.begin(), targetPath, targetPath + strlen(targetPath));

    fsParent->m_Children[nodeName] = linkInode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KVirtualFilesystemBase::LoadInode(Ptr<KFSVolume> volume, ino_t inode)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);
    
    CRITICAL_SCOPE(fsVolume->m_Mutex);

    if (inode == fsVolume->m_RootNode->m_InodeID) {
        return fsVolume->m_RootNode;
    } else {
        return FindInode(volume, ptr_static_cast<KVirtualFSBaseInode>(fsVolume->m_RootNode), inode, false, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::CreateDirectory(Ptr<KFSVolume> volume, Ptr<KInode> parentBase, const char* name, int nameLength, int permission)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    Ptr<KVirtualFSBaseInode> parent = ptr_static_cast<KVirtualFSBaseInode>(parentBase);
    Ptr<KVirtualFSBaseInode> dir    = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(parent), this, S_IFDIR | (permission & ~S_IFMT));

    PString nodeName(name, nameLength);
    if (parent->m_Children.find(nodeName) != parent->m_Children.end())
    {
        PERROR_THROW_CODE(PErrorCode::EXIST);
    }
    parent->m_Children[nodeName] = dir;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::Unlink(Ptr<KFSVolume> volume, Ptr<KInode> parentBase, const char* name, int nameLength)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    Ptr<KVirtualFSBaseInode> parent = ptr_static_cast<KVirtualFSBaseInode>(parentBase);
    PString nodeName(name, nameLength);
    auto nodeIterator = parent->m_Children.find(nodeName);

    if (nodeIterator == parent->m_Children.end()) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    if (nodeIterator->second->IsDirectory()) {
        PERROR_THROW_CODE(PErrorCode::ISDIR);
    }
    parent->m_Children.erase(nodeIterator);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::RemoveDirectory(Ptr<KFSVolume> volume, Ptr<KInode> parentBase, const char* name, int nameLength)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    Ptr<KVirtualFSBaseInode> parent = ptr_static_cast<KVirtualFSBaseInode>(parentBase);
    PString nodeName(name, nameLength);
    auto nodeIterator = parent->m_Children.find(nodeName);

    if (nodeIterator == parent->m_Children.end()) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    if (!nodeIterator->second->IsDirectory()) {
        PERROR_THROW_CODE(PErrorCode::NOTDIR);
    }

    Ptr<KVirtualFSBaseInode> directory = ptr_static_cast<KVirtualFSBaseInode>(nodeIterator->second);
    if (!directory->m_Children.empty()) {
        PERROR_THROW_CODE(PErrorCode::NOTEMPTY);
    }
    parent->m_Children.erase(nodeIterator);
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

size_t KVirtualFilesystemBase::ReadLink(Ptr<KFSVolume> volume, Ptr<KInode> inode, char* buffer, size_t bufferSize)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    if (!S_ISLNK(inode->m_FileMode)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    Ptr<KVirtualFSBaseInode> fsInode = ptr_static_cast<KVirtualFSBaseInode>(inode);

    const size_t length = std::min(fsInode->m_FileData.size(), bufferSize);

    memcpy(buffer, fsInode->m_FileData.data(), length);

    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVirtualFilesystemBase::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    Ptr<KVirtualFSVolume> fsVolume = ptr_static_cast<KVirtualFSVolume>(volume);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

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
    static std::atomic_int32_t nextID = 1000000;
    return nextID++;
}

} // namespace kernel
