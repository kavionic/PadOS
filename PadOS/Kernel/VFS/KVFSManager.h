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
// Created: 19.02.2018 21:39:19

#pragma once


#include <string>
#include <map>

#include "System/Ptr/PtrTarget.h"
#include "System/Ptr/Ptr.h"
#include "Kernel/KMutex.h"

struct device_geometry;

namespace kernel
{

class KFSVolume;
class KFilesystem;
class KFileNode;
class KDeviceNode;
class KRootFilesystem;

class KINode;

typedef size_t disk_read_op(void* cookie, off64_t offset, void* buffer, size_t size);

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
    KVFSManager();
    ~KVFSManager();

    static void RegisterFilesystem(Ptr<KFilesystem> filesystem);

    static int DecodeDiskPartitions(const device_geometry& diskGeom, std::vector<disk_partition_desc>* partitions, disk_read_op* readCallback, void* userData);

    static bool           RegisterVolume(Ptr<KFSVolume> volume);
    static Ptr<KFSVolume> GetVolume(fs_id volumeID);
    static Ptr<KINode>    GetINode(fs_id volumeID, ino_t inodeID, bool crossMount);

private:
    static KMutex s_INodeMapMutex;
    static std::map<std::pair<fs_id, ino_t>, Ptr<KINode>> s_INodeMap;
    static IntrusiveList<KINode>                      s_InodeMRUList;
    static std::map<fs_id, Ptr<KFSVolume>>            s_VolumeMap;

    KVFSManager( const KVFSManager &c );
    KVFSManager& operator=( const KVFSManager &c );
};


} // namespace
