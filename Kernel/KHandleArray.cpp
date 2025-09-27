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
//    static KHandleArrayEmptyBlock instance;
//    static NoPtr<KHandleArrayEmptyBlock> instancePtr; // = ptr_new_cast(&instance);
//    return instancePtr;

    if (__builtin_expect(g_KHandleArrayEmptyBlock == nullptr, 0))
    {
        g_KHandleArrayEmptyBlock = ptr_new_cast(new ((void*)&g_KHandleArrayEmptyBlockBuffer) KHandleArrayEmptyBlock());
    }

    return g_KHandleArrayEmptyBlock;
}

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
