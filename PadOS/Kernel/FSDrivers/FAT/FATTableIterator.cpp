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
// Created: 18/05/25 22:58:12


#include "FATTableIterator.h"
#include "FATVolume.h"
#include "FATFilesystem.h"

namespace kernel
{
    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATTableIterator::FATTableIterator(Ptr<FATVolume> volume, uint32_t startCluster) : m_Volume(volume), m_CurrentCluster(startCluster)
{
    if (!m_Volume->IsDataCluster(m_CurrentCluster)) {
        kernel_log(FATFilesystem::LOGC_FS, KLogSeverity::CRITICAL, "FATTableIterator constructed with invalid cluster %lx\n", m_CurrentCluster);
    }
    
    m_OffsetInSector  = m_CurrentCluster * m_Volume->m_FATBits / 8;
    m_CurrentSector   = m_Volume->m_ReservedSectors + m_Volume->m_ActiveFAT * m_Volume->m_SectorsPerFAT + m_OffsetInSector / m_Volume->m_BytesPerSector;
    m_OffsetInSector %= m_Volume->m_BytesPerSector;
    m_LoadedSector1 = -1;
    m_LoadedSector2 = -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATTableIterator::~FATTableIterator()
{
    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTableIterator::SetCluster(uint32_t cluster)
{
    m_CurrentCluster = cluster;
    m_OffsetInSector  = m_CurrentCluster * m_Volume->m_FATBits / 8;
    m_CurrentSector   = m_Volume->m_ReservedSectors + m_Volume->m_ActiveFAT * m_Volume->m_SectorsPerFAT + m_OffsetInSector / m_Volume->m_BytesPerSector;
    m_OffsetInSector %= m_Volume->m_BytesPerSector;    
    kassert(m_CurrentSector < m_Volume->m_ReservedSectors + (m_Volume->m_ActiveFAT + 1) * m_Volume->m_SectorsPerFAT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTableIterator::Increment()
{
    m_CurrentCluster++;
    
    if (m_CurrentCluster == m_Volume->m_TotalClusters + FATTable::FIRST_DATA_CLUSTER)
    {
        m_CurrentCluster = FATTable::FIRST_DATA_CLUSTER;
        m_OffsetInSector = 2 * m_Volume->m_FATBits / 8; // FIXME!!!!!!!!!!!!!!!!!!!! 12-bit wont work!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        m_CurrentSector = m_Volume->m_ReservedSectors + m_Volume->m_ActiveFAT * m_Volume->m_SectorsPerFAT;
    }
    else
    {
        m_OffsetInSector += m_Volume->m_FATBits / 8;
        if (m_OffsetInSector >= m_Volume->m_BytesPerSector)
        {
            m_OffsetInSector -= m_Volume->m_BytesPerSector;
            m_CurrentSector++;
            kassert(m_CurrentSector < m_Volume->m_ReservedSectors + (m_Volume->m_ActiveFAT + 1) * m_Volume->m_SectorsPerFAT);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
bool FATTableIterator::SetEntry(uint32_t value)
{
    if (!Update()) {
        return false;
    }
    uint8_t* block1 = static_cast<uint8_t*>(m_Block1.m_Buffer);
    uint8_t* block2 = static_cast<uint8_t*>(m_Block2.m_Buffer);
    
    if (m_Volume->m_FATBits == 12)
    {
        uint32_t andmask;
        uint32_t ormask;
            
        if (m_CurrentCluster & 1) {
            ormask = (value & 0xfff) << 4;
            andmask = 0xf;
        } else {
            ormask = value & 0xfff;
            andmask = 0xf000;
        }
        block1[m_OffsetInSector] = (block1[m_OffsetInSector] & andmask) | ormask;
            
        if (m_OffsetInSector == m_Volume->m_BytesPerSector - 1)
        {
            kassert(block2 != nullptr);
            block2[0] = (block2[0] & (andmask >> 8)) | (ormask >> 8);
            m_Volume->GetFATTable()->MirrorFAT(m_LoadedSector2, block2);
            m_Block2.MarkDirty();
        }
        else
        {
            block1[m_OffsetInSector+1] = (block1[m_OffsetInSector+1] & (andmask >> 8)) | (ormask >> 8);
        }
        m_Volume->GetFATTable()->MirrorFAT(m_LoadedSector1, block1);
        m_Block1.MarkDirty();
        return true;
    }
    else if (m_Volume->m_FATBits == 16)
    {
        block1[m_OffsetInSector] = value & 0xff;
        block1[m_OffsetInSector + 1] = (value >> 8) & 0xff;
        m_Block1.MarkDirty();
        return true;
    }
    else if (m_Volume->m_FATBits == 32)
    {
        kassert((value & 0xf0000000) == 0);
        block1[m_OffsetInSector]     = value & 0xff;
        block1[m_OffsetInSector + 1] = (value >> 8) & 0xff;
        block1[m_OffsetInSector + 2] = (value >> 16) & 0xff;
        block1[m_OffsetInSector + 3] = (value >> 24) & 0x0f;
        kassert(value == (block1[m_OffsetInSector] + 0x100*block1[m_OffsetInSector + 1] + 0x10000*block1[m_OffsetInSector + 2] + 0x1000000*block1[m_OffsetInSector + 3]));
        m_Block1.MarkDirty();
        return true;
    }
    else
    {
        kassert(false);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTableIterator::GetEntry(uint32_t* value)
{
    if (!Update()) {
        return false;
    }
    const uint8_t* block1 = static_cast<const uint8_t*>(m_Block1.m_Buffer);
    const uint8_t* block2 = static_cast<const uint8_t*>(m_Block2.m_Buffer);
    
    if (m_Volume->m_FATBits == 12)
    {
        uint16_t val;
        if (m_OffsetInSector == m_Volume->m_BytesPerSector - 1) {
            kassert(block2 != nullptr);
            val = block1[m_OffsetInSector] + 0x100*block2[0];
        } else {
            val = block1[m_OffsetInSector] + 0x100*block1[m_OffsetInSector + 1];
        }
        if (m_CurrentCluster & 1) {
            val >>= 4;
        } else {
            val &= 0xfff;
        }
        if (val > 0xff0) val |= 0x0ffff000;
        
        *value = val;
        return true;
    }
    else if (m_Volume->m_FATBits == 16)
    {
        uint16_t val = block1[m_OffsetInSector] + 0x100*block1[m_OffsetInSector+1];
        if (val > 0xfff0) val |= 0x0fff0000;
        *value = val;
        return true;
    }
    else if (m_Volume->m_FATBits == 32)
    {
        *value = block1[m_OffsetInSector] + 0x100*block1[m_OffsetInSector + 1] + 0x10000*block1[m_OffsetInSector + 2] + 0x1000000*(block1[m_OffsetInSector + 3]&0x0f);
        return true;
    }
    else
    {
        kassert(false);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTableIterator::Update()
{
    kassert(m_Volume->IsDataCluster(m_CurrentCluster));
    kassert(m_OffsetInSector == ((m_CurrentCluster * m_Volume->m_FATBits / 8) % m_Volume->m_BytesPerSector));
    
    if (m_LoadedSector1 != m_CurrentSector)
    {
        if (m_Block2.m_Buffer != nullptr && m_LoadedSector2 == (m_CurrentSector + 1)) {
            m_Block1 = std::move(m_Block2);
        } else {
            m_Block1 = m_Volume->m_BCache.GetBlock(m_CurrentSector, true);
            m_Block2.Reset();
        }
        m_LoadedSector1 = -1;
        m_LoadedSector2 = -1;
        
        if (m_Block1.m_Buffer == nullptr) {
            return false;
        }
        m_LoadedSector1 = m_CurrentSector;
        if (m_OffsetInSector == m_Volume->m_BytesPerSector - 1)
        {
            m_Block2 = m_Volume->m_BCache.GetBlock(m_CurrentSector + 1, true);
            if (m_Block2.m_Buffer == nullptr) {
                m_Block1.Reset();
                m_LoadedSector1 = -1;
                return false;
            }
            m_LoadedSector2 = m_CurrentSector + 1;
        }
        return m_Block1.m_Buffer != nullptr;
    }
    return true;
}

} // namespace
