// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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

#include <utility>
#include <string.h>
#include <fcntl.h>
#include <set>

#include <PadOS/DeviceControl.h>

#include <Kernel/KTime.h>
#include <Kernel/FSDrivers/FAT/FATFilesystem.h>
#include <System/ExceptionHandling.h>
#include <Utils/Utils.h>
#include <Ptr/NoPtr.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KVFSManager.h>

#include "FATVolume.h"
#include "FATINode.h"
#include "FATDirectoryNode.h"
#include "FATDirectoryIterator.h"
#include "FATFileNode.h"

using namespace os;

#define FAT_MAX_FILE_SIZE 0xffffffffLL

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
// Short name cannot be any of the DOS/Win device names (list from wikipedia).
///////////////////////////////////////////////////////////////////////////////

static std::set<String> g_DOSDeviceNames =
{
    "CON        ",
    "PRN        ",
    "AUX        ",
    "CLOCK$     ",
    "NUL        ",
    "COM1       ",
    "COM2       ",
    "COM3       ",
    "COM4       ",
    "LPT1       ",
    "LPT2       ",
    "LPT3       ",
    "LPT4       ", // Only in some versions of DR-DOS,
    "LST        ", // Only in 86-DOS and DOS 1.xx.
    "KEYBD$     ", // Only in multitasking MS-DOS 4.0.
    "SCREEN$    ", // Only in multitasking MS-DOS 4.0.
    "$IDLE$     ", // Only in Concurrent DOS 386, Multiuser DOS and DR DOS 5.0 and higher.
    "CONFIG$    "  // Only in MS-DOS 7.0-8.0.
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATFilesystem::FATFilesystem()
{
	REGISTER_KERNEL_LOG_CATEGORY(LOGC_FS,       KLogSeverity::WARNING);
	REGISTER_KERNEL_LOG_CATEGORY(LOGC_FATTABLE, KLogSeverity::WARNING);
	REGISTER_KERNEL_LOG_CATEGORY(LOGC_DIR,      KLogSeverity::WARNING);
	REGISTER_KERNEL_LOG_CATEGORY(LOGC_FILE,     KLogSeverity::WARNING);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
PErrorCode FATFilesystem::Probe(const char* devicePath, fs_info* fsInfo)
{
    try
    {
        // Attempt to mount volume as a FAT volume
        Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(Mount(-1, devicePath, 0, nullptr, 0));

        if (!vol->CheckMagic(__func__)) {
            return PErrorCode::InvalidArg;
        }


        fsInfo->fi_flags = vol->GetFlags() | uint32_t(FSVolumeFlags::FS_CAN_MOUNT);  // File system flags.
        fsInfo->fi_block_size = vol->m_BytesPerSector * vol->m_SectorsPerCluster; // FS block size.
        fsInfo->fi_io_size = 65536;                                               // IO size - specifies buffer size for file copying
        fsInfo->fi_total_blocks = vol->m_TotalClusters;                            // Total blocks
        fsInfo->fi_free_blocks = vol->m_FreeClusters;                              // Free blocks
        fsInfo->fi_free_user_blocks = fsInfo->fi_free_blocks;

        if (vol->m_VolumeLabelEntry > -2) {
            strncpy(fsInfo->fi_volume_name, vol->m_VolumeLabel, sizeof(fsInfo->fi_volume_name));
        }
        else {
            strcpy(fsInfo->fi_volume_name, "no name    ");
        }
        // XXX: should sanitize name as well

        int  i;
        for (i = 10; i > 0; --i)
        {
            if (fsInfo->fi_volume_name[i] != ' ') {
                break;
            }
        }
        fsInfo->fi_volume_name[i + 1] = 0;
        for (; i >= 0; --i)
        {
            if ((fsInfo->fi_volume_name[i] >= 'A') && (fsInfo->fi_volume_name[i] <= 'Z')) {
                fsInfo->fi_volume_name[i] = char(fsInfo->fi_volume_name[i] + ('a' - 'A'));
            }
        }
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
            kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): failed getting device geometry.\n");
            PERROR_THROW_CODE(result);
        }
    }
    if ((geo.bytes_per_sector != 512) && (geo.bytes_per_sector != 1024) && (geo.bytes_per_sector != 2048)) {
        kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): unsupported device block size (%lu).\n", geo.bytes_per_sector);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    if (geo.removable) {
        kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Mount(): %s is removable.\n", devicePath);
        volumeFlags |= uint32_t(FSVolumeFlags::FS_IS_REMOVABLE);
    }
    if (geo.read_only || (flags & MOUNT_READ_ONLY))
    {
        kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Mount(): %s is read-only.\n", devicePath);
        volumeFlags |= uint32_t(FSVolumeFlags::FS_IS_READONLY);
    }
    else
    {
        // reopen it with read/write permissions
        kclose(deviceFile);
        deviceFile = kopen_trw(devicePath, O_RDWR);
        if (deviceFile < 0) {
            kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount() unable to reopen %s (%s)\n", devicePath, strerror(get_last_error()));
            PERROR_THROW_CODE(PErrorCode(get_last_error()));
        }
    }


    Ptr<FATVolume>  vol = ptr_new<FATVolume>(ptr_tmp_cast(this), volumeID, devicePath);

    vol->ReadSuperBlock(deviceFile);

    vol->m_DeviceFile = deviceFile;
    vol->SetFlags(volumeFlags);

    // Check that the partition is large enough to contain the file system.

    if (vol->m_TotalSectors > geo.sector_count)
    {
        kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): volume extends past end of partition (%ld > %Ld)\n", vol->m_TotalSectors, geo.sector_count);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    // Perform sanity checks on the FAT.

    std::vector<uint8_t> buffer;
    buffer.resize(512);

    // the media descriptor in active FAT should match the one in the BPB
    if (kpread_trw(deviceFile, buffer.data(), buffer.size(), vol->m_BytesPerSector * (vol->m_ReservedSectors + vol->m_ActiveFAT * vol->m_SectorsPerFAT)) != buffer.size()) {
        kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): error reading FAT\n");
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (buffer[0] != vol->m_MediaDescriptor) {
        kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): media descriptor mismatch (%x != %x)\n", buffer[0], vol->m_MediaDescriptor);
        PERROR_THROW_CODE(PErrorCode::IOError);
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
                kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Mount(): checking fat #%ld\n", i);
                buffer2[0] = uint8_t(~buffer[0]);
                if (kpread_trw(deviceFile, buffer2.data(), buffer2.size(), vol->m_BytesPerSector * (vol->m_ReservedSectors + vol->m_SectorsPerFAT * i)) != buffer2.size()) {
                    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): error reading FAT %ld\n", i);
                    PERROR_THROW_CODE(PErrorCode::IOError);
                }

                if (buffer2[0] != vol->m_MediaDescriptor) {
                    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): media descriptor mismatch in fat # %ld (%x != %x)\n", i, buffer2[0], vol->m_MediaDescriptor);
                    PERROR_THROW_CODE(PErrorCode::IOError);
                }
                // checking for exact matches of fats is too restrictive; allow these to go through in case the fat is corrupted for some reason
                if (memcmp(buffer.data(), buffer2.data(), buffer.size()) != 0) {
                    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::WARNING, "FATFilesystem::Mount(): fat %d doesn't match active fat (%d)\n", i, vol->m_ActiveFAT);
                }
            }
        }
    }

    // Now we are convinced of the drive is valid.

    // XXX: if fsinfo exists, read from there?
    vol->m_LastAllocatedCluster = 2;

    if (!vol->m_BCache.SetDevice(deviceFile, vol->m_TotalSectors, vol->m_BytesPerSector)) {
        kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): error initializing block cache (%s)\n", strerror(get_last_error()));
        PERROR_THROW_CODE(PErrorCode(get_last_error()));
    }


    //    uint32_t freeClusters = 0;
    bool     isFreeClustersValid = vol->HasFlag(FSVolumeFlags::FS_IS_READONLY);
    if (!isFreeClustersValid)
    {
        if (vol->m_FSInfoSector != 0xffff)
        {
            KCacheBlockDesc bufferDesc = vol->m_BCache.GetBlock_trw(vol->m_FSInfoSector);
            FATFSInfo* fsInfo = static_cast<FATFSInfo*>(bufferDesc.m_Buffer);
            if (fsInfo != nullptr)
            {
                if (fsInfo->m_Signature1 == 0x41615252 && fsInfo->m_Signature2 == 0x61417272 && fsInfo->m_Signature3 == 0xaa550000)
                {
                    vol->m_FreeClusters = fsInfo->m_FreeClusters;
                    vol->m_LastAllocatedCluster = fsInfo->m_LastAllocatedCluster;
#if 0                    
                    const uint32_t freeClusters = vol->GetFATTable()->CountFreeClusters();
                    if (freeClusters != vol->m_FreeClusters)
                    {
                        vol->m_FreeClusters = freeClusters;
                        kernel_log(LOGC_FS, KLogSeverity::ERROR, "Mismatching free cluster count in fsinfo block. Found %u should be %u\n", vol->m_FreeClusters, freeClusters);
                    }
#endif                    
                    isFreeClustersValid = true;
                }
                else
                {
                    uint32_t signature1 = fsInfo->m_Signature1;
                    uint32_t signature2 = fsInfo->m_Signature2;
                    uint32_t signature3 = fsInfo->m_Signature3;
                    kernel_log(LOGC_FS, KLogSeverity::CRITICAL, "FATFilesystem::Mount(): fsinfo block has invalid magic number %08x, %08x, %08x\n", signature1, signature2, signature3);
                }
            }
            else
            {
                kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::Mount(): error getting fsinfo sector %x\n", vol->m_FSInfoSector);
            }
        }
        if (!isFreeClustersValid)
        {
            vol->m_FreeClusters = vol->GetFATTable()->CountFreeClusters();
            isFreeClustersValid = true;
        }
    }

    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "mounting %s (id %x, device %x, media descriptor %x)\n", vol->m_DevicePath.c_str(), vol->m_VolumeID, deviceFile, vol->m_MediaDescriptor);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "%lx bytes/sector, %lx sectors/cluster\n", vol->m_BytesPerSector, vol->m_SectorsPerCluster);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "%lx reserved sectors, %lx total sectors\n", vol->m_ReservedSectors, vol->m_TotalSectors);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "%lx %d-bit fats, %lx sectors/fat, %lx root entries\n", vol->m_FATCount, vol->m_FATBits, vol->m_SectorsPerFAT, vol->m_RootEntriesCount);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "root directory starts at sector %lx (cluster %lx), data at sector %lx\n", vol->m_RootStart, vol->m_RootINode->m_StartCluster, vol->m_FirstDataSector);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "%lx total clusters, %lx free\n", vol->m_TotalClusters, vol->m_FreeClusters);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "fat mirroring is %s, fs info sector at sector %x\n", (vol->m_FATMirrored) ? "on" : "off", vol->m_FSInfoSector);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "last allocated cluster = %lx\n", vol->m_LastAllocatedCluster);

    if (vol->m_FATBits == 32)
    {
        // Now that the block-cache has been initialized, we can figure out the length of the root directory.
        const size_t chainLength = vol->GetFATTable()->GetChainLength(vol->m_RootINode->m_StartCluster);
        vol->m_RootINode->m_Size = chainLength * vol->m_BytesPerSector * vol->m_SectorsPerCluster;
        vol->m_RootINode->m_EndCluster = vol->GetFATTable()->GetChainEntry(vol->m_RootINode->m_StartCluster, uint32_t(vol->m_RootINode->m_Size / vol->m_BytesPerSector / vol->m_SectorsPerCluster - 1));
    }

    // initialize root inode
    vol->m_RootINode->m_INodeID = vol->m_RootINode->m_ParentINodeID = GENERATE_DIR_CLUSTER_INODEID(vol->m_RootINode->m_StartCluster, vol->m_RootINode->m_StartCluster);
    vol->m_RootINode->m_DirStartIndex = 0xffffffff;
    vol->m_RootINode->m_DirEndIndex = 0xffffffff;
    vol->m_RootINode->m_DOSAttribs = FAT_SUBDIR;
    vol->m_RootINode->m_Time = get_real_time().AsSecondsI();
    vol->AddDirectoryMapping(vol->m_RootINode->m_INodeID);

    // find volume label (supersedes any label in the bpb)
    {
        FATDirectoryIterator diri(vol, vol->m_RootINode->m_StartCluster, 0);
        for (const FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry(); buffer != nullptr; buffer = diri.GetNextRawEntry())
        {
            if ((buffer->m_Normal.m_Attribs & FAT_VOLUME) && (buffer->m_Normal.m_Attribs != 0xf) && (buffer->m_Normal.m_Filename[0] != 0xe5))
            {
                vol->m_VolumeLabelEntry = diri.m_CurrentIndex;
                memcpy(vol->m_VolumeLabel, buffer->m_Normal.m_Filename, sizeof(buffer->m_Normal.m_Filename));
                break;
            }
        }
    }

    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Mount(): Root inode ID = %" PRIx64 "\n", vol->m_RootINode->m_INodeID);
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Mount(): Volume label [%11.11s] (%ld).\n", vol->m_VolumeLabel, vol->m_VolumeLabelEntry);

    return vol;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Unmount(Ptr<KFSVolume> volume)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);

    CRITICAL_SCOPE(vol->m_Mutex);
	
    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Unmount(): %x\n", vol->m_VolumeID);

    vol->UpdateFSInfo();
    vol->m_BCache.Shutdown(true);

    kclose(vol->m_DeviceFile);

    vol->Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Sync(Ptr<KFSVolume> _vol)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    
    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Sync() called on volume %x\n", vol->m_VolumeID);

    vol->UpdateFSInfo();
    vol->m_BCache.Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ReadFSStat(Ptr<KFSVolume> _vol, fs_info* fss)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    int i;

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::ReadFSStat() called.\n");

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

    if (vol->m_VolumeLabelEntry > -2) {
        strncpy(fss->fi_volume_name, vol->m_VolumeLabel, sizeof(fss->fi_volume_name));
    } else {
        strcpy(fss->fi_volume_name, "no name    ");
    }
    // XXX: should sanitize name as well
    for (i=10;i>0;i--) {
        if (fss->fi_volume_name[i] != ' ') {
            break;
        }
    }
    fss->fi_volume_name[i+1] = 0;
    for (; i >= 0; i--)
    {
        if ((fss->fi_volume_name[i] >= 'A') && (fss->fi_volume_name[i] <= 'Z')) {
            fss->fi_volume_name[i] = char(fss->fi_volume_name[i] + ('a' - 'A'));
        }
    }

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
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::WriteFSStat() called.\n");

    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
        PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
    }

    if (mask & WFSSTAT_NAME)
    {
        // sanitize name
        char name[11];

        memset(name, ' ', 11);
        kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::WriteFSStat(): setting name to %s\n", fss->fi_volume_name);
        int i;
        int j;
        for (i = 0, j=0; (i<11) && (fss->fi_volume_name[j]); ++j)
        {
            static const char acceptable[] = "!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~ ";
            char c = fss->fi_volume_name[j];
            if ((c >= 'a') && (c <= 'z')) c = char(c + ('A' - 'a'));
            // spaces acceptable in volume names
            if (strchr(acceptable, c)) {
                name[i++] = c;
            }
        }
        if (i == 0) { // bad name, kiddo
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
        }
        kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::WriteFSStat(): sanitized to [%11.11s].\n", name);

        if (vol->m_VolumeLabelEntry == -1)
        {
            // stored in the bpb
            KCacheBlockDesc bufferDesc = vol->m_BCache.GetBlock_trw(0);
            uint8_t* buffer = static_cast<uint8_t*>(bufferDesc.m_Buffer);
            if (buffer == nullptr) {
                PERROR_THROW_CODE(PErrorCode::IOError);
            }
            if ((buffer[0x26] != 0x29) || memcmp(buffer + 0x2b, vol->m_VolumeLabel, 11))
            {
                kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::WriteFSStat(): label mismatch\n");
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            else
            {
                memcpy(buffer + 0x2b, name, 11);
                vol->m_BCache.MarkBlockDirty(0);
            }
        }
        else if (vol->m_VolumeLabelEntry >= 0)
        {
            FATDirectoryIterator diri(vol, vol->m_RootINode->m_StartCluster, vol->m_VolumeLabelEntry);
            FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();

            // check if it is the same as the old volume label
            if (buffer == nullptr || memcmp(buffer->m_Normal.m_Filename, vol->m_VolumeLabel, sizeof(buffer->m_Normal.m_Filename)) != 0)
            {
                kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::WriteFSStat(): label mismatch.\n");
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            memcpy(buffer->m_Normal.m_Filename, name, sizeof(buffer->m_Normal.m_Filename));
            diri.MarkDirty();
        }
        else
        {
            const uint32_t index = CreateVolumeLabel(vol, name);
            vol->m_VolumeLabelEntry = index;
        }
        memcpy(vol->m_VolumeLabel, name, 11);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> FATFilesystem::LocateInode(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* name, int nameLength)
{
    // Starting at the base, find file in the subdir, and return path string and inode id of file.
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATINode>  dir = ptr_static_cast<FATINode>(parent);
    String         file;

    if (nameLength > 255) {
        PERROR_THROW_CODE(PErrorCode::NameTooLong);
    }
    file.assign(name, nameLength);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::LocateInode(): find %" PRIx64 "/%s\n", dir->m_INodeID, file.c_str());

    const Ptr<FATINode> inode = DoLocateINode(vol, dir, file);
    if (inode == nullptr)
    {
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::LocateInode(): Error finding inode ID for file %s (%s)\n", file.c_str(), strerror(get_last_error()));
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }
    return inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ReleaseInode(KINode* inode)
{
    Ptr<FATVolume> vol  = ptr_static_cast<FATVolume>(inode->m_Volume);
    FATINode*      node = static_cast<FATINode*>(inode);
    
    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (node->IsDeleted())
    {
        CRITICAL_SCOPE(vol->m_Mutex);

        kernel_log(FATFilesystem::LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::ReleaseINode(%" PRIx64 ")\n", node->m_INodeID);

        if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
            kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::ReleaseInode(): deleted inode on read-only volume\n");
            PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
        }

        // clear the fat chain
        if (node->m_StartCluster != 0 && !vol->IsDataCluster(node->m_StartCluster)) {
            kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::ReleaseInode(): invalid start cluster %ld\n", node->m_StartCluster);
        }
        /* XXX: the following assertion was tripped */
        if (node->m_StartCluster == 0 && node->m_Size == 0) {
            kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::ReleaseInode(): inode have start cluster %ld and no size.\n", node->m_StartCluster);
        }
        if (node->m_StartCluster != 0) {
            vol->GetFATTable()->ClearFATChain(node->m_StartCluster);
        }
        // Remove inode ID from the cache.
        if (vol->HasINodeIDToLocationIDMapping(node->m_INodeID)) {
            vol->RemoveINodeIDToLocationIDMapping(node->m_INodeID);
        }
        // If directory, remove from directory mapping.
        if (node->m_DOSAttribs & FAT_SUBDIR) {
            vol->RemoveDirectoryMapping(node->m_INodeID);
        }      
    }
    kernel_log(LOGC_FS, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::ReleaseInode() (inode ID %" PRIx64 ")\n", node->m_INodeID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> FATFilesystem::OpenFile(Ptr<KFSVolume> volume, Ptr<KINode> _node, int openFlags)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATINode>  node = ptr_static_cast<FATINode>(_node);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::OpenFile(): inode ID %" PRIx64 ", openFlags %x\n", node->m_INodeID, openFlags);

    if (openFlags & O_CREAT)
    {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::OpenFile(): called with O_CREAT.\n");
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY) || (node->m_DOSAttribs & FAT_READ_ONLY) || (node->m_DOSAttribs & FAT_SUBDIR)) {
        openFlags = (openFlags & ~O_ACCMODE) | O_RDONLY;
    }

    if ((openFlags & O_TRUNC) && ((openFlags & O_ACCMODE) == O_RDONLY))
    {
        kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::OpenFile(): can't open file for reading with O_TRUNC\n");
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }

    if (openFlags & O_TRUNC)
    {
        kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::OpenFile() called with O_TRUNC set.\n");
        vol->GetFATTable()->SetChainLength(node, 0, true);

        node->m_DOSAttribs = 0;
        node->m_Size = 0;
        node->m_Iteration++;
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

Ptr<KFileNode> FATFilesystem::CreateFile(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* _name, int nameLength, int openFlags, int perms)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATINode>  dir = ptr_static_cast<FATINode>(parent);

    String	name;

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (_name == nullptr) {
        kernel_log(LOGC_FILE, KLogSeverity::CRITICAL, "FATFilesystem::CreateFile() called with null name.\n");
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    
    if (nameLength > 255) {
        PERROR_THROW_CODE(PErrorCode::NameTooLong);
    }

    name.assign(_name, nameLength);
    
    CRITICAL_SCOPE(vol->m_Mutex);

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::CreateFile() called: %" PRIx64 "/%s perms=%o openFlags=%o\n", dir->m_INodeID, name.c_str(), perms, openFlags);

    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::CreateFile() called on read-only volume.\n");
        PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
    }

    if (dir->IsDeleted()) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::CreateFile() called in removed directory.\n");
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }

    if ((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::CreateFile() called with invalid permission bits (%x).\n", perms);
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    if ((openFlags & O_ACCMODE) == O_RDONLY) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "invalid permissions used in creating file.\n");
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }

    Ptr<FATFileNode> fileNode;

    Ptr<FATINode> file = DoLocateINode(vol, dir, name);
    if (file != nullptr)
    {
        if (openFlags & O_EXCL) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::CreateFile() with O_EXCL called on existing file %s.\n", name.c_str());
            PERROR_THROW_CODE(PErrorCode::Exist);
        }
        if (file->m_DOSAttribs & FAT_SUBDIR) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::CreateFile() called on existing directory.\n");
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }
        if (openFlags & O_TRUNC) {
            vol->GetFATTable()->SetChainLength(file, 0, true);
            file->m_Size = 0;
            file->m_Iteration++;
        }
        fileNode = ptr_new<FATFileNode>(openFlags);
    }
    else
    {
        NoPtr<FATINode> dummyObj(ptr_tmp_cast(this), vol, true); // Used only to create directory entry
        Ptr<FATINode> dummy(dummyObj);
        
        dummy->m_ParentINodeID = dir->m_INodeID;
        dummy->m_StartCluster = 0;
        dummy->m_EndCluster = 0;
        dummy->m_DOSAttribs = 0;
        dummy->m_Size = 0;
        dummy->m_Time = get_real_time().AsSecondsI();

        CreateDirectoryEntry(vol, dir, dummy, name, &dummy->m_DirStartIndex, &dummy->m_DirEndIndex);

        dummy->m_INodeID = GENERATE_DIR_INDEX_INODEID(dummy->m_ParentINodeID, dummy->m_DirStartIndex);
        if (vol->HasINodeIDToLocationIDMapping(dummy->m_INodeID))
        {
            dummy->m_INodeID = vol->AllocUniqueINodeID();
            vol->SetINodeIDToLocationIDMapping(dummy->m_INodeID, GENERATE_DIR_INDEX_INODEID(dummy->m_ParentINodeID, dummy->m_DirStartIndex));
        }
        ino_t inodeID = dummy->m_INodeID;

        file = ptr_static_cast<FATINode>(KVFSManager::GetINode_trw(vol->m_VolumeID, inodeID, false));
        fileNode = ptr_new<FATFileNode>(openFlags);
        fileNode->SetINode(file);
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
    Ptr<FATINode>    node     = ptr_static_cast<FATINode>(file->GetINode());

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !fileNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::CloseFile() (inode ID %" PRId64 ")\n", node->m_INodeID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> FATFilesystem::LoadInode(Ptr<KFSVolume> volume, ino_t inodeID)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    ino_t loc;
    ino_t parentINodeID;

    char reenter = vol->m_Mutex.IsLocked();
    CRITICAL_SCOPE(vol->m_Mutex, !reenter);

    if (!vol->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::LoadInode() (inode ID %" PRIx64 ").\n", inodeID);

    if (inodeID == vol->m_RootINode->m_INodeID)
    {
	    return vol->m_RootINode;
    }

    if (!vol->GetINodeIDToLocationIDMapping(inodeID, &loc)) {
	loc = inodeID;
    }
    if (IS_ARTIFICIAL_INODEID(loc) || IS_INVALID_INODEID(loc)) {
	    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::CRITICAL, "FATFilesystem::LoadInode(): unknown inode ID %" PRIx64 " (loc %" PRIx64 ").\n", inodeID, loc);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    parentINodeID = vol->GetDirectoryMapping(DIR_OF_INODEID(loc));
    if (parentINodeID == -1)
    {
	    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::CRITICAL, "FATFilesystem::LoadInode(): unknown directory at cluster %lx.\n", DIR_OF_INODEID(loc));
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    FATDirectoryIterator iter(vol, DIR_OF_INODEID(loc), IS_DIR_CLUSTER_INODEID(loc) ? 0 : INDEX_OF_DIR_INDEX_INODEID(loc));
    if (iter.GetCurrentEntry() == nullptr) {
	    kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::LoadInode(): error initializing directory for inode %" PRIx64 " (loc %" PRIx64 ").\n", inodeID, loc);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
	
    FATDirectoryEntryInfo info;
    for (;;)
    {
        if (!iter.GetNextLFNEntry(&info, nullptr))
        {
            kernel_log(LOGC_FS, KLogSeverity::CRITICAL, "FATFilesystem::LoadInode(): error finding inode %" PRIx64 " (loc %" PRIx64 ") (%s).\n", inodeID, loc, strerror(get_last_error()));
            PERROR_THROW_CODE(PErrorCode::IOError);
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
            kernel_log(LOGC_FS, KLogSeverity::CRITICAL, "FATFilesystem::LoadInode(): error finding inode %" PRIx64 " (loc %" PRIx64 ") (%s).\n", inodeID, loc, strerror(get_last_error()));
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
    }
    
    Ptr<FATINode> entry = ptr_new<FATINode>(ptr_tmp_cast(this), vol, (info.m_DOSAttribs & FAT_SUBDIR) != 0);

    entry->m_INodeID = inodeID;
    entry->m_ParentINodeID = parentINodeID;
    entry->m_DirStartIndex = info.m_StartIndex;
    entry->m_DirEndIndex = info.m_EndIndex;
    entry->m_StartCluster = info.m_StartCluster;
    entry->m_DOSAttribs = info.m_DOSAttribs;
    entry->m_Size = info.m_Size;
    if (info.m_DOSAttribs & FAT_SUBDIR) {
        size_t chainLength = vol->GetFATTable()->GetChainLength(entry->m_StartCluster);
        entry->m_Size = chainLength * vol->m_SectorsPerCluster * vol->m_BytesPerSector;
    }
    if (entry->m_StartCluster != 0)
    {
        entry->m_EndCluster = vol->GetFATTable()->GetChainEntry(info.m_StartCluster, uint32_t((entry->m_Size + vol->m_BytesPerSector * vol->m_SectorsPerCluster - 1) / vol->m_BytesPerSector / vol->m_SectorsPerCluster - 1));
    }                                               
    else
    {
        entry->m_EndCluster = 0;
    }        
    entry->m_Time = FATINode::FATTimeToUnixTime(info.m_FATTime);
    return entry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KDirectoryNode> FATFilesystem::OpenDirectory(Ptr<KFSVolume> volume, Ptr<KINode> _node)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATINode>  node = ptr_static_cast<FATINode>(_node);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::OpenDirectory (inode ID %" PRIx64 ").\n", node->m_INodeID);

    if (!(node->m_DOSAttribs & FAT_SUBDIR))
    {
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::OpenDirectory() ERROR: inode not a directory.\n");
        PERROR_THROW_CODE(PErrorCode::NotDirectory);
    }

    Ptr<FATDirectoryNode> dirNode = ptr_new<FATDirectoryNode>(O_RDONLY);
    dirNode->m_CurrentIndex = 0;
    return dirNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CreateDirectory(Ptr<KFSVolume> volume, Ptr<KINode> parent, const char* _name, int nameLength, int perms)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATINode>  dir = ptr_static_cast<FATINode>(parent);
    uint32_t i;
    String name;

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__))
    {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    if (dir->IsDeleted())
    {
        kernel_log(LOGC_DIR, KLogSeverity::ERROR, "FATFilesystem::CreateDirectory() called in removed directory.\n");
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }
    if (nameLength > 255)
    {
        PERROR_THROW_CODE(PErrorCode::NameTooLong);
    }
    name.assign(_name, nameLength);

    CRITICAL_SCOPE(vol->m_Mutex);

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::CreateDirectory() called: %" PRIx64 "/%s (perm %o)\n", dir->m_INodeID, name.c_str(), perms);

    if ((dir->m_DOSAttribs & FAT_SUBDIR) == 0)
    {
        kernel_log(LOGC_DIR, KLogSeverity::ERROR, "FATFilesystem::CreateDirectory(): inode ID %" PRIx64 " is not a directory\n", dir->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::NotDirectory);
    }

    // S_IFDIR is never set in perms, so we patch it
    perms &= ~S_IFMT;
    perms |= S_IFDIR;

    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY))
    {
        kernel_log(LOGC_DIR, KLogSeverity::ERROR, "FATFilesystem::CreateDirectory() called on read-only volume\n");
        PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
    }

    std::vector<uint8_t> buffer;
    buffer.resize(vol->m_BytesPerSector);

    /* only used to create directory entry */
    NoPtr<FATINode> dummyObj(ptr_tmp_cast(this), vol, true); /* used only to create directory entry */
    Ptr<FATINode> dummy(dummyObj);
    dummy->m_ParentINodeID = dir->m_INodeID;
    dummy->m_StartCluster = vol->GetFATTable()->AllocateClusters(1);

    PScopeFail scopeCleanupFATChain([&vol, &dummy]() {vol->GetFATTable()->ClearFATChain(dummy->m_StartCluster); });

    dummy->m_EndCluster = dummy->m_StartCluster;
    dummy->m_DOSAttribs = FAT_SUBDIR;
    if (!(perms & (S_IWUSR | S_IWGRP | S_IWGRP))) {
        dummy->m_DOSAttribs |= FAT_READ_ONLY;
    }
    dummy->m_Size = vol->m_BytesPerSector * vol->m_SectorsPerCluster;
    dummy->m_Time = get_real_time().AsSecondsI();

    dummy->m_INodeID = GENERATE_DIR_CLUSTER_INODEID(dummy->m_ParentINodeID, dummy->m_StartCluster);
    if(vol->HasINodeIDToLocationIDMapping(dummy->m_INodeID))
    {
        kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::CreateDirectory(): already have ID->location mapping for inode %" PRIx64 "\n", dummy->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    if (vol->HasLocationIDToINodeIDMapping(dummy->m_INodeID))
    {
        kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::CreateDirectory(): already have location->ID mapping for inode %" PRIx64 "\n", dummy->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    vol->AddDirectoryMapping(dummy->m_INodeID);

    PScopeFail scopeCleanupDirMapping([&vol, &dummy]() { vol->RemoveDirectoryMapping(dummy->m_INodeID); });
    

    CreateDirectoryEntry(vol, dir, dummy, name, &dummy->m_DirStartIndex, &dummy->m_DirEndIndex);

    // create '.' and '..' entries and then end of directories
    memset(&buffer[0], ' ', 11);
    memset(&buffer[0x20], ' ', 11);
    buffer[0] = buffer[0x20] = buffer[0x21] = '.';
    buffer[0x0b] = buffer[0x2b] = 0x30;
    i = FATINode::UnixTimeToFATTime(dummy->m_Time);
    buffer[0x16] = uint8_t(i & 0xff);
    buffer[0x17] = uint8_t((i >> 8) & 0xff);
    buffer[0x18] = uint8_t((i >> 16) & 0xff);
    buffer[0x19] = uint8_t((i >> 24) & 0xff);
    i = FATINode::UnixTimeToFATTime(dir->m_Time);
    buffer[0x36] = uint8_t(i & 0xff);
    buffer[0x37] = uint8_t((i >> 8) & 0xff);
    buffer[0x38] = uint8_t((i >> 16) & 0xff);
    buffer[0x39] = uint8_t((i >> 24) & 0xff);
    buffer[0x1a] = dummy->m_StartCluster & 0xff;
    buffer[0x1b] = (dummy->m_StartCluster >> 8) & 0xff;
    if (vol->m_FATBits == 32)
    {
        buffer[0x14] = (dummy->m_StartCluster >> 16) & 0xff;
        buffer[0x15] = (dummy->m_StartCluster >> 24) & 0xff;
    }
    // root directory is always denoted by cluster 0, even for fat32 (!)
    if (dir->m_INodeID != vol->m_RootINode->m_INodeID)
    {
        buffer[0x3a] = dir->m_StartCluster & 0xff;
        buffer[0x3b] = (dir->m_StartCluster >> 8) & 0xff;
        if (vol->m_FATBits == 32)
        {
            buffer[0x34] = (dir->m_StartCluster >> 16) & 0xff;
            buffer[0x35] = (dir->m_StartCluster >> 24) & 0xff;
        }
    }

    FATClusterSectorIterator csi(vol, dummy->m_StartCluster, 0);
    csi.WriteBlock(buffer.data());

    // clear out rest of cluster to keep scandisk happy
    memset(buffer.data(), 0, buffer.size());

    for (i = 1; i < vol->m_SectorsPerCluster; ++i)
    {
        csi.Increment(1);
        KCacheBlockDesc blockDesc = csi.GetBlock_(false);
        if (blockDesc.m_Buffer != nullptr)
        {
            memset(blockDesc.m_Buffer, 0, vol->m_BytesPerSector);
            blockDesc.MarkDirty();
        }
    }
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

void FATFilesystem::Rename(Ptr<KFSVolume> _vol, Ptr<KINode> _odir, const char* pzOldName, int nOldNameLen, Ptr<KINode> _ndir, const char* pzNewName, int nNewNameLen, bool bMustBeDir)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATINode>  odir = ptr_static_cast<FATINode>(_odir);
    Ptr<FATINode>  ndir = ptr_static_cast<FATINode>(_ndir);
    Ptr<FATINode>  file;
    Ptr<FATINode>  file2;
    
    uint32_t ns, ne;
    String   oldname;
    String   newname;

    if ( nOldNameLen > 255 || nNewNameLen > 255 ) {
        PERROR_THROW_CODE(PErrorCode::NameTooLong);
    }
    oldname.assign(pzOldName, nOldNameLen);
    newname.assign(pzNewName, nNewNameLen);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !odir->CheckMagic(__func__) || !ndir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    
    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Rename() called: %" PRIx64 "/%s->%Lx/%s\n", odir->m_INodeID, oldname.c_str(), ndir->m_INodeID, newname.c_str());

    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Rename(): called on read-only volume.\n");
        PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
    }
    
    // locate the file
    file = DoLocateINode(vol, odir, oldname);
    if (file == nullptr) {
        kernel_log(LOGC_FILE, KLogSeverity::CRITICAL, "FATFilesystem::Rename(): can't find file %s in directory %" PRIx64 "\n", oldname.c_str(), odir->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (!file->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    
    // see if file already exists and erase it if it does
    file2 = DoLocateINode(vol, ndir, newname);
    if (file2 != nullptr)
    {
        if (file2->m_DOSAttribs & FAT_SUBDIR)
        {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Rename(): destination already occupied by a directory\n");
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }

        ns = file2->m_DirStartIndex;
        ne = file2->m_DirEndIndex;

        // Mark inode for removal (ReleaseInode() will clear the fat chain).
        // Note we don't have to lock the file because the fat chain doesn't
        // get wiped from the disk until ReleaseInode() is called; we'll
        // have a phantom chain in effect until the last file is closed.
        file2->SetDeletedFlag(true);
    }
    else
    {
        // create the new directory entry
        CreateDirectoryEntry(vol, ndir, file, newname, &ns, &ne);
    }

    // erase old directory entry
    EraseDirectoryEntry(vol, file);
    
    // shrink the directory (an error here is not disastrous)
    CompactDirectory(vol, odir);

    // update inode information
    file->m_ParentINodeID = ndir->m_INodeID;
    file->m_DirStartIndex = ns;
    file->m_DirEndIndex = ne;

    // update vcache
    vol->SetINodeIDToLocationIDMapping(file->m_INodeID, (file->m_Size) ? GENERATE_DIR_CLUSTER_INODEID(file->m_ParentINodeID, file->m_StartCluster) : GENERATE_DIR_INDEX_INODEID(file->m_ParentINodeID, file->m_DirStartIndex));

    // XXX: only write changes in the directory entry if needed
    //      (i.e. old entry, not new)
    file->Write();

    if (file->m_DOSAttribs & FAT_SUBDIR)
    {
        // Update the ".." directory entry.
        FATDirectoryIterator diri(vol, file->m_StartCluster, 1);
        FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();
        if (buffer == nullptr) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Rename(): Error opening directory.\n");
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        if (memcmp(buffer->m_Normal.m_Filename, "..         ", sizeof(buffer->m_Normal.m_Filename))) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Rename(): Invalid directory.\n");
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        if (ndir->m_INodeID == vol->m_RootINode->m_INodeID)
        {
            // Root directory always has cluster = 0
            buffer->m_Normal.m_FirstClusterLow = 0;
            buffer->m_Normal.m_FirstClusterHigh = 0;
        }
        else
        {
            buffer->m_Normal.m_FirstClusterLow  = ndir->m_StartCluster & 0xffff;
            buffer->m_Normal.m_FirstClusterHigh = uint16_t(ndir->m_StartCluster >> 16);
        }
        diri.MarkDirty();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::Unlink(Ptr<KFSVolume> vol, Ptr<KINode> dir, const char* _name, int nameLength)
{
    String name;

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Unlink() called\n");
    
    if ( nameLength > 255 ) {
        PERROR_THROW_CODE(PErrorCode::NameTooLong);
    }
    name.assign(_name, nameLength);

    DoUnlink(vol,dir,name,true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::RemoveDirectory(Ptr<KFSVolume> vol, Ptr<KINode> dir, const char* _name, int nameLength)
{
    String name;
    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::RemoveDirectory() called\n");

    if ( nameLength > 255 ) {
        PERROR_THROW_CODE(PErrorCode::NameTooLong);
    }
    name.assign(_name, nameLength);

    DoUnlink(vol, dir, name, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATFilesystem::Read(Ptr<KFileNode> file, void* buf, size_t len, off64_t pos)
{
    Ptr<FATINode>    node = ptr_static_cast<FATINode>(file->GetINode());
    Ptr<FATVolume>   vol = ptr_static_cast<FATVolume>(node->m_Volume);
    Ptr<FATFileNode> fileNode = ptr_static_cast<FATFileNode>(file);
    size_t bytes_read = 0;
    uint32_t cluster1;
    off64_t diff;

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !fileNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (node->m_DOSAttribs & FAT_SUBDIR) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Read() called on directory %" PRIx64 "\n", node->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::IsDirectory);
    }

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Read() called %d bytes at %" PRId64 " (inode ID %" PRIx64 ")\n", len, pos, node->m_INodeID);

    if (pos < 0) pos = 0;

    if ((node->m_Size == 0) || (len == 0) || (pos >= node->m_Size)) {
        return 0;
    }

    // truncate bytes to read to file size
    if (pos + len >= node->m_Size) len = size_t(node->m_Size - pos);

    if ((fileNode->m_FATIteration == node->m_Iteration) && (pos >= fileNode->m_FATChainIndex * vol->m_BytesPerSector * vol->m_SectorsPerCluster))
    {
        // The cached fat value is both valid and helpful.
        if (!vol->IsDataCluster(fileNode->m_CachedCluster))
        {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Read() invalid m_CachedCluster %ld on inode %" PRIx64 "\n", fileNode->m_CachedCluster, node->m_INodeID);
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
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
        iter.Increment(int(diff));
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
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Read(): error reading cluster %lx, sector %lx\n", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        amt = size_t(vol->m_BytesPerSector - (pos % vol->m_BytesPerSector));
        if (amt > len) amt = len;
        memcpy(buf, static_cast<uint8_t*>(buffer.m_Buffer) + (pos % vol->m_BytesPerSector), amt);
        bytes_read += amt;

        if (bytes_read < len)
        {
            iter.Increment(1);
        }
    }

    // read middle sectors
    while (bytes_read + vol->m_BytesPerSector <= len)
    {
        iter.ReadBlock((uint8_t*)buf + bytes_read);
        bytes_read += vol->m_BytesPerSector;

        if (bytes_read < len)
        {
            iter.Increment(1);
        }
    }

    // read part of remaining sector if needed
    if (bytes_read < len) {
        size_t amt;

        KCacheBlockDesc buffer = iter.GetBlock_(true);
        if (buffer.m_Buffer == nullptr)
        {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Read(): error reading cluster %lx, sector %lx\n", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IOError);
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
    Ptr<FATINode>    node = ptr_static_cast<FATINode>(file->GetINode());
    Ptr<FATVolume>   vol = ptr_static_cast<FATVolume>(node->m_Volume);
    Ptr<FATFileNode> fileNode = ptr_static_cast<FATFileNode>(file);

    size_t   bytesWritten = 0;
    off64_t  diff;

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !fileNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (node->m_DOSAttribs & FAT_SUBDIR) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Write() called on directory %" PRIx64 "\n", node->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::IsDirectory);
    }

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Write() called %d bytes at %" PRId64 " from buffer at %lx (inode ID %" PRIx64 ")\n", len, pos, (uint32_t)buf, node->m_INodeID);

    if ((fileNode->GetOpenFlags() & O_ACCMODE) == O_RDONLY) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Write(): called on file opened as read-only.\n");
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }

    if (pos < 0) pos = 0;

    if (fileNode->GetOpenFlags() & O_APPEND) {
        pos = node->m_Size;
    }

    if (pos >= FAT_MAX_FILE_SIZE)
    {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Write(): write position exceeds fat limits.\n");
        PERROR_THROW_CODE(PErrorCode::FileTooLarge);
    }

    if (pos + len >= FAT_MAX_FILE_SIZE) {
        len = (size_t)(FAT_MAX_FILE_SIZE - pos);
    }

    uint32_t cluster1;

    if (node->m_Size && (fileNode->m_FATIteration == node->m_Iteration) && (pos >= fileNode->m_FATChainIndex * vol->m_BytesPerSector * vol->m_SectorsPerCluster))
    {
        if (!vol->IsDataCluster(fileNode->m_CachedCluster))
        {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Write() invalid m_CachedCluster %ld on inode %" PRIx64 "\n", fileNode->m_CachedCluster, node->m_INodeID);
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
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

    // extend file size if needed
    if (pos + len > node->m_Size)
    {
        const uint32_t bytesPerCluster = vol->m_BytesPerSector * vol->m_SectorsPerCluster;
        const uint32_t clusters = uint32_t((pos + len + bytesPerCluster - 1) / bytesPerCluster);
        if (node->m_Size <= (clusters - 1) * bytesPerCluster)
        {
            vol->GetFATTable()->SetChainLength(node, clusters, true);
            node->m_Iteration++;
        }
        node->m_Size = pos + len;

        // Write to cache/disk now, so inode number calculations by GetNextDirectoryEntry() are correct.
        node->Write();

        kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::Write(): Setting file size to %ld (%ld clusters)\n", node->m_Size, clusters);
    }

    if (cluster1 == 0xffffffff) {
        cluster1 = node->m_StartCluster;
        diff = pos;
    }
    diff /= vol->m_BytesPerSector; // Convert to sectors.

    FATClusterSectorIterator iter(vol, cluster1, 0);

    if (diff != 0)
    {
        iter.Increment(int(diff));
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
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Write(): error writing cluster %lx, sector %lx\n", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        size_t amt = size_t(vol->m_BytesPerSector - (pos % vol->m_BytesPerSector));
        if (amt > len) amt = len;
        memcpy(static_cast<uint8_t*>(buffer.m_Buffer) + (pos % vol->m_BytesPerSector), buf, amt);
        iter.MarkBlockDirty();
        bytesWritten += amt;

        if (bytesWritten < len)
        {
            iter.Increment(1);
        }
    }

    // write middle sectors
    while (bytesWritten + vol->m_BytesPerSector <= len)
    {
        iter.WriteBlock(static_cast<const uint8_t*>(buf) + bytesWritten);
        bytesWritten += vol->m_BytesPerSector;

        if (bytesWritten < len)
        {
            iter.Increment(1);
        }
    }

    // write part of remaining sector if needed
    if (bytesWritten < len)
    {
        size_t amt;

        KCacheBlockDesc buffer = iter.GetBlock_(true);
        if (buffer.m_Buffer == nullptr) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::Write(): error writing cluster %lx, sector %lx\n", iter.m_CurrentCluster, iter.m_CurrentSector);
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        amt = len - bytesWritten;
        memcpy(buffer.m_Buffer, (uint8_t*)buf + bytesWritten, amt);
        iter.MarkBlockDirty();
        bytesWritten += amt;
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

size_t FATFilesystem::ReadDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory, dirent_t* entry, size_t bufSize)
{
    Ptr<FATVolume>        vol = ptr_static_cast<FATVolume>(volume);
    Ptr<FATDirectoryNode> dirNode = ptr_static_cast<FATDirectoryNode>(directory);
    Ptr<FATINode>         dir = ptr_static_cast<FATINode>(directory->GetINode());

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__) || !dirNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::ReadDirectory(): inode ID %" PRIx64 ", index %lx\n", dir->m_INodeID, dirNode->m_CurrentIndex);

    entry->d_reclen = sizeof(*entry);
    // simulate '.' and '..' entries for root directory
    if (dir->m_INodeID == vol->m_RootINode->m_INodeID)
    {
        if (dirNode->m_CurrentIndex >= 2)
        {
            dirNode->m_CurrentIndex -= 2;
        }
        else
        {
            if (dirNode->m_CurrentIndex++ == 0)
            {
                strcpy(entry->d_name, ".");
                entry->d_namlen = 1;
            }
            else
            {
                strcpy(entry->d_name, "..");
                entry->d_namlen = 2;
            }
            entry->d_type = DT_DIR;
            entry->d_ino = vol->m_RootINode->m_INodeID;
            entry->d_volumeid = vol->m_VolumeID;
            return sizeof(dirent_t);
        }
    }

    FATDirectoryIterator diri(vol, dir->m_StartCluster, dirNode->m_CurrentIndex);
    String fileName;
    uint32_t dosAttributes = 0;

    bool entryFound = false;
    if (diri.GetNextDirectoryEntry(dir, &entry->d_ino, &fileName, &dosAttributes))
    {
        entryFound = true;
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::ReadDirectory(): found file '%s' / %" PRId32 "\n", fileName.c_str(), fileName.size());
        if (fileName.size() <= NAME_MAX)
        {
            fileName.copy(entry->d_name, fileName.size());
            entry->d_name[fileName.size()] = 0;
            entry->d_type = (dosAttributes & FAT_SUBDIR) ? DT_DIR : DT_REG;

        }
        else
        {
            PERROR_THROW_CODE(PErrorCode::NameTooLong);
        }
    }
    dirNode->m_CurrentIndex = diri.m_CurrentIndex;

    if (dir->m_INodeID == vol->m_RootINode->m_INodeID) {
        dirNode->m_CurrentIndex += 2;
    }
    if (entryFound)
    {
        entry->d_volumeid = vol->m_VolumeID;
        entry->d_namlen   = static_cast<decltype(entry->d_namlen)>(strlen(entry->d_name));
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::ReadDirectory(): found file %s\n", entry->d_name);
        return sizeof(dirent_t);
    }
    else
    {
        return 0; // End of directory
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
			
void FATFilesystem::RewindDirectory(Ptr<KFSVolume> _vol, Ptr<KDirectoryNode> _dirNode)
{
    Ptr<FATVolume>        vol     = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATDirectoryNode> dirNode = ptr_static_cast<FATDirectoryNode>(_dirNode);
    Ptr<FATINode>         node    = ptr_static_cast<FATINode>(_dirNode->GetINode());

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__) || !dirNode->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::RewindDirectory() (inode ID %" PRIx64 ")\n", node->m_INodeID);

    dirNode->m_CurrentIndex = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATFilesystem::ReadLink(Ptr<KFSVolume> _vol, Ptr<KINode> _node, char* buffer, size_t bufferSize)
{
    // no links in fat...
    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::WARNING, "FATFilesystem::ReadLink() called\n");

    PERROR_THROW_CODE(PErrorCode::InvalidArg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CheckAccess(Ptr<KFSVolume> _vol, Ptr<KINode> _node, int mode)
{
    Ptr<FATVolume> vol  = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATINode>  node = ptr_static_cast<FATINode>(_node);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::CheckAccess(inode ID %" PRIx64 ", mode %x)\n", node->m_INodeID, mode);

    if ((mode & O_ACCMODE) != O_RDONLY)
    {
        if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::CheckAccess(): can't write on read-only volume\n");
            PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
        } else if (node->m_DOSAttribs & FAT_READ_ONLY) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::CheckAccess(): can't open read-only file for writing\n");
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::ReadStat(Ptr<KFSVolume> _vol, Ptr<KINode> _node, struct stat* st)
{
    Ptr<FATVolume> vol  = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATINode>  node = ptr_static_cast<FATINode>(_node);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::ReadStat(inode ID %" PRIx64 ")\n", node->m_INodeID);

    st->st_dev = dev_t(vol->m_VolumeID);
    st->st_ino = node->m_INodeID;
    st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH;
    if (node->m_DOSAttribs & FAT_SUBDIR) {
        st->st_mode &= ~S_IFREG;
        st->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    }
    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY) || (node->m_DOSAttribs & FAT_READ_ONLY)) {
        st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    }
    
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_size = off_t(node->m_Size);
    st->st_blksize = 0x10000; /* this value was chosen arbitrarily */
    st->st_atim = st->st_mtim = st->st_ctim = { .tv_sec = node->m_Time, .tv_nsec = 0 };
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::WriteStat(Ptr<KFSVolume> _vol, Ptr<KINode> _node, const struct stat* st, uint32_t mask)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATINode>  node = ptr_static_cast<FATINode>(_node);
    bool dirty = false;

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::WriteStat(inode ID %" PRIx64 ")\n", node->m_INodeID);

    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
        kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::WriteStat(): read-only volume\n");
        PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
    }

    if (mask & WSTAT_MODE)
    {
        kernel_log(LOGC_FILE, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::WriteStat(): setting file mode to %o\n", st->st_mode);
        if (st->st_mode & S_IWUSR) {
            node->m_DOSAttribs &= uint8_t(~FAT_READ_ONLY);
        } else {
            node->m_DOSAttribs |= FAT_READ_ONLY;
        }
        dirty = true;
    }

    if (mask & WSTAT_SIZE)
    {
        kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::WriteStat(): setting file size to %Lx\n", st->st_size);
        if (node->m_DOSAttribs & FAT_SUBDIR)
        {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::WriteStat(): can't set file size of directory!\n");
            PERROR_THROW_CODE(PErrorCode::IsDirectory);
        }
		if (size64_t(st->st_size) > FAT_MAX_FILE_SIZE)
		{
			kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::WriteStat(): desired file size exceeds fat limit\n");
            PERROR_THROW_CODE(PErrorCode::FileTooLarge);
		}
        uint32_t clusters = (uint32_t(st->st_size) + vol->m_BytesPerSector*vol->m_SectorsPerCluster - 1) / vol->m_BytesPerSector / vol->m_SectorsPerCluster;
        kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::WriteStat(): setting FAT chain length to %lx clusters\n", clusters);
        vol->GetFATTable()->SetChainLength(node, clusters, true);
        node->m_Size = st->st_size;
        // TODO: clear new section if file was extended.
#ifdef FAT_VERIFY_FAT_CHAINS
        kassert(node->m_Size == 0 || vol->GetFATTable()->ValidateChainEntry(node->m_StartCluster, uint32_t((node->m_Size-1) / (vol->m_BytesPerSector * vol->m_SectorsPerCluster)), node->m_EndCluster));
#endif // FAT_VERIFY_FAT_CHAINS
        node->m_Iteration++;
        dirty = true;
    }
    
    if (mask & WSTAT_MTIME)
    {
        kernel_log(LOGC_FILE, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::WriteStat(): setting modification time\n");
        node->m_Time = st->st_mtim.tv_sec;
        dirty = true;
    }

    if (dirty)
    {
        node->Write();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<FATINode>  node = ptr_static_cast<FATINode>(file->GetINode());
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(node->m_Volume);

    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !node->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    switch (request)
    {
	case 10002 : /* return real creation time */
            if (outData != nullptr && outDataLength == sizeof(bigtime_t)) {
	            *static_cast<bigtime_t*>(outData) = node->m_Time;
                return;
            } else {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }	    
	case 100000:
	    kprintf("vol info: %s (device %x, media descriptor %x)\n", vol->m_DevicePath.c_str(), vol->m_DeviceFile, vol->m_MediaDescriptor);
	    kprintf("%lx bytes/sector, %lx sectors/cluster\n", vol->m_BytesPerSector, vol->m_SectorsPerCluster);
	    kprintf("%lx reserved sectors, %lx total sectors\n", vol->m_ReservedSectors, vol->m_TotalSectors);
	    kprintf("%lx %d-bit fats, %lx sectors/fat, %lx root entries\n", vol->m_FATCount, vol->m_FATBits, vol->m_SectorsPerFAT, vol->m_RootEntriesCount);
	    kprintf("root directory starts at sector %lx (cluster %lx), data at sector %lx\n", vol->m_RootStart, vol->m_RootINode->m_StartCluster, vol->m_FirstDataSector);
	    kprintf("%lx total clusters, %lx free\n", vol->m_TotalClusters, vol->m_FreeClusters);
	    kprintf("fat mirroring is %s, fs info sector at sector %x\n", (vol->m_FATMirrored) ? "on" : "off", vol->m_FSInfoSector);
	    kprintf("last allocated cluster = %lx\n", vol->m_LastAllocatedCluster);
	    kprintf("root inode id = %" PRIx64 "\n", vol->m_RootINode->m_INodeID);
	    kprintf("volume label [%11.11s]\n", vol->m_VolumeLabel);
	    return;
			
	case 100001 :
	    kprintf("inode id %Lx, dir inode = %Lx\n", node->m_INodeID, node->m_ParentINodeID);
	    kprintf("si = %lx, ei = %lx\n", node->m_DirStartIndex, node->m_DirEndIndex);
	    kprintf("cluster = %lx (%lx), mode = %lx, size = %Lx\n", node->m_StartCluster, vol->m_FirstDataSector + vol->m_SectorsPerCluster * (node->m_StartCluster - 2), node->m_DOSAttribs, node->m_Size);
	    vol->GetFATTable()->DumpChain(node->m_StartCluster);
	    return;

	case 100004 :
	    kprintf("Dumping inode map for %x\n", vol->m_VolumeID);
	    vol->DumpINodeIDMap();
	    return;

	case 100005 :
	    kprintf("Dumping directory map for %x\n", vol->m_VolumeID);
	    vol->DumpDirectoryMap();
	    return;

	default :
	    kernel_log(LOGC_FS, KLogSeverity::ERROR, "FATFilesystem::DeviceControl(): vol %x, inode %" PRIx64 " code = %d\n", vol->m_VolumeID, node->m_INodeID, request);
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    PERROR_THROW_CODE(PErrorCode::InvalidArg);
}

///////////////////////////////////////////////////////////////////////////////
// Doesn't do any name checking
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATFilesystem::CreateVolumeLabel(Ptr<FATVolume> vol, const char* name)
{
    uint32_t dummy;
    struct FATNewDirEntryInfo info = {
        FAT_ARCHIVE | FAT_VOLUME, 0, 0, 0
    };
    info.time = get_real_time().AsSecondsI();

    // check if name already exists
    if (FindShortName(vol, vol->m_RootINode, name)) {
        PERROR_THROW_CODE(PErrorCode::Exist);
    }
    uint32_t index;
    DoCreateDirectoryEntry(vol, vol->m_RootINode, &info, name, nullptr, 0, &index, &dummy);
    return index;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATVolume::UpdateFSInfo()
{
    if (m_FSInfoSector != 0xffff && !HasFlag(FSVolumeFlags::FS_IS_READONLY))
    {
        KCacheBlockDesc bufferDesc = m_BCache.GetBlock_trw(m_FSInfoSector);
        FATFSInfo* buffer = static_cast<FATFSInfo*>(bufferDesc.m_Buffer);
        if (buffer != nullptr)
        {
            if (buffer->m_Signature1 == 0x41615252 && buffer->m_Signature2 == 0x61417272 && buffer->m_Signature3 == 0xaa550000)
            {
                buffer->m_FreeClusters         = m_FreeClusters;
                buffer->m_LastAllocatedCluster = m_LastAllocatedCluster;
                bufferDesc.MarkDirty();
            }
            else
            {
                uint32_t signature1 = buffer->m_Signature1;
                uint32_t signature2 = buffer->m_Signature2;
                uint32_t signature3 = buffer->m_Signature3;
                kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::CRITICAL, "FATVolume::UpdateFSInfo(): fsinfo block has invalid magic number %08x, %08x, %08x\n", signature1, signature2, signature3);
            }
        }
        else
        {
            kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::ERROR, "FATVolume::UpdateFSInfo(): error getting fsinfo sector %x\n", m_FSInfoSector);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Name is array of char[11] as returned by findfile
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATFilesystem::FindShortName(Ptr<FATVolume> vol, Ptr<FATINode> parent, const char* rawShortName)
{
    FATDirectoryIterator diri(vol, parent->m_StartCluster, 0);
    
    for (FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry(); buffer != nullptr; buffer = diri.GetNextRawEntry())
    {
        if (buffer->m_Normal.m_Filename[0] == 0) {
            break;
        }
        if (buffer->m_Normal.m_Attribs != 0xf) { // Not long file name
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

Ptr<FATINode> FATFilesystem::DoLocateINode(Ptr<FATVolume> vol, Ptr<FATINode> dir, const String& fileName)
{
    ino_t inodeID;

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }        

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::DoLocateINode(): %s in %" PRIx64 "\n", fileName.c_str(), dir->m_INodeID);

    if (fileName == "." && dir->m_INodeID == vol->m_RootINode->m_INodeID)
    {
        inodeID = dir->m_INodeID;
    }
    else if (fileName == ".." && dir->m_INodeID == vol->m_RootINode->m_INodeID)
    {
        inodeID = dir->m_ParentINodeID;
    }
    else
    {
        FATDirectoryIterator diri(vol, dir->m_StartCluster, 0);

        bool found = false;
        for(;;)
        {
            String curName;
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
    return ptr_static_cast<FATINode>(KVFSManager::GetINode_trw(vol->m_VolumeID, inodeID, false));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATFilesystem::IsDirectoryEmpty(Ptr<FATVolume> volume, Ptr<FATINode> dir)
{
    if (!volume->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    FATDirectoryIterator iter(volume, dir->m_StartCluster, 0);

    if (iter.GetCurrentEntry() == nullptr) {
        kernel_log(LOGC_DIR, KLogSeverity::ERROR, "FATFilesystem::IsDirectoryEmpty(): error opening directory\n");
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    uint32_t i = (dir->m_INodeID == volume->m_RootINode->m_INodeID) ? 2 : 0;

    for (; i < 3; ++i)
    {
        String filename;

        if (!iter.GetNextLFNEntry(nullptr, &filename)) {
            return i == 2;
        }

        // weird case where ./.. are stored as long file names
        if ((i == 0 && filename != ".") || (i == 1 && filename != "..") || (i < 2 && iter.m_CurrentIndex != i + 1))
        {
            kernel_log(LOGC_DIR, KLogSeverity::CRITICAL, "FATFilesystem::IsDirectoryEmpty(): malformed directory\n");
            PERROR_THROW_CODE(PErrorCode::NotDirectory);
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::CreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> parent, Ptr<FATINode> node, const String& name, uint32_t* startIndex, uint32_t* endIndex)
{
    struct FATNewDirEntryInfo info;

    // check if name already exists
    if (DoLocateINode(vol, parent, name)  != nullptr)
    {
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::CreateDirectoryEntry(): %s already found in directory %" PRIx64 "\n", name.c_str(), parent->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::Exist);
    }

    // check name legality while converting. We ignore the case conversion
    // flag, i.e. (filename "blah" will always have a patched short name),
    // because the whole case conversion system in dos is brain damaged;
    // remnants of CP/M no less.

    // existing names pose a problem; in these cases, we'll just live with
    // two identical short names. not a great solution, but there's little
    // we can do about it.

    std::vector<wchar16_t> longName;

    longName.resize(258, 0xffff);

    size_t len = name.copy_utf16(longName.data(), longName.size());

    if (len == longName.size())
    {
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATFilesystem::CreateDirectoryEntry(): Error converting utf8 name '%s' to UNICODE. Result to long.\n", name.c_str());
        PERROR_THROW_CODE(PErrorCode::NameTooLong);
    }
    longName[len++] = 0;

    char shortName[11];
    FATDirectoryIterator::GenerateShortName(longName.data(), len, shortName);

    // If there is a long name, patch short name and check for duplication
    if (FATDirectoryIterator::RequiresLongName(longName.data(), len))
    {
        char tempName[11]; // Temporary short name

        memcpy(tempName, shortName, 11);

        bool foundFreeName = false;
        for (int i = 1; i <= 10; ++i)
        {
            FATDirectoryIterator::MungeShortName(shortName, i);
            
            kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::CreateDirectoryEntry(): trying short name %11.11s\n", shortName);
            
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
                kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::CreateDirectoryEntry(): trying short name %11.11s\n", shortName);
                
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
            PERROR_THROW_CODE(PErrorCode::NoSpace); // Failed to find an unused short name.
        }
    }
    else
    {
        len = 0; // Entry doesn't need a long name.
    }

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::CreateDirectoryEntry(): creating directory entry (%11.11s)\n", shortName);

    info.m_DOSAttribs = node->m_DOSAttribs;
    info.cluster = node->m_StartCluster;
    info.size    = size_t(node->m_Size);
    info.time    = node->m_Time;

    DoCreateDirectoryEntry(vol, parent, &info, (char*)shortName, longName.data(), len, startIndex, endIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::DoCreateDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> dir, FATNewDirEntryInfo* info, const char shortName[11], const wchar16_t* longName, uint32_t len, uint32_t* startIndex, uint32_t* endIndex)
{
    if (g_DOSDeviceNames.count(String(shortName, 11)) != 0)
    {
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }
    if ((info->cluster != 0) && !vol->IsDataCluster(info->cluster))
    {
        kernel_log(LOGC_DIR, KLogSeverity::CRITICAL, "FATFilesystem::DoCreateDirectoryEntry(): for bad cluster (%lx)\n", info->cluster);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    /* convert byte length of unicode name to directory entries */
    size_t required_entries = (len + 12) / 13 + 1;

    // find a place to put the entries
    *startIndex = 0;
    bool last_entry = true;
    {
        FATDirectoryIterator diri(vol, dir->m_StartCluster, 0);
        int         loops = 0;
        while (diri.GetCurrentEntry() != nullptr)
        {
            FATDirectoryEntryInfo info;

            if (loops++ > 1000) {
                kernel_log(LOGC_DIR, KLogSeverity::CRITICAL, "FATFilesystem::DoCreateDirectoryEntry(): infinit loop.\n" );
                break;
            }
            if (diri.GetNextLFNEntry(&info, nullptr))
            {
                if (info.m_StartIndex - *startIndex >= required_entries) {
                    last_entry = false;
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
    // If at end of directory, last_entry flag will be true as it should be

    *endIndex = *startIndex + required_entries - 1;

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::DoCreateDirectoryEntry(): directory entry runs from %lx to %lx (dirsize = %" PRIx64 ") (is%s last entry)\n", *startIndex, *endIndex, dir->m_Size, last_entry ? "" : "n't");

    bool wasExpanded = false;
    // check if the directory needs to be expanded
    if (*endIndex * sizeof(FATDirectoryEntry) >= dir->m_Size)
    {
        uint32_t clusters_needed;

        // can't expand fat12 and fat16 root directories :(
        if (IS_FIXED_ROOT(dir->m_StartCluster)) {
            kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::WARNING, "FATFilesystem::DoCreateDirectoryEntry(): out of space in root directory\n");
            PERROR_THROW_CODE(PErrorCode::NoSpace);
        }
        
        // otherwise grow directory to fit
        clusters_needed = ((*endIndex + 1) * sizeof(FATDirectoryEntry) + vol->m_BytesPerSector * vol->m_SectorsPerCluster - 1) / vol->m_BytesPerSector / vol->m_SectorsPerCluster;

        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::DoCreateDirectoryEntry(): expanding directory from %Lx to %lx clusters\n", dir->m_Size/vol->m_BytesPerSector/vol->m_SectorsPerCluster, clusters_needed);

        vol->GetFATTable()->SetChainLength(dir, clusters_needed, true);

        dir->m_Size = vol->m_BytesPerSector * vol->m_SectorsPerCluster * clusters_needed;
        dir->m_Iteration++;
        wasExpanded = true;
    }

    // starting blitting entries
    FATDirectoryIterator diri(vol,dir->m_StartCluster, *startIndex);
    FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();
    uint8_t hash = FATDirectoryIterator::HashMSDOSName(shortName);

    // write lfn entries
    for (size_t i = 1; i < required_entries && buffer != nullptr; ++i, buffer = diri.GetNextRawEntry())
    {
        const wchar16_t* p = longName + (required_entries - i - 1) * 13; // Go to unicode offset
        memset(buffer, 0, sizeof(*buffer));
        
        buffer->m_LFN.m_SequenceNumber = uint8_t(required_entries - i + ((i == 1) ? 0x40 : 0));
        buffer->m_LFN.m_Attribs = 0x0f;
        buffer->m_LFN.m_Hash = hash;
        memcpy(buffer->m_LFN.m_NamePart1, p, sizeof(buffer->m_LFN.m_NamePart1));
        p += ARRAY_COUNT(buffer->m_LFN.m_NamePart1);
        memcpy(buffer->m_LFN.m_NamePart2, p, sizeof(buffer->m_LFN.m_NamePart2));
        p += ARRAY_COUNT(buffer->m_LFN.m_NamePart2);
        memcpy(buffer->m_LFN.m_NamePart3, p, sizeof(buffer->m_LFN.m_NamePart3));
        diri.MarkDirty();
    }

    if (buffer == nullptr) { // This should never happen.
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATFilesystem::DoCreateDirectoryEntry(): Iteration failed\n");
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    // write directory entry
    memcpy(buffer->m_Normal.m_Filename, shortName, sizeof(buffer->m_Normal.m_Filename));
    buffer->m_Normal.m_Attribs = info->m_DOSAttribs;
    memset(buffer->m_Normal.m_Unused1, 0, sizeof(buffer->m_Normal.m_Unused1));
    buffer->m_Normal.m_Time = FATINode::UnixTimeToFATTime(info->time);

    if (info->size == 0) {		// cluster = 0 for 0 byte files
        buffer->m_Normal.m_FirstClusterLow = 0;
        buffer->m_Normal.m_FirstClusterHigh = 0;
    } else {
        buffer->m_Normal.m_FirstClusterLow  = info->cluster & 0xffff;
        buffer->m_Normal.m_FirstClusterHigh = uint16_t(info->cluster >> 16);
    }
    buffer->m_Normal.m_FileSize = (info->m_DOSAttribs & FAT_SUBDIR) ? 0 : info->size;
    diri.MarkDirty();
    
    if (wasExpanded)
    {
        kassert(last_entry);
        // Add end of directory markers to the rest of the cluster. Clear all entries to stop scandisk complaining.
        while ((buffer = diri.GetNextRawEntry()) != nullptr) {
            memset(buffer, 0, sizeof(FATDirectoryEntry));
            diri.MarkDirty();
        }
    }
}

// shrink directory to the size needed
// errors here are neither likely nor problematic
// w95 doesn't seem to do this, so it's possible to create a
// really large directory that consumes all available space!
void FATFilesystem::CompactDirectory(Ptr<FATVolume> vol, Ptr<FATINode> dir)
{
    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::CompactDirectory(): compacting directory with inode ID %" PRIx64 "\n", dir->m_INodeID);

    // root directory can't shrink in fat12 and fat16
    if (IS_FIXED_ROOT(dir->m_StartCluster)) {
        return;
    }

    FATDirectoryIterator    diri(vol, dir->m_StartCluster, 0);
    int                     loops = 0;
    uint32_t                last = 0;

    while (diri.GetCurrentEntry() != nullptr)
    {
        FATDirectoryEntryInfo info;

        if ( loops++ > 1000 ) {
            kernel_log(LOGC_DIR, KLogSeverity::CRITICAL, "FATFilesystem::CompactDirectory(): infinit loop.\n" );
            break;
        }
        
        if (diri.GetNextLFNEntry(&info, nullptr))
        {
            // don't compact away volume labels in the root dir
            if (!(info.m_DOSAttribs & FAT_VOLUME) || (dir->m_INodeID != vol->m_RootINode->m_INodeID)) {
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
                kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::CompactDirectory(): shrinking directory to %lx clusters\n", clusters);
                vol->GetFATTable()->SetChainLength(dir, clusters, true);
                dir->m_Size = clusters*vol->m_BytesPerSector*vol->m_SectorsPerCluster;
                dir->m_Iteration++;
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::EraseDirectoryEntry(Ptr<FATVolume> vol, Ptr<FATINode> node)
{
    uint32_t i;
    FATDirectoryEntryInfo info;
    
    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATFilesystem::EraseDirectoryEntry(): erasing directory entries %lx through %lx\n", node->m_DirStartIndex, node->m_DirEndIndex);
    {
        FATDirectoryIterator diri(vol, CLUSTER_OF_DIR_CLUSTER_INODEID(node->m_ParentINodeID), node->m_DirStartIndex);
        FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();

        // first pass: check if the entry is still valid
        if (buffer == nullptr) {
            kernel_log(LOGC_DIR, KLogSeverity::ERROR, "FATFilesystem::EraseDirectoryEntry(): error reading directory\n");
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        if (!diri.GetNextLFNEntry(&info, nullptr)) {
            PERROR_THROW_CODE(PErrorCode::NoEntry);
        }
    }        
    
    if (info.m_StartIndex != node->m_DirStartIndex || info.m_EndIndex != node->m_DirEndIndex)
    {
        // Any other attributes may be in a state of flux due to wstat calls
        kernel_log(LOGC_DIR, KLogSeverity::CRITICAL, "erase_dir_entry: directory entry doesn't match\n");
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    // second pass: actually erase the entry
    FATDirectoryIterator diri(vol, CLUSTER_OF_DIR_CLUSTER_INODEID(node->m_ParentINodeID), node->m_DirStartIndex);
    FATDirectoryEntryCombo* buffer = diri.GetCurrentEntry();
    for (i = node->m_DirStartIndex; i <= node->m_DirEndIndex && buffer != nullptr; buffer = diri.GetNextRawEntry(), ++i) {
        buffer->m_Normal.m_Filename[0] = 0xe5; // Mark entry erased.
        diri.MarkDirty();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATFilesystem::DoUnlink(Ptr<KFSVolume> _vol, Ptr<KINode> _dir, const String& name, bool removeFile)
{
    Ptr<FATVolume> vol = ptr_static_cast<FATVolume>(_vol);
    Ptr<FATINode>  dir = ptr_static_cast<FATINode>(_dir);

    if (name == "." || name == "..") {
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }
    CRITICAL_SCOPE(vol->m_Mutex);

    if (!vol->CheckMagic(__func__) || !dir->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log(LOGC_FILE, KLogSeverity::INFO_LOW_VOL, "FATFilesystem::DoUnlink(): %" PRIx64 "/%s\n", dir->m_INodeID, name.c_str());

    if (vol->HasFlag(FSVolumeFlags::FS_IS_READONLY)) {
        kernel_log(LOGC_DIR, KLogSeverity::ERROR, "FATFilesystem::DoUnlink(): read-only volume\n");
        PERROR_THROW_CODE(PErrorCode::ReadOnlyFilesystem);
    }

    // locate the file
    Ptr<FATINode> file = DoLocateINode(vol, dir, name);
    if (file == nullptr) {
        kernel_log(LOGC_FILE, KLogSeverity::CRITICAL, "FATFilesystem::DoUnlink(): can't find file %s in directory %" PRIx64 "\n", name.c_str(), dir->m_INodeID);
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }

    if (!file->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (removeFile)
    {
        if (file->m_DOSAttribs & FAT_SUBDIR) {
            PERROR_THROW_CODE(PErrorCode::IsDirectory);
        }
    }
    else
    {
        if ((file->m_DOSAttribs & FAT_SUBDIR) == 0) {
            PERROR_THROW_CODE(PErrorCode::NotDirectory);
        }
        if (file->m_INodeID == vol->m_RootINode->m_INodeID) {
            kernel_log(LOGC_FILE, KLogSeverity::ERROR, "FATFilesystem::DoUnlink(): don't call this on the root directory.\n");
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }
        if (!IsDirectoryEmpty(vol, file)) {
            PERROR_THROW_CODE(PErrorCode::NotEmpty);
        }
    }

    // Erase the entry in the parent directory.
    EraseDirectoryEntry(vol, file);

    // Shrink the parent directory.
    CompactDirectory(vol, dir);

    // Set the loc to a unique value. This effectively removes it from the
    // inode cache without releasing its inode number for reuse. This is ok because the
    // inode is locked in memory after this point and loc will not be
    // referenced from here on.

    vol->SetINodeIDToLocationIDMapping(file->m_INodeID, vol->AllocUniqueINodeID());

    file->SetDeletedFlag(true);
}

} // namespace
