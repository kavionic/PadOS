// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.11.2025 23:30

#include <assert.h>
#include <Utils/RelocMemSection.h>

void p_relocate_memory_sections(const PRelocMemSection* sections, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const PRelocMemSection& section = sections[i];

        if (section.Source != section.Destination)
        {
            assert((section.Size & 3) == 0);

            const uint32_t* src = reinterpret_cast<const uint32_t*>(section.Source);
            uint32_t*       dst = reinterpret_cast<uint32_t*>(section.Destination);
            const size_t    words = section.Size / 4;

            for (size_t j = 0; j < words; ++j) {
                *dst++ = *src++;
            }
        }
    }
}
