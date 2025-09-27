// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 18/05/19 18:04:07


#pragma once

#include "Kernel/VFS/KFilesystem.h"

namespace os
{
class String;
}

namespace kernel
{

class FATVolume;
class FATINode;

//#define FAT_VERIFY_FAT_CHAINS


struct FATNewDirEntryInfo
{
    uint32_t    cluster;
    size_t      size;
    time_t      time;
    uint8_t     m_DOSAttribs;
};


class FATFilesystem : public KFilesystem, public KFilesystemFileOps
{
public:

    DEFINE_KERNEL_LOG_CATEGORY(LOGC_FS);
    DEFINE_KERNEL_LOG_CATEGORY(LOGC_FATTABLE);
    DEFINE_KERNEL_LOG_CATEGORY(LOGC_DIR);
    DEFINE_KERNEL_LOG_CATEGORY(LOGC_FILE);
public:
    FATFilesystem();
    
    virtual int                 Probe(const char* devicePath, fs_info* fsInfo) override;
    virtual Ptr<KFSVolume>      Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;
    virtual int                 Unmount(Ptr<KFSVolume> volume) override;

    virtual int                 Sync(Ptr<KFSVolume> volume) override;

    virtual int                 ReadFSStat(Ptr<KFSVolume> volume, fs_info* fsinfo) override;
    virtual int                 WriteFSStat(Ptr<KFSVolume> volume, const fs_info* fsinfo, uint32_t mask) override;
    
    virtual Ptr<KINode>         LocateInode(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength) override;
    virtual bool                ReleaseInode(KINode* inode) override;
    virtual Ptr<KFileNode>      OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int openFlags) override;
    virtual Ptr<KFileNode>      CreateFile(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int openFlags, int permission) override;
    virtual int                 CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;

    virtual Ptr<KINode>         LoadInode(Ptr<KFSVolume> volume, ino_t inode) override;

    virtual Ptr<KDirectoryNode> OpenDirectory(Ptr<KFSVolume> volume, Ptr<KINode> node) override;
    virtual int                 CreateDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int permission) override;
    virtual int                 CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory) override;

    virtual int                 Rename(Ptr<KFSVolume> volume, Ptr<KINode> oldParent, const char* oldName, int oldNameLen, Ptr<KINode> newParent, const char* newName, int newNameLen, bool mustBeDir) override;
    virtual int                 Unlink(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength) override;
    virtual int                 RemoveDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength) override;
    
    virtual ssize_t             Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    virtual ssize_t             Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    virtual int                 ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize) override;
    virtual int                 RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> dirNode) override;
    virtual int                 ReadLink(Ptr<KFSVolume> volume, Ptr<KINode> node, char* buffer, size_t bufferSize) override;

    virtual int                 CheckAccess(Ptr<KFSVolume> volume, Ptr<KINode> node, int mode) override;

    virtual int                 ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* stat) override;
    virtual int                 WriteStat(Ptr<KFSVolume> volume, Ptr<KINode> node, const struct stat* stat, uint32_t mask) override;

    virtual int                 DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    status_t CreateVolumeLabel(Ptr<FATVolume> vol, const char* name, uint32_t* index);
    status_t FindShortName(Ptr<FATVolume> vol, Ptr<FATINode> parent, const char* rawShortName);
    status_t DoLocateINode(Ptr<FATVolume> vol, Ptr<FATINode> dir, const os::String& fileName, Ptr<FATINode>* node);
    status_t IsDirectoryEmpty(Ptr<FATVolume> volume, Ptr<FATINode> dir);
    status_t CreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> parent, Ptr<FATINode> node, const os::String& name, uint32_t* startIndex, uint32_t* endIndex);
    status_t DoCreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> dir, FATNewDirEntryInfo* info, const char shortName[11], const wchar16_t* longName, uint32_t len, uint32_t* startIndex, uint32_t* endIndex);
    status_t CompactDirectory(Ptr<FATVolume> vol, Ptr<FATINode> dir);
    status_t EraseDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> node);
    int      DoUnlink(Ptr<KFSVolume> volume, Ptr<KINode> parent, const os::String& name, bool removeFile);

};

} // namespace
