// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <cmath>
#include <limits>
#include <system_error>
#include <gtest/gtest.h>
#include <Kernel/Drivers/Video/KDisplayText.h>

namespace kernel
{

class DisplayTextWidthCache : public PDisplayFontCache
{
public:
    void SetGlyph(uint32_t character, const PDisplayFontGlyph* glyph) { m_AsciiGlyphs[character] = glyph; }
    void SetTransform(PPoint baseline, PPoint height, PDisplayFontDirection direction)
    {
        m_KerningScaleX = int32_t(std::round(baseline.x * 65536.0f));
        m_KerningScaleY = int32_t(std::round(baseline.y * 65536.0f));
        m_AscenderOffset = height * -0.5f;
        m_DescenderOffset = height * 0.5f;
        m_Direction = direction;
    }
};

class DisplayTextWidthTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Raster.fill(128);
        m_Glyph.Bounds = PIRect(-1, -3, 5, 1);
        m_Glyph.Advance = PPoint(4.0f, 0.0f);
        m_Glyph.Raster = m_Raster.data();
        m_Space.Advance = PPoint(2.0f, 0.0f);
        m_Font.SetGlyph('A', &m_Glyph);
        m_Font.SetGlyph(' ', &m_Space);
        SetTransform(PPoint(1.0f, 0.0f), PPoint(0.0f, 6.0f), PDisplayFontDirection::Right);
    }

    void SetTransform(PPoint baseline, PPoint height, PDisplayFontDirection direction)
    {
        m_Font.SetTransform(baseline, height, direction);
        m_Baseline = PPoint(std::round(baseline.x * 65536.0f), std::round(baseline.y * 65536.0f)) * (1.0f / 65536.0f);
        m_Height = height;
        m_Glyph.Advance = m_Baseline * 4.0f;
        m_Space.Advance = m_Baseline * 2.0f;
    }

    void Check(std::string_view text, float width, PDrawingMode mode)
    {
        m_Writes.fill(0);
        m_Coverage.fill(0);
        m_Text.Begin(m_Font, text, m_Position, m_BitmapBounds, {}, PPoint(), mode, width);
        while (m_Text.NextSpan())
        {
            const KDisplayTextSpan& span = m_Text.GetSpan();
            ASSERT_EQ(span.Bounds & m_BitmapBounds, span.Bounds);
            for (int y = span.Bounds.top; y < span.Bounds.bottom; ++y)
            {
                const std::span<const uint8_t*> sources = span.HasGlyphs
                    ? m_Text.GetRowSources(span.Bounds.left, y) : std::span<const uint8_t*>();
                for (int x = span.Bounds.left; x < span.Bounds.right; ++x)
                {
                    const size_t index = size_t(y) * 32 + size_t(x);
                    ASSERT_EQ(m_Writes[index], 0) << x << ',' << y;
                    ++m_Writes[index];
                    m_Coverage[index] = KDisplayText::ReadCoverage(sources);
                }
            }
        }
        const float determinant = m_Baseline.x * m_Height.y - m_Baseline.y * m_Height.x;
        for (int y = 0; y < 32; ++y)
        {
            for (int x = 0; x < 32; ++x)
            {
                const size_t index = size_t(y) * 32 + size_t(x);
                const PPoint sample(float(x) + 0.5f - float(m_Position.x), float(y) + 0.5f - float(m_Position.y));
                const float along = (sample.x * m_Height.y - sample.y * m_Height.x) / determinant;
                const float across = (m_Baseline.x * sample.y - m_Baseline.y * sample.x) / determinant;
                if (std::abs(along - width) < 1.0e-4f || std::abs(along) < 1.0e-4f
                    || std::abs(std::abs(across) - 0.5f) < 1.0e-4f) {
                    continue;
                }
                bool hasGlyph = false;
                uint32_t coverage = 0;
                for (const KDisplayTextGlyph& glyph : m_Text.GetGlyphs())
                {
                    if (glyph.Bounds.DoIntersect(PIPoint(x, y)))
                    {
                        hasGlyph = true;
                        coverage += 128 - (coverage * 128 + 127) / 255;
                    }
                }
                const bool background = mode == PDrawingMode::Copy && along > 0.0f && std::abs(across) < 0.5f;
                const bool visible = width > 0.0f && along < width && m_BitmapBounds.DoIntersect(PIPoint(x, y));
                EXPECT_EQ(m_Writes[index], (visible && (hasGlyph || background)) ? 1 : 0) << x << ',' << y;
                EXPECT_EQ(m_Coverage[index], visible ? coverage : 0) << x << ',' << y;
            }
        }
    }

    std::array<uint8_t, 32 * 32> m_Writes{};
    std::array<uint8_t, 32 * 32> m_Coverage{};
    std::array<uint8_t, 24> m_Raster{};
    PDisplayFontGlyph m_Glyph;
    PDisplayFontGlyph m_Space;
    DisplayTextWidthCache m_Font;
    KDisplayText m_Text;
    PPoint m_Baseline;
    PPoint m_Height;
    PIPoint m_Position = PIPoint(12, 12);
    PIRect m_BitmapBounds = PIRect(0, 0, 32, 32);
};

