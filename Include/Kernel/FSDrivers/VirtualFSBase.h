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

#pragma once

#include <map>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Utils/String.h>
#include <Kernel/KMutex.h>
#include <Kernel/VFS/KFileHandle.h>

namespace kernel
{


class KVirtualFSBaseInode : public KInode
{
public:
    KVirtualFSBaseInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KVirtualFSBaseInode* parent, KFilesystemFileOps* fileOps, mode_t fileMode);

    KVirtualFSBaseInode* m_Parent = nullptr;
	std::map<PString, Ptr<KInode>>  m_Children;
    std::vector<uint8_t>            m_FileData;
};

struct KVirtualFSBaseDirectoryNode : public KDirectoryNode
{
	inline KVirtualFSBaseDirectoryNode(int openFlags) : KDirectoryNode(openFlags) {}

	size_t    m_CurrentIndex = 0;
};

class KVirtualFilesystemBase : public KFilesystem, public KFilesystemFileOps
{
public:
	KVirtualFilesystemBase();
	virtual Ptr<KFSVolume>      Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;
	virtual Ptr<KInode>         LocateInode(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength) override;

	virtual Ptr<KDirectoryNode> OpenDirectory(Ptr<KFSVolume> volume, Ptr<KInode> node) override;
	virtual void                CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory) override;

	virtual size_t              ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize) override;
	virtual void                RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> dirNode) override;

	virtual Ptr<KFileNode>      CreateFile(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int flags, int permission) override;

	virtual Ptr<KInode>         LoadInode(Ptr<KFSVolume> volume, ino_t inode) override;
	virtual void                CreateDirectory(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int permission) override;

    virtual size_t              Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;

	virtual void                ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;
	virtual void                WriteStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, const struct stat* stats, uint32_t mask) override;

	static int AllocInodeNumber();
protected:
	Ptr<KInode>       FindInode(Ptr<KVirtualFSBaseInode> parent, ino_t inodeNum, bool remove, Ptr<KVirtualFSBaseInode>* parentNode);
	Ptr<KVirtualFSBaseInode> LocateParentInode(Ptr<KVirtualFSBaseInode> parent, const char* path, int pathLength, bool createParents, int* nameStart);

	KMutex            m_Mutex;
	Ptr<KFSVolume>    m_Volume;
};

} // namespace kernel
