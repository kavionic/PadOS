// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/ServerFontDrawCache.h>

#include <cassert>
#include <utility>

#include <ApplicationServer/Fonts/ServerFontGlyph.h>
#include <ApplicationServer/Fonts/ServerFontInstance.h>

#include <ApplicationServer/Fonts/FontGlyphCache.h>

PServerFontDrawCache::PServerFontDrawCache(Ptr<PServerFontInstance> font, Ptr<PFontGlyphCache> cache)
    : m_Font(std::move(font))
    , m_Cache(std::move(cache))
{
}

PServerFontDrawCache::~PServerFontDrawCache()
{
    m_Font->ReleaseDrawCache(*this);
    while (m_FirstGlyph != nullptr)
    {
        Ptr<PServerFontGlyph> glyph = std::move(m_FirstGlyph);
        m_FirstGlyph = std::move(glyph->GetNextPinnedGlyph());
        m_Cache->UnpinGlyph(*glyph);
    }
    m_Cache->Trim();
}

const PDisplayFontCache* PServerFontDrawCache::GetDisplayCache() const noexcept
{
    return &m_Font->GetDisplayCache();
}

void PServerFontDrawCache::PinGlyph(PServerFontGlyph& glyph)
{
    if (!glyph.IsPinned())
    {
        assert(glyph.GetNextPinnedGlyph() == nullptr);
        m_Cache->PinGlyph(glyph);
        Ptr<PServerFontGlyph> pinned = ptr_tmp_cast(&glyph);
        if (m_LastGlyph != nullptr) {
            m_LastGlyph->GetNextPinnedGlyph() = std::move(pinned);
        } else {
            m_FirstGlyph = std::move(pinned);
        }
        m_LastGlyph = &glyph;
    }
}
