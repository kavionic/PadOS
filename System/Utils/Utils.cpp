// This file is part of PadOS.
//
// Copyright (c) 2016-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include "System/Platform.h"

#include "Utils/Utils.h"

extern unsigned char* _sheap;
extern unsigned char* _eheap;

#define HEAP_START ((uint8_t*)&_sheap)
#define HEAP_END   ((uint8_t*)&_eheap)

static uint32_t g_HeapSize = 0;

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* _sbrk(ptrdiff_t size)
{
    static uint8_t* heap = nullptr;
    uint8_t* prev_heap;


    if (heap == nullptr) {
        heap = HEAP_START;
    }
    prev_heap = heap;

    if ((heap + size) > HEAP_END)
    {
        return nullptr;
    }
    g_HeapSize += size;
    heap += size;
    if (size > 0) {
        memset(prev_heap, 0, size);
    }
    return prev_heap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t get_heap_size()
{
    return g_HeapSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t get_max_heap_size()
{
    return HEAP_END - HEAP_START;
}

} // extern "C"
