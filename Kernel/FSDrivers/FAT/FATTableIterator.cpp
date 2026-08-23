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
// Created: 18/05/25 22:58:12


#include <System/ExceptionHandling.h>
#include <Kernel/KLogging.h>
#include <Kernel/FSDrivers/FAT/FATFilesystem.h>

#include "FATTableIterator.h"
#include "FATVolume.h"

namespace kernel
{
    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATTableIterator::FATTableIterator(Ptr<FATVolume> volume, uint32_t startCluster) : m_Volume(volume), m_CurrentCluster(startCluster)
{
    if (!m_Volume->IsDataCluster(m_CurrentCluster)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATFS, "FATTableIterator constructed with invalid cluster {}", m_CurrentCluster);
    }

    const uint64_t fatByteOffset = uint64_t(m_CurrentCluster) * m_Volume->m_FATBits / 8;
    const uint64_t fatStartSector = uint64_t(m_Volume->m_ReservedSectors) + uint64_t(m_Volume->m_ActiveFAT) * m_Volume->m_SectorsPerFAT;
    m_CurrentSector = static_cast<off64_t>(fatStartSector + fatByteOffset / m_Volume->m_BytesPerSector);
    m_OffsetInSector = static_cast<uint32_t>(fatByteOffset % m_Volume->m_BytesPerSector);
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
    const uint64_t fatByteOffset = uint64_t(m_CurrentCluster) * m_Volume->m_FATBits / 8;
    const uint64_t fatStartSector = uint64_t(m_Volume->m_ReservedSectors) + uint64_t(m_Volume->m_ActiveFAT) * m_Volume->m_SectorsPerFAT;
    const uint64_t fatEndSector = fatStartSector + m_Volume->m_SectorsPerFAT;
    m_CurrentSector = static_cast<off64_t>(fatStartSector + fatByteOffset / m_Volume->m_BytesPerSector);
    m_OffsetInSector = static_cast<uint32_t>(fatByteOffset % m_Volume->m_BytesPerSector);
    kassert(static_cast<uint64_t>(m_CurrentSector) < fatEndSector);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTableIterator::Increment()
{
    ++m_CurrentCluster;
    if (m_CurrentCluster == m_Volume->m_TotalClusters + FATTable::FIRST_DATA_CLUSTER) {
        m_CurrentCluster = FATTable::FIRST_DATA_CLUSTER;
    }
    SetCluster(m_CurrentCluster);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void FATTableIterator::SetEntry(uint32_t value)
{
    Update();
    MirrorBlockArray mirrorBlocks1;
    MirrorBlockArray mirrorBlocks2;
    const size_t mirrorBlockCount1 = AcquireMirrorBlocks(m_LoadedSector1, mirrorBlocks1);
    size_t mirrorBlockCount2 = 0;
    if (m_LoadedSector2 != -1) {
        mirrorBlockCount2 = AcquireMirrorBlocks(m_LoadedSector2, mirrorBlocks2);
    }

    uint8_t* block1 = static_cast<uint8_t*>(m_Block1.m_Buffer);
    uint8_t* block2 = static_cast<uint8_t*>(m_Block2.m_Buffer);
    
    if (m_Volume->m_FATBits == 12)
    {
        uint32_t preserveMask;
        uint32_t entryBits;
            
        if (m_CurrentCluster & 1)
        {
            entryBits = (value & 0xfff) << 4;
            preserveMask = 0xf;
        }
        else
        {
            entryBits = value & 0xfff;
            preserveMask = 0xf000;
        }
        block1[m_OffsetInSector] = uint8_t((block1[m_OffsetInSector] & preserveMask) | entryBits);
            
        if (m_OffsetInSector == m_Volume->m_BytesPerSector - 1)
        {
            kassert(block2 != nullptr);
            block2[0] = uint8_t((block2[0] & (preserveMask >> 8)) | (entryBits >> 8));
        }
        else
        {
            block1[m_OffsetInSector + 1] = uint8_t((block1[m_OffsetInSector + 1] & (preserveMask >> 8)) | (entryBits >> 8));
        }
    }
    else if (m_Volume->m_FATBits == 16)
    {
        block1[m_OffsetInSector] = value & 0xff;
        block1[m_OffsetInSector + 1] = (value >> 8) & 0xff;
    }
    else if (m_Volume->m_FATBits == 32)
    {
        kassert((value & 0xf0000000) == 0);
        block1[m_OffsetInSector]     = value & 0xff;
        block1[m_OffsetInSector + 1] = (value >> 8) & 0xff;
        block1[m_OffsetInSector + 2] = (value >> 16) & 0xff;
        block1[m_OffsetInSector + 3] = uint8_t((block1[m_OffsetInSector + 3] & 0xf0) | ((value >> 24) & 0x0f));
        kassert(value == (block1[m_OffsetInSector] + 0x100*block1[m_OffsetInSector + 1] + 0x10000*block1[m_OffsetInSector + 2] + 0x1000000*(block1[m_OffsetInSector + 3] & 0x0f)));
    }
    else
    {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    m_Block1.MarkDirty();
    CopyToMirrorBlocks(block1, mirrorBlocks1, mirrorBlockCount1);
    if (m_LoadedSector2 != -1)
    {
        m_Block2.MarkDirty();
        CopyToMirrorBlocks(block2, mirrorBlocks2, mirrorBlockCount2);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATTableIterator::GetEntry()
{
    Update();
    const uint8_t* block1 = static_cast<const uint8_t*>(m_Block1.m_Buffer);
    const uint8_t* block2 = static_cast<const uint8_t*>(m_Block2.m_Buffer);
    
    if (m_Volume->m_FATBits == 12)
    {
        uint32_t val;
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
        
        return val;
    }
    else if (m_Volume->m_FATBits == 16)
    {
        uint32_t val = block1[m_OffsetInSector] + 0x100*block1[m_OffsetInSector+1];
        return val;
    }
    else if (m_Volume->m_FATBits == 32)
    {
        return block1[m_OffsetInSector] + 0x100*block1[m_OffsetInSector + 1] + 0x10000*block1[m_OffsetInSector + 2] + 0x1000000*(block1[m_OffsetInSector + 3]&0x0f);
    }
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATTableIterator::AcquireMirrorBlocks(off64_t activeSector, MirrorBlockArray& mirrorBlocks)
{
    if (!m_Volume->m_FATMirrored) {
        return 0;
    }
    if (m_Volume->m_FATCount > FAT_MAX_SUPPORTED_FAT_COUNT)
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTableIterator::AcquireMirrorBlocks(): unsupported FAT count {}.", m_Volume->m_FATCount);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    const off64_t activeFATStartSector = off64_t(m_Volume->m_ReservedSectors) + off64_t(m_Volume->m_ActiveFAT) * m_Volume->m_SectorsPerFAT;
    if (activeSector < activeFATStartSector || activeSector >= activeFATStartSector + m_Volume->m_SectorsPerFAT)
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTableIterator::AcquireMirrorBlocks(): sector {} is outside the active FAT.", activeSector);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    const off64_t sectorOffset = activeSector - activeFATStartSector;
    size_t mirrorBlockCount = 0;
    for (size_t fatIndex = 0; fatIndex < m_Volume->m_FATCount; ++fatIndex)
    {
        if (fatIndex == m_Volume->m_ActiveFAT) {
            continue;
        }
        const off64_t mirrorSector = off64_t(m_Volume->m_ReservedSectors) + off64_t(fatIndex) * m_Volume->m_SectorsPerFAT + sectorOffset;
        mirrorBlocks[mirrorBlockCount] = m_Volume->m_BCache.GetBlock_trw(mirrorSector, false);
        if (mirrorBlocks[mirrorBlockCount].m_Buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        ++mirrorBlockCount;
    }
    return mirrorBlockCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTableIterator::CopyToMirrorBlocks(const uint8_t* sourceBuffer, MirrorBlockArray& mirrorBlocks, size_t mirrorBlockCount)
{
    kassert(mirrorBlockCount <= mirrorBlocks.size());
    for (size_t mirrorIndex = 0; mirrorIndex < mirrorBlockCount; ++mirrorIndex)
    {
        memcpy(mirrorBlocks[mirrorIndex].m_Buffer, sourceBuffer, m_Volume->m_BytesPerSector);
        mirrorBlocks[mirrorIndex].MarkDirty();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTableIterator::Update()
{
    kassert(m_Volume->IsDataCluster(m_CurrentCluster));
    kassert(m_OffsetInSector == ((uint64_t(m_CurrentCluster) * m_Volume->m_FATBits / 8) % m_Volume->m_BytesPerSector));
    
    if (m_LoadedSector1 != m_CurrentSector)
    {
        if (m_Block2.m_Buffer != nullptr && m_LoadedSector2 == m_CurrentSector) {
            m_Block1 = std::move(m_Block2);
        } else {
            m_Block1 = m_Volume->m_BCache.GetBlock_trw(m_CurrentSector, true);
            m_Block2.Reset();
        }
        m_LoadedSector1 = -1;
        m_LoadedSector2 = -1;
        
        if (m_Block1.m_Buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        m_LoadedSector1 = m_CurrentSector;
    }

    // A FAT12 entry may start in an already loaded sector while its final
    // byte resides in the following sector.
    if (m_OffsetInSector == m_Volume->m_BytesPerSector - 1 &&
        m_LoadedSector2 != m_CurrentSector + 1)
    {
        m_Block2.Reset();
        m_LoadedSector2 = -1;

        m_Block2 = m_Volume->m_BCache.GetBlock_trw(m_CurrentSector + 1, true);
        if (m_Block2.m_Buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        m_LoadedSector2 = m_CurrentSector + 1;
    }
}

} // namespace
