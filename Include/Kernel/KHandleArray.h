// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 07.03.2018 16:00:15

#pragma once

#include <System/ErrorCodes.h>

#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "Utils/Utils.h"
#include "Scheduler.h"

namespace kernel
{

static const int KHANDLER_ARRAY_BLOCK_SIZE = 256;


struct KHandleArrayBlock : public PtrTarget
{
    KHandleArrayBlock(bool doInit = true);
    Ptr<PtrTarget> m_Array[KHANDLER_ARRAY_BLOCK_SIZE];
    int   m_UsedIndexes = 0;
};

struct KHandleArrayEmptyBlock : KHandleArrayBlock
{
    static Ptr<KHandleArrayEmptyBlock> GetInstance();
    KHandleArrayEmptyBlock();

    static Ptr<KHandleArrayEmptyBlock> g_KHandleArrayEmptyBlock;
};


class KHandleArrayBase
{
public:
    KHandleArrayBase() {}

    int GetHandleCount() const { return m_TopLevel.m_UsedIndexes; }

    PErrorCode AllocHandle(handle_id& outHandle);
    bool FreeHandle(int handle);
    void CacheBlock(Ptr<KHandleArrayBlock> block);

protected:
    bool                    AllocBuffers();
    Ptr<KHandleArrayBlock>  AllocBlock();

    KHandleArrayBlock      m_TopLevel;
    Ptr<KHandleArrayBlock> m_ReservedBlocks[4];
    volatile int32_t       m_CachedBlockCount = 0;

    int                    m_NextHandle = 0;

    KHandleArrayBase(const KHandleArrayBase &c) = delete;
    KHandleArrayBase& operator=(const KHandleArrayBase &c) = delete;

};

template<typename T>
class KHandleArray : public KHandleArrayBase
{
public:
///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

    void Set(int handle, Ptr<T> object)
    {
        int index1 = (handle >> 16) & 0xff;
        int index2 = (handle >> 8) & 0xff;
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            Ptr<KHandleArrayBlock> block = ptr_static_cast<KHandleArrayBlock>(ptr_static_cast<KHandleArrayBlock>(m_TopLevel.m_Array[index1])->m_Array[index2]);
            if (block != KHandleArrayEmptyBlock::GetInstance())
            {
                int index3 = handle & 0xff;
                block->m_Array[index3] = object;
            }
        } CRITICAL_END;
    }

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

    Ptr<T> Get(int handle) const
    {
        if (handle >= 0)
        {
            int index1 = (handle >> 16) & 0xff;
            int index2 = (handle >> 8) & 0xff;
            int index3 = handle & 0xff;
            CRITICAL_BEGIN(CRITICAL_IRQ)
            {
                Ptr<PtrTarget> object = ptr_static_cast<KHandleArrayBlock>(ptr_static_cast<KHandleArrayBlock>(m_TopLevel.m_Array[index1])->m_Array[index2])->m_Array[index3];
                if (object != KHandleArrayEmptyBlock::GetInstance())
                {
                    return ptr_static_cast<T>(object);
                }
            } CRITICAL_END;
        }
        return nullptr;
    }

    template<typename DELEGATE>
    Ptr<T> GetNext(int handle, DELEGATE delegate) const
    {
        for (int i = (handle != INVALID_HANDLE) ? (handle + 1) : 0; i <= 0xffffff; ++i)
        {
            int index1 = (i >> 16) & 0xff;
            int index2 = (i >> 8) & 0xff;
            int index3 = i & 0xff;

            CRITICAL_BEGIN(CRITICAL_IRQ)
            {
                Ptr<KHandleArrayBlock> block2 = ptr_static_cast<KHandleArrayBlock>(m_TopLevel.m_Array[index1]);
                if (block2 == KHandleArrayEmptyBlock::GetInstance()) {
                    i = ((index1 + 1) << 16) - 1;
                    continue;
                }
                Ptr<KHandleArrayBlock> block3 = ptr_static_cast<KHandleArrayBlock>(block2->m_Array[index2]);
                if (block3 == KHandleArrayEmptyBlock::GetInstance()) {
                    i = (index1 << 16) + ((index2 + 1) << 8) - 1;
                    continue;
                }
                if (block3->m_Array[index3] != KHandleArrayEmptyBlock::GetInstance())
                {
                    Ptr<T> object = ptr_static_cast<T>(block3->m_Array[index3]);

                    if (delegate(object))
                    {
                        return object;
                    }
                }
            } CRITICAL_END;
        }
        return nullptr;
    }
};

} // namespace
