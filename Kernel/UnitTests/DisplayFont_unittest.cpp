// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cmath>
#include <system_error>
#include <gtest/gtest.h>
#include <Kernel/Drivers/Video/KDisplayDriver.h>

namespace kernel
{

// Synthetic fonts expose storage for the renderer tests only.
class DisplayFontTestCache : public PDisplayFontCache
{
public:
    using PDisplayFontCache::m_AsciiGlyphs;
    using PDisplayFontCache::m_MissingGlyph;
    using PDisplayFontCache::m_Characters;
    using PDisplayFontCache::m_CharacterBucketCount;
    using PDisplayFontCache::m_AscenderOffset;
    using PDisplayFontCache::m_DescenderOffset;
    using PDisplayFontCache::m_KerningScaleX;
    using PDisplayFontCache::m_KerningScaleY;
    using PDisplayFontCache::m_Direction;
    using PDisplayFontCache::m_HasKerning;
};

class DisplayFontTextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Raster.fill(128);
        for (size_t i = 0; i < m_Glyphs.size(); ++i)
        {
            PDisplayFontGlyph& glyph = m_Glyphs[i];
            glyph.Bounds = PIRect(0, 0, 6, 6);
            glyph.Advance = PPoint(4.0f, 0.0f);
            glyph.GlyphIndex = uint32_t(i) + 1;
            glyph.Raster = m_Raster.data();
            m_Font.m_AsciiGlyphs[size_t('A') + i] = &glyph;
        }
    }

    void CaptureSpans()
    {
        m_Coverage.fill(0);
        m_WriteCounts.fill(0);
        while (m_Text.NextSpan())
        {
            const KDisplayTextSpan& span = m_Text.GetSpan();
            ASSERT_TRUE(span.Bounds.IsValid());
            const PIRect bounds = span.Bounds & PIRect(0, 0, 32, 32);
            for (int y = bounds.top; y < bounds.bottom; ++y)
            {
                const KDisplayTextGlyph* singleGlyph = span.SingleGlyph;
                const uint8_t* source = nullptr;
                if (singleGlyph != nullptr)
                {
                    ASSERT_TRUE(span.HasGlyphs);
                    ASSERT_EQ(span.Bounds & singleGlyph->Bounds, span.Bounds);
                    const size_t offset = size_t(y - singleGlyph->Bounds.top) * size_t(singleGlyph->Bounds.Width())
                        + size_t(bounds.left - singleGlyph->Bounds.left);
                    source = singleGlyph->Glyph->Raster + offset;
                }
                const std::span<const uint8_t*> sources = (span.HasGlyphs && singleGlyph == nullptr)
                    ? m_Text.GetRowSources(bounds.left, y) : std::span<const uint8_t*>();
                for (int x = bounds.left; x < bounds.right; ++x)
                {
                    const size_t offset = size_t(y) * 32 + size_t(x);
                    ASSERT_EQ(m_WriteCounts[offset], 0) << x << ',' << y;
                    ++m_WriteCounts[offset];
                    m_Coverage[offset] = (source != nullptr) ? *source++ : KDisplayText::ReadCoverage(sources);
                }
            }
        }
    }

    uint8_t Coverage(int x, int y) const { return m_Coverage[size_t(y) * 32 + size_t(x)]; }

    void CheckTextArea(const PIRect& bitmapBounds, PIPoint position, PPoint advance, PDrawingMode mode)
    {
        const PPoint origin = PPoint(position) + m_Font.GetAscenderOffset();
        const PPoint height = m_Font.GetDescenderOffset() - m_Font.GetAscenderOffset();
        const float determinant = advance.x * height.y - advance.y * height.x;
        for (int y = 0; y < 32; ++y)
        {
            for (int x = 0; x < 32; ++x)
            {
                bool insideGlyph = false;
                uint32_t expectedCoverage = 0;
                for (const KDisplayTextGlyph& glyph : m_Text.GetGlyphs())
                {
                    if (glyph.Bounds.DoIntersect(PIPoint(x, y)))
                    {
                        insideGlyph = true;
                        const size_t offset = size_t(y - glyph.Bounds.top) * size_t(glyph.Bounds.Width())
                            + size_t(x - glyph.Bounds.left);
                        const uint32_t alpha = glyph.Glyph->Raster[offset];
                        expectedCoverage += alpha - (expectedCoverage * alpha + 127) / 255;
                    }
                }
                bool insideLogicalArea = false;
                bool onBoundary = false;
                if (mode == PDrawingMode::Copy && determinant != 0.0f)
                {
                    // Independent area test in the font's two basis vectors, rather than sweep intersections.
                    const PPoint delta = PPoint(float(x) + 0.5f, float(y) + 0.5f) - origin;
                    const float along = (delta.x * height.y - delta.y * height.x) / determinant;
                    const float across = (advance.x * delta.y - advance.y * delta.x) / determinant;
                    insideLogicalArea = along > 0.0f && along < 1.0f && across > 0.0f && across < 1.0f;
                    onBoundary = std::abs(along) < 1.0e-5f || std::abs(along - 1.0f) < 1.0e-5f
                        || std::abs(across) < 1.0e-5f || std::abs(across - 1.0f) < 1.0e-5f;
                }
                const size_t offset = size_t(y) * 32 + size_t(x);
                const bool visible = bitmapBounds.DoIntersect(PIPoint(x, y));
                if (!visible || insideGlyph || !onBoundary)
                {
                    const uint8_t expectedWrites = (visible && (insideGlyph || insideLogicalArea)) ? 1 : 0;
                    EXPECT_EQ(m_WriteCounts[offset], expectedWrites) << x << ',' << y;
                }
                EXPECT_EQ(m_Coverage[offset], visible ? expectedCoverage : 0) << x << ',' << y;
            }
        }
    }

    std::array<uint8_t, 32 * 32> m_Coverage;
    std::array<uint8_t, 32 * 32> m_WriteCounts;
    std::array<uint8_t, 36> m_Raster;
    std::array<PDisplayFontGlyph, 3> m_Glyphs;
    PDisplayFontGlyph m_MissingGlyph;
    DisplayFontTestCache m_Font;
    KDisplayText m_Text;
};

