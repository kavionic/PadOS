// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ApplicationServer/Fonts/ServerFontGlyph.h>

// Global eviction policy only. Font instances own their lookup tables and
// glyphs; the LRU contains unpinned glyphs and never keeps a font alive.
// The budget covers glyph allocations, alias capacity, and lookup tables.
// Allocator overhead is estimated; pinned glyphs and fixed table metadata
// can keep usage above the budget until the submission or font is released.
class PFontGlyphCache : public PtrTarget
{
public:
    static constexpr size_t DEFAULT_MEMORY_BUDGET = 512 * 1024;
    // Allow for allocation headers and alignment waste in each heap block.
    static constexpr size_t ALLOCATION_OVERHEAD = 16;

    explicit PFontGlyphCache(size_t memoryBudget = DEFAULT_MEMORY_BUDGET);
    ~PFontGlyphCache();

    static constexpr size_t GetAllocationMemorySize(size_t size) noexcept
    {
        return (size != 0) ? size + ALLOCATION_OVERHEAD : 0;
    }
    size_t GetMemoryUsage() const noexcept { return m_MemoryUsage; }
    void UpdateMemoryUsage(size_t previousSize, size_t newSize) noexcept;
    void RegisterGlyph(PServerFontGlyph& glyph) noexcept;
    void RemoveGlyph(PServerFontGlyph& glyph) noexcept;
    void TouchGlyph(PServerFontGlyph& glyph) noexcept;
    void PinGlyph(PServerFontGlyph& glyph) noexcept;
    void UnpinGlyph(PServerFontGlyph& glyph) noexcept;
    void Trim() noexcept;

private:
    PIntrusiveList<PServerFontGlyph> m_Entries;
    size_t m_MemoryBudget;
    size_t m_MemoryUsage = 0;
};
