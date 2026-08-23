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

#pragma once

#include "Kernel/VFS/KFSVolume.h"
#include "Kernel/VFS/KBlockCache.h"
#include "Signals/SignalTarget.h"
#include "FATTable.h"
#include "FATInode.h"

namespace kernel
{
class FATFilesystem;    
class FATInode;

inline constexpr size_t FAT_VOLUME_LABEL_LENGTH = 11;
inline constexpr char FAT_NO_VOLUME_LABEL[FAT_VOLUME_LABEL_LENGTH + 1] = "NO NAME    ";

struct FATSuperBlock
{
    uint8_t  m_JmpBoot[3];        // 0x00
    uint8_t  m_OEMName[8];        // 0x03
    uint16_t m_BytesPerSector;    // 0x0b
    uint8_t  m_SectorsPerCluster; // 0x0d
    uint16_t m_ReservedSectors;   // 0x0e
    uint8_t  m_FATCount;          // 0x10
    
    uint16_t m_RootDirEntryCount16; // 0x11 Number of entries in the root directory. Only used for FAT16.
    uint16_t m_TotalSectorCount16;  // 0x13
    uint8_t  m_Media;               // 0x15
    uint16_t m_SectorsPerFAT16;     // 0x16
    uint16_t m_SectorsPerTrack;     // 0x18
    uint16_t m_HeadCount;           // 0x1A
    uint32_t m_HiddenSectorCount;   // 0x1C
    uint32_t m_TotalSectorCount32;  // 0x20
    
    union FSDependent
    {
        struct FAT16
        {
            uint8_t  m_DriveNumber;     // 0x24
            uint8_t  m_Reserved1;       // 0x25 Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
            uint8_t  m_BootSignature;   // 0x26 Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
            uint32_t m_VolumeID;        // 0x27
            uint8_t  m_VolumeLabel[FAT_VOLUME_LABEL_LENGTH]; // 0x2B
            uint8_t  m_FilesType[8];    // 0x36
            uint8_t  m_Reserved[28];    // 0x3E
        } __attribute__((packed)) FAT16;
        struct FAT32
        {
            uint32_t m_SectorsPerFAT;    // 0x24
            uint16_t m_ExtendedFlags;    // 0x28
            uint16_t m_FSVersion;        // 0x2A
            uint32_t m_RootDirectory;    // 0x2C
            uint16_t m_FSInfoSector;     // 0x30
            uint16_t m_BackupBootSector; // 0x32
            uint8_t  m_Reserved[12];     // 0x34
            uint8_t  m_DriveNumber;      // 0x40
            uint8_t  m_Reserved1;        // 0x41 Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
            uint8_t  m_BootSignature;    // 0x42 Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
            uint32_t m_VolumeID;         // 0x43
            uint8_t  m_VolumeLabel[FAT_VOLUME_LABEL_LENGTH]; // 0x47
            uint8_t  m_FilesType[8];     // 0x52
        } __attribute__((packed)) FAT32; // 54
    } __attribute__((packed)) m_FSDependent;
    
    uint8_t  m_Unused[420];
    uint16_t m_Signature; // 16 Bits Always 0xAA55    
} __attribute__((packed));
static_assert(sizeof(FATSuperBlock) == 512);

struct FATFSInfo
{
    uint32_t m_Signature1;           // 0-3     0x41615252 - the FSInfo signature
    uint8_t  m_Reserved1[480];       // 4-483   Reserved
    uint32_t m_Signature2;           // 484-487 0x61417272 - a second FSInfo signature
    uint32_t m_FreeClusters;         // 488-491 Free cluster count or 0xffffffff (may be incorrect)
    uint32_t m_LastAllocatedCluster; // 492-495 Last known cluster to be allocated or 0xffffffff (hint only)
    uint8_t  m_Reserved2[12];        // 496-507 Reserved
    uint32_t m_Signature3;           // 508-511 0xaa550000 - sector signature
} __attribute__((packed));

static_assert(sizeof(FATFSInfo) == 512);

class FATVolume : public KFSVolume, public SignalTarget
{
public:
    static const uint32_t MAGIC = 0x2ecf6059;
    static constexpr int32_t INVALID_VOLUME_LABEL_ENTRY = -1;

    class ModificationScope
    {
    public:
        explicit ModificationScope(FATVolume& volume);
        ~ModificationScope();

    private:
        FATVolume& m_Volume;
        bool       m_IsActive;

        ModificationScope(const ModificationScope&) = delete;
        ModificationScope& operator=(const ModificationScope&) = delete;
    };
    
    FATVolume(Ptr<FATFilesystem> filesystem, fs_id volumeID, const PString& devicePath);
    ~FATVolume();

    void ReadSuperBlock(int deviceFile);
    void UpdateFSInfo();

    void InitializeCleanFlagState(const FATVolumeStatus& volumeStatus);
    void StopCleanFlagUpdater();
    void FlushAndMarkClean();
    void MarkMetadataInconsistent() noexcept;

    void RegisterDeferredDeletion() noexcept;
    void CompleteDeferredDeletion(bool cleanupSucceeded) noexcept;

