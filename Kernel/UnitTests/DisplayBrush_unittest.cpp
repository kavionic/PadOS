// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <Kernel/Drivers/Video/KDisplayBrush.h>

namespace kernel
{

// Deliberately retain a scalar four-sample reference independent of prepared
// runs and opaque/direct specializations, to detect optimization regressions.
struct KBrushReferenceAxis
{
    int32_t First;
    int32_t Second;
    uint32_t Fraction;
};

static KBrushReferenceAxis GetBrushReferenceAxis(
    PBrushType type,
    int32_t position,
    int32_t destinationSize,
    int32_t sourceStart,
    int32_t sourceSize,
    int32_t leading,
    int32_t trailing,
    int32_t bitmapSize)
{
    int32_t destinationStart = 0;
    if (type == PBrushType::NineSlice)
    {
        const std::array<int32_t, 4> boundaries = PBrushUtils::SliceAxis(destinationSize, leading, trailing);
        const std::array<int32_t, 4> sources = {0, leading, sourceSize - trailing, sourceSize};
        size_t slice = 0;
        while (slice < 2 && position >= boundaries[slice + 1]) {
            ++slice;
        }
        destinationStart = boundaries[slice];
        destinationSize = boundaries[slice + 1] - destinationStart;
        sourceStart += sources[slice];
        sourceSize = sources[slice + 1] - sources[slice];
    }
    const float scale = float(sourceSize) / float(destinationSize);
    const float coordinate = float(sourceStart) + (float(position - destinationStart) + 0.5f) * scale - 0.5f;
    const int32_t maximum = std::min(sourceStart + sourceSize - 1, bitmapSize - 1);
    const float clamped = std::clamp(coordinate, float(std::max<int32_t>(sourceStart, 0)), float(maximum));
    const int32_t first = int32_t(clamped);
    return {first, std::min(first + 1, maximum), uint32_t((clamped - float(first)) * 256.0f)};
}

static PColor SampleBrushReference(const PDisplayBrushLayer& layer, int32_t x, int32_t y)
{
    if (x < layer.Bounds.left || x >= layer.Bounds.right || y < layer.Bounds.top || y >= layer.Bounds.bottom) {
        return PColor(0, 0, 0, 0);
    }
    const PIRect& source = layer.Description.SourceRect;
    const PBrushBorders& borders = layer.Description.Borders;
    const KBrushReferenceAxis horizontal = GetBrushReferenceAxis(
        layer.Description.Type,
        x - layer.Bounds.left,
        layer.Bounds.Width(),
        source.left,
        source.Width(),
        borders.Left,
        borders.Right,
        layer.Bitmap.Size.x);
    const KBrushReferenceAxis vertical = GetBrushReferenceAxis(
        layer.Description.Type,
        y - layer.Bounds.top,
        layer.Bounds.Height(),
        source.top,
        source.Height(),
        borders.Top,
        borders.Bottom,
        layer.Bitmap.Size.y);
    auto readPixel = [&layer](int32_t sourceX, int32_t sourceY)
    {
        const uint8_t* row = layer.Bitmap.Raster + size_t(sourceY) * layer.Bitmap.BytesPerLine;
        if (layer.Bitmap.ColorSpace == PEColorSpace::RGB16) {
            return PColor::FromRGB16(reinterpret_cast<const uint16_t*>(row)[sourceX]);
        } else if (layer.Bitmap.ColorSpace == PEColorSpace::RGB32) {
            return PColor::FromRGB32(reinterpret_cast<const uint32_t*>(row)[sourceX]);
        } else {
            return PColor::FromRGB32A(reinterpret_cast<const uint32_t*>(row)[sourceX]);
        }
    };
    const std::array<PColor, 4> colors =
    {
        readPixel(horizontal.First, vertical.First), readPixel(horizontal.Second, vertical.First),
        readPixel(horizontal.First, vertical.Second), readPixel(horizontal.Second, vertical.Second)
    };
    const std::array<uint32_t, 4> weights =
    {
        (256 - horizontal.Fraction) * (256 - vertical.Fraction), horizontal.Fraction * (256 - vertical.Fraction),
        (256 - horizontal.Fraction) * vertical.Fraction, horizontal.Fraction * vertical.Fraction
    };
    uint32_t alpha = 0;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    for (size_t index = 0; index < colors.size(); ++index)
    {
        const uint32_t weight = weights[index] * colors[index].GetAlpha();
        alpha += weight;
        red += weight * colors[index].GetRed();
        green += weight * colors[index].GetGreen();
        blue += weight * colors[index].GetBlue();
    }
    if (alpha == 0) {
        return PColor(0, 0, 0, 0);
    }
    return PColor(
        uint8_t((red + alpha / 2) / alpha),
        uint8_t((green + alpha / 2) / alpha),
        uint8_t((blue + alpha / 2) / alpha),
        uint8_t((alpha + 32768) / 65536));
}

class DisplayBrushTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        for (size_t index = 0; index < m_Pixels.size(); ++index) {
            m_Pixels[index] = PColor(uint8_t(index * 9), uint8_t(index * 5), uint8_t(index * 3)).GetColor32();
        }
        m_Layer.Bitmap.ColorSpace = PEColorSpace::RGBA32;
        m_Layer.Bitmap.Size = PIPoint(5, 5);
        m_Layer.Bitmap.BytesPerLine = 5 * sizeof(uint32_t);
        m_Layer.Bitmap.Raster = reinterpret_cast<uint8_t*>(m_Pixels.data());
        m_Layer.Description.Type = PBrushType::NineSlice;
        m_Layer.Description.IsOpaque = false;
        m_Layer.Description.OpaqueSlices = 0;
        m_Layer.Description.SourceRect = PIRect(0, 0, 5, 5);
        m_Layer.Description.Borders = {1, 1, 1, 1};
        m_Layer.Bounds = PIRect(10, 20, 21, 29);
    }

    PColor Sample(int32_t x, int32_t y)
    {
        PColor pixel;
        KDisplayBrush::SampleSpan(m_Layer, x, y, std::span(&pixel, 1));
        return pixel;
    }

    std::array<uint32_t, 25> m_Pixels;
    PDisplayBrushLayer m_Layer;
};

