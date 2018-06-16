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
// Created: 18/05/25 23:04:15

#pragma once

#include <stdint.h>

#include "System/Ptr/Ptr.h"
#include "System/Ptr/PtrTarget.h"
#include "FATTableIterator.h"

#define END_FAT_ENTRY 0x0ffffff8
#define BAD_FAT_ENTRY 0x0ffffff1

namespace kernel
{

class FATVolume;
class FATINode;

// Root directory for FAT12 and FAT16 is hard-coded to 1.
#define IS_FIXED_ROOT(cluster) ((cluster) == 1)

class FATTable : public PtrTarget
{
public:
    static const uint32_t FIRST_DATA_CLUSTER = 2;
public:
    FATTable(Ptr<FATVolume> volume);
    ~FATTable();
    
    bool GetEntry(uint32_t cluster, uint32_t* value);
    bool SetEntry(uint32_t cluster, uint32_t value);
    bool GetChainEntry(uint32_t chainStart, uint32_t index, uint32_t* value);
    bool ValidateChainEntry(uint32_t chainStart, uint32_t index, uint32_t expectedValue);

    bool CountFreeClusters(uint32_t* result);
    bool GetChainLength(uint32_t cluster, size_t* outCount);
    bool SetChainLength(Ptr<FATINode> node, uint32_t clusters, bool updateICache);
    bool AllocateClusters(size_t count, uint32_t* firstCluster);
    bool ClearFATChain(uint32_t cluster);

    bool MirrorFAT(uint32_t sector, const uint8_t* buffer);

    void DumpChain(uint32_t startCluster);
    
private:

    Ptr<FATVolume> m_Volume;

    FATTableIterator m_TableIterator;

    FATTable(const FATTable&) = delete;
    FATTable& operator=(const FATTable&) = delete;
};

} // namespace
