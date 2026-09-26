// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.11.2025 23:30

#pragma once

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const void* Source;
    void*       Destination;
    uint32_t    Size;
} PRelocMemSection;


void p_relocate_memory_sections(const PRelocMemSection* sections, size_t count);


#ifdef __cplusplus
}
#endif