TEST_F(DisplayBrushTest, NineSlicePreservesCornersAndDoesNotBleedAcrossSliceBoundaries)
{
    EXPECT_EQ(Sample(10, 20).GetColor32(), m_Pixels[0]);
    EXPECT_EQ(Sample(20, 20).GetColor32(), m_Pixels[4]);
    EXPECT_EQ(Sample(10, 28).GetColor32(), m_Pixels[20]);
    EXPECT_EQ(Sample(20, 28).GetColor32(), m_Pixels[24]);
    EXPECT_EQ(Sample(11, 21).GetColor32(), m_Pixels[6]);
    EXPECT_EQ(Sample(19, 27).GetColor32(), m_Pixels[18]);
    EXPECT_EQ(Sample(9, 20).GetAlpha(), 0);
    EXPECT_EQ(Sample(21, 20).GetAlpha(), 0);
}

TEST_F(DisplayBrushTest, SmallDestinationsCompressBordersAndCollapseTheCenter)
{
    m_Layer.Bounds = PIRect(0, 0, 1, 1);
    EXPECT_EQ(Sample(0, 0).GetColor32(), m_Pixels[24]);
    m_Layer.Bounds = PIRect(0, 0, 2, 2);
    EXPECT_EQ(Sample(0, 0).GetColor32(), m_Pixels[0]);
    EXPECT_EQ(Sample(1, 0).GetColor32(), m_Pixels[4]);
    EXPECT_EQ(Sample(0, 1).GetColor32(), m_Pixels[20]);
    EXPECT_EQ(Sample(1, 1).GetColor32(), m_Pixels[24]);
    m_Layer.Description.Borders = {0, 0, 0, 0};
    m_Layer.Bounds = PIRect(0, 0, 1, 1);
    EXPECT_EQ(Sample(0, 0).GetColor32(), m_Pixels[12]);
}

TEST_F(DisplayBrushTest, ClippedSpansKeepTheOriginalNineSliceMapping)
{
    for (int32_t y = 19; y <= 29; ++y)
    {
        std::array<PColor, 13> whole;
        KDisplayBrush::SampleSpan(m_Layer, 9, y, whole);
        for (size_t left = 0; left < whole.size(); ++left)
        {
            std::array<PColor, 13> partial;
            const size_t count = whole.size() - left;
            KDisplayBrush::SampleSpan(m_Layer, 9 + int32_t(left), y, std::span(partial).first(count));
            for (size_t index = 0; index < count; ++index) {
                EXPECT_EQ(partial[index], whole[index + left]);
            }
        }
    }
}

