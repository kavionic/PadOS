// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
