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

#include <string.h>

#include "FATTable.h"
#include "FATVolume.h"
#include "Kernel/FSDrivers/FAT/FATFilesystem.h"
#include "FATINode.h"

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

bool FATTable::GetEntry(uint32_t cluster, uint32_t* outValue)
{
    uint32_t value;
    m_TableIterator.SetCluster(cluster);
    if (m_TableIterator.GetEntry(&value))
    {
        if (value == 0 || m_Volume->IsDataCluster(value)) {
            *outValue = value;
            return true;
        }
        if (value >= END_FAT_ENTRY) {
            *outValue = END_FAT_ENTRY;
            return true;
        }	
        if (value >= BAD_FAT_ENTRY) {
            *outValue = BAD_FAT_ENTRY;
            return true;
        }
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::ERROR, "FATTable::GetEntry(): invalid FAT entry %lx for cluster %ld\n", value, cluster);
        return false;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::SetEntry(uint32_t cluster, uint32_t value)
{
    m_TableIterator.SetCluster(cluster);
    return m_TableIterator.SetEntry(value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::GetChainEntry(uint32_t chainStart, uint32_t index, uint32_t* value)
{
    if (!m_Volume->CheckMagic(__func__)) {
        set_last_error(EINVAL);
        return false;
    }        

    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_HIGH_VOL, "FATTable::GetChainEntry(%lu, %lu)\n", chainStart, index);
    uint32_t cluster = chainStart;
    while(index--)
    {
        uint32_t prevCluster = cluster;
        if (!GetEntry(prevCluster, &cluster)) {
            return false;
        }
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_HIGH_VOL, "  %lu -> %lu\n", prevCluster, cluster);
        if (!m_Volume->IsDataCluster(cluster)) {
            break;
        }            
    }
    if (cluster == 0) {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::ERROR, "FATTable::GetChainEntry() failed!\n");
        set_last_error(EINVAL);
        return false;
    }
    *value = cluster;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::ValidateChainEntry(uint32_t chainStart, uint32_t index, uint32_t expectedValue)
{
    uint32_t value;
    
    if (GetChainEntry(chainStart, index, &value)) {
        return value == expectedValue;
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::CountFreeClusters(uint32_t* result)
{
    m_TableIterator.SetCluster(FATTable::FIRST_DATA_CLUSTER);
    uint32_t count = 0;
    for (uint32_t i = 0; i < m_Volume->m_TotalClusters; ++i, m_TableIterator.Increment())
    {
        uint32_t value;
        if (!m_TableIterator.GetEntry(&value)) {
            return false;
        }
        if (value == 0) {
            count++;
        }
    }
    *result = count;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::GetChainLength(uint32_t cluster, size_t* outCount)
{
    size_t count = 0;

    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_HIGH_VOL, "FATTable::GetChainLength() %lx\n", cluster);
        
    if (!m_Volume->CheckMagic(__func__)) {
        return false;
    }
    // not intended for use on root directory
    if (!m_Volume->IsDataCluster(cluster)) {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::GetChainLength() called on invalid cluster (%lx)\n", cluster);
        set_last_error(EINVAL);
        return false;
    }

    while (m_Volume->IsDataCluster(cluster))
    {
        count++;
        // break out of circular fat chains in a sketchy manner
        if (count == m_Volume->m_TotalClusters)
        {
            kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::GetChainLength() circular FAT chain detected\n");
            set_last_error(EINVAL);
            return false; // Circular FAT chain.
        }
        if (!GetEntry(cluster, &cluster)) {
            return false;
        }
    }

    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_HIGH_VOL, "  %lx = %lx\n", cluster, count);

    if (cluster == END_FAT_ENTRY) {
        *outCount = count;
        return true;
    }
    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::GetChainLength() invalid chain. End cluster: %lx\n", cluster);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::SetChainLength(Ptr<FATINode> node, uint32_t clusters, bool updateICache)
{
    uint32_t i;
    uint32_t c, n;

    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_LOW_VOL, "FATTable::SetChainLength(): %" PRIx64 " to %lx clusters (%lx)\n", node->m_INodeID, clusters, node->m_StartCluster);
    
    if (IS_FIXED_ROOT(node->m_StartCluster) || (!m_Volume->IsDataCluster(node->m_StartCluster) && (node->m_StartCluster != 0))) {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::SetChainLength(): called on invalid cluster (%lx)\n", node->m_StartCluster);
        set_last_error(EINVAL);
        return false;
    }

    if (clusters == 0)
    {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_LOW_VOL, "FATTable::SetChainLength(): truncating node to zero bytes\n");
        if (node->m_StartCluster == 0) {
            return true;
        }
        // truncate to zero bytes
        c = node->m_StartCluster;
        node->m_StartCluster = 0;
        node->m_EndCluster = 0;
        bool result = ClearFATChain(c);
        if (updateICache && result) {
            result = m_Volume->SetINodeIDToLocationIDMapping(node->m_INodeID, GENERATE_DIR_INDEX_INODEID(node->m_ParentINodeID, node->m_DirStartIndex));
        }
        // Write to disk so that get_next_dirent doesn't barf
        node->Write();
        return result;
    }

    if (node->m_StartCluster == 0)
    {
        // From 0 clusters to some clusters.
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_LOW_VOL, "FATTable::SetChainLength(): node has no clusters. adding %lx clusters\n", clusters);

        if (!AllocateClusters(clusters, &n)) {
            return false;
        }
        node->m_StartCluster = n;
        GetChainEntry(n, clusters - 1, &node->m_EndCluster);

        bool result = true;
        if (updateICache) {
            result = m_Volume->SetINodeIDToLocationIDMapping(node->m_INodeID, GENERATE_DIR_CLUSTER_INODEID(node->m_ParentINodeID, node->m_StartCluster));
        }
        // Write to disk so that get_next_dirent doesn't barf.
        node->Write();

        return result;
    }

    i = uint32_t((node->m_Size + m_Volume->m_BytesPerSector * m_Volume->m_SectorsPerCluster - 1) / m_Volume->m_BytesPerSector / m_Volume->m_SectorsPerCluster);
    if (i == clusters) return true;

    if (clusters > i)
    {
        // From some clusters to more clusters.
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_LOW_VOL, "FATTable::SetChainLength(): adding %lx new fat entries\n", clusters - i);
        if (!AllocateClusters(clusters - i, &n)) {
            return false;
        }
        kassert(m_Volume->IsDataCluster(n));

        c = node->m_EndCluster;
        GetChainEntry(n, clusters - i - 1, &node->m_EndCluster);

        return SetEntry(c, n);
    }

    // From some clusters to fewer clusters.
    // traverse fat chain
    c = node->m_StartCluster;
    if (!GetEntry(c, &n)) {
        return false;
    }
    for (i = 1; i < clusters; ++i)
    {
        if (!m_Volume->IsDataCluster(n)) {
            break;
        }
        c = n;
        if (!m_Volume->GetFATTable()->GetEntry(c, &n)) {
            return false;
        }
    }

    kassert(i == clusters);
    kassert(n != END_FAT_ENTRY);
    if (i == clusters && n == END_FAT_ENTRY) return true;

    //	if (n < 0) return n;
    if (n != END_FAT_ENTRY && !m_Volume->IsDataCluster(n)) {
        set_last_error(EINVAL);
        return false;
    }

    // clear trailing fat entries
    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_LOW_VOL, "FATTable::SetChainLength(): clearing trailing fat entries\n");
    if (!SetEntry(c, 0x0ffffff8)) {
        return false;
    }
    node->m_EndCluster = c;
    return ClearFATChain(n);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::AllocateClusters(size_t count, uint32_t* firstCluster)
{
    bool result = true;
    uint32_t first = 0, last = 0;
    int32_t clustersFound = 0;

    // mark end of chain for allocations
//    uint32_t value = 0x0ffffff8;
    uint32_t value = 0x0fffffff;

    if (!m_Volume->CheckMagic(__func__)) {
        set_last_error(EINVAL);
        return false;
    }
    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_LOW_VOL, "FATTable::AllocateClusters(): %lx\n", count);

    FATTableIterator tableIterator(m_Volume, m_Volume->m_LastAllocatedCluster);

    for (uint32_t i = 0; i < m_Volume->m_TotalClusters; ++i)
    {
        uint32_t val;
        if (!tableIterator.GetEntry(&val)) {
            kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::AllocateClusters(): failed to read table entry\n");
            result = false;
            break;
        }

        if (val == 0)
        {
            if (!tableIterator.SetEntry(value)) {
                kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::AllocateClusters(): failed to write table entry\n");
                result = false;
                break;
            }
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
                if (!SetEntry(last, tableIterator.GetCurrentCluster())) {
                    result = false;
                    kassert(false);
                    break;
                }
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
    if (!result)
    {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::ERROR, "FATTable::AllocateClusters(): Failed to allocate %ld clusters. Clearing chain (%lx): %s\n", count, first, strerror(result));
        if (first != 0) ClearFATChain(first);
        return false;
    }
    else if (clustersFound != count)
    {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::WARNING, "FATTable::AllocateClusters(): Failed to allocate %ld clusters. Not enough free entries (%ld found)\n", count, clustersFound);
        if (first != 0) ClearFATChain(first);
        set_last_error(ENOSPC);
        return false;
    }
    else
    {
        kassert(m_Volume->IsDataCluster(first));
        size_t chainLength;
        kassert(GetChainLength(first, &chainLength) && chainLength == count);
        *firstCluster = first;
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::ClearFATChain(uint32_t cluster)
{
    if (!m_Volume->IsDataCluster(cluster)) {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::ClearFATChain() called on invalid cluster (%lx)\n", cluster);
        set_last_error(EINVAL);
        return false;
    }

//    ASSERT(count_clusters(vol, cluster) != 0);

    kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_LOW_VOL, "FATTable::ClearFATChain(): Clearing fat chain: %lx\n", cluster);
    while (m_Volume->IsDataCluster(cluster))
    {
        uint32_t nextCluster;
        if (!GetEntry(cluster, &nextCluster)) {
            kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::ERROR, "FATTable::ClearFATChain(): failed clearing fat entry for cluster %lx (%s)\n", cluster, strerror(get_last_error()));
            return false;
        }
        if (!SetEntry(cluster, 0)) {
            kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::ERROR, "FATTable::ClearFATChain(): failed clearing fat entry for cluster %lx (%s)\n", cluster, strerror(get_last_error()));
            return false;
        }
        m_Volume->m_FreeClusters++;
        cluster = nextCluster;
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::INFO_HIGH_VOL, "  clearing cluster: %lx\n", cluster);
    }

    if (cluster != END_FAT_ENTRY) {
        kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::CRITICAL, "FATTable::ClearFATChain(): fat chain terminated improperly with %lx\n", cluster);
    }
    m_Volume->UpdateFSInfo();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATTable::MirrorFAT(uint32_t sector, const uint8_t* buffer)
{
    if (!m_Volume->m_FATMirrored) {
        return true;
    }
    
    sector -= m_Volume->m_ActiveFAT * m_Volume->m_SectorsPerFAT;
    
    for (uint32_t i = 0; i < m_Volume->m_FATCount; ++i)
    {
        if (i == m_Volume->m_ActiveFAT) {
            continue;
        }
        m_Volume->m_BCache.CachedWrite(i * m_Volume->m_SectorsPerFAT + sector, buffer, 1);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATTable::DumpChain(uint32_t startCluster)
{
    kprintf("FAT chain: %lx", startCluster);
    uint32_t cluster = startCluster;
    
    while (m_Volume->IsDataCluster(cluster))
    {
        if (!GetEntry(cluster, &cluster))
        {
            kprintf("\n");
            kernel_log(FATFilesystem::LOGC_FATTABLE, KLogSeverity::ERROR, "FATTable::DumpChain() failed to get FAT table entry for %lx\n", cluster);
            break;
        }
        kprintf(" %lx", cluster);
    }
    kprintf("\n");
}

} // namespace