TEST_F(DisplayFontTextTest, SpacesAdvanceWithoutEmittingDrawableGlyphs)
{
    PDisplayFontGlyph space;
    space.GlyphIndex = 4;
    space.Advance = PPoint(2.0f, 0.0f);
    m_Font.m_AsciiGlyphs[' '] = &space;
    m_Text.Begin(m_Font, "A A", PIPoint(0, 0), PIRect(0, 0, 100, 100));
    ASSERT_EQ(m_Text.GetGlyphs().size(), 2);
    EXPECT_EQ(m_Text.GetGlyphs()[0].Bounds.left, 0);
    EXPECT_EQ(m_Text.GetGlyphs()[1].Bounds.left, 6);
}

TEST_F(DisplayFontTextTest, Utf8CharactersUseInlineKerning)
{
    std::array<const PDisplayFontGlyph*, 4> characters{};
    m_Glyphs[1].Characters.Inline[0] = {0x03b2, nullptr};
    m_Glyphs[1].CharacterCount = 1;
    characters[PDisplayFontCache::GetBucketIndex(0x03b2, characters.size())] = &m_Glyphs[1];
    const int32_t kernings[] = {0, -64};
    m_Font.m_Characters = characters.data();
    m_Font.m_CharacterBucketCount = characters.size();
    m_Font.m_HasKerning = true;
    m_Text.Begin(m_Font, "A\xce\xb2", PIPoint(0, 0), PIRect(0, 0, 100, 100), kernings);
    ASSERT_EQ(m_Text.GetGlyphs().size(), 2);
    EXPECT_EQ(m_Text.GetGlyphs()[1].Bounds.left, 3);
    CaptureSpans();
    EXPECT_EQ(Coverage(3, 0), 192);
}

