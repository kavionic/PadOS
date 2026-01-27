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

#include <Utils/Logging.h>
#include <Kernel/VFS/KFilesystem.h>

class PString;

namespace kernel
{

class FATVolume;
class FATINode;

//#define FAT_VERIFY_FAT_CHAINS


PDEFINE_LOG_CATEGORY(LogCat_FATFS, "FATFS", PLogSeverity::WARNING);
PDEFINE_LOG_CATEGORY(LogCat_FATTABLE, "FATTBL", PLogSeverity::WARNING);
PDEFINE_LOG_CATEGORY(LogCat_FATDIR, "FATDIR", PLogSeverity::WARNING);
PDEFINE_LOG_CATEGORY(LogCat_FATFILE, "FATFIL", PLogSeverity::WARNING);

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
    FATFilesystem();
    
    virtual PErrorCode          Probe(const char* devicePath, fs_info* fsInfo) override;
    virtual Ptr<KFSVolume>      Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;
    virtual void                Unmount(Ptr<KFSVolume> volume) override;

    virtual void                Sync(Ptr<KFSVolume> volume) override;

    virtual void                ReadFSStat(Ptr<KFSVolume> volume, fs_info* fsinfo) override;
    virtual void                WriteFSStat(Ptr<KFSVolume> volume, const fs_info* fsinfo, uint32_t mask) override;
    
    virtual Ptr<KINode>         LocateInode(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength) override;
    virtual void                ReleaseInode(KINode* inode) override;
    virtual Ptr<KFileNode>      OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int openFlags) override;
    virtual Ptr<KFileNode>      CreateFile(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int openFlags, int permission) override;
    virtual void                CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;

    virtual Ptr<KINode>         LoadInode(Ptr<KFSVolume> volume, ino_t inode) override;

    virtual Ptr<KDirectoryNode> OpenDirectory(Ptr<KFSVolume> volume, Ptr<KINode> node) override;
    virtual void                CreateDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int permission) override;
    virtual void                CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory) override;

    virtual void                Rename(Ptr<KFSVolume> volume, Ptr<KINode> oldParent, const char* oldName, int oldNameLen, Ptr<KINode> newParent, const char* newName, int newNameLen, bool mustBeDir) override;
    virtual void                Unlink(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength) override;
    virtual void                RemoveDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength) override;
    
    virtual size_t              Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t              Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual size_t              ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize) override;
    virtual void                RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> dirNode) override;
    virtual size_t              ReadLink(Ptr<KFSVolume> volume, Ptr<KINode> node, char* buffer, size_t bufferSize) override;

    virtual void                CheckAccess(Ptr<KFSVolume> volume, Ptr<KINode> node, int mode) override;

    virtual void                ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* stat) override;
    virtual void                WriteStat(Ptr<KFSVolume> volume, Ptr<KINode> node, const struct stat* stat, uint32_t mask) override;

    virtual void                DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    uint32_t CreateVolumeLabel(Ptr<FATVolume> vol, const char* name);
    bool FindShortName(Ptr<FATVolume> vol, Ptr<FATINode> parent, const char* rawShortName);
    Ptr<FATINode> DoLocateINode(Ptr<FATVolume> vol, Ptr<FATINode> dir, const PString& fileName);
    bool IsDirectoryEmpty(Ptr<FATVolume> volume, Ptr<FATINode> dir);
    void CreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> parent, Ptr<FATINode> node, const PString& name, uint32_t* startIndex, uint32_t* endIndex);
    void DoCreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> dir, FATNewDirEntryInfo* info, const char shortName[11], const wchar16_t* longName, uint32_t len, uint32_t* startIndex, uint32_t* endIndex);
    void CompactDirectory(Ptr<FATVolume> vol, Ptr<FATINode> dir);
    void EraseDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> node);
    void DoUnlink(Ptr<KFSVolume> volume, Ptr<KINode> parent, const PString& name, bool removeFile);

};

} // namespace