TEST_F(DisplayBrushTest, TilingWrapsInBothDirectionsAndUsesAtlasSubrectangles)
{
    m_Layer.Description.Type = PBrushType::BitmapTile;
    m_Layer.Description.SourceRect = PIRect(1, 1, 4, 3);
    EXPECT_EQ(Sample(10, 20).GetColor32(), m_Pixels[6]);
    EXPECT_EQ(Sample(9, 19).GetColor32(), m_Pixels[13]);
    EXPECT_EQ(Sample(16, 24).GetColor32(), m_Pixels[6]);
    const KDisplayBrushSampler sampler(m_Layer);
    std::array<PColor, 17> pixels;
    sampler.SampleSpan(7, 19, pixels);
    for (size_t index = 0; index < pixels.size(); ++index) {
        EXPECT_EQ(pixels[index].GetColor32(), m_Pixels[11 + index % 3]);
    }
}

TEST(DisplayBrush, ScaledSourceClippingSelectsExactlyTheValidDestinationSamples)
{
    for (int32_t sourceSize = 2; sourceSize <= 24; ++sourceSize)
    {
        for (int32_t destinationSize = 1; destinationSize <= 24; ++destinationSize)
        {
            for (int32_t offset = 1; offset < sourceSize; ++offset)
            {
                int32_t expected = 0;
                while (expected < destinationSize && (2 * expected + 1) * sourceSize < (2 * offset + 1) * destinationSize) {
                    ++expected;
                }
                EXPECT_EQ(PBrushUtils::MapBitmapSourceBoundary(offset, sourceSize, destinationSize), expected);
            }
        }
    }
}

TEST_F(DisplayBrushTest, ClippedBitmapSourceAndScrollingKeepTheOriginalTransform)
{
    m_Layer.Description.Type = PBrushType::BitmapStretch;
    m_Layer.Description.SourceRect = PIRect(-1, 0, 6, 5);
    m_Layer.Bounds = PIRect(10, 20, 24, 30);
    const int32_t first = PBrushUtils::MapBitmapSourceBoundary(1, 7, 14);
    const int32_t end = PBrushUtils::MapBitmapSourceBoundary(6, 7, 14);
    EXPECT_EQ(first, 3);
    EXPECT_EQ(end, 13);
    const PIPoint movement(-7, -13);
    for (int32_t y = 20; y < 30; ++y)
    {
        std::array<PColor, 14> whole;
        KDisplayBrush::SampleSpan(m_Layer, 10, y, whole);
        std::array<PColor, 14> clipped;
        KDisplayBrush::SampleSpan(m_Layer, 10 + first, y, std::span(clipped).first(size_t(end - first)));
        for (int32_t x = first; x < end; ++x) {
            EXPECT_EQ(clipped[size_t(x - first)], whole[size_t(x)]);
        }
        m_Layer.Bounds += movement;
        std::array<PColor, 14> moved;
        KDisplayBrush::SampleSpan(m_Layer, 10 + movement.x, y + movement.y, moved);
        EXPECT_EQ(moved, whole);
        m_Layer.Bounds -= movement;
    }
}

TEST_F(DisplayBrushTest, FilteringTransparentPixelsDoesNotIntroduceTheirHiddenColor)
{
    m_Pixels[0] = PColor(255, 0, 0, 0).GetColor32();
    m_Pixels[1] = PColor(0, 0, 255).GetColor32();
    m_Layer.Description.Type = PBrushType::BitmapStretch;
    m_Layer.Description.SourceRect = PIRect(0, 0, 2, 1);
    m_Layer.Bounds = PIRect(0, 0, 3, 1);
    const PColor middle = Sample(1, 0);
    EXPECT_EQ(middle, PColor(0, 0, 255, 128));
    EXPECT_EQ(KDisplayBrush::Composite(middle, PColor(255, 255, 255)), PColor(127, 127, 255));
}

