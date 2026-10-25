// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/FontGlyphCache.h>

#include <cassert>

#include <ApplicationServer/Fonts/ServerFontGlyphTable.h>

PFontGlyphCache::PFontGlyphCache(size_t memoryBudget)
    : m_MemoryBudget(memoryBudget)
{
}

PFontGlyphCache::~PFontGlyphCache()
{
    assert(m_Entries.IsEmpty());
    assert(m_MemoryUsage == 0);
}

void PFontGlyphCache::UpdateMemoryUsage(size_t previousSize, size_t newSize) noexcept
{
    assert(m_MemoryUsage >= previousSize);
    m_MemoryUsage = m_MemoryUsage - previousSize + newSize;
}

void PFontGlyphCache::RegisterGlyph(PServerFontGlyph& glyph) noexcept
{
    assert(!glyph.IsPinned());
    assert(glyph.GetCharacters().empty());
    m_Entries.Insert(nullptr, &glyph);
    UpdateMemoryUsage(0, glyph.GetMemorySize());
}

void PFontGlyphCache::RemoveGlyph(PServerFontGlyph& glyph) noexcept
{
    assert(!glyph.IsPinned());
    m_Entries.Remove(&glyph);
    UpdateMemoryUsage(glyph.GetMemorySize(), 0);
}

void PFontGlyphCache::TouchGlyph(PServerFontGlyph& glyph) noexcept
{
    if (!glyph.IsPinned() && m_Entries.GetFirst() != &glyph)
    {
        m_Entries.Remove(&glyph);
        m_Entries.Insert(nullptr, &glyph);
    }
}

void PFontGlyphCache::PinGlyph(PServerFontGlyph& glyph) noexcept
{
    assert(!glyph.IsPinned());
    m_Entries.Remove(&glyph);
    glyph.SetPinned(true);
}

void PFontGlyphCache::UnpinGlyph(PServerFontGlyph& glyph) noexcept
{
    assert(glyph.IsPinned());
    glyph.SetPinned(false);
    m_Entries.Insert(nullptr, &glyph);
}

void PFontGlyphCache::Trim() noexcept
{
    // A submission can temporarily exceed the budget while its glyphs are pinned.
    while (m_MemoryUsage > m_MemoryBudget)
    {
        PServerFontGlyph* glyph = m_Entries.GetLast();
        if (glyph == nullptr) {
            break;
        }
        glyph->GetOwner().EraseGlyph(*glyph);
    }
}
