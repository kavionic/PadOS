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

static int32_t tzoffset = 120; // Minutes
static int daze[] = { 0,0,31,59,90,120,151,181,212,243,273,304,334,0,0,0 };

#define IS_LEAP_YEAR(y) ((((y) % 4) == 0) && ((y) % 100))

///////////////////////////////////////////////////////////////////////////////
/// Returns leap days since 1970
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int LeapDays(int year, int month)
{
    // Year is 1970-based, month is 0-based
    int result = (year + 2) / 4 - (year + 70) / 100;
    if (IS_LEAP_YEAR(year + 70)) {
        if (month < 2) result--;
    }
    return result;
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
    
    buffer->m_Normal.m_Time = UnixTimeToFATTime(m_MTime.AsSecondsI());
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
    time_t days;

    days = daze[(fatTime>>21)&15] + ((fatTime>>25)+10)*365 + LeapDays((fatTime>>25)+10,((fatTime>>21)&15)-1)+((fatTime>>16)&31)-1;

    return (((days * 24) + ((fatTime>>11)&31)) * 60 + ((fatTime>>5)&63) + tzoffset) * 60 + 2*(fatTime&31);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATInode::UnixTimeToFATTime(time_t unixTime)
{
    uint32_t fatTime, d, y, days;

    fatTime = uint32_t((unixTime % 60) / 2);
    unixTime /= 60;
    unixTime -= tzoffset;
    fatTime += uint32_t((unixTime % 60) << 5);
    unixTime /= 60;
    fatTime += uint32_t((unixTime % 24) << 11);
    unixTime /= 24;

    unixTime -= 10 * 365 + 2; // Convert from 1970-based year to 1980-based year

    for (y = 0;; ++y)
    {
        days = IS_LEAP_YEAR(80 + y) ? 366 : 365;
        if (unixTime < days) break;
        unixTime -= days;
    }

    if (IS_LEAP_YEAR(80+y))
    {
        if (unixTime == 59) {
            d = (1 << 5) + 28; // 2/29, 0 based
            goto bi;
        } else if (unixTime > 59) {
            unixTime--;
        }        
    }

    for (d=0;d<11;d++) {
        if (daze[d+2] > unixTime) {
            break;
        }
    }
    d = (d << 5) + int32_t(unixTime - daze[d+1]);

bi:
    d += (1 << 5) + 1; // Make date 1-based

    return fatTime + (d << 16) + (y << 25);
}


} // namespace

