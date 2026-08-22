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
// Created: 18/05/25 23:04:15

#pragma once

#include <stdint.h>

#include "Ptr/Ptr.h"
#include "Ptr/PtrTarget.h"
#include "FATTableIterator.h"

#define END_FAT_ENTRY 0x0ffffff8
#define BAD_FAT_ENTRY 0x0ffffff7

namespace kernel
{

class FATVolume;
class FATInode;

// Root directory for FAT12 and FAT16 is hard-coded to 1.
#define IS_FIXED_ROOT(cluster) ((cluster) == 1)

struct FATVolumeStatus
{
    bool IsSupported = false;
    bool IsClean = true;
    bool HasHardError = false;
};

class FATTable : public PtrTarget
{
public:
    static const uint32_t FIRST_DATA_CLUSTER = 2;
public:
    FATTable(Ptr<FATVolume> volume);
    ~FATTable();

    FATVolumeStatus ReadVolumeStatus();
    void SetVolumeClean(bool isClean);
    
    uint32_t GetEntry(uint32_t cluster);
    void SetEntry(uint32_t cluster, uint32_t value);
    uint32_t GetChainEntry(uint32_t chainStart, uint32_t index);
#ifdef FAT_VERIFY_FAT_CHAINS
    bool ValidateChainEntry(uint32_t chainStart, uint32_t index, uint32_t expectedValue);
#endif // FAT_VERIFY_FAT_CHAINS

    uint32_t    CountFreeClusters();
    size_t      GetChainLength(uint32_t cluster, uint32_t* endCluster = nullptr);
    void        SetChainLength(Ptr<FATInode> node, uint32_t clusterCount, bool updateICache);
    uint32_t    AllocateClusters(size_t clusterCount, uint32_t* endCluster = nullptr);
    void        ClearFATChain(uint32_t cluster);
    void        ClearFATChainAfterFailureNoThrow(uint32_t startCluster, const char* operation) noexcept;

    void DumpChain(uint32_t startCluster);

private:
    struct VolumeStatusMasks
    {
        uint32_t CleanShutdown;
        uint32_t NoHardError;
        uint32_t ReservedOneBits;
    };

    static VolumeStatusMasks GetVolumeStatusMasks(uint8_t fatBits);
    static uint32_t ReadVolumeStatusEntry(const uint8_t* fatSector, uint8_t fatBits);
    static void WriteVolumeStatusEntry(uint8_t* fatSector, uint8_t fatBits, uint32_t value);

    Ptr<FATVolume> m_Volume;

    FATTableIterator m_TableIterator;

    FATTable(const FATTable&) = delete;
    FATTable& operator=(const FATTable&) = delete;
};

} // namespace