TEST_F(DisplayFontTextTest, CombinesAllEarlierOverlappingGlyphs)
{
    for (PDisplayFontGlyph& glyph : m_Glyphs) {
        glyph.Advance = PPoint(1.0f, 0.0f);
    }
    m_Text.Begin(m_Font, "ABC", PIPoint(0, 0), PIRect(0, 0, 100, 100));
    ASSERT_EQ(m_Text.GetGlyphs().size(), 3);
    CaptureSpans();
    EXPECT_EQ(Coverage(2, 0), 224);
    EXPECT_EQ(Coverage(6, 0), 192);
    EXPECT_EQ(Coverage(7, 0), 128);
}

TEST_F(DisplayFontTextTest, RetainsAnOverhangAcrossAnInterveningDisjointGlyph)
{
    m_Glyphs[0].Bounds = PIRect(0, 0, 12, 1);
    m_Glyphs[1].Bounds = PIRect(0, 3, 1, 4);
    m_Text.Begin(m_Font, "ABC", PIPoint(0, 0), PIRect(0, 0, 100, 100));
    ASSERT_EQ(m_Text.GetGlyphs().size(), 3);
    CaptureSpans();
    EXPECT_EQ(Coverage(8, 0), 192);
}

TEST_F(DisplayFontTextTest, FindsOverlapsAlongEachBaselineDirection)
{
    const PDisplayFontDirection directions[] =
    {
        PDisplayFontDirection::Right, PDisplayFontDirection::Down, PDisplayFontDirection::Left, PDisplayFontDirection::Up
    };
    const PPoint advances[] = {PPoint(4.0f, 0.0f), PPoint(0.0f, 4.0f), PPoint(-4.0f, 0.0f), PPoint(0.0f, -4.0f)};
    for (size_t i = 0; i < std::size(directions); ++i)
    {
        m_Font.m_Direction = directions[i];
        m_Glyphs[0].Advance = advances[i];
        m_Text.Begin(m_Font, "AB", PIPoint(20, 20), PIRect(0, 0, 100, 100));
        ASSERT_EQ(m_Text.GetGlyphs().size(), 2);
        const PIRect overlap = m_Text.GetGlyphs()[0].Bounds & m_Text.GetGlyphs()[1].Bounds;
        ASSERT_TRUE(overlap.IsValid());
        CaptureSpans();
        EXPECT_EQ(Coverage(overlap.left, overlap.top), 192);
    }
}

TEST_F(DisplayFontTextTest, MissingGlyphFallbackPreservesAdvanceAndCoverage)
{
    m_MissingGlyph.Bounds = PIRect(0, 0, 2, 2);
    m_MissingGlyph.Advance = PPoint(3.0f, 0.0f);
    m_MissingGlyph.Raster = m_Raster.data();
    m_Font.m_MissingGlyph = &m_MissingGlyph;
    const int32_t kernings[] = {0, 0, 0, 0};
    for (bool hasKerning : {false, true})
    {
        m_Font.m_HasKerning = hasKerning;
        const std::span<const int32_t> adjustments = hasKerning ? kernings : std::span<const int32_t>();
        m_Text.Begin(m_Font, "A\xef\xb7\x90\x7f" "B", PIPoint(0, 0), PIRect(0, 0, 32, 32), adjustments);
        ASSERT_EQ(m_Text.GetGlyphs().size(), 4);
        EXPECT_EQ(m_Text.GetGlyphs()[0].Glyph, &m_Glyphs[0]);
        EXPECT_EQ(m_Text.GetGlyphs()[1].Glyph, &m_MissingGlyph);
        EXPECT_EQ(m_Text.GetGlyphs()[2].Glyph, &m_MissingGlyph);
        EXPECT_EQ(m_Text.GetGlyphs()[3].Glyph, &m_Glyphs[1]);
        EXPECT_EQ(m_Text.GetGlyphs()[1].Bounds.left, 4);
        EXPECT_EQ(m_Text.GetGlyphs()[2].Bounds.left, 7);
        EXPECT_EQ(m_Text.GetGlyphs()[3].Bounds.left, 10);
        CaptureSpans();
        EXPECT_EQ(Coverage(4, 0), 192);
        EXPECT_EQ(Coverage(7, 0), 128);
        EXPECT_EQ(Coverage(10, 0), 128);
    }
}

