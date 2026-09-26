// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18/05/25 22:58:12

#pragma once

#include <array>

#include "Ptr/Ptr.h"
#include "Kernel/VFS/KBlockCache.h"

namespace kernel
{
inline constexpr size_t FAT_MAX_SUPPORTED_FAT_COUNT = 8;

class FATVolume;

class FATTableIterator
{
public:
    FATTableIterator(Ptr<FATVolume> volume, uint32_t startCluster);
    ~FATTableIterator();
    
    void SetCluster(uint32_t cluster);
    void Increment();
    uint32_t GetCurrentCluster() const { return m_CurrentCluster; }
    
    void SetEntry(uint32_t value);
    uint32_t GetEntry();
    
private:
    using MirrorBlockArray = std::array<KCacheBlockDesc, FAT_MAX_SUPPORTED_FAT_COUNT - 1>;
    static constexpr uint32_t INVALID_SECTOR = UINT32_MAX;

    size_t AcquireMirrorBlocks(uint32_t activeSector, MirrorBlockArray& mirrorBlocks);
    void CopyToMirrorBlocks(const uint8_t* sourceBuffer, MirrorBlockArray& mirrorBlocks, size_t mirrorBlockCount);
    void Update();
    
    Ptr<FATVolume>  m_Volume;
    uint32_t        m_CurrentCluster;
    uint32_t        m_CurrentSector;
    uint32_t        m_OffsetInSector;
    
    uint32_t        m_LoadedSector1;
    uint32_t        m_LoadedSector2;
    KCacheBlockDesc m_Block1;
    KCacheBlockDesc m_Block2;
};

} // namespace
