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
// Created: 18/06/08 23:53:12

#pragma once

#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/Kernel.h"

namespace kernel
{

struct FATDirectoryNode : public KDirectoryNode
{
    FATDirectoryNode(int openFlags) : KDirectoryNode(openFlags), m_Magic(MAGIC) {}
    ~FATDirectoryNode() { m_Magic = ~MAGIC; }
    
    bool CheckMagic(const char* functionName)
    {
        if (m_Magic != MAGIC)
        {
            panic("{} passed file-handle with invalid magic number {:#08x}", functionName, m_Magic);
            return false;
        }
        return true;
    }
    
    static const uint32_t MAGIC = 0x4C3A89D3;

    uint32_t    m_Magic;
    uint32_t    m_CurrentIndex;
};

} // namespace
