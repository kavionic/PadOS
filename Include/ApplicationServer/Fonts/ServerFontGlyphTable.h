// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <ApplicationServer/Fonts/ServerFontGlyph.h>
#include <DeviceControl/DisplayFont.h>
#include <Ptr/Ptr.h>

class PFontGlyphCache;

// Each font owns its glyph-index buckets and Unicode index. The inherited view
// stays at a stable address when either table grows. Only the LRU is global.
class PServerFontGlyphTable : public PDisplayFontCache
{
public:
    static constexpr size_t INITIAL_BUCKET_COUNT = 16;

    explicit PServerFontGlyphTable(Ptr<PFontGlyphCache> cache);
    ~PServerFontGlyphTable();
    PServerFontGlyphTable(const PServerFontGlyphTable&) = delete;
    PServerFontGlyphTable& operator=(const PServerFontGlyphTable&) = delete;

    PFontGlyphCache& GetCache() const noexcept { return *m_Cache; }
    size_t GetMemorySize() const noexcept;
    PServerFontGlyph* FindCharacter(uint32_t character) const noexcept;
    PServerFontGlyph* FindGlyphIndex(uint32_t glyphIndex) const noexcept;
    void StoreGlyph(Ptr<PServerFontGlyph> glyph);
    void AddCharacter(PServerFontGlyph& glyph, uint32_t character);
    void EraseGlyph(PServerFontGlyph& glyph) noexcept;

    void SetAscenderOffset(PPoint offset) noexcept { m_AscenderOffset = offset; }
    void SetDescenderOffset(PPoint offset) noexcept { m_DescenderOffset = offset; }
    void SetDirection(PDisplayFontDirection direction) noexcept { m_Direction = direction; }
    void SetHasKerning(bool hasKerning) noexcept { m_HasKerning = hasKerning; }

    void SetKerningScale(int32_t scaleX, int32_t scaleY) noexcept
    {
        m_KerningScaleX = scaleX;
        m_KerningScaleY = scaleY;
    }

private:
    void ReleaseEmptyIndexes() noexcept;
    void GrowGlyphIndex();
    void GrowCharacterIndex();
    PDisplayFontCharacter* FindCharacterBucket(PDisplayFontGlyph& glyph, size_t index) const noexcept;
    void RemoveCharacter(PServerFontGlyph& glyph, PDisplayFontCharacter& entry) noexcept;

    Ptr<PFontGlyphCache> m_Cache;
    std::vector<Ptr<PServerFontGlyph>> m_GlyphBuckets;
    std::vector<const PDisplayFontGlyph*> m_CharacterBuckets;
    size_t m_GlyphCount = 0;
    size_t m_CharacterCount = 0;
};
