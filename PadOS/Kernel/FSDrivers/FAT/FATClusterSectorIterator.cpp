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

status_t FATClusterSectorIterator::Set(uint32_t cluster, uint32_t sector)
{
    if (!IsValidClusterSector(m_Volume, cluster, sector)) return -1;
    m_CurrentCluster = cluster;
    m_CurrentSector  = sector;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FATClusterSectorIterator::Increment(int sectors)
{
    if (m_CurrentSector == 0xffff) { // check if already at end of chain
        set_last_error(EINVAL);
        return -1;
    }
    if (sectors < 0) {
        set_last_error(EINVAL);
        return -1;
    }    
    if (sectors == 0) {
        return 0;
    }    
    if (IS_FIXED_ROOT(m_CurrentCluster))
    {
        m_CurrentSector += sectors;
        if (m_CurrentSector < m_Volume->m_RootSectorCount) {
            return 0;
        }            
    }
    else
    {
        m_CurrentSector += sectors;
        if (m_CurrentSector < m_Volume->m_SectorsPerCluster) {
            return 0;
        }            
        m_Volume->GetFATTable()->GetChainEntry(m_CurrentCluster, m_CurrentSector / m_Volume->m_SectorsPerCluster, &m_CurrentCluster);

        if (int32_t(m_CurrentCluster) < 0) {
            m_CurrentSector = 0xffff;
            return m_CurrentCluster;
        }

        if (m_Volume->IsDataCluster(m_CurrentCluster)) {
            m_CurrentSector %= m_Volume->m_SectorsPerCluster;
            return 0;
        }
    }

    m_CurrentSector = 0xffff;
    
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KCacheBlockDesc FATClusterSectorIterator::GetBlock()
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        return KCacheBlockDesc();
    }
    return m_Volume->m_BCache.GetBlock(GetBlockSector());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FATClusterSectorIterator::MarkBlockDirty()
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        set_last_error(EINVAL);
        return -1;
    }
    return m_Volume->m_BCache.MarkBlockDirty(GetBlockSector());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FATClusterSectorIterator::ReadBlock(uint8_t* buffer)
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        set_last_error(EINVAL);
        return -1;
    }
    return m_Volume->m_BCache.CachedRead(GetBlockSector(), buffer, 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FATClusterSectorIterator::WriteBlock(const uint8_t* buffer)
{
    if (!IsValidClusterSector(m_Volume, m_CurrentCluster, m_CurrentSector)) {
        set_last_error(EINVAL);
        return -1;
    }
    return m_Volume->m_BCache.CachedWrite(GetBlockSector(), buffer, 1);
}

} // namespace