TEST_F(DisplayBrushTest, TransparentCornersResolveMultipleAncestorLayersOnce)
{
    m_Pixels[0] = PColor(0, 0, 255, 0).GetColor32();
    PDisplayBrushLayer root;
    root.Description.Color = PColor(40, 80, 120);
    PDisplayBrushLayer panel;
    panel.Description.Color = PColor(200, 120, 40, 128);
    panel.Description.IsOpaque = false;
    const std::array layers = {root, panel, m_Layer};
    KDisplayBrush brush;
    brush.SetLayers(layers);
    std::array<PColor, 2> pixels;
    brush.RenderSpan(10, 20, pixels);
    EXPECT_EQ(pixels[0], PColor(120, 100, 80));
    EXPECT_EQ(pixels[1].GetColor32(), m_Pixels[1]);
}

TEST_F(DisplayBrushTest, PreparedSamplerMatchesReferenceAcrossFormatsAndClippedSpans)
{
    std::array<uint16_t, 25> pixels16;
    for (size_t index = 0; index < m_Pixels.size(); ++index)
    {
        const PColor color(uint8_t(index * 9), uint8_t(index * 5), uint8_t(index * 3), uint8_t((index % 6) * 51));
        m_Pixels[index] = color.GetColor32();
        pixels16[index] = color.GetColor16();
    }
    const std::array sizes = {PIPoint(1, 1), PIPoint(2, 2), PIPoint(5, 5), PIPoint(11, 9), PIPoint(3, 7), PIPoint(17, 13)};
    for (PEColorSpace colorSpace : {PEColorSpace::RGB16, PEColorSpace::RGB32, PEColorSpace::RGBA32})
    {
        m_Layer.Bitmap.ColorSpace = colorSpace;
        m_Layer.Bitmap.BytesPerLine = (colorSpace == PEColorSpace::RGB16) ? 5 * sizeof(uint16_t) : 5 * sizeof(uint32_t);
        m_Layer.Bitmap.Raster = (colorSpace == PEColorSpace::RGB16)
            ? reinterpret_cast<uint8_t*>(pixels16.data()) : reinterpret_cast<uint8_t*>(m_Pixels.data());
        for (PBrushType type : {PBrushType::BitmapStretch, PBrushType::NineSlice})
        {
            m_Layer.Description.Type = type;
            for (const PIRect& source : {PIRect(0, 0, 5, 5), PIRect(1, 1, 4, 4), PIRect(2, 2, 3, 3), PIRect(-1, 0, 6, 5)})
            {
                if (type == PBrushType::NineSlice && (source.Width() < 3 || source.left < 0)) {
                    continue;
                }
                m_Layer.Description.SourceRect = source;
                for (const PIPoint& size : sizes)
                {
                    m_Layer.Bounds = PIRect(PIPoint(10, 20), PIPoint(10, 20) + size);
                    const KDisplayBrushSampler sampler(m_Layer);
                    for (int32_t y = m_Layer.Bounds.top - 1; y <= m_Layer.Bounds.bottom; ++y)
                    {
                        std::array<PColor, 19> whole;
                        sampler.SampleSpan(9, y, whole);
                        for (size_t index = 0; index < whole.size(); ++index) {
                            EXPECT_EQ(whole[index], SampleBrushReference(m_Layer, 9 + int32_t(index), y));
                        }
                        for (size_t offset : {size_t(0), size_t(1), size_t(7), size_t(18)})
                        {
                            std::array<PColor, 19> clipped;
                            const size_t count = clipped.size() - offset;
                            sampler.SampleSpan(9 + int32_t(offset), y, std::span(clipped).first(count));
                            for (size_t index = 0; index < count; ++index) {
                                EXPECT_EQ(clipped[index], whole[index + offset]);
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST_F(DisplayBrushTest, OpaqueRgbaSpecializationsMatchAlphaAwareFiltering)
{
    for (PBrushType type : {PBrushType::BitmapStretch, PBrushType::NineSlice})
    {
        m_Layer.Description.Type = type;
        for (const PIPoint& size : {PIPoint(5, 5), PIPoint(11, 9), PIPoint(3, 7)})
        {
            m_Layer.Bounds = PIRect(PIPoint(10, 20), PIPoint(10, 20) + size);
            m_Layer.Description.IsOpaque = true;
            const KDisplayBrushSampler opaque(m_Layer);
            m_Layer.Description.IsOpaque = false;
            const KDisplayBrushSampler general(m_Layer);
            for (int32_t y = 19; y <= m_Layer.Bounds.bottom; ++y)
            {
                std::array<PColor, 13> actual;
                std::array<PColor, 13> expected;
                opaque.SampleSpan(9, y, actual);
                general.SampleSpan(9, y, expected);
                EXPECT_EQ(actual, expected);
            }
        }
    }
}

TEST_F(DisplayBrushTest, OpaqueSliceSelectionPreservesTransparentCorners)
{
    m_Pixels[0] = PColor(255, 0, 0, 0).GetColor32();
    m_Pixels[24] = PColor(0, 255, 0, 85).GetColor32();
    m_Layer.Description.OpaqueSlices = 0x0010;
    const KDisplayBrushSampler sampler(m_Layer);
    for (int32_t y = 19; y <= 29; ++y)
    {
        std::array<PColor, 13> pixels;
        sampler.SampleSpan(9, y, pixels);
        for (size_t index = 0; index < pixels.size(); ++index) {
            EXPECT_EQ(pixels[index], SampleBrushReference(m_Layer, 9 + int32_t(index), y));
        }
    }
}

TEST(DisplayBrush, TransparencyKeyCannotCollideWithVisibleRgb565Pixels)
{
    std::array<PColor, DISPLAY_BRUSH_BLOCK_PIXELS> pixels;
    for (size_t index = 0; index < pixels.size(); ++index) {
        pixels[index] = PColor::FromRGB16(uint16_t(index));
    }
    EXPECT_EQ(KDisplayBrush::FindTransparencyKey(pixels), DISPLAY_BRUSH_BLOCK_PIXELS);
    pixels[7] = PColor(255, 0, 255, 0);
    EXPECT_EQ(KDisplayBrush::FindTransparencyKey(pixels), 7);
    pixels[7] = PColor::FromRGB16(PTransparentColors::RGB16);
    const uint16_t key = KDisplayBrush::FindTransparencyKey(pixels);
    for (PColor pixel : pixels) {
        EXPECT_NE(key, pixel.GetColor16());
    }
}

TEST(DisplayBrush, BackgroundCopyDependsOnSamplingCoordinatesAndScale)
{
    PBrushDescription description;
    const PIRect placement(0, 0, 200, 900);
    const PIPoint scroll(0, -37);
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, placement, placement, scroll));
    description.Type = PBrushType::NineSlice;
    description.SourceRect = PIRect(0, 0, 12, 12);
    description.Borders = {3, 3, 3, 3};
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, placement, placement + scroll, scroll));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, placement, placement, scroll));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, placement, PIRect(0, -37, 201, 863), scroll));
    description.Type = PBrushType::BitmapTile;
    description.SourceRect = PIRect(2, 4, 18, 20);
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, placement, placement + scroll, scroll));
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, placement, placement, PIPoint(16, -32)));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, placement, placement, scroll));
    const PIPoint nestedScroll(12, -8);
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, placement, placement + scroll + nestedScroll, scroll + nestedScroll));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, placement, placement + scroll, scroll + nestedScroll));
}


TEST(DisplayBrush, OpaqueSlicesShieldOnlyTheCoveredPartsFromAncestorChanges)
{
    PBrushDescription description;
    description.Type = PBrushType::NineSlice;
    description.IsOpaque = false;
    description.OpaqueSlices = 0x0010;
    description.Borders = {2, 3, 4, 1};
    const PIRect bounds(10, 20, 30, 40);
    const std::array<PIRect, 9> regions = PBrushUtils::OpaqueRegions(description, bounds, bounds);
    EXPECT_EQ(regions[4], PIRect(12, 23, 26, 39));
    for (size_t index = 0; index < regions.size(); ++index)
    {
        if (index != 4) {
            EXPECT_FALSE(regions[index].IsValid());
        }
    }
    const std::array<PIRect, 9> small = PBrushUtils::OpaqueRegions(description, PIRect(0, 0, 3, 2), bounds);
    EXPECT_FALSE(small[4].IsValid());
    description.IsOpaque = true;
    description.Type = PBrushType::BitmapTile;
    EXPECT_EQ(PBrushUtils::OpaqueRegions(description, PIRect(0, 0, 4, 4), bounds)[0], bounds);
}

} // namespace kernel
