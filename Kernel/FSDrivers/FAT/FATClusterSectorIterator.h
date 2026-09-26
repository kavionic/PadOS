// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18/06/01 0:45:02

#pragma once

#include "Kernel/VFS/KBlockCache.h"

namespace kernel
{
class FATVolume;

struct FATClusterSectorIterator
{
    FATClusterSectorIterator(Ptr<FATVolume> volume, uint32_t cluster, uint32_t sector);
    void Set(uint32_t cluster, uint32_t sector);
    
    uint32_t        GetBlockSector();
    size_t          GetRemainingContiguousSectorCount();
    KCacheBlockDesc GetBlock_(bool doLoad, size_t readAheadBlockCount = 1);
    
    bool Increment(int sectors);

    PErrorCode MarkBlockDirty();
    void        ReadBlock(uint8_t* buffer, size_t readAheadBlockCount = 1);
    void        WriteBlock(const uint8_t* buffer);
    
    Ptr<FATVolume> m_Volume;
    uint32_t       m_CurrentCluster;
    uint32_t       m_CurrentSector;
    uint32_t       m_VisitedClusterCount;
};

} // namespace
