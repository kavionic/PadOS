// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <limits>
#include <string_view>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SIZES_H

#include <ApplicationServer/Fonts/ServerFontGlyphTable.h>
#include <ApplicationServer/Fonts/ServerPreparedText.h>
#include <GUI/FontDescription.h>
#include <Ptr/Ptr.h>

class PFontGlyphCache;
class PServerFontFace;

class PServerFontInstance : public PtrTarget
{
public:
    PServerFontInstance(Ptr<PServerFontFace> face, Ptr<PFontGlyphCache> cache, const PFontDescription& description);
    ~PServerFontInstance();

    const PFontDescription& GetDescription() const { return m_Description; }
    PFontHeight GetHeight() const { return m_Height; }
    bool HasCharacter(uint32_t character) const;
    PFontTextMetrics MeasureText(
        std::string_view text,
        float width = std::numeric_limits<float>::infinity(),
        bool includeLast = false);
    PFontTextMetrics MeasureTextChunk(std::string_view text, float width, bool includeLast, uint32_t& previousGlyph);
    size_t GetTextKerningCount(std::string_view text) const noexcept;
    // A non-null output must have GetTextKerningCount(text) entries.
    PServerPreparedText PrepareText(std::string_view text, int32_t* kernings = nullptr);
    const PDisplayFontCache& GetDisplayCache() const noexcept { return m_Glyphs; }
    void ReleaseDrawCache(const PServerFontDrawCache& cache) noexcept;

private:
    void Activate();
    void InitializeSize();
    void InitializeTransform();
    PServerFontGlyph& GetGlyph(FT_UInt glyphIndex);
    FT_Pos QueryKerning(FT_UInt previousGlyph, FT_UInt glyphIndex);
    PServerFontGlyph& GetCharacter(uint32_t character);
    PFontTextMetrics WalkText(
        std::string_view text,
        float width,
        bool includeLast,
        PServerFontDrawCache* drawCache,
        uint32_t& previousGlyph,
        int32_t* kernings = nullptr);

    PServerFontGlyphTable m_Glyphs;
    PServerFontDrawCache* m_ActiveDrawCache = nullptr;
    Ptr<PServerFontFace> m_Face;
    PFontDescription m_Description;
    PFontHeight m_Height{};
    FT_Matrix m_Transform{};
    FT_Size m_Size = nullptr;
    FT_Int32 m_LoadFlags = FT_LOAD_DEFAULT;
};
