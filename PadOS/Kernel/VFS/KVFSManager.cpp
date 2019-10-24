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

 #include "sam.h"

#include <string.h>
#include <assert.h>

#include "KVFSManager.h"
#include "KDeviceNode.h"
#include "KFSVolume.h"
#include "KINode.h"
#include "DeviceControl/DeviceControl.h"

using namespace kernel;

KINode* const                              KVFSManager::PENDING_INODE = (KINode*)(1);
KMutex                                     KVFSManager::s_INodeMapMutex("inode_map_mutex");
std::map<std::pair<fs_id, ino_t>, KINode*> KVFSManager::s_INodeMap;
IntrusiveList<KINode>                      KVFSManager::s_InodeMRUList;
int                                        KVFSManager::s_UnusedInodeCount = 0;
KConditionVariable                         KVFSManager::s_InodeMapConditionVar("inode_map_condition");

std::map<fs_id, Ptr<KFSVolume>> KVFSManager::s_VolumeMap;

struct PartitionRecord
{
    uint8_t  m_Status;
    uint8_t  m_FirstHead;
    uint16_t m_FirstCyl;
    uint8_t  m_Type;
    uint8_t  m_LastHead;
    uint16_t m_LastCyl;
    uint32_t m_StartLBA;
    uint32_t m_Size;
} __attribute__((packed));

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KVFSManager::KVFSManager()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KVFSManager::~KVFSManager()
{
}


///////////////////////////////////////////////////////////////////////////////
///* Decode a hard-disk partition table.
/// \ingroup DriverAPI
/// \par Description:
///	DecodeDiskPartitions() can be called by block-device drivers to
///	decode a disk's partition table. It will return both primary
///	partitions and logical partitions within the extended partition if
///	one exists. The extended partition itself will not be returned.
///
///	The caller must provide the device-geometry and a callback that
///	will be called to read the primary partition table and any existing
///	nested extended partition tables.
///
///	The partition table is validated and the function will fail if
///	it is found invalid. Checks performed includes overlapping partitions,
///	partitions ending outside the disk and the primary table containing
///	more than one extended partition.
///
/// \par Note:
///	Primary partitions are numbered 0-3 based on their position inside
///	the primary table. Logical partition are numbered from 4 and up.
///	This might leave "holes" in the returned array of partitions.
///	The returned count only indicate the highest used partition number
///	and the caller must check each returned partition and filter out
///	partitions where the type-field is '0'.
/// \param psDevice
///	Pointer to a device_geometry structure describing the disk's geometry.
/// \param pasPartitions
///	Pointer to an array of partition descriptors that will be filled in.
/// \param userData
///	Pointer private to the caller that will be passed back to the pfReadCallback
///	call-back function.
/// \param pfReadCallback
///	Pointer to a function that will be called to read in partition tables.
/// 
/// \return
///	A positive maximum partition-count on success or a negative error
///	code on failure.
/// \sa create_device_node(), delete_device_node()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


