// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 18/05/19 18:15:05

#include <System/Platform.h>

#include <string.h>

#include <Kernel/KLogging.h>
#include <Kernel/FSDrivers/FAT/FATFilesystem.h>
#include <Kernel/VFS/FileIO.h>
#include <System/ExceptionHandling.h>

#include "FATVolume.h"
#include "FATINode.h"

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATVolume::FATVolume(Ptr<FATFilesystem> filesystem, fs_id volumeID, const os::String& devicePath)
    : KFSVolume(volumeID, devicePath), m_Mutex("fatfs_vol_mutex", PEMutexRecursionMode_RaiseError), m_INodeIDMapMutex("fatfs_inodemap_mutex", PEMutexRecursionMode_RaiseError)
{
    m_Magic = MAGIC;

    m_VolumeLabelEntry = -2;	// for now, assume there is no volume entry
    memset(m_VolumeLabel, ' ', 11);
        
    m_RootINode = ptr_new<FATINode>(filesystem, ptr_tmp_cast(this), true);
    m_RootNode = m_RootINode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATVolume::~FATVolume()
{
    m_Magic = ~MAGIC;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::Shutdown()
{
    m_FATTable = nullptr; // Must reset manually to break the reference loop.
}    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::ReadSuperBlock(int deviceFile)
{
    std::vector<uint8_t> buffer;

    buffer.resize(512);

    FATSuperBlock* superBlock = reinterpret_cast<FATSuperBlock*>(buffer.data());
    
      // read in the boot sector
    const size_t bytesRead = kpread_trw(deviceFile, buffer.data(), buffer.size(), 0);
    if (bytesRead != buffer.size()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): error reading boot sector.");
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    
    m_MediaDescriptor = superBlock->m_Media;
    
      // Only check boot signature on hard disks 
    if (superBlock->m_Signature != 0xaa55 && m_MediaDescriptor == 0xf8)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid signature 0x{:x}", uint16_t(superBlock->m_Signature));
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    if (memcmp(superBlock->m_OEMName, "NTFS    ", 8) == 0 || memcmp(superBlock->m_OEMName, "HPFS    ", 8) == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): {}, not FAT.", std::string_view(reinterpret_cast<const char*>(superBlock->m_OEMName), sizeof(superBlock->m_OEMName)));
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
      // First fill in the universal fields from the bpb
    m_BytesPerSector = superBlock->m_BytesPerSector;
    if ((m_BytesPerSector != 512) && (m_BytesPerSector != 1024) && (m_BytesPerSector != 2048))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): unsupported bytes per sector ({})", m_BytesPerSector);
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
	
    m_SectorsPerCluster = superBlock->m_SectorsPerCluster;
    
    bool validSectorsPerCluster = false;
    for (int i = 0; i <=7; ++i)
    {
        if (m_SectorsPerCluster == (1<<i)) {
            validSectorsPerCluster = true;
            break;
        }
    }
    if (!validSectorsPerCluster)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount() sectors/cluster = {}", m_SectorsPerCluster);
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    m_ReservedSectors = superBlock->m_ReservedSectors;

    m_FATCount = superBlock->m_FATCount;
    if (m_FATCount == 0 || m_FATCount > 8)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): unreasonable FAT count ({}).", m_FATCount);
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

      // Check media descriptor versus known types.
    if ((superBlock->m_Media != 0xF0) && (superBlock->m_Media < 0xf8))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid media descriptor byte.");
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }


    uint32_t rootStartCluster;
    uint32_t rootEndCluster;
    uint32_t rootSize = 0;
    
      // now become more discerning citizens
    if (superBlock->m_SectorsPerFAT16 == 0)
    {
	    // FAT32
	    m_FATBits = 32;
	    m_SectorsPerFAT = superBlock->m_FSDependent.FAT32.m_SectorsPerFAT;
        m_TotalSectors  = superBlock->m_TotalSectorCount32;
		
        m_FSInfoSector  = superBlock->m_FSDependent.FAT32.m_FSInfoSector;
	    if ((m_FSInfoSector != 0xffff) && (m_FSInfoSector >= m_ReservedSectors))
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): fsinfo sector too large ({}).", m_FSInfoSector);
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }
        m_FATMirrored = !(superBlock->m_FSDependent.FAT32.m_ExtendedFlags & 0x80);
        m_ActiveFAT = (m_FATMirrored) ? (superBlock->m_FSDependent.FAT32.m_ExtendedFlags & 0xf) : 0;

        m_FirstDataSector = m_ReservedSectors + m_FATCount * m_SectorsPerFAT;
        m_TotalClusters   = (m_TotalSectors - m_FirstDataSector) / m_SectorsPerCluster;

        rootStartCluster = superBlock->m_FSDependent.FAT32.m_RootDirectory;
	    if (rootStartCluster >= m_TotalClusters)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): root inode cluster too large ({}).", rootStartCluster);
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }
    }
    else
    {
        m_SectorsPerFAT = superBlock->m_SectorsPerFAT16;
	  // FAT12 & FAT16
    	if (m_FATCount != 2) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): claims {} fat tables.", m_FATCount);
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }

        m_RootEntriesCount = superBlock->m_RootDirEntryCount16;
	    if (m_RootEntriesCount % (m_BytesPerSector / 0x20))
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid number of root entries.");
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }

	    m_FSInfoSector = 0xffff;
	    m_TotalSectors = superBlock->m_TotalSectorCount16;
	    if (m_TotalSectors == 0) {
	        m_TotalSectors = superBlock->m_TotalSectorCount32;
        }            

	    m_FATMirrored = true;
	    m_ActiveFAT   = 0;

        m_RootStart       = m_ReservedSectors + m_FATCount * m_SectorsPerFAT;
        m_RootSectorCount = m_RootEntriesCount * 0x20 / m_BytesPerSector;
	    rootStartCluster  = 1;
        rootEndCluster    = 1;
        rootSize          = m_RootSectorCount * m_BytesPerSector;
	    m_FirstDataSector = m_RootStart + m_RootSectorCount;
	    m_TotalClusters   = (m_TotalSectors - m_FirstDataSector) / m_SectorsPerCluster;	
		
	    // XXX: uncertain about border cases; win32 sdk says cutoffs are at
	    //      at ff6/ff7 (or fff6/fff7), but that doesn't make much sense
	    if (m_TotalClusters > 0xff1) {
	        m_FATBits = 16;
	    } else {
	        m_FATBits = 12;
        }
        if (superBlock->m_FSDependent.FAT16.m_BootSignature == 0x29)
        {
            // Fill in the volume label
            if (memcmp(superBlock->m_FSDependent.FAT16.m_VolumeLabel, "           ", 11) != 0) {
                memcpy(m_VolumeLabel, superBlock->m_FSDependent.FAT16.m_VolumeLabel, 11);
                m_VolumeLabelEntry = -1;
            }
        }
        
    }
    m_RootINode->m_Size         = rootSize;
    m_RootINode->m_StartCluster = rootStartCluster;
    m_RootINode->m_EndCluster   = rootEndCluster;
    
    m_FATTable = ptr_new<FATTable>(ptr_tmp_cast(this)); // WARNING: Circular reference! Manually broken in Shutdown().
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ino_t FATVolume::AllocUniqueINodeID()
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "Allocate unique inode ID: {:x}", m_CurrentArtificialID);
    return m_CurrentArtificialID++;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::SetINodeIDToLocationIDMapping(ino_t inodeID, ino_t locationID)
{
    CRITICAL_SCOPE(m_INodeIDMapMutex);

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::SetINodeIDToLocationIDMapping({:16x} -> {:16x})", inodeID, locationID);

    auto inodeItr = m_INodeToLocationMap.find(inodeID);
    if (inodeItr != m_INodeToLocationMap.end())
    {
        INodeMapEntry& entry = inodeItr->second;
        if (locationID != entry.m_LocationID)
        {
            auto locItr = m_LocationToINodeMap.find(entry.m_LocationID);
            kassert(locItr != m_LocationToINodeMap.end() && locItr->second == &entry);
            if (locItr != m_LocationToINodeMap.end()) {
                m_LocationToINodeMap.erase(locItr);
            }
            if (inodeID != locationID)
            {
                try {
                    m_LocationToINodeMap[locationID] = &entry;
                } catch(const std::bad_alloc&) {
                    m_INodeToLocationMap.erase(inodeItr);
                    throw;
                }
            }
            else
            {
                m_INodeToLocationMap.erase(inodeItr);
            }
        }            
    }
    else if (inodeID != locationID)
    {
        INodeMapEntry* entry;
        entry = &m_INodeToLocationMap[inodeID];
        entry->m_INodeID = inodeID;
        entry->m_LocationID = locationID;
        try
        {
            m_LocationToINodeMap[locationID] = entry;
        }
        catch(const std::bad_alloc&)
        {
            auto i = m_INodeToLocationMap.find(inodeID);
            kassert(i != m_INodeToLocationMap.end());
            if (i != m_INodeToLocationMap.end()) {
                m_INodeToLocationMap.erase(i);
            }
            throw;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::RemoveINodeIDToLocationIDMapping(ino_t inodeID)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::RemoveINodeIDToLocationIDMapping({:16x})", inodeID);

    CRITICAL_SCOPE(m_INodeIDMapMutex);

    auto inodeItr = m_INodeToLocationMap.find(inodeID);
    if (inodeItr != m_INodeToLocationMap.end())
    {
        INodeMapEntry& entry = inodeItr->second;
        auto locItr = m_LocationToINodeMap.find(entry.m_LocationID);
        kassert(locItr != m_LocationToINodeMap.end() && locItr->second == &entry);
        if (locItr != m_LocationToINodeMap.end()) {
            m_LocationToINodeMap.erase(locItr);
        }
        m_INodeToLocationMap.erase(inodeItr);
        return true;
    }
    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATVolume::RemoveINodeIDToLocationIDMapping({:16x}) failed to find mapping.", inodeID);
    set_last_error(ENOENT);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::GetINodeIDToLocationIDMapping(ino_t inodeID, ino_t* locationID) const
{
    CRITICAL_SHARED_SCOPE(m_INodeIDMapMutex);
    auto i = m_INodeToLocationMap.find(inodeID);
    if (i != m_INodeToLocationMap.end()) {
        *locationID = i->second.m_LocationID;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::GetLocationIDToINodeIDMapping(ino_t locationID, ino_t* inodeID) const
{
    CRITICAL_SHARED_SCOPE(m_INodeIDMapMutex);
    auto i = m_LocationToINodeMap.find(locationID);
    if (i != m_LocationToINodeMap.end()) {
        *inodeID = i->second->m_LocationID;
        return true;
    }
    return false;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::HasINodeIDToLocationIDMapping(ino_t inodeID) const
{
    CRITICAL_SHARED_SCOPE(m_INodeIDMapMutex);
    return m_INodeToLocationMap.find(inodeID) != m_INodeToLocationMap.end();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::HasLocationIDToINodeIDMapping(ino_t locationID) const
{
    CRITICAL_SHARED_SCOPE(m_INodeIDMapMutex);
    return m_LocationToINodeMap.find(locationID) != m_LocationToINodeMap.end();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::DumpINodeIDMap()
{
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "INode map size {}, current artificial ID = {:x}", m_INodeToLocationMap.size(), m_CurrentArtificialID);
    
    for (auto& entry : m_INodeToLocationMap)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{:16x} {:16x}", entry.second.m_INodeID, entry.second.m_LocationID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::AddDirectoryMapping(ino_t inodeID)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::AddDirectoryMapping({:x})", inodeID);

    kassert(IS_DIR_CLUSTER_INODEID(inodeID));
    kassert(inodeID != 0);
    kassert(m_DirectoryMap.find(CLUSTER_OF_DIR_CLUSTER_INODEID(inodeID)) == m_DirectoryMap.end());

    m_DirectoryMap[CLUSTER_OF_DIR_CLUSTER_INODEID(inodeID)] = inodeID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::RemoveDirectoryMapping(ino_t inodeID)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::RemoveDirectoryMapping({:x})", inodeID);

    kassert(IS_DIR_CLUSTER_INODEID(inodeID));
    kassert(inodeID != 0);
    
    auto i = m_DirectoryMap.find(CLUSTER_OF_DIR_CLUSTER_INODEID(inodeID));
    if (i != m_DirectoryMap.end()) {
        m_DirectoryMap.erase(i);
        return true;
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ino_t FATVolume::GetDirectoryMapping(uint32_t startCluster) const
{
    auto i = m_DirectoryMap.find(startCluster);
    if (i != m_DirectoryMap.end()) {
        return i->second;
    } else {
        set_last_error(ENOENT);
        return -1;
    }            
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::DumpDirectoryMap()
{
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{} directory mapping entries.", m_DirectoryMap.size());

    for (auto i : m_DirectoryMap) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{:x}", i.second);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
bool FATVolume::CheckMagic(const char* functionName)
{
    if (m_Magic != MAGIC)
    {
        panic("{} passed volume with invalid magic number {:#08x}", functionName, m_Magic);
        return false;
    }
    return true;
}


} // kernel
