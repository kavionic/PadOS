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
// Created: 18/05/19 18:04:07

#include <System/Platform.h>

#include <string_view>
#include <utility>
#include <string.h>
#include <fcntl.h>
#include <set>

#include <PadOS/DeviceControl.h>

#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/FSDrivers/FAT/FATFilesystem.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>
#include <Ptr/NoPtr.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KVFSManager.h>
#include <Storage/DirectoryEntry.h>

#include "FATVolume.h"
#include "FATInode.h"
#include "FATDirectoryNode.h"
#include "FATDirectoryIterator.h"
#include "FATFileNode.h"


#define FAT_MAX_FILE_SIZE 0xffffffffLL

namespace kernel
{

struct FATNewDirEntryInfo
{
    uint32_t     Cluster = 0;
    size_t       Size = 0;
    TimeValNanos CreateTime;
    TimeValNanos AccessTime;
    TimeValNanos ModificationTime;
    uint8_t      DOSAttribs = 0;
    uint8_t      ShortNameCaseFlags = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Short name cannot be any of the DOS/Win device names (list from wikipedia).
///////////////////////////////////////////////////////////////////////////////

static std::set<PString> g_DOSDeviceBaseNames =
{
    "CON",
    "PRN",
    "AUX",
    "CLOCK$",
    "NUL",
    "COM1",
    "COM2",
    "COM3",
    "COM4",
    "COM5",
    "COM6",
    "COM7",
    "COM8",
    "COM9",
    "LPT1",
    "LPT2",
    "LPT3",
    "LPT4",
    "LPT5",
    "LPT6",
    "LPT7",
    "LPT8",
    "LPT9",
    "LST",      // Only in 86-DOS and DOS 1.xx.
    "KEYBD$",   // Only in multitasking MS-DOS 4.0.
    "SCREEN$",  // Only in multitasking MS-DOS 4.0.
    "$IDLE$",   // Only in Concurrent DOS 386, Multiuser DOS and DR DOS 5.0 and higher.
    "CONFIG$"   // Only in MS-DOS 7.0-8.0.
};

static void ValidateFATNameBuffer(const char* name, int nameLength)
{
    if (name == nullptr || nameLength < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (size_t(nameLength) > FAT_LONG_NAME_MAX_UTF8_LENGTH) {
        PERROR_THROW_CODE(PErrorCode::NAMETOOLONG);
    }
}

static bool IsFATLongNameCharacterValid(uint32_t character)
{
    if (character < 0x20 || character == 0xfffe || character == 0xffff) {
        return false;
    }

    switch (character)
    {
        case '"':
        case '*':
        case '/':
        case ':':
        case '<':
        case '>':
        case '?':
        case '\\':
        case '|':
            return false;
        default:
            return true;
    }
}

static void ValidateNewFATName(const PString& name)
{
    if (name.empty() || name == "." || name == ".." || !name.is_valid_utf8()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (name.back() == ' ' || name.back() == '.') {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    size_t utf16Length = 0;
    for (PString::utf32_iterator iterator = name.utf32_begin(); iterator != name.utf32_end(); ++iterator)
    {
        const uint32_t character = *iterator;
        if (!IsFATLongNameCharacterValid(character)) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

        utf16Length += (character < 0x10000) ? 1 : 2;
        if (utf16Length > FAT_LONG_NAME_MAX_LENGTH) {
            PERROR_THROW_CODE(PErrorCode::NAMETOOLONG);
        }
    }
}

static void SetFATDirectoryEntryTimestamps(
    FATDirectoryEntry& entry,
    const TimeValNanos& createTime,
    const TimeValNanos& accessTime,
    const TimeValNanos& modificationTime)
{
    const uint32_t fatCreateTime = FATInode::UnixTimeToFATTime(createTime.AsSecondsI());
    const uint32_t fatAccessTime = FATInode::UnixTimeToFATTime(accessTime.AsSecondsI());
    const uint32_t fatModificationTime = FATInode::UnixTimeToFATTime(modificationTime.AsSecondsI());

    entry.m_CreateTimeFine = FATInode::TimeValToFATCreateTimeFine(createTime);
    entry.m_CreateTime = uint16_t(fatCreateTime & 0xffff);
    entry.m_CreateDate = uint16_t(fatCreateTime >> 16);
    entry.m_AccessDate = uint16_t(fatAccessTime >> 16);
    entry.m_ModificationTime = uint16_t(fatModificationTime & 0xffff);
    entry.m_ModificationDate = uint16_t(fatModificationTime >> 16);
}

static TimeValNanos FATTimeToTimeValOrFallback(uint32_t fatTime, uint8_t createTimeFine, const TimeValNanos& fallbackTime)
{
    if ((fatTime & 0xffff0000) == 0)
    {
        return fallbackTime;
    }
    return FATInode::FATTimeToTimeVal(fatTime, createTimeFine);
}

static void InitFATDirectoryEntry(
    FATDirectoryEntry& entry,
    const char shortName[11],
    uint8_t shortNameCaseFlags,
    uint8_t dosAttribs,
    uint32_t cluster,
    size_t size,
    const TimeValNanos& createTime,
    const TimeValNanos& accessTime,
    const TimeValNanos& modificationTime)
{
    memcpy(entry.m_Filename, shortName, sizeof(entry.m_Filename));
    entry.m_Attribs = dosAttribs;
    entry.m_ShortNameCaseFlags = shortNameCaseFlags;
    SetFATDirectoryEntryTimestamps(entry, createTime, accessTime, modificationTime);
    if (cluster == 0)
    {
        entry.m_FirstClusterLow = 0;
        entry.m_FirstClusterHigh = 0;
    }
    else
    {
        entry.m_FirstClusterLow = uint16_t(cluster & 0xffff);
        entry.m_FirstClusterHigh = uint16_t(cluster >> 16);
    }
    entry.m_FileSize = (dosAttribs & FAT_SUBDIR) ? 0 : uint32_t(size);
}

static void RestoreFATDirectoryEntriesNoThrow(
    Ptr<FATVolume> volume,
    uint32_t directoryCluster,
    uint32_t startIndex,
    const FATDirectoryEntryCombo* originalEntries,
    size_t savedEntryCount,
    size_t rollbackEntryCount) noexcept
{
    if (rollbackEntryCount != 0)
    {
        try
        {
            FATDirectoryIterator iterator(volume, directoryCluster, startIndex);
            for (size_t entryIndex = 0; entryIndex < rollbackEntryCount; ++entryIndex)
            {
                FATDirectoryEntryCombo* entry = iterator.GetCurrentEntry();
                if (entry == nullptr) {
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
                if (entryIndex < savedEntryCount) {
                    *entry = originalEntries[entryIndex];
                } else {
                    memset(entry, 0, sizeof(*entry));
                }
                iterator.MarkDirty();
                if (entryIndex + 1 < rollbackEntryCount && iterator.GetNextRawEntry() == nullptr) {
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
            }
        }
        catch (const std::exception& exception)
        {
            volume->MarkMetadataInconsistent();
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "RestoreFATDirectoryEntriesNoThrow(): failed to restore directory entries after an entry-creation error: {}", exception.what());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATFilesystem::FATFilesystem()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
PErrorCode FATFilesystem::Probe(const char* devicePath, fs_info* fsInfo)
{
    try
    {
        // Attempt to mount the volume as FAT without modifying it.
        Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(Mount(-1, devicePath, MOUNT_READ_ONLY, nullptr, 0));

        if (!vol->CheckMagic(__func__)) {
            return PErrorCode::INVAL;
        }


        fsInfo->fi_flags = vol->GetFlags() | uint32_t(FSVolumeFlags::FS_CAN_MOUNT);  // File system flags.
        fsInfo->fi_block_size = vol->m_BytesPerSector * vol->m_SectorsPerCluster; // FS block size.
        fsInfo->fi_io_size = 65536;                                               // IO size - specifies buffer size for file copying
        fsInfo->fi_total_blocks = vol->m_TotalClusters;                            // Total blocks
        fsInfo->fi_free_blocks = vol->m_FreeClusters;                              // Free blocks
        fsInfo->fi_free_user_blocks = fsInfo->fi_free_blocks;

        CopyVolumeLabelToFSInfo(*vol, fsInfo);
        Unmount(vol);

        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> FATFilesystem::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    device_geometry geo;

    uint32_t volumeFlags = uint32_t(FSVolumeFlags::FS_IS_PERSISTENT) | uint32_t(FSVolumeFlags::FS_IS_BLOCKBASED);

    // open read-only for now

    int deviceFile = kopen_trw(devicePath, O_RDONLY);

    PScopeFail deviceFileGuard([&deviceFile]() { kclose(deviceFile); });

    // get device characteristics
    const PErrorCode result = kdevice_control(deviceFile, DEVCTL_GET_DEVICE_GEOMETRY, nullptr, 0, &geo, sizeof(geo));
    if (result != PErrorCode::Success)
    {
        struct stat st;
        if ((kread_stat(deviceFile, &st) == PErrorCode::Success) && S_ISREG(st.st_mode))
        {
            // Support mounting disk images
            geo.bytes_per_sector = 512;
            geo.sector_count = st.st_size / 512;
            geo.read_only = !(st.st_mode & S_IWUSR);
            geo.removable = true;
        }
        else
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): failed getting device geometry.");
            PERROR_THROW_CODE(result);
        }
    }
    if ((geo.bytes_per_sector != 512) && (geo.bytes_per_sector != 1024) && (geo.bytes_per_sector != 2048)) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): unsupported device block size ({}).", geo.bytes_per_sector);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (geo.removable) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Mount(): {} is removable.", devicePath);
        volumeFlags |= uint32_t(FSVolumeFlags::FS_IS_REMOVABLE);
    }
    if (geo.read_only)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Mount(): {} is on read-only media.", devicePath);
        volumeFlags |= uint32_t(FSVolumeFlags::FS_IS_READONLY);
    }
    else
    {
        if ((flags & MOUNT_READ_ONLY) != 0)
        {
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Mount(): {} mounted read-only.", devicePath);
            volumeFlags |= uint32_t(FSVolumeFlags::FS_IS_READONLY);
        }

        // reopen it with read/write permissions
        kclose(deviceFile);
        deviceFile = kopen_trw(devicePath, O_RDWR);
        if (deviceFile < 0) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount() unable to reopen {} ({}).", devicePath, strerror(get_last_error()));
            PERROR_THROW_CODE(PErrorCode(get_last_error()));
        }
    }


    Ptr<FATVolume>  vol = ptr_new<FATVolume>(ptr_tmp_cast(this), volumeID, devicePath);

    PScopeFail volumeShutdownGuard([&vol]()
    {
        vol->Shutdown();
    });

    vol->ReadSuperBlock(deviceFile);

    vol->SetFlags(volumeFlags);

    // Check that the partition is large enough to contain the file system.

    const uint64_t volumeByteCount = uint64_t(vol->m_TotalSectors) * vol->m_BytesPerSector;
    const uint64_t deviceByteCount = uint64_t(geo.sector_count) * geo.bytes_per_sector;
    if (volumeByteCount > deviceByteCount)
    {
        kernel_log<PLogSeverity::ERROR>(
            LogCat_FATFS,
            "FATFilesystem::Mount(): volume extends past end of partition ({} bytes > {} bytes).",
            volumeByteCount,
            deviceByteCount);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    // Perform sanity checks on the FAT.

    std::vector<uint8_t> buffer;
    buffer.resize(512);

    // the media descriptor in active FAT should match the one in the BPB
    if (kpread_trw(deviceFile, buffer.data(), buffer.size(), vol->m_BytesPerSector * (vol->m_ReservedSectors + vol->m_ActiveFAT * vol->m_SectorsPerFAT)) != buffer.size()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): error reading FAT.");
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (buffer[0] != vol->m_MediaDescriptor) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): media descriptor mismatch ({:x} != {:x}).", buffer[0], vol->m_MediaDescriptor);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (vol->m_FATMirrored)
    {
        uint32_t i;
        std::vector<uint8_t> buffer2;
        buffer2.resize(512);

        for (i = 0; i < vol->m_FATCount; ++i)
        {
            if (i != vol->m_ActiveFAT)
            {
                kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Mount(): checking fat #{}", i);
                buffer2[0] = uint8_t(~buffer[0]);
                if (kpread_trw(deviceFile, buffer2.data(), buffer2.size(), vol->m_BytesPerSector * (vol->m_ReservedSectors + vol->m_SectorsPerFAT * i)) != buffer2.size()) {
                    kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): error reading FAT {}", i);
                    PERROR_THROW_CODE(PErrorCode::IO);
                }

                if (buffer2[0] != vol->m_MediaDescriptor) {
                    kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): media descriptor mismatch in fat # {} ({:x} != {:x})", i, buffer2[0], vol->m_MediaDescriptor);
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
                // checking for exact matches of fats is too restrictive; allow these to go through in case the fat is corrupted for some reason
                if (memcmp(buffer.data(), buffer2.data(), buffer.size()) != 0) {
                    kernel_log<PLogSeverity::WARNING>(LogCat_FATFS, "FATFilesystem::Mount(): fat {} doesn't match active fat ({}).", i, vol->m_ActiveFAT);
                }
            }
        }
    }

    // Now we are convinced of the drive is valid.

    vol->m_LastAllocatedCluster = FATTable::FIRST_DATA_CLUSTER;

    if (!vol->m_BCache.SetDevice(deviceFile, vol->m_TotalSectors, vol->m_BytesPerSector, geo.read_only)) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): error initializing block cache ({}).", strerror(get_last_error()));
        PERROR_THROW_CODE(PErrorCode(get_last_error()));
    }

    const FATVolumeStatus volumeStatus = vol->GetFATTable()->ReadVolumeStatus();
    const bool canTrustFSInfo = !volumeStatus.IsSupported || (volumeStatus.IsClean && !volumeStatus.HasHardError);

    if (volumeStatus.IsSupported)
    {
        if (!volumeStatus.IsClean) {
            kernel_log<PLogSeverity::WARNING>(LogCat_FATFS, "FATFilesystem::Mount(): volume was not unmounted cleanly and may require repair.");
        }
        if (volumeStatus.HasHardError) {
            kernel_log<PLogSeverity::WARNING>(LogCat_FATFS, "FATFilesystem::Mount(): volume reports a previous disk I/O error and may require repair.");
        }
    }