    void Shutdown();
    void FlushDirtyInodes();

    bool CheckMagic(const char* functionName);
    bool IsReadOnly() const { return HasFlag(FSVolumeFlags::FS_IS_READONLY) || m_BCache.IsReadOnly(); }
    
    bool IsDataCluster(uint32_t cluster) const { return cluster >= FATTable::FIRST_DATA_CLUSTER && cluster < (m_TotalClusters + FATTable::FIRST_DATA_CLUSTER); }
    
    Ptr<FATTable> GetFATTable() { return m_FATTable; }
    
    ino_t AllocUniqueInodeID();
    void  SetInodeIDToLocationIDMapping(ino_t inodeID, ino_t locationID);
    bool  RemoveInodeIDToLocationIDMapping(ino_t inodeID);
    bool  GetInodeIDToLocationIDMapping(ino_t inodeID, ino_t* locationID) const;
    bool  GetLocationIDToInodeIDMapping(ino_t locationID, ino_t* inodeID) const;

    bool  HasInodeIDToLocationIDMapping(ino_t inodeID) const;
    bool  HasLocationIDToInodeIDMapping(ino_t locationID) const;

    void  DumpInodeIDMap();
    
    bool  AddDirectoryMapping(uint32_t startCluster, ino_t inodeID);
    bool  RemoveDirectoryMapping(uint32_t startCluster, ino_t inodeID);
    ino_t GetDirectoryMapping(uint32_t startCluster) const;
    void  DumpDirectoryMap();

    void AddDirtyInode(FATInode* inode) noexcept;
    void RemoveDirtyInode(FATInode* inode) noexcept;
    
    uint32_t	   m_Magic;
    mutable KMutex m_Mutex;
    mutable KMutex m_InodeIDMapMutex;
    int		   m_DeviceFile = -1;       // Block-device file descriptor
    KBlockCache    m_BCache;
    Ptr<FATTable>  m_FATTable;
    
    struct InodeMapEntry {
        ino_t m_InodeID;
        ino_t m_LocationID;
    };
        
    
    std::map<ino_t, InodeMapEntry>  m_InodeToLocationMap;
    std::map<ino_t, InodeMapEntry*> m_LocationToInodeMap;
    ino_t                           m_CurrentArtificialID = ARTIFICIAL_INODEID_BITS;
    
    std::map<uint32_t, ino_t>       m_DirectoryMap;
    
      // info from bpb
    uint8_t	m_FATBits = 0;
    uint32_t	m_BytesPerSector = 0;
    uint32_t	m_ReservedSectors = 0;
    uint32_t	m_SectorsPerFAT = 0;
    uint32_t	m_SectorsPerCluster = 0;
    uint32_t	m_FATCount = 0;
    uint8_t	m_ActiveFAT = 0;
    uint32_t	m_RootEntriesCount = 0;
    uint32_t	m_TotalSectors = 0;
    uint8_t	m_MediaDescriptor = 0;
    uint16_t	m_FSInfoSector = 0;
    uint16_t	m_BackupBootSector = 0;

    uint32_t	m_TotalClusters = 0;      // data clusters, that is
    uint32_t	m_FreeClusters = 0;
    bool	m_FATMirrored = false;    // true if fat mirroring on

    uint32_t      m_RootStart = 0;        // for fat12 + fat16 only
    uint32_t	  m_RootSectorCount = 0;  // for fat12 + fat16 only
    Ptr<FATInode> m_RootInode;            // root directory
    bool          m_HasVolumeLabel = false;
    int32_t	      m_VolumeLabelEntry = INVALID_VOLUME_LABEL_ENTRY; // index in root directory
    char	      m_RawVolumeLabel[FAT_VOLUME_LABEL_LENGTH] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

    uint32_t	  m_FirstDataSector = 0;
    uint32_t      m_LastAllocatedCluster = 0; // last allocated cluster

private:
    using DirtyInodeList = PIntrusiveList<FATInode, &FATInode::m_DirtyListNode>;

    static constexpr TimeValNanos CLEAN_FLAG_UPDATE_DELAY = TimeValNanos::FromSeconds(5.0);

    bool BeginModification();
    void FinishModification() noexcept;
    void SlotBlockCacheReadOnly(PErrorCode error);

    void StartCleanFlagUpdater();
    static void* CleanFlagUpdaterEntry(void* argument);
    void* RunCleanFlagUpdater();

    void SyncCache();

    KConditionVariable m_CleanFlagCondition;
    DirtyInodeList m_DirtyInodes;
    thread_id       m_CleanFlagUpdaterThread = INVALID_HANDLE;
    TimeValNanos    m_CleanCheckpointDeadline = TimeValNanos::infinit;
    size_t          m_ActiveModificationCount = 0;
    size_t          m_DeferredDeletionCount = 0;
    bool            m_CanClearCleanFlag = false;
    bool            m_CanMarkCleanFlag = false;
    bool            m_IsVolumeMarkedClean = false;
    bool            m_StopCleanFlagUpdater = false;
};


} // kernel
