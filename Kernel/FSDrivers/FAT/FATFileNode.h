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
// Created: 18/06/08 23:57:35

#pragma once

#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/Kernel.h"

namespace kernel
{


class FATFileNode : public KFileNode
{
    public:
    static const uint32_t MAGIC = 0x8b10664d;
    FATFileNode(int openFlags) : KFileNode(openFlags) { m_Magic = MAGIC; }
    ~FATFileNode() { m_Magic = ~MAGIC; }
    
    bool CheckMagic(const char* functionName)
    {
        if (m_Magic != MAGIC)
        {
            panic("%s passed file-handle with invalid magic number 0x%08x\n", functionName, m_Magic);
            return false;
        }
        return true;
    }

    uint32_t m_Magic;

    uint32_t m_FATIteration;  // Iteration from the inode. If it don't match the inode, the cached cluster must be discarded.
    uint32_t m_FATChainIndex; // Index in the fat chain
    uint32_t m_CachedCluster;
};

} // namespace