    {
        KScopedLock volumeLock(vol->m_Mutex);
        vol->InitializeCleanFlagState(volumeStatus);
    }

    if (!canTrustFSInfo && vol->m_FSInfoSector != 0xffff) {
        kernel_log<PLogSeverity::WARNING>(LogCat_FATFS, "FATFilesystem::Mount(): ignoring FSInfo allocation hints because the recorded volume state is unsafe.");
    }

    bool isFreeClustersValid = false;
    if (vol->m_FSInfoSector != 0xffff)
    {
        KCacheBlockDesc bufferDesc = vol->m_BCache.GetBlock_trw(vol->m_FSInfoSector);
        FATFSInfo* fsInfo = static_cast<FATFSInfo*>(bufferDesc.m_Buffer);
        if (fsInfo != nullptr)
        {
            if (fsInfo->m_Signature1 == 0x41615252 && fsInfo->m_Signature2 == 0x61417272 && fsInfo->m_Signature3 == 0xaa550000)
            {
                if (canTrustFSInfo)
                {
                    const uint32_t freeClusterCount = fsInfo->m_FreeClusters;
                    const uint32_t lastAllocatedCluster = fsInfo->m_LastAllocatedCluster;
                    if (freeClusterCount <= vol->m_TotalClusters)
                    {
                        vol->m_FreeClusters = freeClusterCount;
                        isFreeClustersValid = true;
                    }
                    else if (freeClusterCount != 0xffffffff)
                    {
                        kernel_log<PLogSeverity::WARNING>(LogCat_FATFS, "FATFilesystem::Mount(): FSInfo free-cluster count {} exceeds volume cluster count {}.", freeClusterCount, vol->m_TotalClusters);
                    }

                    if (vol->IsDataCluster(lastAllocatedCluster)) {
                        vol->m_LastAllocatedCluster = lastAllocatedCluster;
                    } else if (lastAllocatedCluster != 0xffffffff) {
                        kernel_log<PLogSeverity::WARNING>(LogCat_FATFS, "FATFilesystem::Mount(): ignoring invalid FSInfo next-free hint {}.", lastAllocatedCluster);
                    }
                }
            }
            else
            {
                uint32_t signature1 = fsInfo->m_Signature1;
                uint32_t signature2 = fsInfo->m_Signature2;
                uint32_t signature3 = fsInfo->m_Signature3;
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::Mount(): fsinfo block has invalid magic number {:08x}, {:08x}, {:08x}", signature1, signature2, signature3);
            }
        }
        else
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::Mount(): error getting fsinfo sector {:x}", vol->m_FSInfoSector);
        }
    }

    if (!isFreeClustersValid) {
        vol->m_FreeClusters = vol->GetFATTable()->CountFreeClusters();
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "mounting {} (id {:x}, device {:x}, media descriptor {:x})", vol->m_DevicePath.c_str(), vol->m_VolumeID, deviceFile, vol->m_MediaDescriptor);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{:x} bytes/sector, {:x} sectors/cluster", vol->m_BytesPerSector, vol->m_SectorsPerCluster);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{:x} reserved sectors, {:x} total sectors", vol->m_ReservedSectors, vol->m_TotalSectors);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{:x} {}-bit fats, {:x} sectors/fat, {:x} root entries", vol->m_FATCount, vol->m_FATBits, vol->m_SectorsPerFAT, vol->m_RootEntriesCount);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "root directory starts at sector {:x} (cluster {:x}), data at sector {:x}", vol->m_RootStart, vol->m_RootInode->m_StartCluster, vol->m_FirstDataSector);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "{:x} total clusters, {:x} free", vol->m_TotalClusters, vol->m_FreeClusters);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "fat mirroring is {}, fs info sector at sector {:x}", (vol->m_FATMirrored) ? "on" : "off", vol->m_FSInfoSector);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "last allocated cluster = {:x}", vol->m_LastAllocatedCluster);

    // initialize root inode
    vol->m_RootInode->m_InodeID = vol->m_RootInode->m_ParentInodeID = GENERATE_DIR_CLUSTER_INODEID(vol->m_RootInode->m_StartCluster, vol->m_RootInode->m_StartCluster);
    vol->m_RootInode->m_DirStartIndex = 0xffffffff;
    vol->m_RootInode->m_DirEndIndex = 0xffffffff;
    vol->m_RootInode->m_DOSAttribs  = FAT_SUBDIR;
    const TimeValNanos currentTime = get_real_time();
    vol->m_RootInode->m_ATime = FATInode::RoundTimeToFATAccessTime(currentTime);
    vol->m_RootInode->m_CTime = FATInode::RoundTimeToFATCreateTime(currentTime);
    vol->m_RootInode->m_MTime = FATInode::RoundTimeToFATModificationTime(currentTime);
    if (!vol->AddDirectoryMapping(vol->m_RootInode->m_StartCluster, vol->m_RootInode->m_InodeID)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    // find volume label (supersedes any label in the bpb)
    {
        FATDirectoryIterator diri(vol, vol->m_RootInode->m_StartCluster, 0);
        uint32_t traversedClusterCount = 1;
        for (;;)
        {
            const FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();
            if (buffer == nullptr) {
                break;
            }
            if (buffer->m_Normal.m_Filename[0] == 0) {
                break;
            }
            if ((buffer->m_Normal.m_Attribs & FAT_VOLUME) &&
                ((buffer->m_Normal.m_Attribs & FAT_LONG_NAME_ATTRIBUTE_MASK) != FAT_LONG_NAME_ATTRIBUTES) &&
                (buffer->m_Normal.m_Filename[0] != 0xe5))
            {
                vol->m_VolumeLabelEntry = diri.m_CurrentIndex;
                memcpy(vol->m_VolumeLabel, buffer->m_Normal.m_Filename, sizeof(buffer->m_Normal.m_Filename));
                break;
            }
            const uint32_t previousSector = diri.m_SectorIterator.m_CurrentSector;
            if (diri.GetNextRawEntry() == nullptr) {
                break;
            }
            if (!IS_FIXED_ROOT(diri.m_StartingCluster) &&
                previousSector + 1 == vol->m_SectorsPerCluster &&
                diri.m_SectorIterator.m_CurrentSector == 0)
            {
                ++traversedClusterCount;
                if (traversedClusterCount > vol->m_TotalClusters)
                {
                    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::Mount(): circular root-directory cluster chain detected.");
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
            }
        }
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Mount(): Root inode ID = {:x}.", vol->m_RootInode->m_InodeID);
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Mount(): Volume label [{:11.11}] ({}).", vol->m_VolumeLabel, vol->m_VolumeLabelEntry);

    vol->m_DeviceFile = deviceFile;
    return vol;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Unmount(Ptr<KFSVolume> volume)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    {
        KScopedLock volumeLock(vol->m_Mutex);
        vol->FlushDirtyInodes();
    }

    KVFSManager::DetachVolume_trw(vol);
    vol->StopCleanFlagUpdater();
    PScopeExit volumeShutdownGuard([&vol]()
    {
        vol->Shutdown();
    });

    CRITICAL_SCOPE(vol->m_Mutex);

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Unmount(): {:x}", vol->m_VolumeID);

    vol->FlushAndMarkClean();
    if (!vol->m_BCache.Shutdown(true)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Sync(Ptr<KFSVolume> _vol)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    
    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::Sync() called on volume {:x}", vol->m_VolumeID);

    vol->FlushAndMarkClean();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ReadFSStat(Ptr<KFSVolume> _vol, fs_info* fss)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::ReadFSStat() called.");

    // fss->dev and fss->root filled in by kernel
    
    // File system flags.
    fss->fi_flags = vol->GetFlags();
    
    // FS block size.
    fss->fi_block_size = vol->m_BytesPerSector * vol->m_SectorsPerCluster;

    // IO size - specifies buffer size for file copying
    fss->fi_io_size = 65536;
    
    // Total blocks
    fss->fi_total_blocks = vol->m_TotalClusters;

    // Free blocks
    fss->fi_free_blocks = vol->m_FreeClusters;
    fss->fi_free_user_blocks = fss->fi_free_blocks;

    // Device name.
    //	strncpy(fss->device_name, vol->device, sizeof(fss->device_name));

    CopyVolumeLabelToFSInfo(*vol, fss);

    // File system name
    //	strcpy(fss->fsh_name, "fat");
    size_t devPathLen = std::min(sizeof(fss->fi_device_path) - 1, vol->m_DevicePath.size());
    vol->m_DevicePath.copy(fss->fi_device_path, devPathLen);
    fss->fi_device_path[devPathLen] = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::WriteFSStat(Ptr<KFSVolume> _vol, const fs_info* fss, uint32_t mask)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::WriteFSStat() called.");

    if (vol->IsReadOnly()) {
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }

    if (mask & WFSSTAT_NAME)
    {
        char sanitizedName[FAT_VOLUME_LABEL_LENGTH];
        static constexpr char acceptableCharacters[] = "!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~ ";

        size_t inputNameLength = 0;
        while (inputNameLength < sizeof(fss->fi_volume_name) && fss->fi_volume_name[inputNameLength] != '\0') {
            ++inputNameLength;
        }
        const std::string_view inputName(fss->fi_volume_name, inputNameLength);

        memset(sanitizedName, ' ', sizeof(sanitizedName));
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::WriteFSStat(): setting name to '{}'.", inputName);

        size_t outputIndex = 0;
        for (size_t inputIndex = 0; outputIndex < FAT_VOLUME_LABEL_LENGTH && inputIndex < inputNameLength; ++inputIndex)
        {
            char character = fss->fi_volume_name[inputIndex];
            if ((character >= 'a') && (character <= 'z')) {
                character = char(character + ('A' - 'a'));
            }
            if (strchr(acceptableCharacters, character) != nullptr) {
                sanitizedName[outputIndex++] = character;
            }
        }
        if (outputIndex == 0) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::WriteFSStat(): sanitized to [{:11.11}].", sanitizedName);

        if (memcmp(sanitizedName, vol->m_VolumeLabel, FAT_VOLUME_LABEL_LENGTH) == 0) {
            return;
        }

        FATVolume::ModificationScope modificationScope(*vol);

        if (vol->m_VolumeLabelEntry == -1)
        {
            // Stored in the BPB.
            KCacheBlockDesc primaryBufferDesc = vol->m_BCache.GetBlock_trw(0);
            FATSuperBlock* primarySuperBlock = static_cast<FATSuperBlock*>(primaryBufferDesc.m_Buffer);
            if (primarySuperBlock == nullptr) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }

            const bool isFAT32 = vol->m_FATBits == 32;
            uint8_t* const primaryVolumeLabel = isFAT32 ?
                primarySuperBlock->m_FSDependent.FAT32.m_VolumeLabel :
                primarySuperBlock->m_FSDependent.FAT16.m_VolumeLabel;
            const uint8_t primaryBootSignature = isFAT32 ?
                primarySuperBlock->m_FSDependent.FAT32.m_BootSignature :
                primarySuperBlock->m_FSDependent.FAT16.m_BootSignature;

            if (primarySuperBlock->m_Signature != 0xaa55 ||
                primaryBootSignature != 0x29 ||
                memcmp(primaryVolumeLabel, vol->m_VolumeLabel, FAT_VOLUME_LABEL_LENGTH) != 0)
            {
                kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::WriteFSStat(): label mismatch.");
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }

            KCacheBlockDesc backupBufferDesc;
            uint8_t* backupVolumeLabel = nullptr;
            if (vol->m_BackupBootSector != 0)
            {
                KCacheBlockDesc loadedBackupBufferDesc = vol->m_BCache.GetBlock_trw(vol->m_BackupBootSector);
                FATSuperBlock* backupSuperBlock = static_cast<FATSuperBlock*>(loadedBackupBufferDesc.m_Buffer);
                if (backupSuperBlock == nullptr ||
                    backupSuperBlock->m_Signature != 0xaa55 ||
                    backupSuperBlock->m_BytesPerSector != vol->m_BytesPerSector ||
                    backupSuperBlock->m_FSDependent.FAT32.m_BootSignature != 0x29 ||
                    backupSuperBlock->m_FSDependent.FAT32.m_VolumeID != primarySuperBlock->m_FSDependent.FAT32.m_VolumeID ||
                    backupSuperBlock->m_FSDependent.FAT32.m_RootDirectory != primarySuperBlock->m_FSDependent.FAT32.m_RootDirectory ||
                    backupSuperBlock->m_FSDependent.FAT32.m_FSInfoSector != vol->m_FSInfoSector ||
                    backupSuperBlock->m_FSDependent.FAT32.m_BackupBootSector != vol->m_BackupBootSector)
                {
                    kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::WriteFSStat(): backup boot sector mismatch.");
                    PERROR_THROW_CODE(PErrorCode::INVAL);
                }
                backupVolumeLabel = backupSuperBlock->m_FSDependent.FAT32.m_VolumeLabel;
                backupBufferDesc = std::move(loadedBackupBufferDesc);
            }

            memcpy(primaryVolumeLabel, sanitizedName, FAT_VOLUME_LABEL_LENGTH);
            primaryBufferDesc.MarkDirty();
            if (backupVolumeLabel != nullptr)
            {
                memcpy(backupVolumeLabel, sanitizedName, FAT_VOLUME_LABEL_LENGTH);
                backupBufferDesc.MarkDirty();
            }
        }
        else if (vol->m_VolumeLabelEntry >= 0)
        {
            FATDirectoryIterator diri(vol, vol->m_RootInode->m_StartCluster, vol->m_VolumeLabelEntry);
            FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();

            // check if it is the same as the old volume label
            if (buffer == nullptr || memcmp(buffer->m_Normal.m_Filename, vol->m_VolumeLabel, FAT_VOLUME_LABEL_LENGTH) != 0)
            {
                kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::WriteFSStat(): label mismatch.");
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            memcpy(buffer->m_Normal.m_Filename, sanitizedName, FAT_VOLUME_LABEL_LENGTH);
            diri.MarkDirty();
        }
        else
        {
            const uint32_t index = CreateVolumeLabel(vol, sanitizedName);
            vol->m_VolumeLabelEntry = index;
        }
        memcpy(vol->m_VolumeLabel, sanitizedName, FAT_VOLUME_LABEL_LENGTH);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> FATFilesystem::LocateInode(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength)
{
    // Starting at the base, find file in the subdir, and return path string and inode id of file.
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATInode>  dir = ptr_static_cast<FATInode>(parent);
    PString        file;

    ValidateFATNameBuffer(name, nameLength);
    file.assign(name, nameLength);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::LocateInode(): find {:x}/{}", dir->m_InodeID, file.c_str());

    const Ptr<FATInode> inode = DoLocateInode(vol, dir, file);
    if (inode == nullptr)
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::LocateInode(): Error finding inode ID for file {} ({}).", file.c_str(), strerror(get_last_error()));
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ReleaseInode(KInode* inode)
{
    Ptr<FATVolume> vol  = ptr_static_cast<FATVolume>(inode->m_Volume);
    FATInode*      node = static_cast<FATInode*>(inode);
    
    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (node->IsDeleted() || node->IsMetadataDirty())
    {
        KScopedLock volumeLock(vol->m_Mutex);

        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::ReleaseInode({:x})", node->m_InodeID);

        PScopeExit discardPendingMetadata([node]()
        {
            node->DiscardPendingMetadata();
        });

        if (node->IsDeleted())
        {
            PScopeExit removeMappings([&vol, node]()
            {
                if (vol->HasInodeIDToLocationIDMapping(node->m_InodeID)) {
                    vol->RemoveInodeIDToLocationIDMapping(node->m_InodeID);
                }
                if (node->IsDirectory() && !vol->RemoveDirectoryMapping(node->m_StartCluster, node->m_InodeID)) {
                    kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::ReleaseInode(): failed to remove directory mapping for inode {:x}.", node->m_InodeID);
                }
            });

            if (vol->m_BCache.IsReadOnly())
            {
                vol->CompleteDeferredDeletion(false);
            }
            else
            {
                FATVolume::ModificationScope modificationScope(*vol);
                bool deletionCompleted = false;
                PScopeExit completeDeferredDeletion([&vol, &deletionCompleted]()
                {
                    vol->CompleteDeferredDeletion(deletionCompleted);
                });

                if (node->m_StartCluster == 0)
                {
                    if (node->m_Size != 0 || node->IsDirectory())
                    {
                        kernel_log<PLogSeverity::ERROR>(
                            LogCat_FATFS,
                            "FATFilesystem::ReleaseInode(): inode {:x} has no start cluster (size {}, directory {}).",
                            node->m_InodeID,
                            node->m_Size,
                            node->IsDirectory());
                        PERROR_THROW_CODE(PErrorCode::IO);
                    }
                }
                else
                {
                    if (!vol->IsDataCluster(node->m_StartCluster))
                    {
                        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::ReleaseInode(): invalid start cluster {}.", node->m_StartCluster);
                        PERROR_THROW_CODE(PErrorCode::IO);
                    }
                    vol->GetFATTable()->ClearFATChain(node->m_StartCluster);
                }
                deletionCompleted = true;
            }
        }
        else if (!vol->m_BCache.IsReadOnly())
        {
            node->Write();
        }
    }
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFS, "FATFilesystem::ReleaseInode() (inode ID {:x}).", node->m_InodeID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> FATFilesystem::OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> _node, int openFlags)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATInode>  node = ptr_static_cast<FATInode>(_node);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::OpenFile(): inode ID {:x}, openFlags {:x}", node->m_InodeID, openFlags);

    if (openFlags & O_CREAT)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::OpenFile(): called with O_CREAT.");
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    const bool writeAccessRequested = (openFlags & O_ACCMODE) != O_RDONLY;
    const bool truncateRequested = (openFlags & O_TRUNC) != 0;

    if (vol->IsReadOnly() && (writeAccessRequested || truncateRequested))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::OpenFile(): write access requested on read-only volume.");
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }
    if (node->IsDirectory() && (writeAccessRequested || truncateRequested))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::OpenFile(): write access requested on directory.");
        PERROR_THROW_CODE(PErrorCode::ISDIR);
    }
    if ((node->m_DOSAttribs & FAT_READ_ONLY) && (writeAccessRequested || truncateRequested))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::OpenFile(): write access requested on read-only file.");
        PERROR_THROW_CODE(PErrorCode::PERM);
    }
    if (truncateRequested && !writeAccessRequested)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::OpenFile(): O_TRUNC specified without write access.");
        PERROR_THROW_CODE(PErrorCode::PERM);
    }

    if (truncateRequested)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::OpenFile() called with O_TRUNC set.");
        FATVolume::ModificationScope modificationScope(*vol);
        ResizeFile(vol, node, 0, true);
    }

    Ptr<FATFileNode> fileNode = ptr_new<FATFileNode>(openFlags);

    fileNode->m_FATIteration  = node->m_Iteration;
    fileNode->m_FATChainIndex = 0;
    fileNode->m_CachedCluster = node->m_StartCluster;

    return fileNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> FATFilesystem::CreateFile(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* _name, int nameLength, int openFlags, int perms)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATInode>  dir = ptr_static_cast<FATInode>(parent);

    PString name;

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    ValidateFATNameBuffer(_name, nameLength);
    name.assign(_name, nameLength);
    ValidateNewFATName(name);
    
    CRITICAL_SCOPE(vol->m_Mutex);

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::CreateFile() called: {:x}/{} perms={:o} openFlags={:o}", dir->m_InodeID, name.c_str(), perms, openFlags);

    if (vol->IsReadOnly()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CreateFile() called on read-only volume.");
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }

    if (dir->IsDeleted()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CreateFile() called in removed directory.");
        PERROR_THROW_CODE(PErrorCode::PERM);
    }

    uint8_t dosAttribs = FAT_ARCHIVE;
    if ((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
        dosAttribs |= FAT_READ_ONLY;
    }
    const mode_t fileMode = DOSAttribsToFileMode(dosAttribs);

    Ptr<FATFileNode> fileNode;

    Ptr<FATInode> file = DoLocateInode(vol, dir, name);
    if (file != nullptr)
    {
        if (openFlags & O_EXCL) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CreateFile() with O_EXCL called on existing file {}.", name.c_str());
            PERROR_THROW_CODE(PErrorCode::EXIST);
        }
        if (file->IsDirectory()) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CreateFile() called on existing directory.");
            PERROR_THROW_CODE(PErrorCode::PERM);
        }

        const bool writeAccessRequested = (openFlags & O_ACCMODE) != O_RDONLY;
        const bool truncateRequested = (openFlags & O_TRUNC) != 0;

        if ((file->m_DOSAttribs & FAT_READ_ONLY) && (writeAccessRequested || truncateRequested))
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CreateFile(): write access requested on existing read-only file.");
            PERROR_THROW_CODE(PErrorCode::PERM);
        }
        if (truncateRequested && !writeAccessRequested)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CreateFile(): O_TRUNC specified without write access.");
            PERROR_THROW_CODE(PErrorCode::PERM);
        }
        if (truncateRequested)
        {
            FATVolume::ModificationScope modificationScope(*vol);
            ResizeFile(vol, file, 0, true);
        }
        fileNode = ptr_new<FATFileNode>(openFlags);
    }
    else
    {
        FATVolume::ModificationScope modificationScope(*vol);
        NoPtr<FATInode> dummyObj(ptr_tmp_cast(this), vol, fileMode); // Used only to create directory entry
        Ptr<FATInode> dummy(dummyObj);
        
        dummy->m_ParentInodeID = dir->m_InodeID;
        dummy->m_StartCluster = 0;
        dummy->m_EndCluster = 0;
        dummy->m_DOSAttribs = dosAttribs;
        dummy->m_Size = 0;
        const TimeValNanos currentTime = get_real_time();
        dummy->m_ATime = FATInode::RoundTimeToFATAccessTime(currentTime);
        dummy->m_CTime = FATInode::RoundTimeToFATCreateTime(currentTime);
        dummy->m_MTime = FATInode::RoundTimeToFATModificationTime(currentTime);

        CreateDirectoryEntry(vol, dir, dummy, name, nullptr, &dummy->m_DirStartIndex, &dummy->m_DirEndIndex);

        dummy->m_InodeID = GENERATE_DIR_INDEX_INODEID(dummy->m_ParentInodeID, dummy->m_DirStartIndex);
        if (vol->HasInodeIDToLocationIDMapping(dummy->m_InodeID))
        {
            dummy->m_InodeID = vol->AllocUniqueInodeID();
            vol->SetInodeIDToLocationIDMapping(dummy->m_InodeID, GENERATE_DIR_INDEX_INODEID(dummy->m_ParentInodeID, dummy->m_DirStartIndex));
        }
        ino_t inodeID = dummy->m_InodeID;

        file = ptr_static_cast<FATInode>(KVFSManager::GetInode_trw(vol->m_VolumeID, inodeID, false));
        fileNode = ptr_new<FATFileNode>(openFlags);
        fileNode->SetInode(file);
    }

    fileNode->m_FATIteration  = file->m_Iteration;
    fileNode->m_FATChainIndex = 0;
    fileNode->m_CachedCluster = file->m_StartCluster;
    
    return fileNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    Ptr<FATVolume>   vol      = ptr_static_cast<FATVolume>(volume);
    FATFileNode*     fileNode = static_cast<FATFileNode*>(file);
    Ptr<FATInode>    node     = ptr_static_cast<FATInode>(file->GetInode());

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !fileNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::CloseFile() (inode ID {:x}).", node->m_InodeID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> FATFilesystem::LoadInode(Ptr<KFSVolume> volume, ino_t inodeID)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    ino_t loc;
    ino_t parentInodeID;

    char reenter = vol->m_Mutex.IsLocked();
    CRITICAL_SCOPE(vol->m_Mutex, !reenter);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATFilesystem::LoadInode() (inode ID {:x}).", inodeID);

    if (inodeID == vol->m_RootInode->m_InodeID)
    {
	    return vol->m_RootInode;
    }

    if (!vol->GetInodeIDToLocationIDMapping(inodeID, &loc)) {
	loc = inodeID;
    }
    if (IS_ARTIFICIAL_INODEID(loc) || IS_INVALID_INODEID(loc)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::LoadInode(): unknown inode ID {:x} (loc {:x}).", inodeID, loc);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    parentInodeID = vol->GetDirectoryMapping(DIR_OF_INODEID(loc));
    if (parentInodeID == -1)
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::LoadInode(): unknown directory at cluster {:x}.", DIR_OF_INODEID(loc));
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    FATDirectoryIterator iter(vol, DIR_OF_INODEID(loc), IS_DIR_CLUSTER_INODEID(loc) ? 0 : INDEX_OF_DIR_INDEX_INODEID(loc));
    if (iter.GetCurrentEntry() == nullptr) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::LoadInode(): error initializing directory for inode {:x} (loc {:x}).", inodeID, loc);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
	
    FATDirectoryEntryInfo info;
    for (;;)
    {
        if (!iter.GetNextLFNEntry(&info, nullptr))
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::LoadInode(): error finding inode {:x} (loc {:x}) ({}).", inodeID, loc, strerror(get_last_error()));
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        if (IS_DIR_CLUSTER_INODEID(loc))
        {
            if (info.m_StartCluster == CLUSTER_OF_DIR_CLUSTER_INODEID(loc)) {
                break;
            }
        }
        else
        {
            if (info.m_StartIndex == INDEX_OF_DIR_INDEX_INODEID(loc)) {
                break;
            }
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::LoadInode(): error finding inode {:x} (loc {:x}) ({}).", inodeID, loc, strerror(get_last_error()));
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }
    
    Ptr<FATInode> entry = ptr_new<FATInode>(ptr_tmp_cast(this), vol, DOSAttribsToFileMode(info.m_DOSAttribs));

    entry->m_InodeID = inodeID;
    entry->m_ParentInodeID = parentInodeID;
    entry->m_DirStartIndex = info.m_StartIndex;
    entry->m_DirEndIndex  = info.m_EndIndex;
    entry->m_StartCluster = info.m_StartCluster;
    entry->m_DOSAttribs   = info.m_DOSAttribs;
    entry->m_Size = entry->IsDirectory() ? 0 : info.m_Size;
    if (entry->m_StartCluster == 0)
    {
        if ((info.m_DOSAttribs & FAT_SUBDIR) || entry->m_Size != 0)
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::LoadInode(): inode {:x} has no cluster chain for its type or size.", inodeID);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }
    else if (!vol->IsDataCluster(entry->m_StartCluster))
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::LoadInode(): inode {:x} has invalid start cluster {}.", inodeID, entry->m_StartCluster);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    const TimeValNanos modificationTime = FATTimeToTimeValOrFallback(info.m_FATModificationTime, 0, TimeValNanos::zero);
    entry->m_CTime = FATTimeToTimeValOrFallback(info.m_FATCreateTime, info.m_FATCreateTimeFine, modificationTime);
    entry->m_ATime = FATTimeToTimeValOrFallback(info.m_FATAccessTime, 0, modificationTime);
    entry->m_MTime = modificationTime;
    return entry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KDirectoryNode> FATFilesystem::OpenDirectory(Ptr<KFSVolume> volume, Ptr<KInode> _node)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATInode>  node = ptr_static_cast<FATInode>(_node);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::OpenDirectory (inode ID {:x}).", node->m_InodeID);

    if (!node->IsDirectory())
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "FATFilesystem::OpenDirectory() ERROR: inode not a directory.");
        PERROR_THROW_CODE(PErrorCode::NOTDIR);
    }

    Ptr<FATDirectoryNode> dirNode = ptr_new<FATDirectoryNode>(O_RDONLY);
    dirNode->m_CurrentIndex = 0;
    return dirNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CreateDirectory(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* _name, int nameLength, int perms)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATInode>  dir = ptr_static_cast<FATInode>(parent);
    PString name;

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__))
    {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (dir->IsDeleted())
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::CreateDirectory() called in removed directory.");
        PERROR_THROW_CODE(PErrorCode::PERM);
    }
    ValidateFATNameBuffer(_name, nameLength);
    name.assign(_name, nameLength);
    ValidateNewFATName(name);

    CRITICAL_SCOPE(vol->m_Mutex);

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::CreateDirectory() called: {:x}/{} (perm {:o})", dir->m_InodeID, name.c_str(), perms);

    if (!dir->IsDirectory())
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::CreateDirectory(): inode ID {:x} is not a directory.", dir->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::NOTDIR);
    }

    if (vol->IsReadOnly())
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::CreateDirectory() called on read-only volume.");
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }

    FATVolume::ModificationScope modificationScope(*vol);

    std::vector<uint8_t> buffer;
    buffer.resize(vol->m_BytesPerSector);
    

    uint8_t dosAttribs = FAT_SUBDIR;
    if ((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
        dosAttribs |= FAT_READ_ONLY;
    }
    const mode_t fileMode = DOSAttribsToFileMode(dosAttribs);
    /* only used to create directory entry */
    NoPtr<FATInode> dummyObj(ptr_tmp_cast(this), vol, fileMode); /* used only to create directory entry */
    Ptr<FATInode> dummy(dummyObj);
    dummy->m_ParentInodeID = dir->m_InodeID;
    dummy->m_StartCluster = vol->GetFATTable()->AllocateClusters(1);

    PScopeFail scopeCleanupFATChain([&vol, &dummy]()
    {
        vol->GetFATTable()->ClearFATChainAfterFailureNoThrow(dummy->m_StartCluster, "FATFilesystem::CreateDirectory()");
    });

    dummy->m_EndCluster = dummy->m_StartCluster;
    dummy->m_AllocatedClusterCount = 1;
    dummy->m_DOSAttribs = dosAttribs;
    dummy->m_Size = vol->m_BytesPerSector * vol->m_SectorsPerCluster;
    const TimeValNanos currentTime = get_real_time();
    dummy->m_ATime = FATInode::RoundTimeToFATAccessTime(currentTime);
    dummy->m_CTime = FATInode::RoundTimeToFATCreateTime(currentTime);
    dummy->m_MTime = FATInode::RoundTimeToFATModificationTime(currentTime);

    dummy->m_InodeID = GENERATE_DIR_CLUSTER_INODEID(dummy->m_ParentInodeID, dummy->m_StartCluster);
    if(vol->HasInodeIDToLocationIDMapping(dummy->m_InodeID))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::CreateDirectory(): already have ID->location mapping for inode {:x}.", dummy->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (vol->HasLocationIDToInodeIDMapping(dummy->m_InodeID))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::CreateDirectory(): already have location->ID mapping for inode {:x}.", dummy->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (!vol->AddDirectoryMapping(dummy->m_StartCluster, dummy->m_InodeID)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    PScopeFail scopeCleanupDirMapping([&vol, &dummy]() { vol->RemoveDirectoryMapping(dummy->m_StartCluster, dummy->m_InodeID); });

    // create '.' and '..' entries and then end of directories
    memset(buffer.data(), 0, buffer.size());
    char currentDirectoryName[11];
    char parentDirectoryName[11];
    memset(currentDirectoryName, ' ', sizeof(currentDirectoryName));
    memset(parentDirectoryName, ' ', sizeof(parentDirectoryName));
    currentDirectoryName[0] = '.';
    parentDirectoryName[0] = '.';
    parentDirectoryName[1] = '.';

    FATDirectoryEntry* currentDirectoryEntry = reinterpret_cast<FATDirectoryEntry*>(buffer.data());
    FATDirectoryEntry* parentDirectoryEntry = reinterpret_cast<FATDirectoryEntry*>(buffer.data() + sizeof(FATDirectoryEntry));

    InitFATDirectoryEntry(
        *currentDirectoryEntry,
        currentDirectoryName,
        0,
        FAT_SUBDIR,
        dummy->m_StartCluster,
        size_t(dummy->m_Size),
        dummy->m_CTime,
        dummy->m_ATime,
        dummy->m_MTime);

    // root directory is always denoted by cluster 0, even for fat32 (!)
    uint32_t parentCluster = 0;
    if (dir->m_InodeID != vol->m_RootInode->m_InodeID)
    {
        parentCluster = dir->m_StartCluster;
    }
    InitFATDirectoryEntry(
        *parentDirectoryEntry,
        parentDirectoryName,
        0,
        FAT_SUBDIR,
        parentCluster,
        size_t(dir->m_Size),
        dir->m_CTime,
        dir->m_ATime,
        dir->m_MTime);

    FATClusterSectorIterator csi(vol, dummy->m_StartCluster, 0);
    csi.WriteBlock(buffer.data());

    // clear out rest of cluster to keep scandisk happy
    memset(buffer.data(), 0, buffer.size());

    for (size_t sectorIndex = 1; sectorIndex < vol->m_SectorsPerCluster; ++sectorIndex)
    {
        if (!csi.Increment(1)) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        KCacheBlockDesc blockDesc = csi.GetBlock_(false);
        if (blockDesc.m_Buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        memset(blockDesc.m_Buffer, 0, vol->m_BytesPerSector);
        blockDesc.MarkDirty();
    }

    // Publishing the parent entry commits the new directory after its contents
    // are fully initialized.
    CreateDirectoryEntry(vol, dir, dummy, name, nullptr, &dummy->m_DirStartIndex, &dummy->m_DirEndIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Rename(Ptr<KFSVolume> inputVolume, Ptr<KInode> inputOldDirectory, const char* oldNameBuffer, int oldNameLength, Ptr<KInode> inputNewDirectory, const char* newNameBuffer, int newNameLength, bool mustBeDirectory)
{
    Ptr<FATVolume> volume = ptr_static_cast<FATVolume>(inputVolume);
    Ptr<FATInode> oldDirectory = ptr_static_cast<FATInode>(inputOldDirectory);
    Ptr<FATInode> newDirectory = ptr_static_cast<FATInode>(inputNewDirectory);
    Ptr<FATInode> sourceNode;
    Ptr<FATInode> destinationNode;
    
    uint32_t newStartIndex = 0;
    uint32_t newEndIndex = 0;
    PString oldName;
    PString newName;

    ValidateFATNameBuffer(oldNameBuffer, oldNameLength);
    ValidateFATNameBuffer(newNameBuffer, newNameLength);
    oldName.assign(oldNameBuffer, oldNameLength);
    newName.assign(newNameBuffer, newNameLength);
    ValidateNewFATName(newName);

    if (oldName == "." || oldName == ".." || newName == "." || newName == "..") {
        PERROR_THROW_CODE(PErrorCode::PERM);
    }

    CRITICAL_SCOPE(volume->m_Mutex);

    if (!volume->CheckMagic(__func__) || !oldDirectory->CheckMagic(__func__) || !newDirectory->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::Rename() called: {:x}/{}->{:x}/{}", oldDirectory->m_InodeID, oldName.c_str(), newDirectory->m_InodeID, newName.c_str());

    if (volume->IsReadOnly()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Rename(): called on read-only volume.");
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }
    
    // locate the file
    sourceNode = DoLocateInode(volume, oldDirectory, oldName);
    if (sourceNode == nullptr) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::Rename(): can't find file {} in directory {:x}.", oldName.c_str(), oldDirectory->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }

    if (!sourceNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (mustBeDirectory && !sourceNode->IsDirectory()) {
        PERROR_THROW_CODE(PErrorCode::NOTDIR);
    }

    if (sourceNode->IsDirectory() && IsDirectoryAncestor(volume, sourceNode, newDirectory)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    
    // See if the destination already exists.
    destinationNode = DoLocateInode(volume, newDirectory, newName);
    if (destinationNode != nullptr)
    {
        if (!destinationNode->CheckMagic(__func__)) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        if (destinationNode->m_InodeID == sourceNode->m_InodeID) {
            return;
        }

        if (destinationNode->IsDirectory())
        {
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::Rename(): destination already occupied by a directory.");
            PERROR_THROW_CODE(PErrorCode::PERM);
        }
        if (sourceNode->IsDirectory()) {
            PERROR_THROW_CODE(PErrorCode::NOTDIR);
        }

        newStartIndex = destinationNode->m_DirStartIndex;
        newEndIndex = destinationNode->m_DirEndIndex;
    }

    FATVolume::ModificationScope modificationScope(*volume);

    ino_t originalSourceLocationID;
    if (!volume->GetInodeIDToLocationIDMapping(sourceNode->m_InodeID, &originalSourceLocationID)) {
        originalSourceLocationID = sourceNode->m_InodeID;
    }

    ino_t originalDestinationLocationID = 0;
    if (destinationNode != nullptr && !volume->GetInodeIDToLocationIDMapping(destinationNode->m_InodeID, &originalDestinationLocationID)) {
        originalDestinationLocationID = destinationNode->m_InodeID;
    }

    FATDirectoryEntry savedDestinationEntry = {};
    bool destinationEntryCreated = false;
    bool destinationEntryReplaced = false;
    bool sourceMappingChangeAttempted = false;
    bool destinationMappingChangeAttempted = false;
    bool directoryParentEntryUpdated = false;

    PScopeFail rollbackRename([&]()
    {
        if (directoryParentEntryUpdated)
        {
            try {
                UpdateDirectoryParentEntry(volume, sourceNode, oldDirectory);
            } catch (const std::exception& exception) {
                volume->MarkMetadataInconsistent();
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::Rename(): failed to restore the source directory's '..' entry: {}", exception.what());
            }
        }

        if (destinationEntryCreated)
        {
            try {
                EraseDirectoryEntry(volume, newDirectory->m_StartCluster, newStartIndex, newEndIndex);
            } catch (const std::exception& exception) {
                volume->MarkMetadataInconsistent();
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::Rename(): failed to remove the new directory entry during rollback: {}", exception.what());
            }
        }
        else if (destinationEntryReplaced)
        {
            try
            {
                FATDirectoryIterator iterator(volume, newDirectory->m_StartCluster, newEndIndex);
                FATDirectoryEntryCombo* entry = iterator.GetCurrentEntry();
                if (entry == nullptr) {
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
                entry->m_Normal = savedDestinationEntry;
                iterator.MarkDirty();
            }
            catch (const std::exception& exception)
            {
                volume->MarkMetadataInconsistent();
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::Rename(): failed to restore the overwritten destination entry: {}", exception.what());
            }
        }

        if (sourceMappingChangeAttempted)
        {
            try {
                volume->SetInodeIDToLocationIDMapping(sourceNode->m_InodeID, originalSourceLocationID);
            } catch (const std::exception& exception) {
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::Rename(): failed to restore the source inode mapping: {}", exception.what());
            }
        }
        if (destinationMappingChangeAttempted)
        {
            try {
                volume->SetInodeIDToLocationIDMapping(destinationNode->m_InodeID, originalDestinationLocationID);
            } catch (const std::exception& exception) {
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::Rename(): failed to restore the destination inode mapping: {}", exception.what());
            }
        }
    });

    if (destinationNode != nullptr)
    {
        FATDirectoryIterator iterator(volume, newDirectory->m_StartCluster, newEndIndex);
        FATDirectoryEntryCombo* entry = iterator.GetCurrentEntry();
        if (entry == nullptr)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::Rename(): failed to open the destination directory entry.");
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        savedDestinationEntry = entry->m_Normal;
        InitFATDirectoryEntry(
            entry->m_Normal,
            savedDestinationEntry.m_Filename,
            savedDestinationEntry.m_ShortNameCaseFlags,
            sourceNode->m_DOSAttribs,
            sourceNode->m_StartCluster,
            size_t(sourceNode->m_Size),
            sourceNode->m_CTime,
            sourceNode->m_ATime,
            sourceNode->m_MTime);
        iterator.MarkDirty();
        destinationEntryReplaced = true;
    }
    else
    {
        FATInode* collisionExclusion = (oldDirectory->m_InodeID == newDirectory->m_InodeID) ? ptr_raw_pointer_cast(sourceNode) : nullptr;
        CreateDirectoryEntry(volume, newDirectory, sourceNode, newName, collisionExclusion, &newStartIndex, &newEndIndex);
        destinationEntryCreated = true;
    }

    const ino_t newLocationID = volume->IsDataCluster(sourceNode->m_StartCluster)
        ? GENERATE_DIR_CLUSTER_INODEID(newDirectory->m_InodeID, sourceNode->m_StartCluster)
        : GENERATE_DIR_INDEX_INODEID(newDirectory->m_InodeID, newStartIndex);

    if (destinationNode != nullptr)
    {
        destinationMappingChangeAttempted = true;
        volume->SetInodeIDToLocationIDMapping(destinationNode->m_InodeID, volume->AllocUniqueInodeID());
    }
    sourceMappingChangeAttempted = true;
    volume->SetInodeIDToLocationIDMapping(sourceNode->m_InodeID, newLocationID);

    if (sourceNode->IsDirectory() && oldDirectory->m_InodeID != newDirectory->m_InodeID)
    {
        UpdateDirectoryParentEntry(volume, sourceNode, newDirectory);
        directoryParentEntryUpdated = true;
    }

    // Removing the old entry commits the rename. EraseDirectoryEntry() restores
    // a partially erased entry before propagating an error.
    EraseDirectoryEntry(volume, oldDirectory->m_StartCluster, sourceNode->m_DirStartIndex, sourceNode->m_DirEndIndex);

    sourceNode->m_ParentInodeID = newDirectory->m_InodeID;
    sourceNode->m_DirStartIndex = newStartIndex;
    sourceNode->m_DirEndIndex = newEndIndex;
    sourceNode->DiscardPendingMetadata();

    if (destinationNode != nullptr)
    {
        // ReleaseInode() clears the replaced file's chain after its final open
        // handle is closed.
        destinationNode->DiscardPendingMetadata();
        destinationNode->SetDeletedFlag(true);
        volume->RegisterDeferredDeletion();
    }

    CompactDirectoryNoThrow(volume, oldDirectory);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Unlink(Ptr<KFSVolume> vol, Ptr<KInode> dir, const char* _name, int nameLength)
{
    PString name;

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::Unlink() called.");
    
    ValidateFATNameBuffer(_name, nameLength);
    name.assign(_name, nameLength);

    DoUnlink(vol,dir,name,true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::RemoveDirectory(Ptr<KFSVolume> vol, Ptr<KInode> dir, const char* _name, int nameLength)
{
    PString name;
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::RemoveDirectory() called.");

    ValidateFATNameBuffer(_name, nameLength);
    name.assign(_name, nameLength);

    DoUnlink(vol, dir, name, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATFilesystem::Read(Ptr<KFileNode> file, void* buf, size_t len, off64_t pos)
{
    Ptr<FATInode>    node = ptr_static_cast<FATInode>(file->GetInode());
    Ptr<FATVolume>   vol = ptr_static_cast<FATVolume>(node->m_Volume);
    Ptr<FATFileNode> fileNode = ptr_static_cast<FATFileNode>(file);
    size_t bytes_read = 0;
    uint32_t cluster1;
    off64_t diff;

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !fileNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (node->IsDirectory()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Read() called on directory {:x}.", node->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::ISDIR);
    }

    kernel_log<PLogSeverity::INFO_FLOODING>(LogCat_FATFILE, "FATFilesystem::Read() called {} bytes at {} (inode ID {:x}).", len, pos, node->m_InodeID);

    if (pos < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    if ((node->m_Size == 0) || (len == 0) || (pos >= node->m_Size)) {
        return 0;
    }
    if (buf == nullptr) {
        PERROR_THROW_CODE(PErrorCode::FAULT);
    }

    const size_t availableBytes = size_t(node->m_Size - pos);
    if (len > availableBytes) {
        len = availableBytes;
    }

    if ((fileNode->m_FATIteration == node->m_Iteration) && (pos >= fileNode->m_FATChainIndex * vol->m_BytesPerSector * vol->m_SectorsPerCluster))
    {
        // The cached fat value is both valid and helpful.
        if (!vol->IsDataCluster(fileNode->m_CachedCluster))
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Read() invalid m_CachedCluster {} on inode {:x}.", fileNode->m_CachedCluster, node->m_InodeID);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
#ifdef FAT_VERIFY_FAT_CHAINS
        kassert(vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, fileNode->m_FATChainIndex, fileNode->m_CachedCluster));
#endif // FAT_VERIFY_FAT_CHAINS
        cluster1 = fileNode->m_CachedCluster;
        diff = pos - fileNode->m_FATChainIndex * vol->m_BytesPerSector * vol->m_SectorsPerCluster;
    }
    else
    {
        // the fat chain changed, so we have to start from the beginning
        cluster1 = node->m_StartCluster;
        diff = pos;
    }
    diff /= vol->m_BytesPerSector; // convert to sectors

    FATClusterSectorIterator iter(vol, cluster1, 0);

    if (diff != 0)
    {
        if (!iter.Increment(int(diff))) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }

#ifdef FAT_VERIFY_FAT_CHAINS
    kassert(vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, uint32_t(pos / vol->m_BytesPerSector / vol->m_SectorsPerCluster), iter.m_CurrentCluster));
#endif // FAT_VERIFY_FAT_CHAINS

    if ((pos % vol->m_BytesPerSector) != 0)
    {
        // read in partial first sector if necessary
        size_t amt;
        KCacheBlockDesc buffer = iter.GetBlock_(true);
        if (buffer.m_Buffer == nullptr) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Read(): error reading cluster {}, sector {}.", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        amt = size_t(vol->m_BytesPerSector - (pos % vol->m_BytesPerSector));
        if (amt > len) amt = len;
        memcpy(buf, static_cast<uint8_t*>(buffer.m_Buffer) + (pos % vol->m_BytesPerSector), amt);
        bytes_read += amt;

        if (bytes_read < len)
        {
            if (!iter.Increment(1)) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
    }

    // read middle sectors
    while (bytes_read + vol->m_BytesPerSector <= len)
    {
        iter.ReadBlock((uint8_t*)buf + bytes_read);
        bytes_read += vol->m_BytesPerSector;

        if (bytes_read < len)
        {
            if (!iter.Increment(1)) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
    }

    // read part of remaining sector if needed
    if (bytes_read < len) {
        size_t amt;

        KCacheBlockDesc buffer = iter.GetBlock_(true);
        if (buffer.m_Buffer == nullptr)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Read(): error reading cluster {}, sector {}.", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        amt = len - bytes_read;
        memcpy((uint8_t*)buf + bytes_read, buffer.m_Buffer, amt);
        bytes_read += amt;
    }

    if (len)
    {
        fileNode->m_FATIteration = node->m_Iteration;
        fileNode->m_FATChainIndex = uint32_t((pos + len - 1) / vol->m_BytesPerSector / vol->m_SectorsPerCluster);
        fileNode->m_CachedCluster = iter.m_CurrentCluster;
#ifdef FAT_VERIFY_FAT_CHAINS
        kassert(vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, fileNode->m_FATChainIndex, fileNode->m_CachedCluster));
#endif // FAT_VERIFY_FAT_CHAINS
    }
    return bytes_read;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATFilesystem::Write(Ptr<KFileNode> file, const void* buf, size_t len, off64_t pos)
{
    Ptr<FATInode>    node = ptr_static_cast<FATInode>(file->GetInode());
    Ptr<FATVolume>   vol = ptr_static_cast<FATVolume>(node->m_Volume);
    Ptr<FATFileNode> fileNode = ptr_static_cast<FATFileNode>(file);

    size_t   bytesWritten = 0;
    off64_t  diff;

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !fileNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (node->IsDirectory()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Write() called on directory {:x}.", node->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::ISDIR);
    }

    kernel_log<PLogSeverity::INFO_FLOODING>(LogCat_FATFILE, "FATFilesystem::Write() called {} bytes at {} from buffer at {:x} (inode ID {:x}).", len, pos, (intptr_t)buf, node->m_InodeID);

    if ((fileNode->GetOpenFlags() & O_ACCMODE) == O_RDONLY) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Write(): called on file opened as read-only.");
        PERROR_THROW_CODE(PErrorCode::PERM);
    }
    if (vol->IsReadOnly())
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Write(): called on read-only volume.");
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }

    if (pos < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    if (fileNode->GetOpenFlags() & O_APPEND) {
        pos = node->m_Size;
    }

    if (len == 0) {
        return 0;
    }
    if (buf == nullptr) {
        PERROR_THROW_CODE(PErrorCode::FAULT);
    }

    if (pos >= FAT_MAX_FILE_SIZE)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Write(): write position exceeds fat limits.");
        PERROR_THROW_CODE(PErrorCode::FBIG);
    }

    const off64_t maximumWriteLength = FAT_MAX_FILE_SIZE - pos;
    if (len > size_t(maximumWriteLength)) {
        len = size_t(maximumWriteLength);
    }

    FATVolume::ModificationScope modificationScope(*vol);

    const off64_t oldFileSize = node->m_Size;
    const off64_t writeEnd = pos + off64_t(len);
    const uint32_t requiredClusterCount = GetFileClusterCount(vol, writeEnd);

    bool modificationMetadataUpdated = false;
    PScopeFail markPartiallyWrittenFileModified([&node, &bytesWritten, &modificationMetadataUpdated]()
    {
        if (bytesWritten != 0 && !modificationMetadataUpdated) {
            node->MarkContentsModified();
        }
    });

    uint32_t cluster1;

    if (node->m_Size && (fileNode->m_FATIteration == node->m_Iteration) && (pos >= fileNode->m_FATChainIndex * vol->m_BytesPerSector * vol->m_SectorsPerCluster))
    {
        if (!vol->IsDataCluster(fileNode->m_CachedCluster))
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Write() invalid m_CachedCluster {} on inode {:x}.", fileNode->m_CachedCluster, node->m_InodeID);
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }

#ifdef FAT_VERIFY_FAT_CHAINS
        kassert(vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, fileNode->m_FATChainIndex, fileNode->m_CachedCluster));
#endif // FAT_VERIFY_FAT_CHAINS
        cluster1 = fileNode->m_CachedCluster;
        diff = pos - fileNode->m_FATChainIndex * vol->m_BytesPerSector * vol->m_SectorsPerCluster;
    }
    else
    {
        cluster1 = 0xffffffff;
        diff = 0;
    }

    if (writeEnd > oldFileSize)
    {
        if (requiredClusterCount > GetFileClusterCount(vol, oldFileSize))
        {
            EnsureClusterChainMetadataLoaded(vol, node);
            if (requiredClusterCount > node->m_AllocatedClusterCount) {
                vol->GetFATTable()->SetChainLength(node, requiredClusterCount, true);
            }
        }
        if (pos > oldFileSize) {
            ClearFileRange(vol, node, oldFileSize, pos);
        }
    }

    if (cluster1 == 0xffffffff) {
        cluster1 = node->m_StartCluster;
        diff = pos;
    }
    diff /= vol->m_BytesPerSector; // Convert to sectors.

    FATClusterSectorIterator iter(vol, cluster1, 0);

    if (diff != 0)
    {
        if (!iter.Increment(int(diff))) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }

#ifdef FAT_VERIFY_FAT_CHAINS
    kassert(vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, uint32_t(pos / vol->m_BytesPerSector / vol->m_SectorsPerCluster), iter.m_CurrentCluster));
#endif // FAT_VERIFY_FAT_CHAINS

    // Write partial first sector if necessary
    if ((pos % vol->m_BytesPerSector) != 0)
    {
        KCacheBlockDesc buffer = iter.GetBlock_(true);
        if (buffer.m_Buffer == nullptr)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Write(): error writing cluster {}, sector {}.", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        size_t amt = size_t(vol->m_BytesPerSector - (pos % vol->m_BytesPerSector));
        if (amt > len) amt = len;
        memcpy(static_cast<uint8_t*>(buffer.m_Buffer) + (pos % vol->m_BytesPerSector), buf, amt);
        if (iter.MarkBlockDirty() != PErrorCode::Success) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        bytesWritten += amt;

        if (bytesWritten < len)
        {
            if (!iter.Increment(1)) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
    }

    // write middle sectors
    while (bytesWritten + vol->m_BytesPerSector <= len)
    {
        iter.WriteBlock(static_cast<const uint8_t*>(buf) + bytesWritten);
        bytesWritten += vol->m_BytesPerSector;

        if (bytesWritten < len)
        {
            if (!iter.Increment(1)) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
    }

    // write part of remaining sector if needed
    if (bytesWritten < len)
    {
        size_t amt;

        KCacheBlockDesc buffer = iter.GetBlock_(true);
        if (buffer.m_Buffer == nullptr) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::Write(): error writing cluster {}, sector {}.", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        amt = len - bytesWritten;
        memcpy(buffer.m_Buffer, (uint8_t*)buf + bytesWritten, amt);
        if (iter.MarkBlockDirty() != PErrorCode::Success) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        bytesWritten += amt;
    }

    node->MarkContentsModified();
    modificationMetadataUpdated = true;

    if (writeEnd > oldFileSize)
    {
        PScopeFail restoreFileSize([&node, oldFileSize]()
        {
            node->m_Size = oldFileSize;
        });

        node->m_Size = writeEnd;
        node->Write();
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::Write(): setting file size to {} ({} clusters).", node->m_Size, requiredClusterCount);
    }

    if (len)
    {
        fileNode->m_FATIteration = node->m_Iteration;
        fileNode->m_FATChainIndex = uint32_t((pos + len - 1) / vol->m_BytesPerSector / vol->m_SectorsPerCluster);
        fileNode->m_CachedCluster = iter.m_CurrentCluster;

#ifdef FAT_VERIFY_FAT_CHAINS
        kassert(vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, fileNode->m_FATChainIndex, fileNode->m_CachedCluster));
#endif // FAT_VERIFY_FAT_CHAINS
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATFilesystem::ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, void* buffer, size_t bufferSize)
{
    PDirEntryWriter entryWriter(buffer, bufferSize);
    if (!entryWriter.IsValid() || bufferSize < PGetDirEntryRecordSize(0)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    Ptr<FATVolume>        vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATDirectoryNode> dirNode = ptr_static_cast<FATDirectoryNode>(directory);
    Ptr<FATInode>         dir = ptr_static_cast<FATInode>(directory->GetInode());

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__) || !dirNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::ReadDirectory(): inode ID {:x}, index {}.", dir->m_InodeID, dirNode->m_CurrentIndex);

    const bool isRootDirectory = dir->m_InodeID == vol->m_RootInode->m_InodeID;

    while (isRootDirectory && dirNode->m_CurrentIndex < 2)
    {
        const char* name = (dirNode->m_CurrentIndex == 0) ? "." : "..";
        const size_t nameLength = dirNode->m_CurrentIndex + 1;
        dirent_t* entry = entryWriter.AddEntry(name, nameLength);
        if (entry == nullptr)
        {
            if (entryWriter.GetBytesWritten() == 0) {
                PERROR_THROW_CODE(PErrorCode::NAMETOOLONG);
            }
            return entryWriter.GetBytesWritten();
        }

        entry->d_type = DT_DIR;
        entry->d_ino = vol->m_RootInode->m_InodeID;
        entry->d_volumeid = vol->m_VolumeID;
        dirNode->m_CurrentIndex++;
    }

    const uint32_t iteratorIndex = static_cast<uint32_t>(
        dirNode->m_CurrentIndex - (isRootDirectory ? 2 : 0));
    FATDirectoryIterator directoryIterator(vol, dir->m_StartCluster, iteratorIndex);

    for (;;)
    {
        if (entryWriter.GetRemainingSize() < PGetDirEntryRecordSize(0)) {
            break;
        }

        PString fileName;
        ino_t inodeID;
        uint32_t dosAttributes = 0;
        if (!directoryIterator.GetNextDirectoryEntry(dir, &inodeID, &fileName, &dosAttributes))
        {
            dirNode->m_CurrentIndex = directoryIterator.m_CurrentIndex + (isRootDirectory ? 2 : 0);
            break;
        }

        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::ReadDirectory(): found file '{}' / {}.", fileName.c_str(), fileName.size());

        dirent_t* entry = entryWriter.AddEntry(fileName.c_str(), fileName.size());
        if (entry == nullptr)
        {
            if (entryWriter.GetBytesWritten() == 0) {
                PERROR_THROW_CODE(PErrorCode::NAMETOOLONG);
            }
            break;
        }

        entry->d_ino = inodeID;
        entry->d_type = ((dosAttributes & FAT_SUBDIR) != 0) ? DT_DIR : DT_REG;
        entry->d_volumeid = vol->m_VolumeID;
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::ReadDirectory(): found file '{}'.", entry->d_name);
        dirNode->m_CurrentIndex = directoryIterator.m_CurrentIndex + (isRootDirectory ? 2 : 0);
    }
    return entryWriter.GetBytesWritten();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
			
void FATFilesystem::RewindDirectory(Ptr<KFSVolume> _vol, Ptr<KDirectoryNode> _dirNode)
{
    Ptr<FATVolume>        vol     = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATDirectoryNode> dirNode = ptr_static_cast<FATDirectoryNode>(_dirNode);
    Ptr<FATInode>         node    = ptr_static_cast<FATInode>(_dirNode->GetInode());

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !dirNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::RewindDirectory() (inode ID {:x}).", node->m_InodeID);

    dirNode->m_CurrentIndex = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATFilesystem::ReadLink(Ptr<KFSVolume> _vol, Ptr<KInode> _node, char* buffer, size_t bufferSize)
{
    // no links in fat...
    kernel_log<PLogSeverity::WARNING>(LogCat_FATFS, "FATFilesystem::ReadLink() called.");

    PERROR_THROW_CODE(PErrorCode::INVAL);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CheckAccess(Ptr<KFSVolume> _vol, Ptr<KInode> _node, int mode)
{
    Ptr<FATVolume> vol  = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATInode>  node = ptr_static_cast<FATInode>(_node);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFS, "FATFilesystem::CheckAccess(inode ID {:x}, mode {:x})", node->m_InodeID, mode);

    if ((mode & O_ACCMODE) != O_RDONLY)
    {
        if (vol->IsReadOnly()) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CheckAccess(): can't write on read-only volume.");
            PERROR_THROW_CODE(PErrorCode::ROFS);
        } else if (node->IsDirectory()) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::CheckAccess(): can't open read-only file for writing.");
            PERROR_THROW_CODE(PErrorCode::PERM);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    Ptr<FATVolume> fsVolume = ptr_static_cast<FATVolume>(volume);
    Ptr<FATInode>  fsInode  = ptr_static_cast<FATInode>(inode);

    CRITICAL_SCOPE(fsVolume->m_Mutex);

    if (!fsVolume->CheckMagic(__func__) || !fsInode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::ReadStat(inode ID {:x})", fsInode->m_InodeID);

    KFilesystemFileOps::ReadStat(volume, inode, statBuf);

    statBuf->st_size    = fsInode->IsDirectory() ? 0 : fsInode->m_Size;
    statBuf->st_blksize = fsVolume->m_BytesPerSector * fsVolume->m_SectorsPerCluster;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::WriteStat(Ptr<KFSVolume> _vol, Ptr<KInode> _node, const struct stat* st, uint32_t mask)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATInode>  node = ptr_static_cast<FATInode>(_node);
    bool dirty = false;

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::WriteStat(inode ID {:x})", node->m_InodeID);

    if (vol->IsReadOnly()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::WriteStat(): read-only volume.");
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }

    if (mask & WSTAT_SIZE)
    {
        if (node->IsDirectory())
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::WriteStat(): can't set file size of directory!");
            PERROR_THROW_CODE(PErrorCode::ISDIR);
        }
        if (st->st_size < 0) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        if (st->st_size > FAT_MAX_FILE_SIZE) {
            PERROR_THROW_CODE(PErrorCode::FBIG);
        }
    }

    const uint32_t metadataMask = WSTAT_MODE | WSTAT_ATIME | WSTAT_MTIME | WSTAT_CTIME;
    const bool metadataChangeRequested = (mask & metadataMask) != 0;
    const bool sizeChangeRequested = (mask & WSTAT_SIZE) != 0 && st->st_size != node->m_Size;
    if (!metadataChangeRequested && !sizeChangeRequested) {
        return;
    }

    FATVolume::ModificationScope modificationScope(*vol);

    const mode_t oldFileMode = node->m_FileMode;
    const uint8_t oldDOSAttribs = node->m_DOSAttribs;
    const TimeValNanos oldAccessTime = node->m_ATime;
    const TimeValNanos oldModificationTime = node->m_MTime;
    const TimeValNanos oldCreationTime = node->m_CTime;
    PScopeFail restoreMetadata([&node, oldFileMode, oldDOSAttribs, oldAccessTime, oldModificationTime, oldCreationTime]()
    {
        node->m_FileMode = oldFileMode;
        node->m_DOSAttribs = oldDOSAttribs;
        node->m_ATime = oldAccessTime;
        node->m_MTime = oldModificationTime;
        node->m_CTime = oldCreationTime;
    });

    if (mask & WSTAT_MODE)
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFILE, "FATFilesystem::WriteStat(): setting file mode to {:o}.", st->st_mode);

        node->m_FileMode = (node->m_FileMode & S_IFMT) | (st->st_mode & ~S_IFMT);
        if (node->m_FileMode & (S_IWUSR | S_IWGRP | S_IWOTH))
        {
            node->m_FileMode |= S_IWUSR | S_IWGRP | S_IWOTH;
            node->m_DOSAttribs &= uint8_t(~FAT_READ_ONLY);
        }
        else
        {
            node->m_DOSAttribs |= FAT_READ_ONLY;
        }
        dirty = true;
    }

    if (mask & WSTAT_ATIME)
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFILE, "FATFilesystem::WriteStat(): setting access time.");
        node->m_ATime = FATInode::RoundTimeToFATAccessTime(TimeValNanos::FromTimespec(st->st_atim));
        dirty = true;
    }

    if (mask & WSTAT_MTIME)
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFILE, "FATFilesystem::WriteStat(): setting modification time.");
        node->m_MTime = FATInode::RoundTimeToFATModificationTime(TimeValNanos::FromTimespec(st->st_mtim));
        dirty = true;
    }

    if (mask & WSTAT_CTIME)
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATFILE, "FATFilesystem::WriteStat(): setting creation time.");
        node->m_CTime = FATInode::RoundTimeToFATCreateTime(TimeValNanos::FromTimespec(st->st_ctim));
        dirty = true;
    }

    if ((mask & WSTAT_SIZE) && st->st_size != node->m_Size)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::WriteStat(): setting file size to {:x}.", st->st_size);
        ResizeFile(vol, node, st->st_size, (mask & WSTAT_MTIME) == 0);
#ifdef FAT_VERIFY_FAT_CHAINS
        kassert(node->m_Size == 0 || vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, uint32_t((node->m_Size - 1) / (vol->m_BytesPerSector * vol->m_SectorsPerCluster)), node->m_EndCluster));
#endif // FAT_VERIFY_FAT_CHAINS
        dirty = false;
    }

    if (dirty) {
        node->Write();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Sync(Ptr<KFileNode> file)
{
    Sync(file->GetInode()->m_Volume);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<FATInode>  node = ptr_static_cast<FATInode>(file->GetInode());
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(node->m_Volume);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    switch (request)
    {
	case 100000:
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "vol info: {} (device {:x}, media descriptor {:x})", vol->m_DevicePath.c_str(), vol->m_DeviceFile, vol->m_MediaDescriptor);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "{:x} bytes/sector, {:x} sectors/cluster", vol->m_BytesPerSector, vol->m_SectorsPerCluster);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "{:x} reserved sectors, {:x} total sectors", vol->m_ReservedSectors, vol->m_TotalSectors);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "{:x} {}-bit fats, {:x} sectors/fat, {:x} root entries", vol->m_FATCount, vol->m_FATBits, vol->m_SectorsPerFAT, vol->m_RootEntriesCount);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "root directory starts at sector {:x} (cluster {:x}), data at sector {:x}", vol->m_RootStart, vol->m_RootInode->m_StartCluster, vol->m_FirstDataSector);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "{:x} total clusters, {:x} free", vol->m_TotalClusters, vol->m_FreeClusters);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "fat mirroring is {}, fs info sector at sector {:x}", (vol->m_FATMirrored) ? "on" : "off", vol->m_FSInfoSector);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "last allocated cluster = {:x}", vol->m_LastAllocatedCluster);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "root inode id = {:x}", vol->m_RootInode->m_InodeID);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "volume label [{:11.11}]", vol->m_VolumeLabel);
	    return;
			
	case 100001 :
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "inode id {:x}, dir inode = {:x}", node->m_InodeID, node->m_ParentInodeID);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "si = {:x}, ei = {:x}", node->m_DirStartIndex, node->m_DirEndIndex);
	    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "cluster = {:x} ({:x}), mode = {:x}, size = {:x}", node->m_StartCluster, vol->m_FirstDataSector + vol->m_SectorsPerCluster * (node->m_StartCluster - 2), node->m_DOSAttribs, node->m_Size);
	    vol->GetFATTable()->DumpChain(node->m_StartCluster);
	    return;

	case 100004 :
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "Dumping inode map for {:x}", vol->m_VolumeID);
	    vol->DumpInodeIDMap();
	    return;

	case 100005 :
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "Dumping directory map for {:x}", vol->m_VolumeID);
	    vol->DumpDirectoryMap();
	    return;

	default :
        kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATFilesystem::DeviceControl(): vol {:x}, inode {:x} code = {}.", vol->m_VolumeID, node->m_InodeID, request);
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PERROR_THROW_CODE(PErrorCode::INVAL);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