int KVFSManager::DecodeDiskPartitions(const device_geometry& diskGeom, std::vector<disk_partition_desc>* partitions, disk_read_op* readCallback, void* userData)
{
    std::vector<uint8_t> buffer;
    buffer.resize(512);
    PartitionRecord* recordTable = reinterpret_cast<PartitionRecord*>(&buffer[0x1be]);
    off64_t diskSize = diskGeom.sector_count * diskGeom.bytes_per_sector;
    off64_t tablePos = 0;
    off64_t extStart = 0;
    off64_t firstExtended = 0;
    int	    numExtended;
    int	    numActive;

    static const size_t MAX_PARTITIONS = 64; // Just a sanity check in case there is some kind of circular loop with the extended partition

    for (;partitions->size() < MAX_PARTITIONS ;)
    {
        if (readCallback( userData, tablePos, buffer.data(), buffer.size()) != buffer.size()) {
	    kprintf( "ERROR: KVFSManager::DecodeDiskPartitions() failed to read MBR\n" );
	    return 0;
        }
        if (*reinterpret_cast<uint16_t*>(&buffer[0x1fe]) != 0xaa55) {
	    kprintf( "ERROR: KVFSManager::DecodeDiskPartitions() Invalid partition table signature %04x\n", *reinterpret_cast<uint16_t*>(&buffer[0x1fe]));
	    return 0;
        }

        numActive   = 0;
        numExtended = 0;
    
        for (int i = 0 ; i < 4 ; ++i)
        {
	    if ( recordTable[i].m_Status & 0x80 ) {
	        numActive++;
	    }
	    if ( recordTable[i].m_Type == 0x05 || recordTable[i].m_Type == 0x0f || recordTable[i].m_Type == 0x85 ) {
	        numExtended++;
	    }
	    if ( numActive > 1 ) {
	        kprintf( "WARNING: KVFSManager::DecodeDiskPartitions() more than one active partitions\n" );
	    }
	    if ( numExtended > 1 ) {
	        kprintf( "ERROR: KVFSManager::DecodeDiskPartitions() more than one extended partitions\n" );
	        set_last_error(EINVAL);
	        return -1;
	    }
        }
        for (int i = 0 ; i < 4 && partitions->size() < MAX_PARTITIONS; ++i)
        {
	    if (recordTable[i].m_Type == 0) {
	        continue;
	    }
            disk_partition_desc partitionDesc;
            memset(&partitionDesc, 0, sizeof(partitionDesc));
            
	    if (recordTable[i].m_Type == 0x05 || recordTable[i].m_Type == 0x0f || recordTable[i].m_Type == 0x85)
            {
	        extStart = uint64_t(recordTable[i].m_StartLBA) * uint64_t(diskGeom.bytes_per_sector); // + nTablePos;
	        if (firstExtended == 0) {
                    partitions->push_back(partitionDesc);
	        }
	        continue;
	    }
	    partitionDesc.p_type   = recordTable[i].m_Type;
	    partitionDesc.p_status = recordTable[i].m_Status;
	    partitionDesc.p_start  = uint64_t(recordTable[i].m_StartLBA) * uint64_t(diskGeom.bytes_per_sector) + tablePos;
	    partitionDesc.p_size   = uint64_t(recordTable[i].m_Size) * uint64_t(diskGeom.bytes_per_sector);

	    if ( partitionDesc.p_start + partitionDesc.p_size > diskSize )
            {
	        kprintf("ERROR: Partition %d extend outside the disk/extended partition\n", partitions->size());
	        set_last_error(EINVAL);
	        return -1;
	    }
	
	    for (size_t j = 0 ; j < partitions->size() ; ++j)
            {
                const disk_partition_desc& curPartition = (*partitions)[j];
	        if ( partitionDesc.p_type == 0 ) {
		    continue;
	        }
	        if (curPartition.p_start + curPartition.p_size > partitionDesc.p_start &&
		     curPartition.p_start <  partitionDesc.p_start + partitionDesc.p_size ) {
		    kprintf("ERROR: KVFSManager::DecodeDiskPartitions() partition %d overlap partition %d\n", j, partitions->size());
		    set_last_error(EINVAL);
		    return -1;
	        }
	        if ( (partitionDesc.p_status & 0x80) != 0 && (curPartition.p_status & 0x80) != 0 ) {
		    kprintf( "ERROR: KVFSManager::DecodeDiskPartitions() more than one active partitions\n" );
		    set_last_error(EINVAL);
		    return -1;
	        }
	        if ( partitionDesc.p_type == 0x05 && curPartition.p_type == 0x05 ) {
		    kprintf( "ERROR: KVFSManager::DecodeDiskPartitions() more than one extended partitions\n" );
		    set_last_error(EINVAL);
		    return -1;
	        }
	    }
            partitions->push_back(partitionDesc);
        }
        if (extStart != 0)
        {
	    tablePos = firstExtended + extStart;
	    if ( firstExtended == 0 ) {
	        firstExtended = extStart;
	    }
	    extStart = 0;
        }
        else
        {
            break;
        }
    }
    return partitions->size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KVFSManager::RegisterVolume(Ptr<KFSVolume> volume)
{
    if (volume == nullptr || s_VolumeMap.find(volume->m_VolumeID) != s_VolumeMap.end()) {
        kprintf("ERROR: KVFSManager::RegisterVolume() failed to register volume %p\n", ptr_raw_pointer_cast(volume));
        return false;
    }
    try {
        s_VolumeMap[volume->m_VolumeID] = volume;
        return true;
    } catch(std::bad_alloc&) {
        kprintf("ERROR: KVFSManager::RegisterVolume() not enough memory to register volume %p\n", ptr_raw_pointer_cast(volume));
        set_last_error(ENOMEM);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KVFSManager::GetVolume(fs_id volumeID)
{
    auto i = s_VolumeMap.find(volumeID);
    if (i != s_VolumeMap.end()) {
        return i->second;
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> KVFSManager::GetINode(fs_id volumeID, ino_t inodeID, bool crossMount)
{
    for (;;)
    {
        CRITICAL_SCOPE(s_INodeMapMutex);
    
        auto i = s_INodeMap.find(std::make_pair(volumeID, inodeID));
    
        if (i != s_INodeMap.end())
        {
            if (i->second == PENDING_INODE) {
                s_InodeMapConditionVar.Wait(s_INodeMapMutex);
                continue;
            }
            if (i->second->GetPtrCount() == 0) {
                kassert(i->second->GetList() == &s_InodeMRUList);
                s_InodeMRUList.Remove(i->second);
                s_UnusedInodeCount--;
            }
            Ptr<KINode> inode = ptr_tmp_cast(i->second);
            if (crossMount && inode->m_MountRoot != nullptr) {
                inode = inode->m_MountRoot;
            }
            return inode;
        }
        else
        {
            Ptr<KFSVolume> volume = GetVolume(volumeID);
            if (volume != nullptr && volume->m_Filesystem != nullptr)
            {
                auto key = std::make_pair(volumeID, inodeID);
                s_INodeMap[key] = PENDING_INODE; // Make sure no other thread attempts to load the same inode while we unlock s_INodeMapMutex.
                
                s_INodeMapMutex.Unlock();
                Ptr<KINode> inode = volume->m_Filesystem->LoadInode(volume, inodeID);
                s_INodeMapMutex.Lock();

                if (inode != nullptr)
                {
                    s_INodeMap[key] = ptr_raw_pointer_cast(inode);
                    if (crossMount && inode->m_MountRoot != nullptr) {
                        inode = inode->m_MountRoot;
                    }
                }
                return inode;
            }
        }
    }        
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::InodeReleased(KINode* inode)
{
    CRITICAL_SCOPE(s_INodeMapMutex);
    if (inode->GetPtrCount() == 0)
    {
        s_InodeMRUList.Append(inode);
        s_UnusedInodeCount++;
        if (s_UnusedInodeCount > MAX_INODE_CACHE_COUNT) {
            kassert(s_InodeMRUList.m_First != nullptr);
            DiscardInode(s_InodeMRUList.m_First); // Discard oldest cached inode.
        }        
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::FlushInodes()
{
    if (s_InodeMRUList.m_First != nullptr)
    {
        CRITICAL_SCOPE(s_INodeMapMutex);

        time_t curTime = get_system_time() / 1000000;
        while(s_InodeMRUList.m_First != nullptr && curTime > (s_InodeMRUList.m_First->m_LastUseTime + 1))
        {
            DiscardInode(s_InodeMRUList.m_First);
        }            
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KVFSManager::DiscardInode(KINode* inode)
{
    kassert(s_INodeMapMutex.IsLocked());
    kassert(inode->GetPtrCount() == 0);
    s_InodeMRUList.Remove(inode);
    s_UnusedInodeCount--;
    
    auto key = std::make_pair(inode->m_Volume->m_VolumeID, inode->m_INodeID);
    auto i = s_INodeMap.find(key);
    kassert(i != s_INodeMap.end());
    kassert(i->second != PENDING_INODE);
    i->second = PENDING_INODE;

    s_INodeMapMutex.Unlock();
    inode->m_Filesystem->ReleaseInode(inode);
    delete inode;
    s_INodeMapMutex.Lock();
    
    i = s_INodeMap.find(key);
    kassert(i != s_INodeMap.end());
    
    kassert(i->second == PENDING_INODE);
    s_INodeMap.erase(i);
    s_InodeMapConditionVar.Wakeup(0);
}