TEST_F(DisplayFontTextTest, RejectsMissingGlyphs)
{
    EXPECT_THROW(m_Text.Begin(m_Font, "D", PIPoint(0, 0), PIRect(0, 0, 100, 100)), std::system_error);
    EXPECT_TRUE(m_Text.GetGlyphs().empty());
    EXPECT_FALSE(m_Text.NextSpan());
}

TEST_F(DisplayFontTextTest, InlineKerningsFollowDecodedCharactersIncludingSpaces)
{
    PDisplayFontGlyph space;
    space.GlyphIndex = 4;
    space.Advance = PPoint(2.0f, 0.0f);
    m_Font.m_AsciiGlyphs[' '] = &space;
    std::array<const PDisplayFontGlyph*, 4> characters{};
    m_Glyphs[1].Characters.Inline[0] = {0x03b2, nullptr};
    m_Glyphs[1].CharacterCount = 1;
    characters[PDisplayFontCache::GetBucketIndex(0x03b2, characters.size())] = &m_Glyphs[1];
    m_Font.m_Characters = characters.data();
    m_Font.m_CharacterBucketCount = characters.size();
    m_Font.m_HasKerning = true;
    const int32_t kernings[] = {0, 0, -64, 128};
    m_Text.Begin(m_Font, "A \xce\xb2" "A", PIPoint(0, 0), PIRect(0, 0, 100, 100), kernings);
    ASSERT_EQ(m_Text.GetGlyphs().size(), 3);
    EXPECT_EQ(m_Text.GetGlyphs()[0].Bounds.left, 0);
    EXPECT_EQ(m_Text.GetGlyphs()[1].Bounds.left, 5);
    EXPECT_EQ(m_Text.GetGlyphs()[2].Bounds.left, 11);
}

TEST_F(DisplayFontTextTest, InlineKerningsRotateWithFreeTypeRounding)
{
    m_Font.m_KerningScaleX = 56756;
    m_Font.m_KerningScaleY = -32768;
    m_Font.m_HasKerning = true;
    for (PDisplayFontGlyph& glyph : m_Glyphs) {
        glyph.Advance = PPoint(3.46875f, -2.0f);
    }
    const std::array<PIRect, 3> expected = {PIRect(20, 20, 26, 26), PIRect(23, 18, 29, 24), PIRect(25, 17, 31, 23)};
    const int32_t kernings[] = {0, -1, -128};
    m_Text.Begin(m_Font, "ABC", PIPoint(20, 20), PIRect(0, 0, 100, 100), kernings);
    ASSERT_EQ(m_Text.GetGlyphs().size(), expected.size());
    for (size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(m_Text.GetGlyphs()[index].Bounds, expected[index]);
    }
    // A 1/64-pixel adjustment at 30 degrees rounds to one fixed-point unit on each axis.
    EXPECT_EQ(m_Font.TransformKerning(-1), PPoint(-FONT_F26DOT6_TO_FLOAT, FONT_F26DOT6_TO_FLOAT));
    EXPECT_EQ(m_Font.TransformKerning(1), PPoint(FONT_F26DOT6_TO_FLOAT, -FONT_F26DOT6_TO_FLOAT));
    EXPECT_EQ(m_Font.TransformKerning(-128), PPoint(-1.734375f, 1.0f));
}

TEST_F(DisplayFontTextTest, RejectsInlineKerningCountMismatch)
{
    m_Font.m_HasKerning = true;
    EXPECT_THROW(m_Text.Begin(m_Font, "AB", PIPoint(0, 0), PIRect(0, 0, 100, 100)), std::system_error);
    const int32_t shortKernings[] = {0};
    EXPECT_THROW(m_Text.Begin(m_Font, "AB", PIPoint(0, 0), PIRect(0, 0, 100, 100), shortKernings), std::system_error);
    const int32_t longKernings[] = {0, 0};
    EXPECT_THROW(m_Text.Begin(m_Font, "A", PIPoint(0, 0), PIRect(0, 0, 100, 100), longKernings), std::system_error);
}

