// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/ServerFontInstance.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <utility>

#include <ApplicationServer/Fonts/ServerFontDrawCache.h>
#include <ApplicationServer/Fonts/ServerFontFace.h>
#include <ApplicationServer/Fonts/ServerFontGlyph.h>
#include <ApplicationServer/Fonts/ServerFontLibrary.h>
#include <System/ExceptionHandling.h>
#include <Utils/UTF8Utils.h>

#include <ApplicationServer/Fonts/FontGlyphCache.h>

PServerFontInstance::PServerFontInstance(
    Ptr<PServerFontFace> face,
    Ptr<PFontGlyphCache> cache,
    const PFontDescription& description)
    : m_Glyphs(std::move(cache))
    , m_Face(std::move(face))
    , m_Description(description)
{
    PServerFontLibrary::CheckError(FT_New_Size(m_Face->GetHandle(), &m_Size));
    PScopeFail rollback([this] { FT_Done_Size(m_Size); });
    InitializeSize();
    InitializeTransform();
    m_Glyphs.SetHasKerning(FT_HAS_KERNING(m_Face->GetHandle()));
    if (std::abs(m_Transform.yx) > std::abs(m_Transform.xx)) {
        m_Glyphs.SetDirection((m_Transform.yx > 0) ? PDisplayFontDirection::Up : PDisplayFontDirection::Down);
    } else {
        m_Glyphs.SetDirection((m_Transform.xx < 0) ? PDisplayFontDirection::Left : PDisplayFontDirection::Right);
    }
    if (!m_Description.Hinting) {
        m_LoadFlags |= FT_LOAD_NO_HINTING;
    }
    if (FT_IS_SCALABLE(m_Face->GetHandle())) {
        m_LoadFlags |= FT_LOAD_NO_BITMAP;
    }
    if (m_Description.RenderMode == PFontRenderMode::Monochrome) {
        m_LoadFlags |= FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME;
    }
    const FT_Size_Metrics& metrics = m_Size->metrics;
    m_Height.ascender = std::ceil(float(metrics.ascender) * FONT_F26DOT6_TO_FLOAT);
    m_Height.descender = std::ceil(float(-metrics.descender) * FONT_F26DOT6_TO_FLOAT);
    m_Height.line_gap = std::max(
        0.0f,
        std::ceil(float(metrics.height) * FONT_F26DOT6_TO_FLOAT) - m_Height.ascender - m_Height.descender);
    FT_Vector ascender = {0, FT_Pos(m_Height.ascender * FONT_FLOAT_TO_F26DOT6)};
    FT_Vector descender = {0, -FT_Pos(m_Height.descender * FONT_FLOAT_TO_F26DOT6)};
    FT_Vector_Transform(&ascender, &m_Transform);
    FT_Vector_Transform(&descender, &m_Transform);
    m_Glyphs.SetAscenderOffset(PPoint(float(ascender.x) * FONT_F26DOT6_TO_FLOAT, -float(ascender.y) * FONT_F26DOT6_TO_FLOAT));
    m_Glyphs.SetDescenderOffset(PPoint(float(descender.x) * FONT_F26DOT6_TO_FLOAT, -float(descender.y) * FONT_F26DOT6_TO_FLOAT));
}

PServerFontInstance::~PServerFontInstance()
{
    FT_Done_Size(m_Size);
}

bool PServerFontInstance::HasCharacter(uint32_t character) const
{
    return FT_Get_Char_Index(m_Face->GetHandle(), character) != 0;
}

PFontTextMetrics PServerFontInstance::MeasureText(std::string_view text, float width, bool includeLast)
{
    uint32_t previousGlyph = 0;
    return MeasureTextChunk(text, width, includeLast, previousGlyph);
}

PFontTextMetrics PServerFontInstance::MeasureTextChunk(
    std::string_view text,
    float width,
    bool includeLast,
    uint32_t& previousGlyph)
{
    return WalkText(text, width, includeLast, nullptr, previousGlyph);
}

