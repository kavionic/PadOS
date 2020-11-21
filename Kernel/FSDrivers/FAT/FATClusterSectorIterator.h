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
// Created: 18/06/01 0:45:02

#pragma once

#include "Kernel/VFS/KBlockCache.h"

namespace kernel
{
class FATVolume;

struct FATClusterSectorIterator
{
    FATClusterSectorIterator(Ptr<FATVolume> volume, uint32_t cluster, uint32_t sector);
    status_t Set(uint32_t cluster, uint32_t sector);
    
    off64_t         GetBlockSector();
    KCacheBlockDesc GetBlock(bool doLoad);
    
    status_t        Increment(int sectors);

    status_t        MarkBlockDirty();
    status_t        ReadBlock(uint8_t* buffer);
    status_t        WriteBlock(const uint8_t* buffer);
    
    Ptr<FATVolume> m_Volume;
    uint32_t       m_CurrentCluster;
    uint32_t       m_CurrentSector;
};

} // namespace