TEST_F(DisplayFontTextTest, SpansCoverEachRequiredPixelOnceWithFinalCoverage)
{
    struct Layout
    {
        PDisplayFontDirection Direction;
        PPoint Advance;
        PPoint Ascender;
        PPoint Descender;
    };
    const Layout layouts[] =
    {
        {PDisplayFontDirection::Right, PPoint(4, 0), PPoint(0, -3), PPoint(0, 2)},
        {PDisplayFontDirection::Left, PPoint(-4, 0), PPoint(0, 3), PPoint(0, -2)},
        {PDisplayFontDirection::Down, PPoint(0, 4), PPoint(3, 0), PPoint(-2, 0)},
        {PDisplayFontDirection::Up, PPoint(0, -4), PPoint(-3, 0), PPoint(2, 0)},
        {PDisplayFontDirection::Right, PPoint(4, 2), PPoint(1, -2), PPoint(-1, 2)},
        {PDisplayFontDirection::Down, PPoint(2, 4), PPoint(2, -1), PPoint(-2, 1)},
        {PDisplayFontDirection::Left, PPoint(-4, 2), PPoint(1, 2), PPoint(-1, -2)},
        {PDisplayFontDirection::Up, PPoint(2, -4), PPoint(-2, -1), PPoint(2, 1)},
        {PDisplayFontDirection::Right, PPoint(4, 0), PPoint(-2, -3), PPoint(2, 3)}
    };
    m_Glyphs[0].Bounds = PIRect(-2, -1, 4, 5);
    m_Glyphs[1].Bounds = PIRect(1, -3, 7, 3);
    m_Glyphs[2].Bounds = PIRect(-5, -2, 1, 4);
    const uint8_t coverages[] = {0, 64, 128, 255};
    for (size_t index = 0; index < m_Raster.size(); ++index) {
        m_Raster[index] = coverages[index % std::size(coverages)];
    }
    const PDrawingMode modes[] = {PDrawingMode::Copy, PDrawingMode::Overlay};
    const PIRect bitmapBounds[] = {PIRect(0, 0, 32, 32), PIRect(10, 9, 20, 19)};
    for (const Layout& layout : layouts)
    {
        m_Font.m_Direction = layout.Direction;
        m_Font.m_AscenderOffset = layout.Ascender;
        m_Font.m_DescenderOffset = layout.Descender;
        for (PDisplayFontGlyph& glyph : m_Glyphs) {
            glyph.Advance = layout.Advance;
        }
        for (PDrawingMode mode : modes)
        {
            for (const PIRect& bounds : bitmapBounds)
            {
                const PIPoint position(12, 12);
                const PPoint advance = layout.Advance * 3.0f;
                m_Text.Begin(m_Font, "ABC", position, bounds, {}, advance, mode);
                CaptureSpans();
                CheckTextArea(bounds, position, advance, mode);
            }
        }
    }
}

