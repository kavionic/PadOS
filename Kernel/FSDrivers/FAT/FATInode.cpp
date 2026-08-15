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

#include <Kernel/KLogging.h>

#include "FATInode.h"
#include "FATVolume.h"
#include "FATDirectoryIterator.h"
#include "Kernel/FSDrivers/FAT/FATFilesystem.h"

namespace kernel
{

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

bool FATInode::Write()
{
    FATDirectoryEntryCombo* buffer;

    // don't update entries of deleted files
    if (IsDeleted()) return true;

    // XXX: should check if directory position is still valid even
    // though we do the IsDeleted() check above

    Ptr<FATVolume> volume = ptr_static_cast<FATVolume>(m_Volume);
    if ((m_StartCluster != 0) && !volume->IsDataCluster(m_StartCluster)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATInode::Write() called on invalid cluster ({}).", m_StartCluster);
        set_last_error(EINVAL);
        return false;
    }

    FATDirectoryIterator diri(volume, CLUSTER_OF_DIR_CLUSTER_INODEID(m_ParentInodeID), m_DirEndIndex);
    buffer = diri.GetCurrentEntry();
    if (buffer == nullptr) {
        set_last_error(ENOENT);
        return false;
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
    buffer->m_Normal.m_FirstClusterLow  = uint16_t(m_StartCluster & 0xffff);	// starting cluster
    buffer->m_Normal.m_FirstClusterHigh = uint16_t(m_StartCluster >> 16);
    
    if (IsDirectory()) {
        buffer->m_Normal.m_FileSize = 0;
    } else {
        buffer->m_Normal.m_FileSize = uint32_t(m_Size);
    }
    diri.MarkDirty();
    return true;
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