TEST_F(DisplayTextWidthTest, CopyClipsOverlapsAndPadsWithOneWritePerPixel)
{
    Check("AA", 5.0f, PDrawingMode::Copy);
    Check("A ", 12.0f, PDrawingMode::Copy);
    Check("A", 4.25f, PDrawingMode::Copy);
    Check("A", 4.75f, PDrawingMode::Copy);
}

TEST_F(DisplayTextWidthTest, OverlayClipsWithoutAddingBackgroundSpans)
{
    Check("AA", 5.0f, PDrawingMode::Overlay);
    Check("A ", 12.0f, PDrawingMode::Overlay);
}

TEST_F(DisplayTextWidthTest, EmptyAndSpaceOnlyCopyFillTheSpecifiedArea)
{
    Check("", 11.0f, PDrawingMode::Copy);
    Check("  ", 11.0f, PDrawingMode::Copy);
    Check("", 11.0f, PDrawingMode::Overlay);
    Check("  ", 11.0f, PDrawingMode::Overlay);
}

TEST_F(DisplayTextWidthTest, NonpositiveWidthsHaveNoSpans)
{
    Check("AA", 0.0f, PDrawingMode::Copy);
    Check("AA", -1.0f, PDrawingMode::Overlay);
}

TEST_F(DisplayTextWidthTest, NaNWidthIsRejected)
{
    const float width = std::numeric_limits<float>::quiet_NaN();
    EXPECT_THROW(
        m_Text.Begin(m_Font, "A", m_Position, m_BitmapBounds, {}, PPoint(), PDrawingMode::Copy, width),
        std::system_error);
}

TEST_F(DisplayTextWidthTest, LeadingOverhangRemainsVisible)
{
    Check("A", 8.0f, PDrawingMode::Copy);
    EXPECT_EQ(m_Writes[size_t(m_Position.y - 1) * 32 + size_t(m_Position.x - 1)], 1);
}

TEST_F(DisplayTextWidthTest, AxisAlignedRotationsClipInTheDirectionOfAdvance)
{
    SetTransform(PPoint(-1.0f, 0.0f), PPoint(0.0f, -6.0f), PDisplayFontDirection::Left);
    Check("AA", 5.0f, PDrawingMode::Copy);
    SetTransform(PPoint(0.0f, -1.0f), PPoint(6.0f, 0.0f), PDisplayFontDirection::Up);
    Check("AA", 5.0f, PDrawingMode::Copy);
    SetTransform(PPoint(0.0f, 1.0f), PPoint(-6.0f, 0.0f), PDisplayFontDirection::Down);
    Check("AA", 5.0f, PDrawingMode::Overlay);
    Check("", 8.0f, PDrawingMode::Copy);
}

TEST_F(DisplayTextWidthTest, RotationAndShearClipSpansWithoutDuplicatingWrites)
{
    SetTransform(PPoint(0.6f, -0.8f), PPoint(4.8f, 3.6f), PDisplayFontDirection::Up);
    Check("AA", 5.25f, PDrawingMode::Copy);
    Check("AA", 5.25f, PDrawingMode::Overlay);
    Check("", 8.25f, PDrawingMode::Copy);
    SetTransform(PPoint(1.0f, 0.0f), PPoint(-3.0f, 6.0f), PDisplayFontDirection::Right);
    Check("AA", 5.25f, PDrawingMode::Copy);
    Check("AA", 5.25f, PDrawingMode::Overlay);
    SetTransform(PPoint(-0.6f, 0.8f), PPoint(-4.8f, -3.6f), PDisplayFontDirection::Down);
    Check("AA", 5.25f, PDrawingMode::Copy);
    Check("", 8.25f, PDrawingMode::Copy);
}

TEST_F(DisplayTextWidthTest, ClippingRespectsBitmapEdgesAndResetsBetweenDraws)
{
    m_BitmapBounds = PIRect(10, 10, 18, 16);
    SetTransform(PPoint(1.0f, 0.0f), PPoint(-3.0f, 6.0f), PDisplayFontDirection::Right);
    Check("AA", 5.25f, PDrawingMode::Copy);
    SetTransform(PPoint(1.0f, 0.0f), PPoint(0.0f, 6.0f), PDisplayFontDirection::Right);
    Check("A ", 12.0f, PDrawingMode::Copy);
    m_Text.Begin(m_Font, "AA", m_Position, m_BitmapBounds);
    bool reachesBitmapEnd = false;
    while (m_Text.NextSpan()) {
        reachesBitmapEnd |= m_Text.GetSpan().Bounds.right == m_BitmapBounds.right;
    }
    EXPECT_TRUE(reachesBitmapEnd);
}

} // namespace kernel
