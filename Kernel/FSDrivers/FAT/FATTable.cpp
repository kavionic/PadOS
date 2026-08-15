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
// Created: 18/05/25 23:04:14

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

        node->m_StartCluster = 0;
        node->m_EndCluster = 0;
        node->m_AllocatedClusterCount = 0;

        if (!node->Write())
        {
            node->m_StartCluster = oldStartCluster;
            node->m_EndCluster = oldEndCluster;
            node->m_AllocatedClusterCount = oldClusterCount;
            PERROR_THROW_CODE(PErrorCode::IO);
        }

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

        node->m_StartCluster = newStartCluster;
        node->m_EndCluster = newEndCluster;
        node->m_AllocatedClusterCount = clusterCount;

        if (!node->Write())
        {
            node->m_StartCluster = 0;
            node->m_EndCluster = 0;
            node->m_AllocatedClusterCount = 0;
            ClearFATChain(newStartCluster);
            PERROR_THROW_CODE(PErrorCode::IO);
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
        [this, &firstCluster]()
        {
            if (firstCluster != 0)
            {
                try
                {
                    ClearFATChain(firstCluster);
                }
                catch (const std::exception& error)
                {
                    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::AllocateClusters(): failed to release partially allocated chain: {}.", error.what());
                }
            }
        });

    for (uint32_t clusterIndex = 0; clusterIndex < m_Volume->m_TotalClusters; ++clusterIndex)
    {
        const uint32_t value = tableIterator.GetEntry();

        if (value == 0)
        {
            tableIterator.SetEntry(END_FAT_ENTRY);
            if (m_Volume->m_FreeClusters > 0) {
                m_Volume->m_FreeClusters--;
            }
            if (allocatedClusterCount == 0)
            {
                kassert(firstCluster == 0);
                firstCluster = tableIterator.GetCurrentCluster();
                lastCluster = firstCluster;
            }
            else
            {
                kassert(m_Volume->IsDataCluster(firstCluster));
                kassert(m_Volume->IsDataCluster(lastCluster));

                // Set previous last cluster to point to us
                SetEntry(lastCluster, tableIterator.GetCurrentCluster());
                lastCluster = tableIterator.GetCurrentCluster();
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

void FATTable::MirrorFAT(uint32_t sector, const uint8_t* buffer)
{
    if (!m_Volume->m_FATMirrored) {
        return;
    }
    
    sector -= m_Volume->m_ActiveFAT * m_Volume->m_SectorsPerFAT;
    
    for (uint32_t i = 0; i < m_Volume->m_FATCount; ++i)
    {
        if (i == m_Volume->m_ActiveFAT) {
            continue;
        }
        m_Volume->m_BCache.CachedWrite_trw(i * m_Volume->m_SectorsPerFAT + sector, buffer, 1);
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

} // namespace