size_t PServerFontInstance::GetTextKerningCount(std::string_view text) const noexcept
{
    size_t count = 0;
    if (m_Glyphs.HasKerning())
    {
        while (!text.empty())
        {
            size_t byteLength = 0;
            utf8_to_unicode(text.data(), text.size(), byteLength);
            text.remove_prefix(byteLength);
            ++count;
        }
    }
    return count;
}

PServerPreparedText PServerFontInstance::PrepareText(std::string_view text, int32_t* kernings)
{
    Ptr<PServerFontDrawCache> cache = ptr_tmp_cast(m_ActiveDrawCache);
    if (cache == nullptr)
    {
        cache = ptr_new<PServerFontDrawCache>(ptr_tmp_cast(this), ptr_tmp_cast(&m_Glyphs.GetCache()));
        m_ActiveDrawCache = ptr_raw_pointer_cast(cache);
    }
    uint32_t previousGlyph = 0;
    const PFontTextMetrics metrics = WalkText(
        text,
        std::numeric_limits<float>::infinity(),
        false,
        ptr_raw_pointer_cast(cache),
        previousGlyph,
        kernings);
    return {std::move(cache), metrics};
}

void PServerFontInstance::ReleaseDrawCache(const PServerFontDrawCache& cache) noexcept
{
    assert(m_ActiveDrawCache == &cache);
    m_ActiveDrawCache = nullptr;
}

void PServerFontInstance::Activate()
{
    PServerFontLibrary::CheckError(FT_Activate_Size(m_Size));
    FT_Set_Transform(m_Face->GetHandle(), &m_Transform, nullptr);
}

void PServerFontInstance::InitializeSize()
{
    FT_Face face = m_Face->GetHandle();
    PServerFontLibrary::CheckError(FT_Activate_Size(m_Size));
    if (FT_IS_SCALABLE(face))
    {
        const FT_F26Dot6 pointSize = FT_F26Dot6(std::round(m_Description.PointSize * FONT_FLOAT_TO_F26DOT6));
        PServerFontLibrary::CheckError(FT_Set_Char_Size(face, 0, pointSize, FONT_DPI, FONT_DPI));
    }
    else
    {
        if (face->num_fixed_sizes == 0 || m_Description.Rotation != 0.0f || m_Description.Shear != 90.0f) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        int bestStrike = 0;
        const float pixelSize = m_Description.PointSize * float(FONT_DPI) / 72.0f;
        float bestDistance = std::numeric_limits<float>::infinity();
        for (int i = 0; i < face->num_fixed_sizes; ++i)
        {
            const float distance = std::abs(float(face->available_sizes[i].y_ppem) * FONT_F26DOT6_TO_FLOAT - pixelSize);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestStrike = i;
            }
        }
        PServerFontLibrary::CheckError(FT_Select_Size(face, bestStrike));
    }
}

void PServerFontInstance::InitializeTransform()
{
    const float rotation = m_Description.Rotation * std::numbers::pi_v<float> / 180.0f;
    const float shear = m_Description.Shear * std::numbers::pi_v<float> / 180.0f;
    const float shearFactor = (m_Description.Shear == 90.0f) ? 0.0f : std::cos(shear) / std::sin(shear);
    const float cosine = std::cos(rotation);
    const float sine = std::sin(rotation);
    m_Transform.xx = FT_Fixed(std::round(cosine * 65536.0f));
    m_Transform.xy = FT_Fixed(std::round((cosine * shearFactor - sine) * 65536.0f));
    m_Transform.yx = FT_Fixed(std::round(sine * 65536.0f));
    m_Transform.yy = FT_Fixed(std::round((sine * shearFactor + cosine) * 65536.0f));
    m_Glyphs.SetKerningScale(int32_t(m_Transform.xx), -int32_t(m_Transform.yx));
}

