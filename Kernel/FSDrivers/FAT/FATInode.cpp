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
// Created: 18/05/19 13:40:19

#include "System/Platform.h"

#include <string.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>

#include "FATInode.h"
#include "FATVolume.h"
#include "FATDirectoryIterator.h"
#include "Kernel/FSDrivers/FAT/FATFilesystem.h"

namespace kernel
{

static bool GetFATInodeWriteLocation(const FATInode& inode, FATVolume& volume, uint32_t& parentCluster, uint32_t& expectedStartCluster)
{
    if (!volume.GetDirectoryStartCluster(inode.m_ParentInodeID, &parentCluster))
    {
        kernel_log<PLogSeverity::CRITICAL>(
            LogCat_FATFS,
            "FATInode::Write(): inode {:x} has invalid parent inode {:x}.",
            inode.m_InodeID,
            inode.m_ParentInodeID);
        return false;
    }

    if (inode.m_DirEndIndex < inode.m_DirStartIndex ||
        inode.m_DirEndIndex - inode.m_DirStartIndex > FAT_LONG_NAME_MAX_ENTRY_COUNT)
    {
        kernel_log<PLogSeverity::CRITICAL>(
            LogCat_FATFS,
            "FATInode::Write(): inode {:x} has invalid directory location {}:{} through {}.",
            inode.m_InodeID,
            parentCluster,
            inode.m_DirStartIndex,
            inode.m_DirEndIndex);
        return false;
    }

    ino_t locationID;
    if (!volume.GetInodeIDToLocationIDMapping(inode.m_InodeID, &locationID)) {
        locationID = inode.m_InodeID;
    }

    if (IS_ARTIFICIAL_INODEID(locationID) ||
        IS_INVALID_INODEID(locationID) ||
        DIR_OF_INODEID(locationID) != parentCluster)
    {
        kernel_log<PLogSeverity::CRITICAL>(
            LogCat_FATFS,
            "FATInode::Write(): inode {:x} has invalid location mapping {:x}.",
            inode.m_InodeID,
            locationID);
        return false;
    }

    if (IS_DIR_INDEX_INODEID(locationID))
    {
        if (INDEX_OF_DIR_INDEX_INODEID(locationID) != inode.m_DirStartIndex)
        {
            kernel_log<PLogSeverity::CRITICAL>(
                LogCat_FATFS,
                "FATInode::Write(): inode {:x} is mapped to directory entry {}, not {}.",
                inode.m_InodeID,
                INDEX_OF_DIR_INDEX_INODEID(locationID),
                inode.m_DirStartIndex);
            return false;
        }
        expectedStartCluster = 0;
        return true;
    }

    if (IS_DIR_CLUSTER_INODEID(locationID))
    {
        expectedStartCluster = CLUSTER_OF_DIR_CLUSTER_INODEID(locationID);
        if (!volume.IsDataCluster(expectedStartCluster))
        {
            kernel_log<PLogSeverity::CRITICAL>(
                LogCat_FATFS,
                "FATInode::Write(): inode {:x} is mapped to invalid cluster {}.",
                inode.m_InodeID,
                expectedStartCluster);
            return false;
        }
        return true;
    }

    kernel_log<PLogSeverity::CRITICAL>(
        LogCat_FATFS,
        "FATInode::Write(): inode {:x} has unsupported location mapping {:x}.",
        inode.m_InodeID,
        locationID);
    return false;
}

static bool IsFATInodeWriteEntryValid(const FATInode& inode, const FATVolume& volume, const FATDirectoryEntry& entry, uint32_t expectedStartCluster)
{
    const uint8_t firstFilenameCharacter = uint8_t(entry.m_Filename[0]);
    const bool isLongNameEntry = (entry.m_Attribs & FAT_LONG_NAME_ATTRIBUTE_MASK) == FAT_LONG_NAME_ATTRIBUTES;
    const bool isVolumeLabel = (entry.m_Attribs & FAT_VOLUME) != 0;
    const bool isDotEntry =
        memcmp(entry.m_Filename, ".          ", sizeof(entry.m_Filename)) == 0 ||
        memcmp(entry.m_Filename, "..         ", sizeof(entry.m_Filename)) == 0;
    const bool entryIsDirectory = (entry.m_Attribs & FAT_SUBDIR) != 0;

    if (firstFilenameCharacter == 0 ||
        firstFilenameCharacter == 0xe5 ||
        isLongNameEntry ||
        isVolumeLabel ||
        isDotEntry ||
        entryIsDirectory != inode.IsDirectory())
    {
        kernel_log<PLogSeverity::CRITICAL>(
            LogCat_FATFS,
            "FATInode::Write(): directory entry {} no longer identifies inode {:x} (first byte {:02x}, attributes {:02x}).",
            inode.m_DirEndIndex,
            inode.m_InodeID,
            uint32_t(firstFilenameCharacter),
            uint32_t(entry.m_Attribs));
        return false;
    }

    uint32_t entryStartCluster = entry.m_FirstClusterLow;
    if (volume.m_FATBits == 32) {
        entryStartCluster |= uint32_t(entry.m_FirstClusterHigh) << 16;
    }

    if (entryStartCluster != expectedStartCluster)
    {
        kernel_log<PLogSeverity::CRITICAL>(
            LogCat_FATFS,
            "FATInode::Write(): directory entry {} refers to cluster {}, expected {} for inode {:x}.",
            inode.m_DirEndIndex,
            entryStartCluster,
            expectedStartCluster,
            inode.m_InodeID);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATInode::FATInode(Ptr<FATFilesystem> filesystem, Ptr<KFSVolume> volume, mode_t fileMode)
    : KInode(filesystem, volume, ptr_raw_pointer_cast(filesystem), fileMode)
    , m_Magic(MAGIC)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATInode::~FATInode()
{
    kassert(!m_MetadataDirty);
    kassert(!m_DirtyListNode.IsListMember());
    m_Magic = ~MAGIC;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATInode::CheckMagic(const char* functionName)
{
    if (m_Magic != MAGIC)
    {
        panic("{} passed inode with invalid magic number {:#08x}", functionName, m_Magic);
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATInode::MarkMetadataDirty() noexcept
{
    if (!m_MetadataDirty)
    {
        FATVolume* volume = static_cast<FATVolume*>(ptr_raw_pointer_cast(m_Volume));
        kassert(volume != nullptr);
        volume->AddDirtyInode(this);
        m_MetadataDirty = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATInode::DiscardPendingMetadata() noexcept
{
    if (m_MetadataDirty)
    {
        FATVolume* volume = static_cast<FATVolume*>(ptr_raw_pointer_cast(m_Volume));
        kassert(volume != nullptr);
        volume->RemoveDirtyInode(this);
        m_MetadataDirty = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATInode::MarkContentsModified(bool updateModificationTime, bool updateAccessTime) noexcept
{
    if (updateModificationTime || updateAccessTime)
    {
        const TimeValNanos currentTime = get_real_time();
        if (updateModificationTime) {
            m_MTime = RoundTimeToFATModificationTime(currentTime);
        }
        if (updateAccessTime) {
            m_ATime = RoundTimeToFATAccessTime(currentTime);
        }
    }
    m_DOSAttribs |= FAT_ARCHIVE;

    if (!IsDeleted()) {
        MarkMetadataDirty();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATInode::Write()
{
    // don't update entries of deleted files
    if (IsDeleted())
    {
        DiscardPendingMetadata();
        return;
    }

    Ptr<FATVolume> volume = ptr_static_cast<FATVolume>(m_Volume);

    // The root inode has no containing directory entry in which to persist
    // metadata. Keep explicit metadata changes in memory, consistent with
    // directory-content timestamp updates.
    if (m_InodeID == volume->m_RootInode->m_InodeID)
    {
        DiscardPendingMetadata();
        return;
    }

    if ((m_StartCluster != 0) && !volume->IsDataCluster(m_StartCluster))
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATInode::Write() called on invalid cluster ({}).", m_StartCluster);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    uint32_t parentCluster;
    uint32_t expectedStartCluster;
    if (!GetFATInodeWriteLocation(*this, *volume, parentCluster, expectedStartCluster)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    {
        FATDirectoryIterator directoryIterator(volume, parentCluster, m_DirEndIndex);
        FATDirectoryEntryCombo* buffer = directoryIterator.GetCurrentEntry();
        if (buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        if (!IsFATInodeWriteEntryValid(*this, *volume, buffer->m_Normal, expectedStartCluster)) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        buffer->m_Normal.m_Attribs = m_DOSAttribs; // file attributes

        const uint32_t createTime = UnixTimeToFATTime(m_CTime.AsSecondsI());
        const uint32_t accessTime = UnixTimeToFATTime(m_ATime.AsSecondsI());
        const uint32_t modificationTime = UnixTimeToFATTime(m_MTime.AsSecondsI());

        buffer->m_Normal.m_CreateTimeFine = TimeValToFATCreateTimeFine(m_CTime);
        buffer->m_Normal.m_CreateTime = uint16_t(createTime & 0xffff);
        buffer->m_Normal.m_CreateDate = uint16_t(createTime >> 16);
        buffer->m_Normal.m_AccessDate = uint16_t(accessTime >> 16);
        buffer->m_Normal.m_ModificationTime = uint16_t(modificationTime & 0xffff);
        buffer->m_Normal.m_ModificationDate = uint16_t(modificationTime >> 16);
        buffer->m_Normal.m_FirstClusterLow = uint16_t(m_StartCluster & 0xffff);
        buffer->m_Normal.m_FirstClusterHigh = uint16_t(m_StartCluster >> 16);

        if (IsDirectory()) {
            buffer->m_Normal.m_FileSize = 0;
        } else {
            buffer->m_Normal.m_FileSize = uint32_t(m_Size);
        }
        directoryIterator.MarkDirty();
    }

    DiscardPendingMetadata();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

time_t FATInode::FATTimeToUnixTime(uint32_t fatTime)
{
    tm timeInfo = {};
    timeInfo.tm_sec = int((fatTime & 0x1f) * 2);
    timeInfo.tm_min = int((fatTime >> 5) & 0x3f);
    timeInfo.tm_hour = int((fatTime >> 11) & 0x1f);
    timeInfo.tm_mday = int((fatTime >> 16) & 0x1f);
    timeInfo.tm_mon = int((fatTime >> 21) & 0x0f) - 1;
    timeInfo.tm_year = int((fatTime >> 25) & 0x7f) + 80;
    timeInfo.tm_isdst = -1;
    return mktime(&timeInfo);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos FATInode::FATTimeToTimeVal(uint32_t fatTime, uint8_t createTimeFine)
{
    if (createTimeFine > 199)
    {
        createTimeFine = 199;
    }
    return TimeValNanos::FromSeconds(FATTimeToUnixTime(fatTime)) + TimeValNanos::FromMilliseconds(bigtime_t(createTimeFine) * 10);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATInode::UnixTimeToFATTime(time_t unixTime)
{
    const uint32_t minimumFATTime = (1u << 21) | (1u << 16); // 1980-01-01 00:00:00
    const uint32_t maximumFATTime = (127u << 25) | (12u << 21) | (31u << 16) | (23u << 11) | (59u << 5) | 29u; // 2107-12-31 23:59:58

    const tm* timeInfo = localtime(&unixTime);
    if (timeInfo == nullptr) {
        return (unixTime < 0) ? minimumFATTime : maximumFATTime;
    }
    if (timeInfo->tm_year < 80) {
        return minimumFATTime;
    }
    if (timeInfo->tm_year > 207) {
        return maximumFATTime;
    }

    uint32_t fatTime = uint32_t(timeInfo->tm_sec / 2);
    fatTime |= uint32_t(timeInfo->tm_min) << 5;
    fatTime |= uint32_t(timeInfo->tm_hour) << 11;
    fatTime |= uint32_t(timeInfo->tm_mday) << 16;
    fatTime |= uint32_t(timeInfo->tm_mon + 1) << 21;
    fatTime |= uint32_t(timeInfo->tm_year - 80) << 25;
    return fatTime;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t FATInode::TimeValToFATCreateTimeFine(TimeValNanos time)
{
    const timespec timeSpec = time.AsTimespec();
    uint32_t createTimeFine = uint32_t((timeSpec.tv_sec & 1) * 100 + timeSpec.tv_nsec / 10000000);
    if (createTimeFine > 199)
    {
        createTimeFine = 199;
    }
    return uint8_t(createTimeFine);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos FATInode::RoundTimeToFATCreateTime(TimeValNanos time)
{
    return FATTimeToTimeVal(UnixTimeToFATTime(time.AsSecondsI()), TimeValToFATCreateTimeFine(time));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos FATInode::RoundTimeToFATAccessTime(TimeValNanos time)
{
    const uint32_t fatTime = UnixTimeToFATTime(time.AsSecondsI());
    return FATTimeToTimeVal(fatTime & 0xffff0000u, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos FATInode::RoundTimeToFATModificationTime(TimeValNanos time)
{
    return FATTimeToTimeVal(UnixTimeToFATTime(time.AsSecondsI()), 0);
}


} // namespace
