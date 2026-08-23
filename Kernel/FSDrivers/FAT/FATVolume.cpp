// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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

#include <Kernel/KTime.h>
#include <Kernel/KThread.h>
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

FATVolume::ModificationScope::ModificationScope(FATVolume& volume)
    : m_Volume(volume)
    , m_IsActive(volume.BeginModification())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATVolume::ModificationScope::~ModificationScope()
{
    if (m_IsActive) {
        m_Volume.FinishModification();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATVolume::FATVolume(Ptr<FATFilesystem> filesystem, fs_id volumeID, const PString& devicePath)
    : KFSVolume(volumeID, devicePath)
    , m_Mutex("fatfs_vol_mutex", PEMutexRecursionMode_RaiseError)
    , m_InodeIDMapMutex("fatfs_inodemap_mutex", PEMutexRecursionMode_RaiseError)
    , m_CleanFlagCondition("fat_clean_flag")
{
    m_Magic = MAGIC;

    m_BCache.SignalBecameReadOnly.Connect(this, &FATVolume::SlotBlockCacheReadOnly);
        
    m_RootInode = ptr_new<FATInode>(filesystem, ptr_tmp_cast(this), S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO);
    m_RootNode = m_RootInode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATVolume::~FATVolume()
{
    kassert(m_DirtyInodes.IsEmpty());
    kassert(m_CleanFlagUpdaterThread == INVALID_HANDLE);
    kassert(m_ActiveModificationCount == 0);
    kassert(m_DeferredDeletionCount == 0);
    m_Magic = ~MAGIC;
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

        m_BackupBootSector = superBlock->m_FSDependent.FAT32.m_BackupBootSector;
        if (m_BackupBootSector != 0 && m_BackupBootSector >= m_ReservedSectors)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): invalid backup boot sector ({}).", m_BackupBootSector);
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
        if ((rootDirectoryByteCount % m_BytesPerSector) != 0)
        {
            kernel_log<PLogSeverity::ERROR>(
                LogCat_FATFS,
                "FATFilesystem::Mount(): FAT12/16 root-directory entry count {} does not fill a whole number of sectors.",
                m_RootEntriesCount);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        m_FSInfoSector = 0xffff;
        m_BackupBootSector = 0;
        m_FATMirrored = true;
        m_ActiveFAT = 0;

        m_RootStart = m_ReservedSectors + m_FATCount * m_SectorsPerFAT;
        m_RootSectorCount = static_cast<uint32_t>(rootDirectorySectorCount);
        rootStartCluster = 1;
        rootEndCluster = 1;
        rootSize = m_RootSectorCount * m_BytesPerSector;
    }

    const uint8_t bootSignature = (m_FATBits == 32) ?
        superBlock->m_FSDependent.FAT32.m_BootSignature :
        superBlock->m_FSDependent.FAT16.m_BootSignature;
    const uint8_t* const volumeLabel = (m_FATBits == 32) ?
        superBlock->m_FSDependent.FAT32.m_VolumeLabel :
        superBlock->m_FSDependent.FAT16.m_VolumeLabel;
    if (bootSignature == 0x29 &&
        memcmp(volumeLabel, FAT_NO_VOLUME_LABEL, FAT_VOLUME_LABEL_LENGTH) != 0 &&
        memcmp(volumeLabel, "           ", FAT_VOLUME_LABEL_LENGTH) != 0)
    {
        memcpy(m_RawVolumeLabel, volumeLabel, FAT_VOLUME_LABEL_LENGTH);
        m_HasVolumeLabel = true;
    }

    m_RootInode->m_Size         = rootSize;
    m_RootInode->m_StartCluster = rootStartCluster;
    m_RootInode->m_EndCluster   = rootEndCluster;
    
    m_FATTable = ptr_new<FATTable>(ptr_tmp_cast(this)); // WARNING: Circular reference! Manually broken in Shutdown().
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::InitializeCleanFlagState(const FATVolumeStatus& volumeStatus)
{
    kassert(m_Mutex.IsLocked());
    kassert(m_CleanFlagUpdaterThread == INVALID_HANDLE);

    m_CanClearCleanFlag     = volumeStatus.IsSupported && !IsReadOnly();
    m_CanMarkCleanFlag      = m_CanClearCleanFlag && volumeStatus.IsClean;
    m_IsVolumeMarkedClean   = volumeStatus.IsSupported&& volumeStatus.IsClean;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::StopCleanFlagUpdater()
{
    thread_id updaterThread;
    {
        KScopedLock volumeLock(m_Mutex);

        updaterThread = m_CleanFlagUpdaterThread;
        if (updaterThread == INVALID_HANDLE) {
            return;
        }
        m_StopCleanFlagUpdater = true;
        m_CleanFlagCondition.WakeupAll();
    }

    kthread_join_trw(updaterThread);

    KScopedLock volumeLock(m_Mutex);
    kassert(m_CleanFlagUpdaterThread == updaterThread);
    m_CleanFlagUpdaterThread = INVALID_HANDLE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::FlushAndMarkClean()
{
    kassert(m_Mutex.IsLocked());

    const bool shouldMarkClean =
        m_CanMarkCleanFlag &&
        !m_IsVolumeMarkedClean &&
        m_ActiveModificationCount == 0 &&
        m_DeferredDeletionCount == 0;

    FlushDirtyInodes();
    UpdateFSInfo();
    SyncCache();

    if (shouldMarkClean)
    {
        m_FATTable->SetVolumeClean(true);
        SyncCache();
        m_IsVolumeMarkedClean = true;
        m_CleanCheckpointDeadline = TimeValNanos::infinit;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::MarkMetadataInconsistent() noexcept
{
    kassert(m_Mutex.IsLocked());

    m_CanMarkCleanFlag = false;
    m_CleanCheckpointDeadline = TimeValNanos::infinit;
    m_CleanFlagCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::RegisterDeferredDeletion() noexcept
{
    kassert(m_Mutex.IsLocked());
    ++m_DeferredDeletionCount;
    m_CleanFlagCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::CompleteDeferredDeletion(bool cleanupSucceeded) noexcept
{
    kassert(m_Mutex.IsLocked());
    kassert(m_DeferredDeletionCount != 0);

    if (m_DeferredDeletionCount != 0) {
        --m_DeferredDeletionCount;
    }

    if (!cleanupSucceeded)
    {
        MarkMetadataInconsistent();
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATVolume::CompleteDeferredDeletion(): disabling clean-flag updates after a deferred deletion failed.");
    }
    else if (
        m_CanMarkCleanFlag &&
        !m_IsVolumeMarkedClean &&
        m_DeferredDeletionCount == 0 &&
        m_ActiveModificationCount == 0)
    {
        m_CleanCheckpointDeadline = kget_monotonic_time() + CLEAN_FLAG_UPDATE_DELAY;
    }
    m_CleanFlagCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::Shutdown()
{
    kassert(m_CleanFlagUpdaterThread == INVALID_HANDLE);
    kassert(m_DirtyInodes.IsEmpty());
    m_FATTable = nullptr;
    m_BCache.SetDevice(-1, 0, 0);

    if (m_DeviceFile != -1)
    {
        kclose(m_DeviceFile);
        m_DeviceFile = -1;
    }

    if (m_RootInode != nullptr) {
        m_RootInode->Detach();
    }
    m_RootNode = nullptr;
    m_RootInode = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::FlushDirtyInodes()
{
    kassert(m_Mutex.IsLocked());

    const size_t dirtyInodeCount = m_DirtyInodes.GetCount();
    PErrorCode firstError = PErrorCode::Success;

    for (size_t dirtyInodeIndex = 0; dirtyInodeIndex < dirtyInodeCount; ++dirtyInodeIndex)
    {
        FATInode* dirtyInode = m_DirtyInodes.GetFirst();
        kassert(dirtyInode != nullptr);

        try
        {
            dirtyInode->Write();
            if (dirtyInode->IsMetadataDirty())
            {
                kernel_log<PLogSeverity::CRITICAL>(
                    LogCat_FATFS,
                    "FATVolume::FlushDirtyInodes(): inode {:x} remained dirty after a successful write.",
                    dirtyInode->m_InodeID);
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
        PERROR_CATCH([&firstError](PErrorCode error)
        {
            if (firstError == PErrorCode::Success) {
                firstError = error;
            }
        });

        if (dirtyInode->IsMetadataDirty())
        {
            kassert(dirtyInode->m_DirtyListNode.IsListMember(&m_DirtyInodes));
            m_DirtyInodes.Remove(dirtyInode);
            m_DirtyInodes.Append(dirtyInode);
        }
    }

    if (firstError != PErrorCode::Success) {
        PERROR_THROW_CODE(firstError);
    }
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
    KScopedLock inodeMapLock(m_InodeIDMapMutex);

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x})", inodeID, locationID);

    auto inodeIterator = m_InodeToLocationMap.find(inodeID);
    if (inodeIterator != m_InodeToLocationMap.end())
    {
        InodeMapEntry& entry = inodeIterator->second;
        auto oldLocationIterator = m_LocationToInodeMap.find(entry.m_LocationID);
        if (oldLocationIterator == m_LocationToInodeMap.end() || oldLocationIterator->second != &entry)
        {
            const ino_t currentOwner = (oldLocationIterator != m_LocationToInodeMap.end() && oldLocationIterator->second != nullptr) ? oldLocationIterator->second->m_InodeID : 0;
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): reverse map at existing location {:16x} is owned by {:16x}.", inodeID, locationID, entry.m_LocationID, currentOwner);
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        if (locationID != entry.m_LocationID)
        {
            auto newLocationIterator = m_LocationToInodeMap.find(locationID);
            if (newLocationIterator != m_LocationToInodeMap.end())
            {
                const ino_t currentOwner = (newLocationIterator->second != nullptr) ? newLocationIterator->second->m_InodeID : 0;
                kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): destination location is already owned by {:16x}.", inodeID, locationID, currentOwner);
                PERROR_THROW_CODE(PErrorCode::IO);
            }

            if (inodeID != locationID)
            {
                const auto insertionResult = m_LocationToInodeMap.try_emplace(locationID, &entry);
                if (!insertionResult.second)
                {
                    kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): failed to insert an unclaimed destination location.", inodeID, locationID);
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
                entry.m_LocationID = locationID;
                m_LocationToInodeMap.erase(oldLocationIterator);
            }
            else
            {
                m_LocationToInodeMap.erase(oldLocationIterator);
                m_InodeToLocationMap.erase(inodeIterator);
            }
        }
    }
    else
    {
        auto locationIterator = m_LocationToInodeMap.find(locationID);
        if (locationIterator != m_LocationToInodeMap.end())
        {
            const ino_t currentOwner = (locationIterator->second != nullptr) ? locationIterator->second->m_InodeID : 0;
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): destination location is already owned by {:16x}.", inodeID, locationID, currentOwner);
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        if (inodeID != locationID)
        {
            const auto forwardInsertionResult = m_InodeToLocationMap.try_emplace(inodeID, InodeMapEntry{inodeID, locationID});
            if (!forwardInsertionResult.second)
            {
                kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): inode appeared in the forward map during a locked update.", inodeID, locationID);
                PERROR_THROW_CODE(PErrorCode::IO);
            }
            PScopeFail rollbackForwardMapping([this, inodeID]()
            {
                m_InodeToLocationMap.erase(inodeID);
            });

            InodeMapEntry& entry = forwardInsertionResult.first->second;
            const auto reverseInsertionResult = m_LocationToInodeMap.try_emplace(locationID, &entry);
            if (!reverseInsertionResult.second)
            {
                kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::SetInodeIDToLocationIDMapping({:16x} -> {:16x}): location appeared in the reverse map during a locked update.", inodeID, locationID);
                PERROR_THROW_CODE(PErrorCode::IO);
            }
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

bool FATVolume::GetDirectoryStartCluster(ino_t inodeID, uint32_t* startCluster) const
{
    ino_t locationID = inodeID;
    if (IS_ARTIFICIAL_INODEID(locationID) && !GetInodeIDToLocationIDMapping(inodeID, &locationID)) {
        return false;
    }
    if (!IS_DIR_CLUSTER_INODEID(locationID)) {
        return false;
    }

    const uint32_t cluster = CLUSTER_OF_DIR_CLUSTER_INODEID(locationID);
    if (!IsDataCluster(cluster) && !IS_FIXED_ROOT(cluster)) {
        return false;
    }

    *startCluster = cluster;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::AddDirectoryMapping(uint32_t startCluster, ino_t inodeID)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATVolume::AddDirectoryMapping({}, {:x})", startCluster, inodeID);

    uint32_t inodeStartCluster;
    if ((!IsDataCluster(startCluster) && !IS_FIXED_ROOT(startCluster)) ||
        !GetDirectoryStartCluster(inodeID, &inodeStartCluster) ||
        inodeStartCluster != startCluster)
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

    if ((!IsDataCluster(startCluster) && !IS_FIXED_ROOT(startCluster)) || inodeID == 0 || IS_INVALID_INODEID(inodeID))
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::AddDirtyInode(FATInode* inode) noexcept
{
    kassert(m_Mutex.IsLocked());
    kassert(inode != nullptr);
    kassert(inode->m_Volume == this);
    kassert(!inode->m_DirtyListNode.IsListMember());

    m_DirtyInodes.Append(inode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::RemoveDirtyInode(FATInode* inode) noexcept
{
    kassert(m_Mutex.IsLocked());
    kassert(inode != nullptr);
    kassert(inode->m_Volume == this);

    if (inode->m_DirtyListNode.IsListMember())
    {
        kassert(inode->m_DirtyListNode.IsListMember(&m_DirtyInodes));
        m_DirtyInodes.Remove(inode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATVolume::BeginModification()
{
    kassert(m_Mutex.IsLocked());

    if (!m_CanClearCleanFlag) {
        return false;
    }

    if (m_CanMarkCleanFlag)
    {
        StartCleanFlagUpdater();
        m_CleanCheckpointDeadline = kget_monotonic_time() + CLEAN_FLAG_UPDATE_DELAY;
        m_CleanFlagCondition.WakeupAll();
    }

    if (m_IsVolumeMarkedClean)
    {
        m_FATTable->SetVolumeClean(false);
        SyncCache();
        m_IsVolumeMarkedClean = false;
    }

    if (!m_CanMarkCleanFlag) {
        return false;
    }

    ++m_ActiveModificationCount;
    m_CleanCheckpointDeadline = TimeValNanos::infinit;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::FinishModification() noexcept
{
    kassert(m_Mutex.IsLocked());
    kassert(m_ActiveModificationCount != 0);

    if (m_ActiveModificationCount != 0) {
        --m_ActiveModificationCount;
    }
    if (m_ActiveModificationCount == 0)
    {
        m_CleanCheckpointDeadline = kget_monotonic_time() + CLEAN_FLAG_UPDATE_DELAY;
        m_CleanFlagCondition.WakeupAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::SlotBlockCacheReadOnly(PErrorCode error)
{
    KScopedLock volumeLock(m_Mutex);

    if (!HasFlag(FSVolumeFlags::FS_IS_READONLY))
    {
        SetFlags(GetFlags() | uint32_t(FSVolumeFlags::FS_IS_READONLY));
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FAT volume {} became read-only after a device write failure: {}.", m_DevicePath.c_str(), p_strerror(error));
    }

    m_CanClearCleanFlag = false;
    m_CanMarkCleanFlag = false;
    m_IsVolumeMarkedClean = false;
    m_CleanCheckpointDeadline = TimeValNanos::infinit;

    while (m_DirtyInodes.GetFirst() != nullptr) {
        m_DirtyInodes.GetFirst()->DiscardPendingMetadata();
    }
    m_CleanFlagCondition.WakeupAll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::StartCleanFlagUpdater()
{
    kassert(m_Mutex.IsLocked());

    if (m_CleanFlagUpdaterThread == INVALID_HANDLE)
    {
        PThreadAttribs threadAttributes("fat_clean_flag", 0, PThreadDetachState_Joinable, 4096);
        m_CleanFlagUpdaterThread = kthread_spawn_trw(
            &threadAttributes,
            nullptr,
#ifdef PADOS_MODULE_USER_SPACE
            nullptr,
#endif // PADOS_MODULE_USER_SPACE
            KSpawnThreadFlag::Privileged,
            nullptr,
            CleanFlagUpdaterEntry,
            this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* FATVolume::CleanFlagUpdaterEntry(void* argument)
{
    return static_cast<FATVolume*>(argument)->RunCleanFlagUpdater();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* FATVolume::RunCleanFlagUpdater()
{
    KScopedLock volumeLock(m_Mutex);

    for (;;)
    {
        if (m_StopCleanFlagUpdater) {
            break;
        }

        if (
            !m_CanMarkCleanFlag ||
            m_IsVolumeMarkedClean ||
            m_ActiveModificationCount != 0 ||
            m_DeferredDeletionCount != 0)
        {
            m_CleanFlagCondition.Wait(m_Mutex);
            continue;
        }

        const TimeValNanos currentTime = kget_monotonic_time();
        if (currentTime < m_CleanCheckpointDeadline)
        {
            m_CleanFlagCondition.WaitDeadline(m_Mutex, m_CleanCheckpointDeadline);
            continue;
        }

        try {
            FlushAndMarkClean();
        }
        PERROR_CATCH(([this](PErrorCode error)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FAT clean-flag update failed with error {}. Retrying after the idle delay.", p_strerror(error));
            m_CleanCheckpointDeadline = kget_monotonic_time() + CLEAN_FLAG_UPDATE_DELAY;
        }));
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::SyncCache()
{
    kassert(m_Mutex.IsLocked());

    if (!m_BCache.Sync() || m_BCache.GetDeviceDirtyBlockCount() != 0) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
}


} // kernel
