// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
            panic("{} passed file-handle with invalid magic number {:#08x}", functionName, m_Magic);
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
