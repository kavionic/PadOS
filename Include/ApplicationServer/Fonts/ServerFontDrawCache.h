// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Ptr/Ptr.h>

class PDisplayFontCache;
class PFontGlyphCache;
class PServerFontGlyph;
class PServerFontInstance;

// One active batch per font instance. It owns a chain of distinct glyphs and
// keeps its font alive; the font's non-owning back pointer is cleared on release.
class PServerFontDrawCache : public PtrTarget
{
public:
    PServerFontDrawCache(Ptr<PServerFontInstance> font, Ptr<PFontGlyphCache> cache);
    ~PServerFontDrawCache();
    const PDisplayFontCache* GetDisplayCache() const noexcept;
    void PinGlyph(PServerFontGlyph& glyph);

private:
    Ptr<PServerFontInstance> m_Font;
    Ptr<PFontGlyphCache> m_Cache;
    Ptr<PServerFontGlyph> m_FirstGlyph;
    PServerFontGlyph* m_LastGlyph = nullptr;
};