class DisplayFontRasterDriver : public KDisplayDriver
{
public:
    void WaitIdle() override {}
    bool Open() override { return true; }
    void Close() override {}
    void PowerLost(bool hasPower) override {}
    PDisplayBitmap* GetScreenBitmap() override { return nullptr; }
    int GetScreenModeCount() override { return 0; }
    bool GetScreenModeDesc(size_t index, PScreenMode& outMode) override { return false; }
    bool SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate) override { return false; }
    PIPoint GetResolution() override { return PIPoint(); }
    int GetBytesPerLine() override { return 0; }
    PEColorSpace GetColorSpace() override { return PEColorSpace::RGB32; }
    void SetColor(size_t index, PColor color) override {}
    bool SetMouseCursorBitmap(const PMouseCursorBitmap& cursor) override { return false; }
    void SetMouseCursorVisible(bool visible) override {}
    void SetMousePos(PIPoint position) override {}

    void Render(
        PDisplayBitmap& bitmap,
        const PIPoint& position,
        std::string_view text,
        const PDisplayFontCache& font,
        PPoint advance,
        std::span<const PIRect> clips,
        PColor background,
        PColor foreground,
        PDrawingMode mode)
    {
        CRITICAL_SCOPE(m_Mutex);
        WriteFontString(&bitmap, position, text, font, {}, clips, background, foreground, advance, mode);
    }

    void CheckPaletteEndpoints(PColor background, PColor foreground)
    {
        CRITICAL_SCOPE(m_Mutex);
        const std::array<uint32_t, 256>& palette = PrepareFontPalette(PEColorSpace::RGB16, background, foreground);
        EXPECT_EQ(palette[0], background.GetColor16());
        EXPECT_EQ(palette[255], foreground.GetColor16());
        const uint32_t lowCoverage = palette[1];
        const uint32_t halfCoverage = palette[128];
        constexpr uint32_t COLOR_KEYS[] = {0x1234, 0};
        for (uint32_t colorKey : COLOR_KEYS)
        {
            const std::array<uint32_t, 256>& keyed = PrepareFontPalette(PEColorSpace::RGB16, background, foreground, colorKey);
            EXPECT_EQ(keyed[0], colorKey);
            EXPECT_EQ(keyed[1], lowCoverage);
            EXPECT_EQ(keyed[128], halfCoverage);
            EXPECT_EQ(keyed[255], foreground.GetColor16());
        }
        const std::array<uint32_t, 256>& restored = PrepareFontPalette(PEColorSpace::RGB16, background, foreground);
        EXPECT_EQ(restored[0], background.GetColor16());
        EXPECT_EQ(restored[1], lowCoverage);
        EXPECT_EQ(restored[128], halfCoverage);
        EXPECT_EQ(restored[255], foreground.GetColor16());
    }
};

class DisplayFontRasterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Driver = ptr_new<DisplayFontRasterDriver>();
        m_Pixels.fill(PColor(16, 32, 48).GetColor32());
        m_Bitmap.ColorSpace = PEColorSpace::RGBA32;
        m_Bitmap.Size = PIPoint(16, 12);
        m_Bitmap.BytesPerLine = 16 * sizeof(uint32_t);
        m_Bitmap.Raster = reinterpret_cast<uint8_t*>(m_Pixels.data());
        m_Glyph.Bounds = PIRect(-1, -2, 1, 0);
        m_Glyph.Advance = PPoint(4.0f, 0.0f);
        m_Glyph.GlyphIndex = 1;
        m_Glyph.Raster = m_Raster.data();
        m_Space.Advance = PPoint(2.0f, 0.0f);
        m_Space.GlyphIndex = 2;
        m_Font.m_AsciiGlyphs['A'] = &m_Glyph;
        m_Font.m_AsciiGlyphs[' '] = &m_Space;
        m_Font.m_AscenderOffset = PPoint(0.0f, -3.0f);
        m_Font.m_DescenderOffset = PPoint(0.0f, 1.0f);
    }

    void Draw(std::string_view text, PPoint advance, PDrawingMode mode)
    {
        m_Driver->Render(m_Bitmap, m_Position, text, m_Font, advance, m_Clips, m_Background, m_Foreground, mode);
    }

    uint32_t Pixel(size_t x, size_t y) const { return m_Pixels[y * 16 + x]; }

    Ptr<DisplayFontRasterDriver> m_Driver;
    std::array<uint32_t, 16 * 12> m_Pixels;
    std::array<uint8_t, 4> m_Raster = {0, 128, 255, 0};
    std::array<PIRect, 1> m_Clips = {PIRect(0, 0, 16, 12)};
    PDisplayBitmap m_Bitmap;
    DisplayFontTestCache m_Font;
    PDisplayFontGlyph m_Glyph;
    PDisplayFontGlyph m_Space;
    PIPoint m_Position = PIPoint(3, 4);
    PColor m_Background = PColor(0, 0, 0);
    PColor m_Foreground = PColor(255, 255, 255);
};

