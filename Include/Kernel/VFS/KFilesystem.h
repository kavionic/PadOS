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
#include <errno.h>
#include <stddef.h>

#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "Kernel/Kernel.h"
#include "System/System.h"

namespace os
{
struct IOSegment;

static constexpr uint32_t WSTAT_MODE    = 0x0001;
static constexpr uint32_t WSTAT_UID     = 0x0002;
static constexpr uint32_t WSTAT_GID     = 0x0004;
static constexpr uint32_t WSTAT_SIZE    = 0x0008;
static constexpr uint32_t WSTAT_ATIME   = 0x0010;
static constexpr uint32_t WSTAT_MTIME   = 0x0020;
static constexpr uint32_t WSTAT_CTIME   = 0x0040;
static constexpr uint32_t WSTAT_MASK    = 0x007f;

static constexpr uint32_t WFSSTAT_NAME = 0x0001;

static constexpr uint32_t  FSINFO_VERSION = 1;
}

namespace kernel
{


class KFSVolume;
class KFileNode;
class KDirectoryNode;
class KINode;

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
    dev_t    fi_dev;			/* Device ID */
    ino_t    fi_root;			/* Root inode number */
    uint32_t fi_flags;			/* Filesystem flags (defined above) */
    int	     fi_block_size;		/* Fundamental block size */
    int	     fi_io_size;			/* Optimal IO size */
    off_t    fi_total_blocks;		/* Total number of blocks in FS */
    off_t    fi_free_blocks;		/* Number of free blocks in FS */
    off_t    fi_free_user_blocks;		/* Number of blocks available for non-root users */
    off_t    fi_total_inodes;		/* Total number of inodes (-1 if inodes are allocated dynamically) */
    off_t    fi_free_inodes;		/* Number of free inodes (-1 if inodes are allocated dynamically) */
    char     fi_device_path[1024];	/* Device backing the FS (might not be a real
					 * device unless the FS_IS_BLOCKBASED is set) */
    char     fi_mount_args[1024];		/* Arguments given to FS when mounted */
    char     fi_volume_name[256];		/* Volume name */
    char     fi_driver_name[OS_NAME_LENGTH];	/* Name of filesystem driver */
} fs_info;

enum class dir_entry_type : uint32_t
{
    DT_UNKNOWN,
    DT_DIRECTORY,
    DT_FILE,
    DT_BLOCK_DEV,
    DT_CHARACTER_DEV,
    DT_FIFO,
    DT_SYMLINK,
    DT_SOCKET
};

#define NAME_MAX 255

struct dir_entry
{
    ino_t          d_inode;
    dir_entry_type d_type;
    fs_id          d_volumeid;
    size_t         d_reclength;
    size_t         d_namelength;
    char           d_name[NAME_MAX + 1];
};

class KFilesystemFileOps
{
public:
    IFLASHC virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> node, int openFlags);
    IFLASHC virtual int            CloseFile(Ptr<KFSVolume> volume, KFileNode* file);

    IFLASHC virtual Ptr<KDirectoryNode> OpenDirectory(Ptr<KFSVolume> volume, Ptr<KINode> node);
    IFLASHC virtual int                 CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory);

protected:
    IFLASHC virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length);
    IFLASHC virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length);
public:
    IFLASHC virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, const os::IOSegment* segments, size_t segmentCount);
    IFLASHC virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const os::IOSegment* segments, size_t segmentCount);

    IFLASHC virtual int     ReadLink(Ptr<KFSVolume> volume, Ptr<KINode> node, char* buffer, size_t bufferSize);
    IFLASHC virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

    IFLASHC virtual int     ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dir_entry* entry, size_t bufSize);
    IFLASHC virtual int     RewindDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> dirNode);

    IFLASHC virtual int     CheckAccess(Ptr<KFSVolume> volume, Ptr<KINode> node, int mode);

    IFLASHC virtual int     ReadStat(Ptr<KFSVolume> volume, Ptr<KINode> node, struct stat* result);
    IFLASHC virtual int     WriteStat(Ptr<KFSVolume> volume, Ptr<KINode> node, const struct stat* value, uint32_t mask);
    
};

class KFilesystem : public PtrTarget
{
public:
    IFLASHC virtual int             Probe(const char* devicePath, fs_info* fsInfo);
    IFLASHC virtual Ptr<KFSVolume>  Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength);
    IFLASHC virtual int             Unmount(Ptr<KFSVolume> volume);
    
    IFLASHC virtual int             Sync(Ptr<KFSVolume> volume);
    
    IFLASHC virtual int             ReadFSStat(Ptr<KFSVolume> volume, fs_info* fsinfo);
    IFLASHC virtual int             WriteFSStat(Ptr<KFSVolume> volume, const fs_info* fsinfo, uint32_t mask);
    IFLASHC virtual Ptr<KINode>     LocateInode(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* path, int pathLength);
    IFLASHC virtual bool            ReleaseInode(KINode* inode);
    IFLASHC virtual Ptr<KFileNode>  CreateFile(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int openFlags, int permission);

    IFLASHC virtual Ptr<KINode>     LoadInode(Ptr<KFSVolume> volume, ino_t inode);

    IFLASHC virtual int             CreateDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength, int permission);

    IFLASHC virtual int             Rename(Ptr<KFSVolume> volume, Ptr<KINode> oldParent, const char* oldName, int oldNameLen, Ptr<KINode> newParent, const char* newName, int newNameLen, bool mustBeDir);
    IFLASHC virtual int             Unlink(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength);
    IFLASHC virtual int             RemoveDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength);
};

} // namespace
