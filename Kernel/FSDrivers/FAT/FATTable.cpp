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
    PERROR_THROW_CODE(PErrorCode::IOError);
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
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "FATTable::GetChainEntry({}, {})", chainStart, index);
    uint32_t cluster = chainStart;
    while(index--)
    {
        uint32_t prevCluster = cluster;
        cluster = GetEntry(prevCluster);
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "  {} -> {}", prevCluster, cluster);
        if (!m_Volume->IsDataCluster(cluster)) {
            break;
        }            
    }
    if (cluster == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATTABLE, "FATTable::GetChainEntry({}, {}) failed!", chainStart, index);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    return cluster;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#ifdef FAT_VERIFY_FAT_CHAINS
bool FATTable::ValidateChainEntry(uint32_t chainStart, uint32_t index, uint32_t expectedValue)
{
    uint32_t value;
    
    if (GetChainEntry(chainStart, index, &value))
    {
        if (value != expectedValue) {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATTABLE, "ValidateChainEntry({}, {}, {}) unexpected value {}!", chainStart, index, expectedValue, value);
        }
        return value == expectedValue;
    }
    else
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATTABLE, "ValidateChainEntry({}, {}, {}) failed!", chainStart, index, expectedValue);
        return false;
    }
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

size_t FATTable::GetChainLength(uint32_t cluster)
{
    size_t count = 0;

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "FATTable::GetChainLength() {}", cluster);
        
    if (!m_Volume->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    // not intended for use on root directory
    if (!m_Volume->IsDataCluster(cluster)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::GetChainLength() called on invalid cluster ({}).", cluster);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    while (m_Volume->IsDataCluster(cluster))
    {
        count++;
        // break out of circular fat chains in a sketchy manner
        if (count == m_Volume->m_TotalClusters)
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::GetChainLength() circular FAT chain detected.");
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        cluster = GetEntry(cluster);
    }

    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "  {:x} = {:x}", cluster, count);

    if (cluster == END_FAT_ENTRY) {
        return count;
    }
    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::GetChainLength() invalid chain. End cluster: {:x}", cluster);
    PERROR_THROW_CODE(PErrorCode::IOError);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::SetChainLength(Ptr<FATInode> node, uint32_t clusters, bool updateICache)
{
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): {:x} to {} clusters ({}).", node->m_InodeID, clusters, node->m_StartCluster);
    
    if (IS_FIXED_ROOT(node->m_StartCluster) || (!m_Volume->IsDataCluster(node->m_StartCluster) && (node->m_StartCluster != 0))) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::SetChainLength(): called on invalid cluster ({}).", node->m_StartCluster);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    if (clusters == 0)
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): truncating node to zero bytes.");
        if (node->m_StartCluster == 0) {
            return;
        }
        // truncate to zero bytes
        uint32_t c = node->m_StartCluster;
        node->m_StartCluster = 0;
        node->m_EndCluster = 0;
        ClearFATChain(c);
        if (updateICache) {
            m_Volume->SetInodeIDToLocationIDMapping(node->m_InodeID, GENERATE_DIR_INDEX_INODEID(node->m_ParentInodeID, node->m_DirStartIndex));
        }
        // Write to disk so that get_next_dirent doesn't barf
        node->Write();
        return;
    }

    if (node->m_StartCluster == 0)
    {
        // From 0 clusters to some clusters.
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): node has no clusters. adding {} clusters.", clusters);

        uint32_t newChainStart = AllocateClusters(clusters);

        node->m_StartCluster = newChainStart;
        node->m_EndCluster = GetChainEntry(newChainStart, clusters - 1);

        if (updateICache) {
            m_Volume->SetInodeIDToLocationIDMapping(node->m_InodeID, GENERATE_DIR_CLUSTER_INODEID(node->m_ParentInodeID, node->m_StartCluster));
        }
        // Write to disk so that get_next_dirent doesn't barf.
        node->Write();

        return;
    }

    uint32_t currentClusterCount = uint32_t((node->m_Size + m_Volume->m_BytesPerSector * m_Volume->m_SectorsPerCluster - 1) / m_Volume->m_BytesPerSector / m_Volume->m_SectorsPerCluster);
    if (currentClusterCount == clusters) return;

    if (clusters > currentClusterCount)
    {
        

        // From some clusters to more clusters.
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): adding {} new fat entries.", clusters - currentClusterCount);
        const uint32_t newChainStart = AllocateClusters(clusters - currentClusterCount);
        kassert(m_Volume->IsDataCluster(newChainStart));

        uint32_t prevEndCluster = node->m_EndCluster;
        node->m_EndCluster = GetChainEntry(newChainStart, clusters - currentClusterCount - 1);

        SetEntry(prevEndCluster, newChainStart);

        return;
    }

    // From some clusters to fewer clusters.
    // traverse fat chain
    
    uint32_t c = node->m_StartCluster;
    uint32_t newChainEnd = GetEntry(c);

    uint32_t newEndIndex;
    for (newEndIndex = 1; newEndIndex < clusters; ++newEndIndex)
    {
        if (!m_Volume->IsDataCluster(newChainEnd)) {
            break;
        }
        c = newChainEnd;
        newChainEnd = m_Volume->GetFATTable()->GetEntry(c);
    }

    kassert(newEndIndex == clusters);
    kassert(newChainEnd != END_FAT_ENTRY);
    if (newEndIndex == clusters && newChainEnd == END_FAT_ENTRY) return;

    //	if (n < 0) return n;
    if (newChainEnd != END_FAT_ENTRY && !m_Volume->IsDataCluster(newChainEnd)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

    // clear trailing fat entries
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::SetChainLength(): clearing trailing fat entries.");
    SetEntry(c, END_FAT_ENTRY);
    node->m_EndCluster = c;
    ClearFATChain(newChainEnd);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t FATTable::AllocateClusters(size_t count)
{
    uint32_t first = 0, last = 0;
    int32_t clustersFound = 0;

    // mark end of chain for allocations
//    uint32_t value = 0x0ffffff8;
    uint32_t value = 0x0fffffff;

    if (!m_Volume->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IOError);
    }
    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::AllocateClusters(): {:x}", count);

    FATTableIterator tableIterator(m_Volume, m_Volume->m_LastAllocatedCluster);

    PScopeFail scopeCleanup([this, &first]() { if (first != 0) ClearFATChain(first); });

    for (uint32_t i = 0; i < m_Volume->m_TotalClusters; ++i)
    {
        uint32_t val = tableIterator.GetEntry();

        if (val == 0)
        {
            tableIterator.SetEntry(value);
            m_Volume->m_FreeClusters--;
            if (clustersFound == 0)
            {
                kassert(first == 0);
                first = last = tableIterator.GetCurrentCluster();
            }
            else
            {
                kassert(m_Volume->IsDataCluster(first));
                kassert(m_Volume->IsDataCluster(last));
                
                // Set previous last cluster to point to us
                SetEntry(last, tableIterator.GetCurrentCluster());
                last = tableIterator.GetCurrentCluster();
            }
            m_Volume->m_LastAllocatedCluster = last;
            if (++clustersFound == count) {
                break;
            }                
        }
        tableIterator.Increment();
    }
    m_Volume->UpdateFSInfo();
    if (clustersFound != count)
    {
        kernel_log<PLogSeverity::WARNING>(LogCat_FATTABLE, "FATTable::AllocateClusters(): Failed to allocate {} clusters. Not enough free entries ({} found).", count, clustersFound);
        PERROR_THROW_CODE(PErrorCode::NoSpace);
    }
    else
    {
        kassert(m_Volume->IsDataCluster(first));
#ifdef FAT_VERIFY_FAT_CHAINS
        size_t chainLength;
        kassert(GetChainLength(first, &chainLength) && chainLength == count);
#endif // FAT_VERIFY_FAT_CHAINS

        return first;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::ClearFATChain(uint32_t cluster)
{
    if (!m_Volume->IsDataCluster(cluster)) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::ClearFATChain() called on invalid cluster ({}).", cluster);
        PERROR_THROW_CODE(PErrorCode::IOError);
    }

//    ASSERT(count_clusters(vol, cluster) != 0);

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "FATTable::ClearFATChain(): Clearing fat chain: {}", cluster);
    while (m_Volume->IsDataCluster(cluster))
    {
        const uint32_t nextCluster = GetEntry(cluster);
        SetEntry(cluster, 0);

        m_Volume->m_FreeClusters++;
        cluster = nextCluster;
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATTABLE, "  clearing cluster: {}", cluster);
    }

    if (cluster != END_FAT_ENTRY) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATTABLE, "FATTable::ClearFATChain(): fat chain terminated improperly with {}.", cluster);
    }
    m_Volume->UpdateFSInfo();
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
    
    while (m_Volume->IsDataCluster(cluster))
    {
        cluster = GetEntry(cluster);
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATTABLE, "  {:x}", cluster);
    }
}

} // namespace