TEST_F(DisplayFontRasterTest, CopyCoversLogicalAreaSpacesAndOverhangs)
{
    Draw("A ", PPoint(6.0f, 0.0f), PDrawingMode::Copy);
    for (size_t y = 0; y < 12; ++y)
    {
        for (size_t x = 0; x < 16; ++x)
        {
            uint32_t expected = PColor(16, 32, 48).GetColor32();
            if ((x >= 3 && x < 9 && y >= 1 && y < 5) || (x == 2 && y >= 2 && y < 4)) {
                expected = m_Background.GetColor32();
            }
            if (x == 3 && y == 2) {
                expected = PColor(128, 128, 128).GetColor32();
            }
            if (x == 2 && y == 3) {
                expected = m_Foreground.GetColor32();
            }
            EXPECT_EQ(Pixel(x, y), expected) << x << ',' << y;
        }
    }
}

TEST_F(DisplayFontRasterTest, CopyClearsSpaceOnlyTextAndHonoursClipping)
{
    m_Clips[0] = PIRect(4, 2, 6, 4);
    Draw("  ", PPoint(4.0f, 0.0f), PDrawingMode::Copy);
    for (size_t y = 0; y < 12; ++y)
    {
        for (size_t x = 0; x < 16; ++x)
        {
            const uint32_t expected = (x >= 4 && x < 6 && y >= 2 && y < 4)
                ? m_Background.GetColor32() : PColor(16, 32, 48).GetColor32();
            EXPECT_EQ(Pixel(x, y), expected) << x << ',' << y;
        }
    }
}

TEST_F(DisplayFontRasterTest, OverlayPreservesTransparentPixelsAndCharacterGaps)
{
    Draw(" A ", PPoint(8.0f, 0.0f), PDrawingMode::Overlay);
    for (size_t y = 0; y < 12; ++y)
    {
        for (size_t x = 0; x < 16; ++x)
        {
            uint32_t expected = PColor(16, 32, 48).GetColor32();
            if (x == 5 && y == 2) {
                expected = PColor(128, 128, 128).GetColor32();
            }
            if (x == 4 && y == 3) {
                expected = m_Foreground.GetColor32();
            }
            EXPECT_EQ(Pixel(x, y), expected) << x << ',' << y;
        }
    }
}

TEST_F(DisplayFontRasterTest, ClippedSingleGlyphUsesTheOriginalRasterStride)
{
    const std::array<uint8_t, 12> raster = {2, 3, 4, 5, 6, 0, 128, 9, 10, 255, 64, 13};
    m_Glyph.Bounds = PIRect(-1, -2, 3, 1);
    m_Glyph.Raster = raster.data();
    m_Clips[0] = PIRect(3, 3, 5, 5);
    const PDrawingMode modes[] = {PDrawingMode::Copy, PDrawingMode::Overlay};
    for (PDrawingMode mode : modes)
    {
        const uint32_t untouched = PColor(16, 32, 48).GetColor32();
        m_Pixels.fill(untouched);
        Draw("A", PPoint(4.0f, 0.0f), mode);
        for (size_t y = 0; y < 12; ++y)
        {
            for (size_t x = 0; x < 16; ++x)
            {
                uint32_t expected = untouched;
                if (x == 3 && y == 3 && mode == PDrawingMode::Copy) {
                    expected = m_Background.GetColor32();
                }
                if (x == 4 && y == 3) {
                    expected = PColor(128, 128, 128).GetColor32();
                }
                if (x == 3 && y == 4) {
                    expected = m_Foreground.GetColor32();
                }
                if (x == 4 && y == 4) {
                    expected = PColor(64, 64, 64).GetColor32();
                }
                EXPECT_EQ(Pixel(x, y), expected) << x << ',' << y;
            }
        }
    }
}

TEST_F(DisplayFontRasterTest, FullyTransparentForegroundLeavesOverlayUntouched)
{
    m_Foreground = PColor(255, 255, 255, 0);
    Draw("A", PPoint(4.0f, 0.0f), PDrawingMode::Overlay);
    for (uint32_t pixel : m_Pixels) {
        EXPECT_EQ(pixel, PColor(16, 32, 48).GetColor32());
    }
    Draw("A", PPoint(4.0f, 0.0f), PDrawingMode::Copy);
    EXPECT_EQ(Pixel(2, 3), m_Background.GetColor32());
    EXPECT_EQ(Pixel(6, 4), m_Background.GetColor32());
}

