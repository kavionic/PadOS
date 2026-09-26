// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 19.02.2018 21:39:19

#pragma once


#include <string>
#include <map>

#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "Kernel/KMutex.h"
#include "Kernel/KConditionVariable.h"

struct device_geometry;

namespace kernel
{

class KFSVolume;
class KFilesystem;
class KFileNode;
class KDeviceNode;
class KRootFilesystem;

class KInode;

typedef void disk_read_op(void* cookie, off64_t offset, void* buffer, size_t size);

struct disk_partition_desc
{
    off64_t p_start;	/* Offset in bytes */
    off64_t p_size;	/* Size in bytes   */
    int     p_type;	/* Type as found in the partition table	*/
    int     p_status;	/* Status as found in partition table (bit 7=active) */
};


class KVFSManager
{
public:
    static constexpr size_t DISK_PARTITION_TABLE_MINIMUM_BUFFER_SIZE = 512;

    KVFSManager();
    ~KVFSManager();

    static void RegisterFilesystem(Ptr<KFilesystem> filesystem);

    static std::vector<disk_partition_desc> DecodeDiskPartitions_trw(void* blockBuffer, size_t bufferSize, const device_geometry& diskGeom, disk_read_op* readCallback, void* userData);

    static void           RegisterVolume_trw(Ptr<KFSVolume> volume);
    static void           DetachVolume_trw(Ptr<KFSVolume> volume);
    static Ptr<KFSVolume> GetVolume(fs_id volumeID);
    static Ptr<KInode>    GetInode_trw(KFSVolume& volume, ino_t inodeID, bool crossMount);
    static bool           InodeReleased(KInode* inode);
    static void           FlushInodes();
    static void           FlushInodes(KFSVolume* volume);
    static void           DiscardDirtyInodes(KFSVolume* volume) noexcept;
    static void           MarkInodeDirty(KInode* inode) noexcept;
    static void           DiscardInodeDirtyState(KInode* inode) noexcept;

private:
    static Ptr<KInode> TryAcquireInodeReference(KInode* inode);
    static PErrorCode FlushInodesInternal(KFSVolume* volume);
    static PErrorCode FlushInodes_pl(KFSVolume* volume);
    static void DeleteReleasedInodes(KFSVolume* volume);
    static PErrorCode WriteDirtyInodes(KFSVolume* volume);
    static KInode* FindFirstUnusedInode();
    static void DiscardInode(KInode* inode);
    static void DeleteInode(KInode* inode);
    
    static constexpr size_t         MAX_INODE_CACHE_COUNT = 256;

    static inline KInode* const PENDING_INODE = reinterpret_cast<KInode*>(intptr_t(1));
    
    static KMutex                          s_InodeMapMutex;
    static PIntrusiveList<KInode>          s_InodeLRUList;
    static std::map<fs_id, Ptr<KFSVolume>> s_VolumeMap;
    static KConditionVariable              s_InodeMapConditionVar;

    KVFSManager( const KVFSManager &c ) = delete;
    KVFSManager& operator=( const KVFSManager &c ) = delete;
};


} // namespace
