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
#include "FATInode.h"


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATVolume::FATVolume(Ptr<FATFilesystem> filesystem, fs_id volumeID, const PString& devicePath)
    : KFSVolume(volumeID, devicePath), m_Mutex("fatfs_vol_mutex", PEMutexRecursionMode_RaiseError), m_InodeIDMapMutex("fatfs_inodemap_mutex", PEMutexRecursionMode_RaiseError)
{
    m_Magic = MAGIC;

    m_VolumeLabelEntry = -2;	// for now, assume there is no volume entry
    memset(m_VolumeLabel, ' ', 11);
        
    m_RootInode = ptr_new<FATInode>(filesystem, ptr_tmp_cast(this), S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
    m_RootNode = m_RootInode;
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

    buffer.resize(sizeof(FATSuperBlock));

    const FATSuperBlock* superBlock = reinterpret_cast<const FATSuperBlock*>(buffer.data());
    
    // Read the boot sector.
    const size_t bytesRead = kpread_trw(deviceFile, buffer.data(), buffer.size(), 0);
    if (bytesRead != buffer.size()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): error reading boot sector.");
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    
    m_MediaDescriptor = superBlock->m_Media;
    
    if (superBlock->m_Signature != 0xaa55)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid signature 0x{:x}", uint16_t(superBlock->m_Signature));
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (memcmp(superBlock->m_OEMName, "NTFS    ", 8) == 0 || memcmp(superBlock->m_OEMName, "HPFS    ", 8) == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): {}, not FAT.", std::string_view(reinterpret_cast<const char*>(superBlock->m_OEMName), sizeof(superBlock->m_OEMName)));
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    // Read and validate the fields common to all FAT BPBs.
    m_BytesPerSector = superBlock->m_BytesPerSector;
    if ((m_BytesPerSector != 512) && (m_BytesPerSector != 1024) && (m_BytesPerSector != 2048))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): unsupported bytes per sector ({})", m_BytesPerSector);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
	
    m_SectorsPerCluster = superBlock->m_SectorsPerCluster;
    const bool validSectorsPerCluster =
        m_SectorsPerCluster != 0 &&
        (m_SectorsPerCluster & (m_SectorsPerCluster - 1)) == 0 &&
        m_SectorsPerCluster <= 128;
    const uint32_t bytesPerCluster = m_BytesPerSector * m_SectorsPerCluster;
    if (!validSectorsPerCluster || bytesPerCluster > 32 * 1024)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid cluster size ({} sectors, {} bytes).", m_SectorsPerCluster, bytesPerCluster);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    m_ReservedSectors = superBlock->m_ReservedSectors;
    if (m_ReservedSectors == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): volume has no reserved sectors.");
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    m_FATCount = superBlock->m_FATCount;
    if (m_FATCount == 0 || m_FATCount > FAT_MAX_SUPPORTED_FAT_COUNT)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): unreasonable FAT count ({}).", m_FATCount);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    // Check the media descriptor against the values defined for FAT.
    if ((superBlock->m_Media != 0xF0) && (superBlock->m_Media < 0xf8))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid media descriptor byte.");
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    m_RootEntriesCount = superBlock->m_RootDirEntryCount16;
    m_TotalSectors = (superBlock->m_TotalSectorCount16 != 0) ? superBlock->m_TotalSectorCount16 : superBlock->m_TotalSectorCount32;
    if (m_TotalSectors == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): volume contains no sectors.");
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    const bool hasFAT32BPB = superBlock->m_SectorsPerFAT16 == 0;
    m_SectorsPerFAT = (superBlock->m_SectorsPerFAT16 != 0) ? superBlock->m_SectorsPerFAT16 : superBlock->m_FSDependent.FAT32.m_SectorsPerFAT;
    if (m_SectorsPerFAT == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): FAT contains no sectors.");
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    const uint64_t rootDirectoryByteCount = uint64_t(m_RootEntriesCount) * 32;
    const uint64_t rootDirectorySectorCount = (rootDirectoryByteCount + m_BytesPerSector - 1) / m_BytesPerSector;
    const uint64_t fatRegionSectorCount = uint64_t(m_FATCount) * m_SectorsPerFAT;
    const uint64_t firstDataSector = uint64_t(m_ReservedSectors) + fatRegionSectorCount + rootDirectorySectorCount;

    if (firstDataSector >= m_TotalSectors)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): metadata extends through or past the end of the volume ({} >= {}).", firstDataSector, m_TotalSectors);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    m_FirstDataSector = static_cast<uint32_t>(firstDataSector);
    const uint64_t dataSectorCount = uint64_t(m_TotalSectors) - firstDataSector;
    m_TotalClusters = static_cast<uint32_t>(dataSectorCount / m_SectorsPerCluster);
    if (m_TotalClusters == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): volume contains no data clusters.");
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    if (m_TotalClusters < 4085) {
        m_FATBits = 12;
    } else if (m_TotalClusters < 65525) {
        m_FATBits = 16;
    } else {
        m_FATBits = 32;
    }

    if ((m_FATBits == 32) != hasFAT32BPB)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): {}-bit FAT does not match the BPB layout.", m_FATBits);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    if (m_FATBits == 32 && m_TotalClusters > 0x0ffffff5)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): FAT32 volume contains too many clusters ({}).", m_TotalClusters);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    const uint64_t fatEntryCount = uint64_t(m_TotalClusters) + FATTable::FIRST_DATA_CLUSTER;
    const uint64_t requiredFATByteCount = (fatEntryCount * m_FATBits + 7) / 8;
    const uint64_t availableFATByteCount = uint64_t(m_SectorsPerFAT) * m_BytesPerSector;
    if (requiredFATByteCount > availableFATByteCount)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): FAT is too small ({} bytes available, {} required).", availableFATByteCount, requiredFATByteCount);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    uint32_t rootStartCluster;
    uint32_t rootEndCluster;
    uint32_t rootSize = 0;
    
    if (m_FATBits == 32)
    {
        if (m_RootEntriesCount != 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): FAT32 root-entry count is not zero ({}).", m_RootEntriesCount);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        if (superBlock->m_FSDependent.FAT32.m_FSVersion != 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): unsupported FAT32 version 0x{:04x}.", superBlock->m_FSDependent.FAT32.m_FSVersion);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        m_FSInfoSector = superBlock->m_FSDependent.FAT32.m_FSInfoSector;
        if ((m_FSInfoSector != 0xffff) && ((m_FSInfoSector == 0) || (m_FSInfoSector >= m_ReservedSectors)))
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid FSInfo sector ({}).", m_FSInfoSector);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        const uint16_t extendedFlags = superBlock->m_FSDependent.FAT32.m_ExtendedFlags;
        m_FATMirrored = (extendedFlags & 0x80) == 0;
        m_ActiveFAT = m_FATMirrored ? 0 : uint8_t(extendedFlags & 0x0f);
        if (m_ActiveFAT >= m_FATCount)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): active FAT index {} exceeds FAT count {}.", m_ActiveFAT, m_FATCount);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        rootStartCluster = superBlock->m_FSDependent.FAT32.m_RootDirectory;
        if (!IsDataCluster(rootStartCluster))
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid root-directory cluster ({}).", rootStartCluster);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        rootEndCluster = 0;
    }
    else
    {
        if (m_RootEntriesCount == 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): FAT12/16 volume contains no root-directory entries.");
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        m_FSInfoSector = 0xffff;
        m_FATMirrored = true;
        m_ActiveFAT = 0;

        m_RootStart = m_ReservedSectors + m_FATCount * m_SectorsPerFAT;
        m_RootSectorCount = static_cast<uint32_t>(rootDirectorySectorCount);
        rootStartCluster = 1;
        rootEndCluster = 1;
        rootSize = m_RootSectorCount * m_BytesPerSector;

        if (superBlock->m_FSDependent.FAT16.m_BootSignature == 0x29)
        {
            // Fill in the volume label
            if (memcmp(superBlock->m_FSDependent.FAT16.m_VolumeLabel, "           ", 11) != 0)
            {
                memcpy(m_VolumeLabel, superBlock->m_FSDependent.FAT16.m_VolumeLabel, 11);
                m_VolumeLabelEntry = -1;
            }
        }
    }

    m_RootInode->m_Size         = rootSize;
    m_RootInode->m_StartCluster = rootStartCluster;
    m_RootInode->m_EndCluster   = rootEndCluster;
    
    m_FATTable = ptr_new<FATTable>(ptr_tmp_cast(this)); // WARNING: Circular reference! Manually broken in Shutdown().
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ino_t FATVolume::AllocUniqueInodeID()
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "Allocate unique inode ID: {:x}", m_CurrentArtificialID);
    return m_CurrentArtificialID++;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::SetInodeIDToLocationIDMapping(ino_t inodeID, ino_t locationID)
{
    CRITICAL_SCOPE(m_InodeIDMapMutex);

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x})", inodeID, locationID);

    auto inodeItr = m_InodeToLocationMap.find(inodeID);
    if (inodeItr != m_InodeToLocationMap.end())
    {
        InodeMapEntry& entry = inodeItr->second;
        if (locationID != entry.m_LocationID)
        {
            auto locItr = m_LocationToInodeMap.find(entry.m_LocationID);
            if (locItr != m_LocationToInodeMap.end() && locItr->second == &entry)
            {
                m_LocationToInodeMap.erase(locItr);
            }
            else
            {
                const ino_t staleOwner = (locItr != m_LocationToInodeMap.end()) ? locItr->second->m_InodeID : 0;
                kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): reverse map at {:16x} owned by {:16x}.", inodeID, locationID, entry.m_LocationID, staleOwner);
            }
            if (inodeID != locationID)
            {
                try
                {
                    auto [it, inserted] = m_LocationToInodeMap.try_emplace(locationID, &entry);
                    if (!inserted)
                    {
                        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): new location already claimed by {:16x}, taking over.", inodeID, locationID, it->second->m_InodeID);
                        it->second = &entry;
                    }
                    entry.m_LocationID = locationID;
                }
                catch(const std::bad_alloc&)
                {
                    m_InodeToLocationMap.erase(inodeItr);
                    throw;
                }
            }
            else
            {
                m_InodeToLocationMap.erase(inodeItr);
            }
        }            
    }
    else if (inodeID != locationID)
    {
        InodeMapEntry* entry;
        entry = &m_InodeToLocationMap[inodeID];
        entry->m_InodeID = inodeID;
        entry->m_LocationID = locationID;
        try
        {
            auto [it, inserted] = m_LocationToInodeMap.try_emplace(locationID, entry);
            if (!inserted) {
                // Location already claimed — log and take it over.
                kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): location already claimed by {:16x}, taking over.", inodeID, locationID, it->second->m_InodeID);
                it->second = entry;
            }
        }
        catch(const std::bad_alloc&)
        {
            auto i = m_InodeToLocationMap.find(inodeID);
            kassert(i != m_InodeToLocationMap.end());
            if (i != m_InodeToLocationMap.end()) {
                m_InodeToLocationMap.erase(i);
            }
            throw;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::RemoveInodeIDToLocationIDMapping(ino_t inodeID)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::RemoveInodeIDToLocationIDMapping({:16x})", inodeID);

    CRITICAL_SCOPE(m_InodeIDMapMutex);

    auto inodeItr = m_InodeToLocationMap.find(inodeID);
    if (inodeItr != m_InodeToLocationMap.end())
    {
        InodeMapEntry& entry = inodeItr->second;
        auto locItr = m_LocationToInodeMap.find(entry.m_LocationID);
        if (locItr != m_LocationToInodeMap.end() && locItr->second == &entry) {
            m_LocationToInodeMap.erase(locItr);
        } else {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::RemoveInodeIDToLocationIDMapping({:16x}): reverse map inconsistency at location {:16x}.", inodeID, entry.m_LocationID);
        }
        m_InodeToLocationMap.erase(inodeItr);
        return true;
    }
    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATVolume::RemoveInodeIDToLocationIDMapping({:16x}) failed to find mapping.", inodeID);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::GetInodeIDToLocationIDMapping(ino_t inodeID, ino_t* locationID) const
{
    CRITICAL_SHARED_SCOPE(m_InodeIDMapMutex);
    auto i = m_InodeToLocationMap.find(inodeID);
    if (i != m_InodeToLocationMap.end()) {
        *locationID = i->second.m_LocationID;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::GetLocationIDToInodeIDMapping(ino_t locationID, ino_t* inodeID) const
{
    CRITICAL_SHARED_SCOPE(m_InodeIDMapMutex);
    auto i = m_LocationToInodeMap.find(locationID);
    if (i != m_LocationToInodeMap.end()) {
        *inodeID = i->second->m_InodeID;
        return true;
    }
    return false;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::HasInodeIDToLocationIDMapping(ino_t inodeID) const
{
    CRITICAL_SHARED_SCOPE(m_InodeIDMapMutex);
    return m_InodeToLocationMap.find(inodeID) != m_InodeToLocationMap.end();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::HasLocationIDToInodeIDMapping(ino_t locationID) const
{
    CRITICAL_SHARED_SCOPE(m_InodeIDMapMutex);
    return m_LocationToInodeMap.find(locationID) != m_LocationToInodeMap.end();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::DumpInodeIDMap()
{
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "Inode map size {}, current artificial ID = {:x}", m_InodeToLocationMap.size(), m_CurrentArtificialID);
    
    for (auto& entry : m_InodeToLocationMap)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{:16x} {:16x}", entry.second.m_InodeID, entry.second.m_LocationID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::AddDirectoryMapping(uint32_t startCluster, ino_t inodeID)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::AddDirectoryMapping({}, {:x})", startCluster, inodeID);

    if ((!IsDataCluster(startCluster) && !IS_FIXED_ROOT(startCluster)) || !IS_DIR_CLUSTER_INODEID(inodeID) || inodeID == 0 || CLUSTER_OF_DIR_CLUSTER_INODEID(inodeID) != startCluster)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::AddDirectoryMapping(): invalid mapping from cluster {} to inode {:x}.", startCluster, inodeID);
        return false;
    }

    const auto insertionResult = m_DirectoryMap.try_emplace(startCluster, inodeID);
    const auto iterator = insertionResult.first;
    const bool inserted = insertionResult.second;
    if (!inserted && iterator->second != inodeID)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::AddDirectoryMapping(): cluster {} is already mapped to inode {:x}, not {:x}.", startCluster, iterator->second, inodeID);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::RemoveDirectoryMapping(uint32_t startCluster, ino_t inodeID)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::RemoveDirectoryMapping({}, {:x})", startCluster, inodeID);

    if ((!IsDataCluster(startCluster) && !IS_FIXED_ROOT(startCluster)) || !IS_DIR_CLUSTER_INODEID(inodeID) || inodeID == 0 || CLUSTER_OF_DIR_CLUSTER_INODEID(inodeID) != startCluster)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::RemoveDirectoryMapping(): invalid mapping from cluster {} to inode {:x}.", startCluster, inodeID);
        return false;
    }

    auto iterator = m_DirectoryMap.find(startCluster);
    if (iterator != m_DirectoryMap.end() && iterator->second == inodeID)
    {
        m_DirectoryMap.erase(iterator);
        return true;
    }
    return false;
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
