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
// Created: 18/05/25 22:58:12

#pragma once

#include "System/Ptr/Ptr.h"
#include "Kernel/VFS/KBlockCache.h"

namespace kernel
{
class FATVolume;

class FATTableIterator
{
public:
    FATTableIterator(Ptr<FATVolume> volume, uint32_t startCluster);
    ~FATTableIterator();
    
    void SetCluster(uint32_t cluster);
    void Increment();
    uint32_t GetCurrentCluster() const { return m_CurrentCluster; }
    
    bool SetEntry(uint32_t value);
    bool GetEntry(uint32_t* value);
    
private:
    bool Update();
    
    Ptr<FATVolume>  m_Volume;
    uint32_t        m_CurrentCluster;
    off64_t         m_CurrentSector;
    uint32_t        m_OffsetInSector;
    
    off64_t         m_LoadedSector1;
    off64_t         m_LoadedSector2;
    KCacheBlockDesc m_Block1;
    KCacheBlockDesc m_Block2;
};

} // namespace
