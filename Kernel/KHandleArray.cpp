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
// Created: 07.03.2018 16:00:14

#include "System/Platform.h"

#include "Kernel/KHandleArray.h"
#include "Ptr/NoPtr.h"

using namespace kernel;

uint8_t g_KHandleArrayEmptyBlockBuffer[sizeof(KHandleArrayEmptyBlock)];
Ptr<KHandleArrayEmptyBlock> KHandleArrayEmptyBlock::g_KHandleArrayEmptyBlock;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KHandleArrayEmptyBlock> KHandleArrayEmptyBlock::GetInstance()
{
    if (__builtin_expect(g_KHandleArrayEmptyBlock == nullptr, 0))
    {
        g_KHandleArrayEmptyBlock = ptr_new_cast(new ((void*)&g_KHandleArrayEmptyBlockBuffer) KHandleArrayEmptyBlock());
    }

    return g_KHandleArrayEmptyBlock;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KHandleArrayEmptyBlock::KHandleArrayEmptyBlock() : KHandleArrayBlock(false)
{
    for (int i = 0; i < KHANDLER_ARRAY_BLOCK_SIZE; ++i) {
        m_Array[i] = ptr_tmp_cast(this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KHandleArrayBlock::KHandleArrayBlock(bool doInit)
{
    if (doInit)
    {
        for (int i = 0; i < KHANDLER_ARRAY_BLOCK_SIZE; ++i) {
            m_Array[i] = KHandleArrayEmptyBlock::GetInstance();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KHandleArrayBase::AllocHandle(handle_id& outHandle)
{
    bool needBuffsers = false;
    bool allocBuffersFailed = false;
    for (;;)
    {
        if (needBuffsers && m_CachedBlockCount < 2)
        {
            if (!AllocBuffers()) {
                allocBuffersFailed = true;
            }
            needBuffsers = false;
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
                        needBuffsers = true;
                        continue;
                    }
                }
                m_TopLevel.m_Array[index1] = block2;
            }
            Ptr<KHandleArrayBlock> block3 = ptr_static_cast<KHandleArrayBlock>(block2->m_Array[index2]);
            if (block3 == KHandleArrayEmptyBlock::GetInstance())
            {
                block3 = AllocBlock();
                if (block3 == nullptr)
                {
                    if (block2->m_UsedIndexes == 0)
                    {
                        m_TopLevel.m_Array[index1] = KHandleArrayEmptyBlock::GetInstance();
                        CacheBlock(block2);
                    }
                    if (allocBuffersFailed) {
                        return PErrorCode::NoMemory;
                    } else {
                        m_NextHandle--;
                        needBuffsers = true;
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
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KHandleArrayBase::FreeHandle(int handle)
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
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KHandleArrayBase::CacheBlock(Ptr<KHandleArrayBlock> block)
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

///////////////////////////////////////////////////////////////////////////////
/// Make sure there is at least 2 blocks available in m_ReservedBlocks.
/// This ensures that AllocHandle() can finish without invoking the global
/// allocator while interrupts are disabled.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KHandleArrayBase::AllocBuffers()
{
    if (m_CachedBlockCount < 2)
    {
        for (;;)
        {
            Ptr<KHandleArrayBlock> block;
            try {
                block = ptr_new<KHandleArrayBlock>();
            }
            catch (const std::bad_alloc& error) {
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
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<kernel::KHandleArrayBlock> KHandleArrayBase::AllocBlock()
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
