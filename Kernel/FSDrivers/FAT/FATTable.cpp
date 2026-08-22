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
// Created: 18/05/25 23:04:14

#include <array>
#include <utility>
#include <string.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KLogging.h>
#include <Kernel/FSDrivers/FAT/FATFilesystem.h>

#include "FATTable.h"
#include "FATVolume.h"
#include "FATInode.h"

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATTable::FATTable(Ptr<FATVolume> volume) : m_Volume(volume), m_TableIterator(volume, FIRST_DATA_CLUSTER)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATTable::~FATTable()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATVolumeStatus FATTable::ReadVolumeStatus()
{
    FATVolumeStatus status;
    if (m_Volume->m_FATBits == 12) {
        return status;
    }

    const off64_t fatStartSector = off64_t(m_Volume->m_ReservedSectors) + off64_t(m_Volume->m_ActiveFAT) * m_Volume->m_SectorsPerFAT;
    KCacheBlockDesc fatBlock = m_Volume->m_BCache.GetBlock_trw(fatStartSector, true);
    if (fatBlock.m_Buffer == nullptr) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    const VolumeStatusMasks masks = GetVolumeStatusMasks(m_Volume->m_FATBits);
    const uint32_t entryValue = ReadVolumeStatusEntry(static_cast<const uint8_t*>(fatBlock.m_Buffer), m_Volume->m_FATBits);
    if ((entryValue & masks.ReservedOneBits) != masks.ReservedOneBits)
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::ReadVolumeStatus(): reserved FAT[1] entry has invalid value {:#010x}.", entryValue);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    status.IsSupported = true;
    status.IsClean = (entryValue & masks.CleanShutdown) != 0;
    status.HasHardError = (entryValue & masks.NoHardError) == 0;
    return status;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::SetVolumeClean(bool isClean)
{
    if (m_Volume->m_FATBits == 12) {
        return;
    }

//    kernel_log<PLogSeverity::WARNING>(LogCat_FATTABLE, "FATTable::SetVolumeClean(): Mark volume {}.", isClean ? "clean" : "dirty");

    const VolumeStatusMasks masks = GetVolumeStatusMasks(m_Volume->m_FATBits);
    std::array<KCacheBlockDesc, FAT_MAX_SUPPORTED_FAT_COUNT> fatBlocks;
    std::array<uint32_t, FAT_MAX_SUPPORTED_FAT_COUNT> entryValues;
    size_t fatBlockCount = 0;

    const size_t firstFATIndex = m_Volume->m_FATMirrored ? 0 : m_Volume->m_ActiveFAT;
    const size_t endFATIndex = m_Volume->m_FATMirrored ? m_Volume->m_FATCount : firstFATIndex + 1;
    for (size_t fatIndex = firstFATIndex; fatIndex < endFATIndex; ++fatIndex)
    {
        kassert(fatBlockCount < fatBlocks.size());
        const off64_t fatStartSector = off64_t(m_Volume->m_ReservedSectors) + off64_t(fatIndex) * m_Volume->m_SectorsPerFAT;
        fatBlocks[fatBlockCount] = m_Volume->m_BCache.GetBlock_trw(fatStartSector, true);
        if (fatBlocks[fatBlockCount].m_Buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::IO);
        }

        entryValues[fatBlockCount] = ReadVolumeStatusEntry(static_cast<const uint8_t*>(fatBlocks[fatBlockCount].m_Buffer), m_Volume->m_FATBits);
        if ((entryValues[fatBlockCount] & masks.ReservedOneBits) != masks.ReservedOneBits)
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::SetVolumeClean(): FAT {} reserved FAT[1] entry has invalid value {:#010x}.", fatIndex, entryValues[fatBlockCount]);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        ++fatBlockCount;
    }

    for (size_t blockIndex = 0; blockIndex < fatBlockCount; ++blockIndex)
    {
        const uint32_t newEntryValue = isClean ? (entryValues[blockIndex] | masks.CleanShutdown) : (entryValues[blockIndex] & ~masks.CleanShutdown);
        if (newEntryValue != entryValues[blockIndex])
        {
            WriteVolumeStatusEntry(static_cast<uint8_t*>(fatBlocks[blockIndex].m_Buffer), m_Volume->m_FATBits, newEntryValue);
            fatBlocks[blockIndex].MarkDirty();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATTable::GetEntry(uint32_t cluster)
{
    m_TableIterator.SetCluster(cluster);
    const uint32_t value = m_TableIterator.GetEntry();

    if (value == 0 || m_Volume->IsDataCluster(value)) {
        return value;
    }
    if (value >= END_FAT_ENTRY) {
        return END_FAT_ENTRY;
    }	
    if (value >= BAD_FAT_ENTRY) {
        return BAD_FAT_ENTRY;
    }
    kernel_log<PLogSeverity::ERROR>(LogCat_FATTABLE, "FATTable::GetEntry(): invalid FAT entry {:x} for cluster {}.", value, cluster);
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::SetEntry(uint32_t cluster, uint32_t value)
{
    m_TableIterator.SetCluster(cluster);
    m_TableIterator.SetEntry(value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATTable::GetChainEntry(uint32_t chainStart, uint32_t index)
{
    if (!m_Volume->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (!m_Volume->IsDataCluster(chainStart) || index > m_Volume->m_TotalClusters)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATTABLE, "FATTable::GetChainEntry({}, {}) called with an invalid chain or index.", chainStart, index);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "FATTable::GetChainEntry({}, {})", chainStart, index);
    uint32_t cluster = chainStart;
    for (uint32_t chainIndex = 0; chainIndex < index; ++chainIndex)
    {
        const uint32_t previousCluster = cluster;
        cluster = GetEntry(previousCluster);
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "  {} -> {}", previousCluster, cluster);
        if (!m_Volume->IsDataCluster(cluster)) {
            break;
        }
    }
    return cluster;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#ifdef FAT_VERIFY_FAT_CHAINS
bool FATTable::ValidateChainEntry(uint32_t chainStart, uint32_t index, uint32_t expectedValue)
{
    const uint32_t value = GetChainEntry(chainStart, index);
    if (value != expectedValue) {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATTABLE, "ValidateChainEntry({}, {}, {}) unexpected value {}!", chainStart, index, expectedValue, value);
    }
    return value == expectedValue;
}
#endif // FAT_VERIFY_FAT_CHAINS

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATTable::CountFreeClusters()
{
    m_TableIterator.SetCluster(FATTable::FIRST_DATA_CLUSTER);
    uint32_t count = 0;
    for (uint32_t i = 0; i < m_Volume->m_TotalClusters; ++i, m_TableIterator.Increment())
    {
        const uint32_t value = m_TableIterator.GetEntry();
        if (value == 0) {
            count++;
        }
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t FATTable::GetChainLength(uint32_t cluster, uint32_t* endCluster)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "FATTable::GetChainLength() {}", cluster);
        
    if (!m_Volume->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    // not intended for use on root directory
    if (!m_Volume->IsDataCluster(cluster)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::GetChainLength() called on invalid cluster ({}).", cluster);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    for (size_t count = 1; count <= m_Volume->m_TotalClusters; ++count)
    {
        const uint32_t nextCluster = GetEntry(cluster);
        if (nextCluster == END_FAT_ENTRY)
        {
            kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "  {:x} = {:x}", nextCluster, count);
            if (endCluster != nullptr) {
                *endCluster = cluster;
            }
            return count;
        }
        if (!m_Volume->IsDataCluster(nextCluster))
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::GetChainLength() invalid chain. Cluster {} points to {:x}.", cluster, nextCluster);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
        cluster = nextCluster;
    }

    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::GetChainLength() circular FAT chain detected.");
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::SetChainLength(Ptr<FATInode> node, uint32_t clusterCount, bool updateICache)
{
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): {:x} to {} clusters ({}).", node->m_InodeID, clusterCount, node->m_StartCluster);

    const bool hasClusterChain = node->m_StartCluster != 0;
    if ((hasClusterChain && (IS_FIXED_ROOT(node->m_StartCluster) || !m_Volume->IsDataCluster(node->m_StartCluster) || !m_Volume->IsDataCluster(node->m_EndCluster) || node->m_AllocatedClusterCount == 0)) ||
        (!hasClusterChain && (node->m_EndCluster != 0 || node->m_AllocatedClusterCount != 0)))
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::SetChainLength(): inode has inconsistent chain metadata ({}, {}, {}).", node->m_StartCluster, node->m_EndCluster, node->m_AllocatedClusterCount);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (clusterCount > m_Volume->m_TotalClusters) {
        PERROR_THROW_CODE(PErrorCode::NOSPC);
    }

    if (clusterCount == node->m_AllocatedClusterCount) {
        return;
    }

    if (clusterCount == 0)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): truncating node to zero bytes.");
        const uint32_t oldStartCluster = node->m_StartCluster;
        const uint32_t oldEndCluster = node->m_EndCluster;
        const uint32_t oldClusterCount = node->m_AllocatedClusterCount;

        {
            PScopeFail restoreNodeChain([&node, oldStartCluster, oldEndCluster, oldClusterCount]()
            {
                node->m_StartCluster = oldStartCluster;
                node->m_EndCluster = oldEndCluster;
                node->m_AllocatedClusterCount = oldClusterCount;
            });

            node->m_StartCluster = 0;
            node->m_EndCluster = 0;
            node->m_AllocatedClusterCount = 0;

            node->Write();
        }

        PScopeFail markCommittedChainChangeInconsistent([this]()
        {
            m_Volume->MarkMetadataInconsistent();
        });

        node->m_Iteration++;
        if (updateICache) {
            m_Volume->SetInodeIDToLocationIDMapping(node->m_InodeID, GENERATE_DIR_INDEX_INODEID(node->m_ParentInodeID, node->m_DirStartIndex));
        }
        ClearFATChain(oldStartCluster);
        return;
    }

    if (!hasClusterChain)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): node has no clusters. adding {} clusters.", clusterCount);

        uint32_t newEndCluster;
        const uint32_t newStartCluster = AllocateClusters(clusterCount, &newEndCluster);

        {
            PScopeFail rollbackNewChain([this, &node, newStartCluster]()
            {
                node->m_StartCluster = 0;
                node->m_EndCluster = 0;
                node->m_AllocatedClusterCount = 0;
                ClearFATChainAfterFailureNoThrow(newStartCluster, "FATTable::SetChainLength()");
            });

            node->m_StartCluster = newStartCluster;
            node->m_EndCluster = newEndCluster;
            node->m_AllocatedClusterCount = clusterCount;

            node->Write();
        }

        node->m_Iteration++;
        if (updateICache) {
            m_Volume->SetInodeIDToLocationIDMapping(node->m_InodeID, GENERATE_DIR_CLUSTER_INODEID(node->m_ParentInodeID, node->m_StartCluster));
        }
        return;
    }

    if (clusterCount > node->m_AllocatedClusterCount)
    {
        const uint32_t additionalClusterCount = clusterCount - node->m_AllocatedClusterCount;
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): adding {} new fat entries.", additionalClusterCount);

        uint32_t newEndCluster;
        const uint32_t newStartCluster = AllocateClusters(additionalClusterCount, &newEndCluster);
        kassert(m_Volume->IsDataCluster(newStartCluster));

        PScopeFail rollbackExtension([this, newStartCluster]()
        {
            ClearFATChainAfterFailureNoThrow(newStartCluster, "FATTable::SetChainLength()");
        });

        SetEntry(node->m_EndCluster, newStartCluster);

        node->m_EndCluster = newEndCluster;
        node->m_AllocatedClusterCount = clusterCount;
        node->m_Iteration++;
        return;
    }

    const uint32_t newEndCluster = GetChainEntry(node->m_StartCluster, clusterCount - 1);
    if (!m_Volume->IsDataCluster(newEndCluster)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    const uint32_t trailingChainStart = GetEntry(newEndCluster);
    if (!m_Volume->IsDataCluster(trailingChainStart))
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::SetChainLength(): chain ended before its cached length ({}).", node->m_AllocatedClusterCount);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): clearing trailing fat entries.");
    SetEntry(newEndCluster, END_FAT_ENTRY);

    PScopeFail markCommittedChainChangeInconsistent([this]()
    {
        m_Volume->MarkMetadataInconsistent();
    });

    node->m_EndCluster = newEndCluster;
    node->m_AllocatedClusterCount = clusterCount;
    node->m_Iteration++;
    ClearFATChain(trailingChainStart);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATTable::AllocateClusters(size_t clusterCount, uint32_t* endCluster)
{
    uint32_t firstCluster = 0;
    uint32_t lastCluster = 0;
    uint32_t detachedCluster = 0;
    size_t allocatedClusterCount = 0;

    if (!m_Volume->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (clusterCount == 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (clusterCount > m_Volume->m_TotalClusters) {
        PERROR_THROW_CODE(PErrorCode::NOSPC);
    }
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::AllocateClusters(): {:x}", clusterCount);

    FATTableIterator tableIterator(m_Volume, m_Volume->m_LastAllocatedCluster);

    PScopeFail scopeCleanup(
        [this, &firstCluster, &detachedCluster]()
        {
            ClearFATChainAfterFailureNoThrow(firstCluster, "FATTable::AllocateClusters()");
            ClearFATChainAfterFailureNoThrow(detachedCluster, "FATTable::AllocateClusters()");
        });

    for (uint32_t clusterIndex = 0; clusterIndex < m_Volume->m_TotalClusters; ++clusterIndex)
    {
        const uint32_t value = tableIterator.GetEntry();

        if (value == 0)
        {
            tableIterator.SetEntry(END_FAT_ENTRY);
            detachedCluster = tableIterator.GetCurrentCluster();
            if (m_Volume->m_FreeClusters > 0) {
                m_Volume->m_FreeClusters--;
            }
            if (allocatedClusterCount == 0)
            {
                kassert(firstCluster == 0);
                firstCluster = detachedCluster;
                lastCluster = firstCluster;
                detachedCluster = 0;
            }
            else
            {
                kassert(m_Volume->IsDataCluster(firstCluster));
                kassert(m_Volume->IsDataCluster(lastCluster));

                // Set previous last cluster to point to us
                SetEntry(lastCluster, detachedCluster);
                lastCluster = detachedCluster;
                detachedCluster = 0;
            }
            m_Volume->m_LastAllocatedCluster = lastCluster;
            if (++allocatedClusterCount == clusterCount) {
                break;
            }
        }
        tableIterator.Increment();
    }
    m_Volume->UpdateFSInfo();
    if (allocatedClusterCount != clusterCount)
    {
        kernel_log<PLogSeverity::WARNING>(LogCat_FATTABLE, "FATTable::AllocateClusters(): Failed to allocate {} clusters. Not enough free entries ({} found).", clusterCount, allocatedClusterCount);
        PERROR_THROW_CODE(PErrorCode::NOSPC);
    }
    else
    {
        kassert(m_Volume->IsDataCluster(firstCluster));
        kassert(m_Volume->IsDataCluster(lastCluster));
#ifdef FAT_VERIFY_FAT_CHAINS
        kassert(GetChainLength(firstCluster) == clusterCount);
#endif // FAT_VERIFY_FAT_CHAINS

        if (endCluster != nullptr) {
            *endCluster = lastCluster;
        }
        return firstCluster;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::ClearFATChain(uint32_t cluster)
{
    if (!m_Volume->IsDataCluster(cluster)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::ClearFATChain() called on invalid cluster ({}).", cluster);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

//    ASSERT(count_clusters(vol, cluster) != 0);

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::ClearFATChain(): Clearing fat chain: {}", cluster);
    size_t clearedClusterCount = 0;
    while (m_Volume->IsDataCluster(cluster) && clearedClusterCount < m_Volume->m_TotalClusters)
    {
        const uint32_t nextCluster = GetEntry(cluster);
        if (nextCluster == 0) {
            break;
        }
        SetEntry(cluster, 0);

        if (m_Volume->m_FreeClusters < m_Volume->m_TotalClusters) {
            m_Volume->m_FreeClusters++;
        }
        clearedClusterCount++;
        cluster = nextCluster;
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "  clearing cluster: {}", cluster);
    }

    m_Volume->UpdateFSInfo();

    if (cluster != END_FAT_ENTRY)
    {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::ClearFATChain(): fat chain terminated improperly with {}.", cluster);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::ClearFATChainAfterFailureNoThrow(uint32_t startCluster, const char* operation) noexcept
{
    if (startCluster != 0)
    {
        try {
            ClearFATChain(startCluster);
        } catch (const std::exception& exception) {
            m_Volume->MarkMetadataInconsistent();
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "{}: failed to release FAT chain {} during rollback: {}.", operation, startCluster, exception.what());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::DumpChain(uint32_t startCluster)
{
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FAT chain: {:x}", startCluster);
    uint32_t cluster = startCluster;

    size_t clusterCount = 0;
    while (m_Volume->IsDataCluster(cluster) && clusterCount < m_Volume->m_TotalClusters)
    {
        cluster = GetEntry(cluster);
        clusterCount++;
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "  {:x}", cluster);
    }
    if (m_Volume->IsDataCluster(cluster)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::DumpChain(): circular FAT chain detected.");
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATTable::VolumeStatusMasks FATTable::GetVolumeStatusMasks(uint8_t fatBits)
{
    if (fatBits == 16) {
        return {0x00008000, 0x00004000, 0x00003fff};
    }
    if (fatBits == 32) {
        return {0x08000000, 0x04000000, 0x03ffffff};
    }
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATTable::ReadVolumeStatusEntry(const uint8_t* fatSector, uint8_t fatBits)
{
    const size_t entryOffset = fatBits / 8;
    if (fatBits == 16) {
        return uint32_t(fatSector[entryOffset]) | (uint32_t(fatSector[entryOffset + 1]) << 8);
    }
    if (fatBits == 32)
    {
        return uint32_t(fatSector[entryOffset]) |
               (uint32_t(fatSector[entryOffset + 1]) << 8) |
               (uint32_t(fatSector[entryOffset + 2]) << 16) |
               (uint32_t(fatSector[entryOffset + 3]) << 24);
    }
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::WriteVolumeStatusEntry(uint8_t* fatSector, uint8_t fatBits, uint32_t value)
{
    const size_t entryOffset = fatBits / 8;
    fatSector[entryOffset] = uint8_t(value);
    fatSector[entryOffset + 1] = uint8_t(value >> 8);
    if (fatBits == 32)
    {
        fatSector[entryOffset + 2] = uint8_t(value >> 16);
        fatSector[entryOffset + 3] = uint8_t(value >> 24);
    }
    else if (fatBits != 16) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
}

} // namespace
