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
// Created: 23.02.2018 01:53:33

#pragma once


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <errno.h>
#include <stddef.h>

#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "Kernel/Kernel.h"
#include "System/System.h"


typedef struct iovec iovec_t;

namespace kernel
{


class KFSVolume;
class KFileNode;
class KDirectoryNode;
class KInode;

#define MOUNT_READ_ONLY 0x0001

  // Flags returned in the fi_flags field of fs_info.
enum class FSVolumeFlags : uint32_t
{
    FS_IS_READONLY   = 0x00000001, // Set if mounted read-only or resides on a RO media.
    FS_IS_REMOVABLE  = 0x00000002, // Set if mounted on a removable media.
    FS_IS_PERSISTENT = 0x00000004, // Set if data written to the FS is preserved across reboots.
    FS_IS_SHARED     = 0x00000008, // Set if the FS is shared across multiple machines (Network FS).
    FS_IS_BLOCKBASED = 0x00000010, // Set if the FS use a regular block-device (or loopback from a single file) to store its data.
    FS_CAN_MOUNT     = 0x00000020  // Set by probe() if the FS can mount the given device.
};

typedef struct
{
    dev_t    fi_dev;                /* Device ID */
    ino_t    fi_root;               /* Root inode number */
    uint32_t fi_flags;              /* Filesystem flags (defined above) */
    int      fi_block_size;         /* Fundamental block size */
    int      fi_io_size;            /* Optimal IO size */
    off_t    fi_total_blocks;       /* Total number of blocks in FS */
    off_t    fi_free_blocks;        /* Number of free blocks in FS */
    off_t    fi_free_user_blocks;   /* Number of blocks available for non-root users */
    off_t    fi_total_inodes;       /* Total number of inodes (-1 if inodes are allocated dynamically) */
    off_t    fi_free_inodes;        /* Number of free inodes (-1 if inodes are allocated dynamically) */
    char     fi_device_path[1024];  /* Device backing the FS (might not be a real
                                     * device unless the FS_IS_BLOCKBASED is set) */
    char     fi_mount_args[1024];   /* Arguments given to FS when mounted */
    char     fi_volume_name[256];   /* Volume name */
    char     fi_driver_name[OS_NAME_LENGTH]; /* Name of filesystem driver */
} fs_info;

class KFilesystemFileOps
{
public:
    virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> node, int openFlags);
    virtual void           CloseFile(Ptr<KFSVolume> volume, KFileNode* file);

    virtual Ptr<KDirectoryNode> OpenDirectory(Ptr<KFSVolume> volume, Ptr<KInode> node);
    virtual void                CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory);

protected:
    virtual size_t  Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position);
    virtual size_t  Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position);
public:
    virtual size_t  Read(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position);
    virtual size_t  Write(Ptr<KFileNode> file, const iovec_t* segments, size_t segmentCount, off64_t position);

    virtual size_t  ReadLink(Ptr<KFSVolume> volume, Ptr<KInode> inode, char* buffer, size_t bufferSize);
    virtual void    DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

    virtual size_t  ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize);
    virtual void    RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> dirNode);

    virtual void    CheckAccess(Ptr<KFSVolume> volume, Ptr<KInode> node, int mode);

    virtual void    ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) = 0;
    virtual void    WriteStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, const struct stat* value, uint32_t mask);

    virtual void    Sync(Ptr<KFileNode> file);

};

class KFilesystem : public PtrTarget
{
public:
    virtual PErrorCode      Probe(const char* devicePath, fs_info* fsInfo);
    virtual Ptr<KFSVolume>  Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength);
    virtual void            Unmount(Ptr<KFSVolume> volume);
    
    virtual void            Sync(Ptr<KFSVolume> volume);
    
    virtual void            ReadFSStat(Ptr<KFSVolume> volume, fs_info* fsinfo);
    virtual void            WriteFSStat(Ptr<KFSVolume> volume, const fs_info* fsinfo, uint32_t mask);
    virtual Ptr<KInode>     LocateInode(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* path, int pathLength);
    virtual void            ReleaseInode(KInode* inode);
    virtual Ptr<KFileNode>  CreateFile(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int openFlags, int permission);
    virtual void            CreateSymlink(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, const char* targetPath);

    virtual Ptr<KInode>     LoadInode(Ptr<KFSVolume> volume, ino_t inode);

    virtual void            CreateDirectory(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int permission);

    virtual void            Rename(Ptr<KFSVolume> volume, Ptr<KInode> oldParent, const char* oldName, int oldNameLen, Ptr<KInode> newParent, const char* newName, int newNameLen, bool mustBeDir);
    virtual void            Unlink(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength);
    virtual void            RemoveDirectory(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength);
};

} // namespace