mode_t FATFilesystem::DOSAttribsToFileMode(uint8_t dosAttribs)
{
    return ((dosAttribs & FAT_SUBDIR) ? (S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH) : S_IFREG) |
           ((dosAttribs & FAT_READ_ONLY) ? (S_IRUSR | S_IRGRP | S_IROTH) : (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CopyVolumeLabelToFSInfo(const FATVolume& volume, fs_info* fsInfo)
{
    const bool hasVolumeLabel = volume.m_VolumeLabelEntry > -2;
    const char* const sourceLabel = hasVolumeLabel ? volume.m_VolumeLabel : "no name";
    size_t labelLength = hasVolumeLabel ? FAT_VOLUME_LABEL_LENGTH : sizeof("no name") - 1;

    while (labelLength > 0 && sourceLabel[labelLength - 1] == ' ') {
        --labelLength;
    }

    for (size_t index = 0; index < labelLength; ++index)
    {
        char character = sourceLabel[index];
        if ((character >= 'A') && (character <= 'Z')) {
            character = char(character + ('a' - 'A'));
        }
        fsInfo->fi_volume_name[index] = character;
    }
    fsInfo->fi_volume_name[labelLength] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Doesn't do any name checking
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATFilesystem::CreateVolumeLabel(Ptr<FATVolume> vol, const char* name)
{
    uint32_t dummy;
    FATNewDirEntryInfo info;
    const TimeValNanos currentTime = get_real_time();
    info.CreateTime = FATInode::RoundTimeToFATCreateTime(currentTime);
    info.AccessTime = FATInode::RoundTimeToFATAccessTime(currentTime);
    info.ModificationTime = FATInode::RoundTimeToFATModificationTime(currentTime);
    info.DOSAttribs = FAT_VOLUME;

    // check if name already exists
    if (FindShortName(vol, vol->m_RootInode, name)) {
        PERROR_THROW_CODE(PErrorCode::EXIST);
    }
    uint32_t index;
    DoCreateDirectoryEntry(vol, vol->m_RootInode, &info, name, nullptr, 0, &index, &dummy);
    return index;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::UpdateFSInfo()
{
    if (m_FSInfoSector != 0xffff && !IsReadOnly())
    {
        KCacheBlockDesc bufferDesc = m_BCache.GetBlock_trw(m_FSInfoSector);
        FATFSInfo* buffer = static_cast<FATFSInfo*>(bufferDesc.m_Buffer);
        if (buffer != nullptr)
        {
            if (buffer->m_Signature1 == 0x41615252 && buffer->m_Signature2 == 0x61417272 && buffer->m_Signature3 == 0xaa550000)
            {
                if (buffer->m_FreeClusters != m_FreeClusters || buffer->m_LastAllocatedCluster != m_LastAllocatedCluster)
                {
                    buffer->m_FreeClusters = m_FreeClusters;
                    buffer->m_LastAllocatedCluster = m_LastAllocatedCluster;
                    bufferDesc.MarkDirty();
                }
            }
            else
            {
                const uint32_t signature1 = buffer->m_Signature1;
                const uint32_t signature2 = buffer->m_Signature2;
                const uint32_t signature3 = buffer->m_Signature3;
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATVolume::UpdateFSInfo(): fsinfo block has invalid magic number {:08x}, {:08x}, {:08x}", signature1, signature2, signature3);
            }
        }
        else
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFS, "FATVolume::UpdateFSInfo(): error getting fsinfo sector {}.", m_FSInfoSector);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Name is array of char[11] as returned by findfile
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATFilesystem::FindShortName(Ptr<FATVolume> vol, Ptr<FATInode> parent, const char* rawShortName)
{
    FATDirectoryIterator diri(vol, parent->m_StartCluster, 0);
    
    for (FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry(); buffer != nullptr; buffer = diri.GetNextRawEntry())
    {
        if (buffer->m_Normal.m_Filename[0] == 0) {
            break;
        }
        if ((buffer->m_Normal.m_Attribs & FAT_LONG_NAME_ATTRIBUTE_MASK) != FAT_LONG_NAME_ATTRIBUTES) {
            if (memcmp(rawShortName, buffer->m_Normal.m_Filename, sizeof(buffer->m_Normal.m_Filename)) == 0) {
                return true;
            }
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATFilesystem::FindNameCollision(Ptr<FATVolume> volume, Ptr<FATInode> parent, const PString& name, FATInode* excludedNode)
{
    FATDirectoryIterator iterator(volume, parent->m_StartCluster, 0);
    FATDirectoryEntryInfo entryInfo;
    PString filename;
    PString shortFilename;

    for (;;)
    {
        filename.clear();
        shortFilename.clear();
        if (!iterator.GetNextLFNEntry(&entryInfo, &filename, &shortFilename)) {
            return false;
        }

        const bool isExcludedEntry = excludedNode != nullptr &&
                                     parent->m_InodeID == excludedNode->m_ParentInodeID &&
                                     entryInfo.m_EndIndex == excludedNode->m_DirEndIndex;
        if (!isExcludedEntry)
        {
            const bool longNameCollision = filename.compare_nocase(name) == 0;
            const bool shortNameCollision = shortFilename != filename && shortFilename.compare_nocase(name) == 0;
            if (longNameCollision || shortNameCollision) {
                return true;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<FATInode> FATFilesystem::DoLocateInode(Ptr<FATVolume> vol, Ptr<FATInode> dir, const PString& fileName)
{
    ino_t inodeID;

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }        

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::DoLocateInode(): {} in {:x}.", fileName.c_str(), dir->m_InodeID);

    if (fileName == "." && dir->m_InodeID == vol->m_RootInode->m_InodeID)
    {
        inodeID = dir->m_InodeID;
    }
    else if (fileName == ".." && dir->m_InodeID == vol->m_RootInode->m_InodeID)
    {
        inodeID = dir->m_ParentInodeID;
    }
    else
    {
        FATDirectoryIterator diri(vol, dir->m_StartCluster, 0);

        bool found = false;
        for(;;)
        {
            PString curName;
            if (!diri.GetNextDirectoryEntry(dir, &inodeID, &curName, nullptr)) {
                return nullptr;
            }
            if (curName == fileName) {
                found = true;
                break;
            }
        }
        if (!found) {
            return nullptr;
        }
    }
    return ptr_static_cast<FATInode>(KVFSManager::GetInode_trw(vol->m_VolumeID, inodeID, false));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATFilesystem::IsDirectoryEmpty(Ptr<FATVolume> volume, Ptr<FATInode> dir)
{
    if (!volume->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    FATDirectoryIterator iter(volume, dir->m_StartCluster, 0);

    if (iter.GetCurrentEntry() == nullptr) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::IsDirectoryEmpty(): error opening directory.");
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    uint32_t i = (dir->m_InodeID == volume->m_RootInode->m_InodeID) ? 2 : 0;

    for (; i < 3; ++i)
    {
        PString filename;

        if (!iter.GetNextLFNEntry(nullptr, &filename)) {
            return i == 2;
        }

        // weird case where ./.. are stored as long file names
        if ((i == 0 && filename != ".") || (i == 1 && filename != "..") || (i < 2 && iter.m_CurrentIndex != i + 1))
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::IsDirectoryEmpty(): malformed directory.");
            PERROR_THROW_CODE(PErrorCode::NOTDIR);
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATFilesystem::IsDirectoryAncestor(Ptr<FATVolume> volume, Ptr<FATInode> ancestor, Ptr<FATInode> directory)
{
    Ptr<FATInode> currentDirectory = directory;

    for (uint32_t directoryDepth = 0; directoryDepth <= volume->m_TotalClusters; ++directoryDepth)
    {
        if (currentDirectory->m_InodeID == ancestor->m_InodeID) {
            return true;
        }
        if (currentDirectory->m_InodeID == volume->m_RootInode->m_InodeID) {
            return false;
        }

        currentDirectory = ptr_static_cast<FATInode>(KVFSManager::GetInode_trw(volume->m_VolumeID, currentDirectory->m_ParentInodeID, false));
        if (currentDirectory == nullptr || !currentDirectory->IsDirectory()) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }

    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::IsDirectoryAncestor(): cycle in directory hierarchy.");
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::EnsureClusterChainMetadataLoaded(Ptr<FATVolume> volume, Ptr<FATInode> node)
{
    if (node->m_StartCluster == 0 || node->m_AllocatedClusterCount != 0 || IS_FIXED_ROOT(node->m_StartCluster)) {
        return;
    }

    uint32_t endCluster;
    const size_t chainLength = volume->GetFATTable()->GetChainLength(node->m_StartCluster, &endCluster);
    if (!node->IsDirectory() && chainLength < GetFileClusterCount(volume, node->m_Size))
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATFilesystem::EnsureClusterChainMetadataLoaded(): inode {:x} has a chain that is too short for its size.", node->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    node->m_EndCluster = endCluster;
    node->m_AllocatedClusterCount = uint32_t(chainLength);
    if (node->IsDirectory()) {
        node->m_Size = chainLength * volume->m_SectorsPerCluster * volume->m_BytesPerSector;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATFilesystem::GetFileClusterCount(Ptr<FATVolume> volume, off64_t fileSize)
{
    if (fileSize == 0) {
        return 0;
    }

    const uint32_t bytesPerCluster = volume->m_BytesPerSector * volume->m_SectorsPerCluster;
    return uint32_t((fileSize + bytesPerCluster - 1) / bytesPerCluster);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ClearFileRange(Ptr<FATVolume> volume, Ptr<FATInode> node, off64_t startPosition, off64_t endPosition)
{
    if (startPosition >= endPosition) {
        return;
    }

    const uint32_t bytesPerSector = volume->m_BytesPerSector;
    FATClusterSectorIterator iterator(volume, node->m_StartCluster, 0);

    const off64_t firstSectorIndex = startPosition / bytesPerSector;
    if (!iterator.Increment(int(firstSectorIndex))) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    off64_t currentPosition = startPosition;

    while (currentPosition < endPosition)
    {
        const size_t sectorOffset = size_t(currentPosition % bytesPerSector);
        size_t bytesToClear = bytesPerSector - sectorOffset;
        if (off64_t(bytesToClear) > endPosition - currentPosition) {
            bytesToClear = size_t(endPosition - currentPosition);
        }

        const bool doLoad = sectorOffset != 0 || bytesToClear != bytesPerSector;
        KCacheBlockDesc block = iterator.GetBlock_(doLoad);
        if (block.m_Buffer == nullptr)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::ClearFileRange(): error reading cluster {}, sector {}.", iterator.m_CurrentCluster, iterator.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        memset(static_cast<uint8_t*>(block.m_Buffer) + sectorOffset, 0, bytesToClear);
        block.MarkDirty();

        currentPosition += bytesToClear;
        if (currentPosition < endPosition && !iterator.Increment(1)) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ResizeFile(Ptr<FATVolume> volume, Ptr<FATInode> node, off64_t fileSize, bool updateModificationTime)
{
    if (fileSize < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (fileSize > FAT_MAX_FILE_SIZE) {
        PERROR_THROW_CODE(PErrorCode::FBIG);
    }

    const off64_t oldFileSize = node->m_Size;
    if (fileSize == oldFileSize) {
        return;
    }

    EnsureClusterChainMetadataLoaded(volume, node);

    const uint32_t oldClusterCount = node->m_AllocatedClusterCount;
    const uint32_t newClusterCount = GetFileClusterCount(volume, fileSize);
    const TimeValNanos oldModificationTime = node->m_MTime;
    const uint8_t oldDOSAttribs = node->m_DOSAttribs;
    const bool metadataWasDirty = node->IsMetadataDirty();

    auto restoreModificationMetadata = [&]()
    {
        node->m_MTime = oldModificationTime;
        node->m_DOSAttribs = oldDOSAttribs;
        if (!metadataWasDirty) {
            node->DiscardPendingMetadata();
        }
    };

    if (fileSize < oldFileSize)
    {
        PScopeFail restoreFileMetadata([&node, oldFileSize, newClusterCount, &restoreModificationMetadata]()
        {
            if (newClusterCount != 0)
            {
                node->m_Size = oldFileSize;
                restoreModificationMetadata();
            }
        });

        node->m_Size = fileSize;
        node->MarkContentsModified(updateModificationTime);
        if (newClusterCount != 0) {
            node->Write();
        }
    }

    {
        PScopeFail restoreFileMetadata([&node, oldFileSize, oldClusterCount, newClusterCount, &restoreModificationMetadata]()
        {
            if (node->m_AllocatedClusterCount == oldClusterCount && newClusterCount == 0)
            {
                node->m_Size = oldFileSize;
                restoreModificationMetadata();
            }
        });
        volume->GetFATTable()->SetChainLength(node, newClusterCount, true);
    }

    if (fileSize > oldFileSize)
    {
        ClearFileRange(volume, node, oldFileSize, fileSize);

        PScopeFail restoreFileMetadata([&node, oldFileSize, &restoreModificationMetadata]()
        {
            node->m_Size = oldFileSize;
            restoreModificationMetadata();
        });

        node->m_Size = fileSize;
        node->MarkContentsModified(updateModificationTime);
        node->Write();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::UpdateDirectoryParentEntry(Ptr<FATVolume> volume, Ptr<FATInode> directory, Ptr<FATInode> parent)
{
    if (!directory->IsDirectory() || directory->m_InodeID == volume->m_RootInode->m_InodeID) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }

    uint32_t parentCluster = 0;
    if (parent->m_InodeID != volume->m_RootInode->m_InodeID)
    {
        if (!parent->IsDirectory() || !volume->IsDataCluster(parent->m_StartCluster)) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        parentCluster = parent->m_StartCluster;
    }

    FATDirectoryIterator iterator(volume, directory->m_StartCluster, 1);
    FATDirectoryEntryCombo* entry = iterator.GetCurrentEntry();
    if (entry == nullptr || memcmp(entry->m_Normal.m_Filename, "..         ", sizeof(entry->m_Normal.m_Filename)) != 0 || !(entry->m_Normal.m_Attribs & FAT_SUBDIR))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::UpdateDirectoryParentEntry(): directory inode {:x} has an invalid '..' entry.", directory->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    entry->m_Normal.m_FirstClusterLow = uint16_t(parentCluster & 0xffff);
    entry->m_Normal.m_FirstClusterHigh = uint16_t(parentCluster >> 16);
    iterator.MarkDirty();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATInode> parent, Ptr<FATInode> node, const PString& name, FATInode* collisionExclusion, uint32_t* startIndex, uint32_t* endIndex)
{
    struct FATNewDirEntryInfo info;

    // FAT names and their 8.3 aliases share a case-insensitive namespace even
    // though PadOS deliberately performs ordinary path lookup case-sensitively.
    if (FindNameCollision(vol, parent, name, collisionExclusion))
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::CreateDirectoryEntry(): {} conflicts with an existing name or short-name alias in directory {:x}.", name.c_str(), parent->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::EXIST);
    }

    std::vector<wchar16_t> longName;

    longName.resize(FAT_LONG_NAME_MAX_ENTRY_COUNT * FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY, 0xffff);

    const size_t nameLength = name.copy_utf16(longName.data(), longName.size());

    if (nameLength > FAT_LONG_NAME_MAX_LENGTH)
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::CreateDirectoryEntry(): Error converting utf8 name '{}' to UNICODE. Result too long.", name.c_str());
        PERROR_THROW_CODE(PErrorCode::NAMETOOLONG);
    }
    longName[nameLength] = 0;
    uint32_t longNameLength = static_cast<uint32_t>(nameLength);

    char shortName[11];
    FATDirectoryIterator::GenerateShortName(longName.data(), nameLength, shortName);

    // If there is a long name, patch the short name and check for duplication.
    // Otherwise, preserve uniformly lowercase components with the NT case flags.
    if (FATDirectoryIterator::RequiresLongName(longName.data(), nameLength, info.ShortNameCaseFlags))
    {
        char tempName[11]; // Temporary short name

        memcpy(tempName, shortName, 11);

        bool foundFreeName = false;
        for (int i = 1; i <= 10; ++i)
        {
            FATDirectoryIterator::MungeShortName(shortName, i);
            
            kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::CreateDirectoryEntry(): trying short name [{:11.11}].", shortName);
            
            if (!FindShortName(vol, parent, shortName))
            {
                foundFreeName = true;
                break;
            }
            memcpy(shortName, tempName, 11);
        }

        if (!foundFreeName)
        {
            for (int i = 0; i < 1000; ++i)
            {
                memcpy(shortName, tempName, 11);
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::CreateDirectoryEntry(): trying short name [{:11.11}].", shortName);
                
                int value = (uint32_t(kget_monotonic_time().AsMicroseconds() / 1024)) % 99999 + 1;

                FATDirectoryIterator::MungeShortName(shortName, value);
                if (!FindShortName(vol, parent, shortName))
                {
                    foundFreeName = true;
                    break;
                }
            }
        }
        if (!foundFreeName)
        {
            PERROR_THROW_CODE(PErrorCode::NOSPC); // Failed to find an unused short name.
        }
    }
    else
    {
        longNameLength = 0; // Entry doesn't need a long name.
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "FATFilesystem::CreateDirectoryEntry(): creating directory entry [{:11.11}].", shortName);

    info.DOSAttribs = node->m_DOSAttribs;
    info.Cluster = node->m_StartCluster;
    info.Size    = size_t(node->m_Size);
    info.CreateTime = node->m_CTime;
    info.AccessTime = node->m_ATime;
    info.ModificationTime = node->m_MTime;

    DoCreateDirectoryEntry(vol, parent, &info, shortName, longName.data(), longNameLength, startIndex, endIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::DoCreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATInode> dir, FATNewDirEntryInfo* info, const char shortName[11], const wchar16_t* longName, uint32_t longNameLength, uint32_t* startIndex, uint32_t* endIndex)
{
    size_t shortNameBaseLength = 0;
    while (shortNameBaseLength < 8 && shortName[shortNameBaseLength] != ' ') {
        ++shortNameBaseLength;
    }
    if (g_DOSDeviceBaseNames.count(PString(shortName, shortNameBaseLength)) != 0)
    {
        PERROR_THROW_CODE(PErrorCode::PERM);
    }
    if ((info->Cluster != 0) && !vol->IsDataCluster(info->Cluster))
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::DoCreateDirectoryEntry(): for bad cluster ({}).", info->Cluster);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (longNameLength > FAT_LONG_NAME_MAX_LENGTH) {
        PERROR_THROW_CODE(PErrorCode::NAMETOOLONG);
    }

    EnsureClusterChainMetadataLoaded(vol, dir);

    const size_t requiredEntryCount = (longNameLength + FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY - 1) / FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY + 1;

    // find a place to put the entries
    *startIndex = 0;
    bool isLastEntry = true;
    {
        FATDirectoryIterator diri(vol, dir->m_StartCluster, 0);
        const uint32_t directoryEntryCount = static_cast<uint32_t>(dir->m_Size / sizeof(FATDirectoryEntry));
        while (diri.m_CurrentIndex < directoryEntryCount && diri.GetCurrentEntry() != nullptr)
        {
            FATDirectoryEntryInfo info;

            if (diri.GetNextLFNEntry(&info, nullptr))
            {
                if (info.m_StartIndex - *startIndex >= requiredEntryCount) {
                    isLastEntry = false;
                    break;
                }
                *startIndex = diri.m_CurrentIndex;
            }
            else
            {
                // hit end of directory marker
                break;
            }
        }
    }
    // If at end of directory, isLastEntry will be true as it should be.

    *endIndex = *startIndex + static_cast<uint32_t>(requiredEntryCount) - 1;

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::DoCreateDirectoryEntry(): directory entry runs from {:x} to {:x} (dirsize = {}){}", *startIndex, *endIndex, dir->m_Size, isLastEntry ? " (last entry)" : "");

    const uint32_t originalDirectoryEntryCount = static_cast<uint32_t>(dir->m_Size / sizeof(FATDirectoryEntry));
    const bool directoryNeedsExpansion = *endIndex >= originalDirectoryEntryCount;
    uint32_t requiredClusterCount = 0;
    if (directoryNeedsExpansion)
    {
        // can't expand fat12 and fat16 root directories :(
        if (IS_FIXED_ROOT(dir->m_StartCluster)) {
            kernel_log<PLogSeverity::WARNING>(LogCat_FATDIR, "FATFilesystem::DoCreateDirectoryEntry(): out of space in root directory.");
            PERROR_THROW_CODE(PErrorCode::NOSPC);
        }
        const uint32_t directoryEntriesPerCluster = vol->m_BytesPerSector * vol->m_SectorsPerCluster / sizeof(FATDirectoryEntry);
        requiredClusterCount = *endIndex / directoryEntriesPerCluster + 1;
    }

    FATDirectoryEntryCombo savedEntries[FAT_LONG_NAME_MAX_ENTRY_COUNT + 2];
    size_t savedEntryCount = 0;
    auto saveOriginalEntry = [&](const FATDirectoryEntryCombo& entry, uint32_t entryIndex)
    {
        if (entryIndex < originalDirectoryEntryCount)
        {
            kassert(entryIndex == *startIndex + savedEntryCount);
            kassert(savedEntryCount < ARRAY_COUNT(savedEntries));
            savedEntries[savedEntryCount++] = entry;
        }
    };

    FATDirectoryEntry shortDirectoryEntry = {};
    InitFATDirectoryEntry(
        shortDirectoryEntry,
        shortName,
        info->ShortNameCaseFlags,
        info->DOSAttribs,
        info->Cluster,
        info->Size,
        info->CreateTime,
        info->AccessTime,
        info->ModificationTime);

    size_t rollbackEntryCount = 0;
    bool rollbackNeeded = false;
    PScopeFail restoreDirectoryEntries(
        [&]()
        {
            if (rollbackNeeded) {
                RestoreFATDirectoryEntriesNoThrow(vol, dir->m_StartCluster, *startIndex, savedEntries, savedEntryCount, rollbackEntryCount);
            }
        });

    bool wasExpanded = false;
    if (directoryNeedsExpansion)
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::DoCreateDirectoryEntry(): expanding directory from {} to {} clusters.", dir->m_Size/vol->m_BytesPerSector/vol->m_SectorsPerCluster, requiredClusterCount);

        vol->GetFATTable()->SetChainLength(dir, requiredClusterCount, true);

        dir->m_Size = vol->m_BytesPerSector * vol->m_SectorsPerCluster * requiredClusterCount;
        wasExpanded = true;
    }

    // Write everything except the short entry first. The short entry is the
    // commit point that makes the new name visible.
    FATDirectoryIterator diri(vol,dir->m_StartCluster, *startIndex);
    FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();
    if (buffer == nullptr) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    const uint8_t hash = FATDirectoryIterator::HashMSDOSName(shortName);

    // write lfn entries
    for (size_t entryIndex = 1; entryIndex < requiredEntryCount && buffer != nullptr; ++entryIndex, buffer = diri.GetNextRawEntry())
    {
        const wchar16_t* namePart = longName + (requiredEntryCount - entryIndex - 1) * FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY;
        saveOriginalEntry(*buffer, diri.m_CurrentIndex);
        rollbackEntryCount = size_t(diri.m_CurrentIndex - *startIndex) + 1;
        rollbackNeeded = true;
        memset(buffer, 0, sizeof(*buffer));
        
        buffer->m_LFN.m_SequenceNumber = uint8_t(requiredEntryCount - entryIndex + ((entryIndex == 1) ? 0x40 : 0));
        buffer->m_LFN.m_Attribs = FAT_LONG_NAME_ATTRIBUTES;
        buffer->m_LFN.m_Hash = hash;
        memcpy(buffer->m_LFN.m_NamePart1, namePart, sizeof(buffer->m_LFN.m_NamePart1));
        namePart += ARRAY_COUNT(buffer->m_LFN.m_NamePart1);
        memcpy(buffer->m_LFN.m_NamePart2, namePart, sizeof(buffer->m_LFN.m_NamePart2));
        namePart += ARRAY_COUNT(buffer->m_LFN.m_NamePart2);
        memcpy(buffer->m_LFN.m_NamePart3, namePart, sizeof(buffer->m_LFN.m_NamePart3));
        diri.MarkDirty();
    }

    if (buffer == nullptr) { // This should never happen.
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::DoCreateDirectoryEntry(): Iteration failed.");
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    saveOriginalEntry(*buffer, diri.m_CurrentIndex);
    rollbackEntryCount = requiredEntryCount;

    const uint32_t directoryEntryCount = static_cast<uint32_t>(dir->m_Size / sizeof(FATDirectoryEntry));
    size_t entriesToClear = 0;
    if (isLastEntry && size_t(*endIndex) + 1 < directoryEntryCount)
    {
        entriesToClear = wasExpanded ? directoryEntryCount - size_t(*endIndex) - 1 : 1;
    }

    if (entriesToClear != 0)
    {
        FATDirectoryIterator clearIterator(vol, dir->m_StartCluster, *endIndex + 1);
        for (size_t entryIndex = 0; entryIndex < entriesToClear; ++entryIndex)
        {
            FATDirectoryEntryCombo* entry = clearIterator.GetCurrentEntry();
            if (entry == nullptr) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
            saveOriginalEntry(*entry, clearIterator.m_CurrentIndex);
            if (!wasExpanded) {
                rollbackEntryCount = size_t(clearIterator.m_CurrentIndex - *startIndex) + 1;
            }
            rollbackNeeded = true;
            memset(entry, 0, sizeof(*entry));
            clearIterator.MarkDirty();
            if (entryIndex + 1 < entriesToClear && clearIterator.GetNextRawEntry() == nullptr) {
                PERROR_THROW_CODE(PErrorCode::IO);
            }
        }
    }

    // Nothing below this point can throw. Publishing the short entry commits
    // the operation after all supporting records and the end marker are ready.
    buffer->m_Normal = shortDirectoryEntry;
    diri.MarkDirty();
}

// shrink directory to the size needed
// errors here are neither likely nor problematic
// w95 doesn't seem to do this, so it's possible to create a
// really large directory that consumes all available space!
void FATFilesystem::CompactDirectory(Ptr<FATVolume> vol, Ptr<FATInode> dir)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::CompactDirectory(): compacting directory with inode ID {:x}.", dir->m_InodeID);

    // root directory can't shrink in fat12 and fat16
    if (IS_FIXED_ROOT(dir->m_StartCluster)) {
        return;
    }

    EnsureClusterChainMetadataLoaded(vol, dir);

    FATDirectoryIterator    diri(vol, dir->m_StartCluster, 0);
    uint32_t                last = 0;
    const uint32_t          directoryEntryCount = static_cast<uint32_t>(dir->m_Size / sizeof(FATDirectoryEntry));

    while (diri.m_CurrentIndex < directoryEntryCount && diri.GetCurrentEntry() != nullptr)
    {
        FATDirectoryEntryInfo info;

        if (diri.GetNextLFNEntry(&info, nullptr))
        {
            // don't compact away volume labels in the root dir
            if (!(info.m_DOSAttribs & FAT_VOLUME) || (dir->m_InodeID != vol->m_RootInode->m_InodeID)) {
                last = diri.m_CurrentIndex;
            }            
        }
        else
        {
            uint32_t clusters = (last + vol->m_BytesPerSector / 0x20 * vol->m_SectorsPerCluster - 1) / (vol->m_BytesPerSector / 0x20) / vol->m_SectorsPerCluster;

            // Special case for FAT32 root directory. We don't want it to disappear.
            if (clusters == 0) clusters = 1;

            if (clusters * vol->m_BytesPerSector * vol->m_SectorsPerCluster < dir->m_Size)
            {
                kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::CompactDirectory(): shrinking directory to {} clusters.", clusters);
                vol->GetFATTable()->SetChainLength(dir, clusters, true);
                dir->m_Size = clusters*vol->m_BytesPerSector*vol->m_SectorsPerCluster;
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CompactDirectoryNoThrow(Ptr<FATVolume> vol, Ptr<FATInode> dir) noexcept
{
    try {
        CompactDirectory(vol, dir);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::CompactDirectoryNoThrow(): failed to compact directory inode {:x}: {}", dir->m_InodeID, exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::EraseDirectoryEntry(Ptr<FATVolume> vol, uint32_t parentCluster, uint32_t startIndex, uint32_t endIndex)
{
    FATDirectoryEntryInfo info;

    if ((!vol->IsDataCluster(parentCluster) && !IS_FIXED_ROOT(parentCluster)) || endIndex < startIndex || endIndex - startIndex > FAT_LONG_NAME_MAX_ENTRY_COUNT)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::EraseDirectoryEntry(): invalid directory cluster or entry range {} through {}.", startIndex, endIndex);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATFilesystem::EraseDirectoryEntry(): erasing directory entries {} through {}.", startIndex, endIndex);

    {
        FATDirectoryIterator iterator(vol, parentCluster, startIndex);
        if (iterator.GetCurrentEntry() == nullptr)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::EraseDirectoryEntry(): error reading directory.");
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        if (!iterator.GetNextLFNEntry(&info, nullptr)) {
            PERROR_THROW_CODE(PErrorCode::NOENT);
        }
    }

    if (info.m_StartIndex != startIndex || info.m_EndIndex != endIndex)
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::EraseDirectoryEntry(): directory entry doesn't match.");
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    const size_t entryCount = size_t(endIndex - startIndex) + 1;
    FATDirectoryEntryCombo originalEntries[FAT_LONG_NAME_MAX_ENTRY_COUNT + 1];
    size_t erasedEntryCount = 0;

    PScopeFail restoreErasedEntries([&]()
    {
        if (erasedEntryCount != 0)
        {
            try
            {
                FATDirectoryIterator restoreIterator(vol, parentCluster, startIndex);
                for (size_t entryIndex = 0; entryIndex < erasedEntryCount; ++entryIndex)
                {
                    FATDirectoryEntryCombo* entry = restoreIterator.GetCurrentEntry();
                    if (entry == nullptr) {
                        PERROR_THROW_CODE(PErrorCode::IO);
                    }
                    *entry = originalEntries[entryIndex];
                    restoreIterator.MarkDirty();
                    if (entryIndex + 1 < erasedEntryCount) {
                        restoreIterator.GetNextRawEntry();
                    }
                }
            }
            catch (const std::exception& exception)
            {
                vol->MarkMetadataInconsistent();
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::EraseDirectoryEntry(): failed to restore a partially erased directory entry: {}", exception.what());
            }
        }
    });

    FATDirectoryIterator iterator(vol, parentCluster, startIndex);
    for (size_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
    {
        FATDirectoryEntryCombo* entry = iterator.GetCurrentEntry();
        if (entry == nullptr) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        originalEntries[entryIndex] = *entry;
        entry->m_Normal.m_Filename[0] = char(0xe5);
        iterator.MarkDirty();
        ++erasedEntryCount;
        if (entryIndex + 1 < entryCount) {
            iterator.GetNextRawEntry();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::DoUnlink(Ptr<KFSVolume> _vol, Ptr<KInode> _dir, const PString& name, bool removeFile)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATInode>  dir = ptr_static_cast<FATInode>(_dir);

    if (name == "." || name == "..") {
        PERROR_THROW_CODE(PErrorCode::PERM);
    }
    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::DoUnlink(): {:x}/{}", dir->m_InodeID, name.c_str());

    if (vol->IsReadOnly()) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FATFilesystem::DoUnlink(): read-only volume.");
        PERROR_THROW_CODE(PErrorCode::ROFS);
    }

    // locate the file
    Ptr<FATInode> file = DoLocateInode(vol, dir, name);
    if (file == nullptr) {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATFILE, "FATFilesystem::DoUnlink(): can't find file {} in directory {:x}.", name.c_str(), dir->m_InodeID);
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }

    if (!file->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (removeFile)
    {
        if (file->IsDirectory()) {
            PERROR_THROW_CODE(PErrorCode::ISDIR);
        }
    }
    else
    {
        if (!file->IsDirectory()) {
            PERROR_THROW_CODE(PErrorCode::NOTDIR);
        }
        if (file->m_InodeID == vol->m_RootInode->m_InodeID) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATFILE, "FATFilesystem::DoUnlink(): don't call this on the root directory.");
            PERROR_THROW_CODE(PErrorCode::PERM);
        }
        if (!IsDirectoryEmpty(vol, file)) {
            PERROR_THROW_CODE(PErrorCode::NOTEMPTY);
        }
    }

    FATVolume::ModificationScope modificationScope(*vol);

    ino_t originalLocationID;
    if (!vol->GetInodeIDToLocationIDMapping(file->m_InodeID, &originalLocationID)) {
        originalLocationID = file->m_InodeID;
    }

    bool mappingChangeAttempted = false;
    PScopeFail restoreInodeMapping([&]()
    {
        if (mappingChangeAttempted)
        {
            try {
                vol->SetInodeIDToLocationIDMapping(file->m_InodeID, originalLocationID);
            } catch (const std::exception& exception) {
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATFilesystem::DoUnlink(): failed to restore the inode mapping: {}", exception.what());
            }
        }
    });

    // Move the inode away from its on-disk location before making that
    // location available for reuse. Roll the mapping back if erasing fails.
    mappingChangeAttempted = true;
    vol->SetInodeIDToLocationIDMapping(file->m_InodeID, vol->AllocUniqueInodeID());

    EraseDirectoryEntry(vol, dir->m_StartCluster, file->m_DirStartIndex, file->m_DirEndIndex);
    file->DiscardPendingMetadata();
    file->SetDeletedFlag(true);
    vol->RegisterDeferredDeletion();

    CompactDirectoryNoThrow(vol, dir);
}

} // namespace