TEST_F(DisplayFontRasterTest, CopyFollowsShearInsteadOfClearingTheBoundingBox)
{
    m_Font.m_AscenderOffset = PPoint(0.0f, -2.0f);
    m_Font.m_DescenderOffset = PPoint(2.0f, 2.0f);
    Draw("  ", PPoint(4.0f, 0.0f), PDrawingMode::Copy);
    EXPECT_EQ(Pixel(3, 2), m_Background.GetColor32());
    EXPECT_EQ(Pixel(8, 5), m_Background.GetColor32());
    EXPECT_EQ(Pixel(3, 4), PColor(16, 32, 48).GetColor32());
    EXPECT_EQ(Pixel(8, 2), PColor(16, 32, 48).GetColor32());
}

TEST_F(DisplayFontRasterTest, CopyFollowsRotatedBaseline)
{
    m_Position = PIPoint(6, 5);
    m_Font.m_AscenderOffset = PPoint(2.0f, -2.0f);
    m_Font.m_DescenderOffset = PPoint(-2.0f, 2.0f);
    m_Space.Advance = PPoint(4.0f, 4.0f);
    Draw(" ", PPoint(4.0f, 4.0f), PDrawingMode::Copy);
    EXPECT_EQ(Pixel(8, 6), m_Background.GetColor32());
    EXPECT_EQ(Pixel(6, 5), m_Background.GetColor32());
    EXPECT_EQ(Pixel(4, 3), PColor(16, 32, 48).GetColor32());
    EXPECT_EQ(Pixel(11, 10), PColor(16, 32, 48).GetColor32());
}

TEST_F(DisplayFontRasterTest, CopyRetainsCoverageFromOverlappingGlyphs)
{
    m_Glyph.Advance = PPoint(0.0f, 0.0f);
    Draw("AA", PPoint(), PDrawingMode::Copy);
    EXPECT_EQ(Pixel(3, 2), PColor(192, 192, 192).GetColor32());
    EXPECT_EQ(Pixel(2, 3), m_Foreground.GetColor32());
}

TEST_F(DisplayFontRasterTest, Rgb16CopyAndOverlayUseTheSamePaletteColours)
{
    m_Driver->CheckPaletteEndpoints(PColor(0, 0, 0), PColor(255, 255, 255));
    m_Driver->CheckPaletteEndpoints(PColor(0, 0, 0), PColor::FromRGB16(PTransparentColors::RGB16));
    m_Driver->CheckPaletteEndpoints(PColor(0, 0, 0), PColor(0, 0, 0));
    m_Driver->CheckPaletteEndpoints(PColor(32, 64, 96), PColor(192, 224, 255));
    std::array<uint16_t, 16 * 12> pixels;
    const uint16_t untouched = PColor(16, 32, 48).GetColor16();
    pixels.fill(untouched);
    m_Bitmap.ColorSpace = PEColorSpace::RGB16;
    m_Bitmap.BytesPerLine = 16 * sizeof(uint16_t);
    m_Bitmap.Raster = reinterpret_cast<uint8_t*>(pixels.data());
    Draw("A", PPoint(4.0f, 0.0f), PDrawingMode::Overlay);
    EXPECT_EQ(pixels[2 * 16 + 2], untouched);
    EXPECT_EQ(pixels[3 * 16 + 2], m_Foreground.GetColor16());
    Draw("A", PPoint(4.0f, 0.0f), PDrawingMode::Copy);
    EXPECT_EQ(pixels[2 * 16 + 2], m_Background.GetColor16());
    EXPECT_EQ(pixels[2 * 16 + 3], PColor(128, 128, 128).GetColor16());
    EXPECT_EQ(pixels[3 * 16 + 2], m_Foreground.GetColor16());
    pixels.fill(untouched);
    Draw("A", PPoint(4.0f, 0.0f), PDrawingMode::Overlay);
    EXPECT_EQ(pixels[2 * 16 + 2], untouched);
    EXPECT_EQ(pixels[2 * 16 + 3], PColor(128, 128, 128).GetColor16());
    EXPECT_EQ(pixels[3 * 16 + 2], m_Foreground.GetColor16());
}

} // namespace kernel
