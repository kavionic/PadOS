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
// Created: 18/06/01 0:45:02

#include <System/ExceptionHandling.h>
#include <Kernel/FSDrivers/FAT/FATFilesystem.h>

#include "FATClusterSectorIterator.h"
#include "FATVolume.h"

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool IsValidClusterSector(Ptr<FATVolume> volume, uint32_t cluster, uint32_t sector)
{
    if ((volume->m_FATBits != 32) && IS_FIXED_ROOT(cluster))
    {
        if (sector >= volume->m_RootSectorCount) {
            return false;
        }            
        return true;
    }
    if (sector >= volume->m_SectorsPerCluster) return false;
    if (!volume->IsDataCluster(cluster))       return false;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off64_t FATClusterSectorIterator::GetBlockSector()
{
    // Presumes the caller has already called IsValidClusterSector() on the argument.
    kassert(IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector));

    if (IS_FIXED_ROOT(m_CurrentCluster)) {
        return m_Volume->m_RootStart + m_CurrentSector;
    }
    return m_Volume->m_FirstDataSector + off64_t(m_CurrentCluster - 2) * m_Volume->m_SectorsPerCluster + m_CurrentSector;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATClusterSectorIterator::FATClusterSectorIterator(Ptr<FATVolume> volume, uint32_t cluster, uint32_t sector)
{
    m_Volume         = volume;
    m_CurrentCluster = cluster;
    m_CurrentSector  = sector;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATClusterSectorIterator::Set(uint32_t cluster, uint32_t sector)
{
    if (!IsValidClusterSector(m_Volume, cluster, sector)) PERROR_THROW_CODE(PErrorCode::IOError);
 
    m_CurrentCluster = cluster;
    m_CurrentSector  = sector;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATClusterSectorIterator::Increment(int sectors)
{
    if (m_CurrentSector == 0xffff) { // check if already at end of chain
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    if (sectors < 0) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }    
    if (sectors == 0) {
        return;
    }    
    if (IS_FIXED_ROOT(m_CurrentCluster))
    {
        m_CurrentSector += sectors;
        if (m_CurrentSector < m_Volume->m_RootSectorCount) {
            return;
        }            
    }
    else
    {
        m_CurrentSector += sectors;
        if (m_CurrentSector < m_Volume->m_SectorsPerCluster) {
            return;
        }    
        m_CurrentCluster = m_Volume->GetFATTable()->GetChainEntry(m_CurrentCluster, m_CurrentSector / m_Volume->m_SectorsPerCluster);

        if (int32_t(m_CurrentCluster) < 0) {
            m_CurrentSector = 0xffff;
            return;
        }

        if (m_Volume->IsDataCluster(m_CurrentCluster)) {
            m_CurrentSector %= m_Volume->m_SectorsPerCluster;
            return;
        }
    }

    m_CurrentSector = 0xffff;
    
    PERROR_THROW_CODE(PErrorCode::IOError);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockDesc FATClusterSectorIterator::GetBlock_(bool doLoad)
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    return m_Volume->m_BCache.GetBlock_trw(GetBlockSector(), doLoad);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode FATClusterSectorIterator::MarkBlockDirty()
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        return PErrorCode::InvalidArg;
    }
    return m_Volume->m_BCache.MarkBlockDirty(GetBlockSector()) ? PErrorCode::Success : PErrorCode::NoEntry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATClusterSectorIterator::ReadBlock(uint8_t* buffer)
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    m_Volume->m_BCache.CachedRead_trw(GetBlockSector(), buffer, 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATClusterSectorIterator::WriteBlock(const uint8_t* buffer)
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    m_Volume->m_BCache.CachedWrite_trw(GetBlockSector(), buffer, 1);
}

} // namespace
