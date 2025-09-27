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


template<typename T>
class KHandleArray
{
public:
    KHandleArray() {}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

    int GetHandleCount() const { return m_TopLevel.m_UsedIndexes; }

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

    PErrorCode AllocHandle(handle_id& outHandle)
    {
        bool allocBuffersFailed = false;
        for (;;)
        {
            if (m_CachedBlockCount < 2) {
                if (!AllocBuffers()) {
                    allocBuffersFailed = true;
                }
            }
            CRITICAL_BEGIN(CRITICAL_IRQ)
            {
                int handle = m_NextHandle & 0x00ffffff;
                m_NextHandle++;

                int index1 = (handle >> 16) & 0xff;
                int index2 = (handle >> 8) & 0xff;
                int index3 = handle & 0xff;
                Ptr<KHandleArrayBlock> block2 = ptr_static_cast<KHandleArrayBlock>(m_TopLevel.m_Array[index1]);
                if (block2 == KHandleArrayEmptyBlock::GetInstance())
                {
                    block2 = AllocBlock();
                    if (block2 == nullptr)
                    {
                        if (allocBuffersFailed) {
                            return PErrorCode::NoMemory;
                        } else {
                            m_NextHandle--;
                            continue;
                        }
                    }
                    m_TopLevel.m_Array[index1] = block2;
                }
                Ptr<KHandleArrayBlock> block3 = ptr_static_cast<KHandleArrayBlock>(block2->m_Array[index2]);
                if (block3 == KHandleArrayEmptyBlock::GetInstance())
                {
                    block3 = AllocBlock();
                    if (block3 == nullptr) {
                        if (block2->m_UsedIndexes == 0)
                        {
                            m_TopLevel.m_Array[index1] = KHandleArrayEmptyBlock::GetInstance();
                            CacheBlock(block2);
                        }
                        if (allocBuffersFailed) {
                            return PErrorCode::NoMemory;
                        } else {
                            m_NextHandle--;
                            continue;
                        }
                    }
                    block2->m_Array[index2] = block3;
                    block2->m_UsedIndexes++;
                }
                if (block3->m_Array[index3] != KHandleArrayEmptyBlock::GetInstance()) continue; // Index has wrapped and we hit a used slot.
                block3->m_Array[index3] = nullptr;
                block3->m_UsedIndexes++;
                m_TopLevel.m_UsedIndexes++; // Toplevel block counts total number of used handles.
                outHandle = handle;
                return PErrorCode::Success;
            } CRITICAL_END;
        }
    }

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

    bool FreeHandle(int handle)
    {
        int index1 = (handle >> 16) & 0xff;
        int index2 = (handle >> 8) & 0xff;
        int index3 = handle & 0xff;

        // Keep the block pointers outside the critical section to make
        // sure we are not attempting to free any memory inside it.
        Ptr<KHandleArrayBlock> block2;
        Ptr<KHandleArrayBlock> block3;
        Ptr<PtrTarget>         object;
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            block2 = ptr_static_cast<KHandleArrayBlock>(m_TopLevel.m_Array[index1]);
            block3 = ptr_static_cast<KHandleArrayBlock>(block2->m_Array[index2]);
            object = block3->m_Array[index3];
            if (object != KHandleArrayEmptyBlock::GetInstance())
            {
                m_TopLevel.m_UsedIndexes--; // Toplevel block counts total number of used handles.
                block3->m_UsedIndexes--;
                if (block3->m_UsedIndexes > 0)
                {
                    block3->m_Array[index3] = KHandleArrayEmptyBlock::GetInstance();
                }
                else
                {
                    CacheBlock(block3);
                    block2->m_Array[index2] = KHandleArrayEmptyBlock::GetInstance();
                    block2->m_UsedIndexes--;
                    if (block2->m_UsedIndexes == 0)
                    {
                        CacheBlock(block2);
                        m_TopLevel.m_Array[index1] = KHandleArrayEmptyBlock::GetInstance();
                    }
                }
                return true;
            }
        } CRITICAL_END;
        return false;
    }

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

private:

///////////////////////////////////////////////////////////////////////////////
/// Make sure there is at least 2 blocks available in m_ReservedBlocks.
/// This ensures that AllocHandle() can finish without invoking the global
/// allocator while interrupts are disabled.
///////////////////////////////////////////////////////////////////////////////

    bool AllocBuffers()
    {
        if (m_CachedBlockCount < 2)
        {
            for (;;)
            {
                Ptr<KHandleArrayBlock> block;
                try {
                    block = ptr_new<KHandleArrayBlock>();
                } catch (const std::bad_alloc& error) {
                    return m_CachedBlockCount != 0;
                }
                CRITICAL_BEGIN(CRITICAL_IRQ)
                {
                    CacheBlock(block);
                    if (m_CachedBlockCount >= 2) {
                        return true;
                    }
                } CRITICAL_END;
            }
        }
        return true;
    }

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

    Ptr<KHandleArrayBlock> AllocBlock()
    {
        for (uint32_t i = 0; i < ARRAY_COUNT(m_ReservedBlocks); ++i)
        {
            if (m_ReservedBlocks[i] != nullptr)
            {
                Ptr<KHandleArrayBlock> block = m_ReservedBlocks[i];
                m_ReservedBlocks[i] = nullptr;
                m_CachedBlockCount--;
                return block;
            }
        }
        return nullptr;
    }

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

    void CacheBlock(Ptr<KHandleArrayBlock> block)
    {
        for (uint32_t i = 0; i < ARRAY_COUNT(m_ReservedBlocks); ++i)
        {
            if (m_ReservedBlocks[i] == nullptr) {
                m_ReservedBlocks[i] = block;
                m_CachedBlockCount++;
                break;
            }
        }
    }

    KHandleArrayBlock      m_TopLevel;
    Ptr<KHandleArrayBlock> m_ReservedBlocks[4];
    volatile int32_t       m_CachedBlockCount = 0;

    int                    m_NextHandle = 0;

    KHandleArray(const KHandleArray &c) = delete;
    KHandleArray& operator=(const KHandleArray &c) = delete;

};

} // namespace