PServerFontGlyph& PServerFontInstance::GetGlyph(FT_UInt glyphIndex)
{
    PServerFontGlyph* cached = m_Glyphs.FindGlyphIndex(glyphIndex);
    if (cached != nullptr) {
        return *cached;
    }
    Activate();
    PServerFontLibrary::CheckError(FT_Load_Glyph(m_Face->GetHandle(), glyphIndex, m_LoadFlags));
    FT_GlyphSlot slot = m_Face->GetHandle()->glyph;
    const bool monochrome = m_Description.RenderMode == PFontRenderMode::Monochrome;
    const FT_Render_Mode mode = monochrome ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL;
    PServerFontLibrary::CheckError(FT_Render_Glyph(slot, mode));
    Ptr<PServerFontGlyph> glyph = PServerFontGlyph::Create(m_Glyphs, slot, monochrome);
    PServerFontGlyph* result = ptr_raw_pointer_cast(glyph);
    m_Glyphs.StoreGlyph(std::move(glyph));
    return *result;
}

FT_Pos PServerFontInstance::QueryKerning(FT_UInt previousGlyph, FT_UInt glyphIndex)
{
    if (previousGlyph == 0 || glyphIndex == 0 || !m_Glyphs.HasKerning()) {
        return 0;
    }
    FT_Face face = m_Face->GetHandle();
    if (face->size != m_Size) {
        PServerFontLibrary::CheckError(FT_Activate_Size(m_Size));
    }
    FT_Vector kerning{};
    const FT_UInt mode = m_Description.Hinting ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED;
    PServerFontLibrary::CheckError(FT_Get_Kerning(face, previousGlyph, glyphIndex, mode, &kerning));
    return kerning.x;
}

PServerFontGlyph& PServerFontInstance::GetCharacter(uint32_t character)
{
    PServerFontGlyph* cached = m_Glyphs.FindCharacter(character);
    if (cached != nullptr) {
        return *cached;
    }
    // Reclaim previous misses before growing the cache. The returned glyph stays
    // owned until WalkText consumes its metrics or pins it for the submission.
    m_Glyphs.GetCache().Trim();
    const FT_UInt glyphIndex = FT_Get_Char_Index(m_Face->GetHandle(), character);
    PServerFontGlyph& glyph = GetGlyph(glyphIndex);
    m_Glyphs.AddCharacter(glyph, character);
    return glyph;
}

PFontTextMetrics PServerFontInstance::WalkText(
    std::string_view text,
    float width,
    bool includeLast,
    PServerFontDrawCache* drawCache,
    uint32_t& previousGlyph,
    int32_t* kernings)
{
    PScopeExit trimCache([this] { m_Glyphs.GetCache().Trim(); });
    PFontTextMetrics result;
    if (std::isnan(width) || width < 0.0f) {
        return result;
    }
    while (result.ByteCount < text.size())
    {
        size_t byteLength = 0;
        const uint32_t character = utf8_to_unicode(text.data() + result.ByteCount, text.size() - result.ByteCount, byteLength);
        PServerFontGlyph& entry = GetCharacter(character);
        const FT_Pos adjustment = QueryKerning(previousGlyph, entry.GlyphIndex);
        const float kerningWidth = float(adjustment) * FONT_F26DOT6_TO_FLOAT;
        const PPoint kerningAdvance = m_Glyphs.TransformKerning(int32_t(adjustment));
        if (kernings != nullptr) {
            *kernings++ = int32_t(adjustment);
        }
        const float nextWidth = result.Width + kerningWidth + entry.Width;
        const bool exceedsWidth = nextWidth > width;
        if (exceedsWidth && !includeLast) {
            break;
        }
        result.Advance += kerningAdvance;
        if (drawCache != nullptr) {
            drawCache->PinGlyph(entry);
        } else {
            m_Glyphs.GetCache().TouchGlyph(entry);
        }
        result.Advance += entry.Advance;
        result.Width = nextWidth;
        result.ByteCount += byteLength;
        previousGlyph = entry.GlyphIndex;
        if (exceedsWidth) {
            break;
        }
    }
    return result;
}
