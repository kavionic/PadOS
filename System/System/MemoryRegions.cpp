// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <System/MemoryRegions.h>

const PMemoryRegionDefinition* p_find_memory_region(const PMemoryRegionTable& table, const char* name)
{
    if (name != nullptr)
    {
        for (size_t i = 0; i < table.Count; ++i)
        {
            if (strcmp(table.Regions[i].Name, name) == 0) {
                return &table.Regions[i];
            }
        }
    }
    return nullptr;
}

const PMemoryRegionDefinition* p_find_memory_region(const PMemoryRegionTable& table, uintptr_t address, size_t size)
{
    for (size_t i = 0; i < table.Count; ++i)
    {
        const PMemoryRegionDefinition& region = table.Regions[i];
        if (address >= region.Address && address - region.Address < region.Size
            && size <= region.Size - (address - region.Address)) {
            return &region;
        }
    }
    return nullptr;
}
