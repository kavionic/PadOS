// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <limits>
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

static std::array<int32_t, 4> BrushReferenceBoundaries(float size, int32_t pixels, int32_t before, int32_t after)
{
    const float borders = float(before + after);
    if (borders >= size && borders > 0.0f)
    {
        const int32_t boundary = int32_t(std::floor(double(before) * double(pixels) / double(borders)));
        return {0, boundary, boundary, pixels};
    }
    return {0, int32_t(std::round(float(before) * float(pixels) / size)),
        pixels - int32_t(std::round(float(after) * float(pixels) / size)), pixels};
}

static KBrushReferenceAxis GetBrushReferenceAxis(
    int32_t position,
    int32_t destinationStart,
    int32_t destinationSize,
    int32_t sourceStart,
    int32_t sourceSize,
    int32_t bitmapSize,
    bool tile,
    float brushScale)
{
    if (tile)
    {
        destinationSize = std::max<int32_t>(1, int32_t(std::round(float(sourceSize) * brushScale)));
        position = destinationStart + (position - destinationStart) % destinationSize;
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
    const PImageDescription& image = layer.Description.Image;
    const PIRect& source = image.SourceRect;
    const PIRect slice(image.SliceRect);
    const PIPoint size(
        std::max<int32_t>(1, int32_t(std::round(image.LayoutSize.x * layer.Description.Scale.x))),
        std::max<int32_t>(1, int32_t(std::round(image.LayoutSize.y * layer.Description.Scale.y))));
    const int32_t localX = ((x - layer.Bounds.left) % size.x + size.x) % size.x;
    const int32_t localY = ((y - layer.Bounds.top) % size.y + size.y) % size.y;
    const std::array<int32_t, 4> destinationX = BrushReferenceBoundaries(
        image.LayoutSize.x,
        size.x,
        slice.left - source.left,
        source.right - slice.right);
    const std::array<int32_t, 4> destinationY = BrushReferenceBoundaries(
        image.LayoutSize.y,
        size.y,
        slice.top - source.top,
        source.bottom - slice.bottom);
    const std::array<int32_t, 4> sourceX = {source.left, slice.left, slice.right, source.right};
    const std::array<int32_t, 4> sourceY = {source.top, slice.top, slice.bottom, source.bottom};
    size_t column = 0;
    size_t row = 0;
    while (column < 2 && localX >= destinationX[column + 1]) {
        ++column;
    }
    while (row < 2 && localY >= destinationY[row + 1]) {
        ++row;
    }
    const PImageTilingFlag horizontalFlag = (row == 0) ? PImageTilingFlag::TopEdge
        : (row == 2) ? PImageTilingFlag::BottomEdge : PImageTilingFlag::CenterHorizontal;
    const PImageTilingFlag verticalFlag = (column == 0) ? PImageTilingFlag::LeftEdge
        : (column == 2) ? PImageTilingFlag::RightEdge : PImageTilingFlag::CenterVertical;
    const KBrushReferenceAxis horizontal = GetBrushReferenceAxis(
        localX,
        destinationX[column],
        destinationX[column + 1] - destinationX[column],
        sourceX[column],
        sourceX[column + 1] - sourceX[column],
        layer.Bitmap.Size.x,
        column == 1 && image.Tiling.Has(horizontalFlag),
        layer.Description.Scale.x);
    const KBrushReferenceAxis vertical = GetBrushReferenceAxis(
        localY,
        destinationY[row],
        destinationY[row + 1] - destinationY[row],
        sourceY[row],
        sourceY[row + 1] - sourceY[row],
        layer.Bitmap.Size.y,
        row == 1 && image.Tiling.Has(verticalFlag),
        layer.Description.Scale.y);
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
        m_Layer.Description.Type = PBrushType::Image;
        m_Layer.Description.Style = PBrushStyle::TexturePattern;
        m_Layer.Description.Image.IsOpaque = false;
        m_Layer.Description.Image.OpaqueSlices = 0;
        m_Layer.Description.Image.SourceRect = PIRect(0, 0, 5, 5);
        m_Layer.Description.Image.SliceRect = PRect(1, 1, 4, 4);
        m_Layer.Description.Image.LayoutSize = PPoint(11, 9);
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
    EXPECT_EQ(Sample(9, 20), Sample(20, 20));
    EXPECT_EQ(Sample(21, 20), Sample(10, 20));
}

TEST_F(DisplayBrushTest, SmallDestinationsCompressBordersAndCollapseTheCenter)
{
    m_Layer.Bounds = PIRect(0, 0, 1, 1);
    m_Layer.Description.Image.LayoutSize = PPoint(1, 1);
    EXPECT_EQ(Sample(0, 0).GetColor32(), m_Pixels[24]);
    m_Layer.Bounds = PIRect(0, 0, 2, 2);
    m_Layer.Description.Image.LayoutSize = PPoint(2, 2);
    EXPECT_EQ(Sample(0, 0).GetColor32(), m_Pixels[0]);
    EXPECT_EQ(Sample(1, 0).GetColor32(), m_Pixels[4]);
    EXPECT_EQ(Sample(0, 1).GetColor32(), m_Pixels[20]);
    EXPECT_EQ(Sample(1, 1).GetColor32(), m_Pixels[24]);
    m_Layer.Description.Image.SliceRect = PRect(m_Layer.Description.Image.SourceRect);
    m_Layer.Bounds = PIRect(0, 0, 1, 1);
    m_Layer.Description.Image.LayoutSize = PPoint(1, 1);
    EXPECT_EQ(Sample(0, 0).GetColor32(), m_Pixels[12]);
}

TEST(DisplayBrush, CompressedBordersPreserveExactRatiosForLargeLayouts)
{
    for (const int32_t leading : {1, 32767, 8388607})
    {
        const int32_t trailing = 16000000 - leading;
        for (const int32_t pixels : {1, 65535, 15999999})
        {
            const std::array<int32_t, 4> boundaries = PImageUtils::LayoutAxis(1.0f, pixels, leading, trailing);
            const int32_t expected = int32_t(uint64_t(leading) * pixels / (leading + trailing));
            EXPECT_EQ(boundaries, (std::array<int32_t, 4>{0, expected, expected, pixels}));
        }
    }
}

TEST_F(DisplayBrushTest, VerySmallLayoutsStillProduceAValidSinglePixelTile)
{
    for (const PRect& slice : {PRect(0, 0, 5, 5), PRect(1, 1, 4, 4)})
    {
        m_Layer.Description.Image.SliceRect = slice;
        m_Layer.Description.Image.LayoutSize = PPoint(std::numeric_limits<float>::min());
        EXPECT_EQ(Sample(10, 20).GetColor32(), (slice.left == 0.0f) ? m_Pixels[12] : m_Pixels[24]);
    }
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
    m_Layer.Description.Type = PBrushType::Bitmap;
    m_Layer.Description.Image.SourceRect = PIRect(1, 1, 4, 3);
    m_Layer.Description.Image.SliceRect = PRect(1, 1, 4, 3);
    m_Layer.Description.Image.LayoutSize = PPoint(3, 2);
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

TEST(DisplayBrush, LargeScaledSourceClippingPreservesCenteredPixelBoundaries)
{
    for (const int32_t sourceSize : {2, 32767, 65535, 1000003, 15999999})
    {
        for (const int32_t destinationSize : {1, 32767, 65535, 1000003, 15999999, 1600000000})
        {
            for (const int32_t offset : std::array<int32_t, 5>{0, 1, sourceSize / 2, sourceSize - 1, sourceSize})
            {
                const int64_t numerator = (int64_t(offset) * 2 + 1) * destinationSize - sourceSize;
                const int64_t denominator = int64_t(sourceSize) * 2;
                const int64_t boundary = numerator / denominator + ((numerator % denominator > 0) ? 1 : 0);
                const int32_t expected = int32_t(std::clamp<int64_t>(boundary, 0, destinationSize));
                EXPECT_EQ(PBrushUtils::MapBitmapSourceBoundary(offset, sourceSize, destinationSize), expected);
            }
        }
    }
}

TEST_F(DisplayBrushTest, DistantTextureOriginsPreserveTilingAndClippedSpans)
{
    std::array<PColor, 19> original;
    KDisplayBrush::SampleSpan(m_Layer, -3, -7, original);
    for (const PIPoint& movement : {PIPoint(15000000, -15000000), PIPoint(-15000000, 15000000)})
    {
        m_Layer.Bounds += movement;
        std::array<PColor, 19> translated;
        KDisplayBrush::SampleSpan(m_Layer, -3 + movement.x, -7 + movement.y, translated);
        EXPECT_EQ(translated, original);
        m_Layer.Bounds -= movement;
    }
}

TEST_F(DisplayBrushTest, ClippedBitmapSourceAndScrollingKeepTheOriginalTransform)
{
    m_Layer.Description.Type = PBrushType::Image;
    m_Layer.Description.Image.SourceRect = PIRect(-1, 0, 6, 5);
    m_Layer.Description.Image.SliceRect = PRect(-1, 0, 6, 5);
    m_Layer.Description.Image.LayoutSize = PPoint(14, 10);
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
        KDisplayBrush::SampleSpan(m_Layer, 10 + first, y, std::span(clipped).first(end - first));
        for (int32_t x = first; x < end; ++x) {
            EXPECT_EQ(clipped[x - first], whole[x]);
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
    m_Layer.Description.Type = PBrushType::Image;
    m_Layer.Description.Image.SourceRect = PIRect(0, 0, 2, 1);
    m_Layer.Description.Image.SliceRect = PRect(0, 0, 2, 1);
    m_Layer.Description.Image.LayoutSize = PPoint(3, 1);
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
        for (bool sliced : {false, true})
        {
            m_Layer.Description.Type = PBrushType::Image;
            for (const PIRect& source : {PIRect(0, 0, 5, 5), PIRect(1, 1, 4, 4), PIRect(2, 2, 3, 3), PIRect(-1, 0, 6, 5)})
            {
                if (sliced && (source.Width() < 3 || source.left < 0)) {
                    continue;
                }
                m_Layer.Description.Image.SourceRect = source;
                m_Layer.Description.Image.SliceRect = sliced ? PRect(source).GetResized(1.0f, 1.0f, -1.0f, -1.0f) : PRect(source);
                for (const PIPoint& size : sizes)
                {
                    m_Layer.Bounds = PIRect(PIPoint(10, 20), PIPoint(10, 20) + size);
                    m_Layer.Description.Image.LayoutSize = PPoint(size);
                    const KDisplayBrushSampler sampler(m_Layer);
                    for (int32_t y = m_Layer.Bounds.top - 1; y <= m_Layer.Bounds.bottom; ++y)
                    {
                        std::array<PColor, 19> whole;
                        sampler.SampleSpan(9, y, whole);
                        for (size_t index = 0; index < whole.size(); ++index)
                        {
                            const PColor expected = SampleBrushReference(m_Layer, 9 + int32_t(index), y);
                            EXPECT_EQ(whole[index].GetAlpha(), expected.GetAlpha());
                            if (expected.GetAlpha() != 0) {
                                EXPECT_EQ(whole[index], expected);
                            }
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
    for (bool sliced : {false, true})
    {
        m_Layer.Description.Type = PBrushType::Image;
        m_Layer.Description.Image.SliceRect = sliced ? PRect(1, 1, 4, 4) : PRect(0, 0, 5, 5);
        for (const PIPoint& size : {PIPoint(5, 5), PIPoint(11, 9), PIPoint(3, 7)})
        {
            m_Layer.Bounds = PIRect(PIPoint(10, 20), PIPoint(10, 20) + size);
            m_Layer.Description.Image.LayoutSize = PPoint(size);
            m_Layer.Description.Image.IsOpaque = true;
            const KDisplayBrushSampler opaque(m_Layer);
            m_Layer.Description.Image.IsOpaque = false;
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
    m_Layer.Description.Image.OpaqueSlices = 0x0010;
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
    description.Type = PBrushType::Image;
    description.Style = PBrushStyle::TexturePattern;
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, placement, placement + scroll, scroll));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, placement, placement, scroll));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, placement, PIRect(0, -37, 201, 863), scroll));
    const PIRect tile(0, 0, 16, 16);
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, tile, tile, PIPoint(16, -32)));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, tile, tile, scroll));
    const PIPoint nestedScroll(12, -8);
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, tile, tile + scroll + nestedScroll, scroll + nestedScroll));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, tile, tile + scroll, scroll + nestedScroll));
    const PIPoint distantMovement(15000000, -15000000);
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, tile, tile + distantMovement, distantMovement));
    EXPECT_FALSE(PBrushUtils::CanTranslate(description, tile, tile + distantMovement, distantMovement + PIPoint(1, 0)));
    description.Style = PBrushStyle::None;
    EXPECT_TRUE(PBrushUtils::CanTranslate(description, tile, tile, nestedScroll));
}

TEST(DisplayBrush, OpaqueSlicesShieldRepeatedRegionsFromAncestorChanges)
{
    PBrushDescription description;
    description.Type = PBrushType::Image;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 10, 10);
    description.Image.SliceRect = PRect(2, 3, 6, 9);
    description.Image.LayoutSize = PPoint(20, 20);
    description.Image.IsOpaque = false;
    description.Image.OpaqueSlices = 0x0010;
    const PIRect bounds(10, 20, 30, 40);
    std::vector<PIRect> regions;
    PBrushUtils::VisitOpaqueRegions(description, bounds, PIRect(10, 20, 50, 40), [&regions](const PIRect& region) { regions.push_back(region); });
    ASSERT_EQ(regions.size(), 2);
    EXPECT_EQ(regions[0], PIRect(12, 23, 26, 39));
    EXPECT_EQ(regions[1], PIRect(32, 23, 46, 39));
    regions.clear();
    const PIPoint movement(-15000000, 15000000);
    PBrushUtils::VisitOpaqueRegions(
        description,
        bounds + movement,
        PIRect(10, 20, 50, 40) + movement,
        [&regions](const PIRect& region) { regions.push_back(region); });
    ASSERT_EQ(regions.size(), 2);
    EXPECT_EQ(regions[0], PIRect(12, 23, 26, 39) + movement);
    EXPECT_EQ(regions[1], PIRect(32, 23, 46, 39) + movement);
    regions.clear();
    description.Image.LayoutSize = PPoint(3, 2);
    PBrushUtils::VisitOpaqueRegions(description, PIRect(0, 0, 3, 2), bounds, [&regions](const PIRect& region) { regions.push_back(region); });
    EXPECT_TRUE(regions.empty());
}

TEST_F(DisplayBrushTest, EveryTilingMaskMatchesReferenceWithScaleAndPartialTiles)
{
    for (uint8_t mask = 0; mask < 0x40; ++mask)
    {
        m_Layer.Description.Image.Tiling = PImageTilingFlag(mask);
        for (const PPoint& scale : {PPoint(1, 1), PPoint(1.5f, 2.0f), PPoint(0.5f, 0.75f)})
        {
            m_Layer.Description.Scale = scale;
            const KDisplayBrushSampler sampler(m_Layer);
            for (int32_t y = 17; y < 43; ++y)
            {
                std::array<PColor, 33> pixels;
                sampler.SampleSpan(5, y, pixels);
                for (size_t index = 0; index < pixels.size(); ++index) {
                    EXPECT_EQ(pixels[index], SampleBrushReference(m_Layer, 5 + int32_t(index), y)) << unsigned(mask) << ',' << index << ',' << y;
                }
                for (size_t offset : {size_t(1), size_t(11), size_t(24)})
                {
                    std::array<PColor, 33> clipped;
                    sampler.SampleSpan(5 + int32_t(offset), y, std::span(clipped).first(pixels.size() - offset));
                    for (size_t index = 0; index < pixels.size() - offset; ++index) {
                        EXPECT_EQ(clipped[index], pixels[index + offset]);
                    }
                }
            }
        }
    }
}

TEST_F(DisplayBrushTest, CenterAxesAndEachEdgeHaveIndependentTilingRules)
{
    const PImageTilingFlag flags[] = {
        PImageTilingFlag::TopEdge, PImageTilingFlag::BottomEdge, PImageTilingFlag::LeftEdge,
        PImageTilingFlag::RightEdge, PImageTilingFlag::CenterHorizontal, PImageTilingFlag::CenterVertical
    };
    // These samples repeat back to the first source texel only for their flag.
    const PIPoint positions[] = {
        PIPoint(14, 20), PIPoint(14, 28), PIPoint(10, 24),
        PIPoint(20, 24), PIPoint(14, 21), PIPoint(11, 24)
    };
    const size_t sourceIndices[] = {1, 21, 5, 9, 6, 6};
    for (size_t index = 0; index < std::size(flags); ++index)
    {
        m_Layer.Description.Image.Tiling = flags[index];
        EXPECT_EQ(Sample(positions[index].x, positions[index].y).GetColor32(), m_Pixels[sourceIndices[index]]);
        m_Layer.Description.Image.Tiling = PImageTiling::zero;
        EXPECT_NE(Sample(positions[index].x, positions[index].y).GetColor32(), m_Pixels[sourceIndices[index]]);
    }
}

TEST_F(DisplayBrushTest, ThreeSliceImagesSupportEitherOrientation)
{
    for (const PRect& slice : {PRect(1, 0, 4, 5), PRect(0, 1, 5, 4)})
    {
        m_Layer.Description.Image.SliceRect = slice;
        m_Layer.Description.Image.Tiling = PImageTilingFlag::Center;
        for (int32_t y = 19; y < 30; ++y)
        {
            for (int32_t x = 9; x < 22; ++x) {
                EXPECT_EQ(Sample(x, y), SampleBrushReference(m_Layer, x, y));
            }
        }
    }
}

TEST_F(DisplayBrushTest, UnslicedTiledCentersKeepTheirScaleWhenTheLayoutSizeCompensates)
{
    m_Layer.Description.Image.SliceRect = PRect(0, 0, 5, 5);
    m_Layer.Description.Image.LayoutSize = PPoint(10, 10);
    m_Layer.Description.Scale = PPoint(0.5f, 0.5f);
    m_Layer.Description.Image.Tiling = PImageTilingFlag::Center;
    const KDisplayBrushSampler sampler(m_Layer);
    for (int32_t y = 18; y < 28; ++y)
    {
        std::array<PColor, 13> pixels;
        sampler.SampleSpan(8, y, pixels);
        for (size_t index = 0; index < pixels.size(); ++index) {
            EXPECT_EQ(pixels[index], SampleBrushReference(m_Layer, 8 + int32_t(index), y));
        }
    }
}

TEST_F(DisplayBrushTest, ViewRelativeLayoutsMatchAbsoluteRenderingAcrossSizesScalesAndClipping)
{
    m_Pixels[0] = PColor(255, 0, 0, 0).GetColor32();
    m_Layer.Description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    for (bool sliced : {false, true})
    {
        m_Layer.Description.Image.SliceRect = sliced ? PRect(1, 1, 4, 4) : PRect(0, 0, 5, 5);
        for (const PPoint& fraction : {PPoint(1, 1), PPoint(0.5f, 0.5f)})
        {
            m_Layer.Description.Image.LayoutSize = fraction;
            for (const PIPoint& viewSize : {PIPoint(5, 5), PIPoint(22, 18), PIPoint(7, 9)})
            {
                m_Layer.ReferenceSize = viewSize;
                const PIPoint tileSize(int32_t(std::round(float(viewSize.x) * fraction.x)), int32_t(std::round(float(viewSize.y) * fraction.y)));
                for (const PPoint& scale : {PPoint(1, 1), PPoint(2, 2), PPoint(0.5f, 1.5f)})
                {
                    m_Layer.Description.Scale = scale;
                    for (uint8_t mask : {uint8_t(0x00), uint8_t(0x15), uint8_t(0x2a), uint8_t(0x3f)})
                    {
                        m_Layer.Description.Image.Tiling = PImageTilingFlag(mask);
                        PDisplayBrushLayer absolute = m_Layer;
                        absolute.Description.Image.LayoutMode = PImageLayoutMode::Absolute;
                        absolute.Description.Image.LayoutSize = PPoint(tileSize) / scale;
                        const KDisplayBrushSampler sampler(m_Layer);
                        for (int32_t y = 19; y <= 20 + viewSize.y; ++y)
                        {
                            std::array<PColor, 27> whole;
                            sampler.SampleSpan(9, y, whole);
                            for (size_t index = 0; index < whole.size(); ++index)
                            {
                                const PColor expected = SampleBrushReference(absolute, 9 + int32_t(index), y);
                                EXPECT_EQ(whole[index].GetAlpha(), expected.GetAlpha());
                                if (expected.GetAlpha() != 0) {
                                    EXPECT_EQ(whole[index], expected);
                                }
                            }
                            std::array<PColor, 7> clipped;
                            sampler.SampleSpan(13, y, clipped);
                            for (size_t index = 0; index < clipped.size(); ++index) {
                                EXPECT_EQ(clipped[index], whole[index + 4]);
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST_F(DisplayBrushTest, RelativeBackgroundMappingsSurviveMovementButChangeOnResize)
{
    m_Layer.Description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    m_Layer.Description.Image.LayoutSize = PPoint(0.5f, 0.5f);
    m_Layer.ReferenceSize = PIPoint(22, 18);
    const PIPoint size = PBrushUtils::TileSize(m_Layer.Description, m_Layer.ReferenceSize);
    ASSERT_EQ(size, PIPoint(11, 9));
    const PDisplayBrushLayer original = m_Layer;
    const PIPoint movement(-7, -13);
    m_Layer.Bounds += movement;
    EXPECT_TRUE(PBrushUtils::CanTranslate(original.Description, original.Bounds, m_Layer.Bounds, movement));
    for (int32_t y = 19; y < 40; ++y)
    {
        std::array<PColor, 25> expected;
        std::array<PColor, 25> moved;
        KDisplayBrush::SampleSpan(original, 8, y, expected);
        KDisplayBrush::SampleSpan(m_Layer, 8 + movement.x, y + movement.y, moved);
        EXPECT_EQ(moved, expected);
    }
    m_Layer.ReferenceSize = PIPoint(26, 22);
    const PIPoint resized = PBrushUtils::TileSize(m_Layer.Description, m_Layer.ReferenceSize);
    const PIRect resizedBounds(m_Layer.Bounds.TopLeft(), m_Layer.Bounds.TopLeft() + resized);
    EXPECT_FALSE(PBrushUtils::CanTranslate(original.Description, original.Bounds, resizedBounds, movement));
    EXPECT_TRUE(PBrushUtils::CanTranslate(original.Description, original.Bounds, original.Bounds, PIPoint(11, -18)));
    EXPECT_FALSE(PBrushUtils::CanTranslate(original.Description, original.Bounds, original.Bounds, PIPoint(1, 0)));
}

TEST(DisplayBrush, RelativeOpaqueSlicesUseTheSameScaledBordersAsRendering)
{
    PBrushDescription description;
    description.Type = PBrushType::Image;
    description.Style = PBrushStyle::TexturePattern;
    description.Scale = PPoint(2, 3);
    description.Image.SourceRect = PIRect(0, 0, 10, 10);
    description.Image.SliceRect = PRect(2, 3, 6, 9);
    description.Image.LayoutSize = PPoint(0.5f, 1);
    description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    description.Image.IsOpaque = false;
    description.Image.OpaqueSlices = 0x0010;
    const PIPoint tileSize = PBrushUtils::TileSize(description, PIPoint(40, 20));
    ASSERT_EQ(tileSize, PIPoint(20, 20));
    const PIRect bounds(PIPoint(10, 20), PIPoint(10, 20) + tileSize);
    std::vector<PIRect> regions;
    PBrushUtils::VisitOpaqueRegions(description, bounds, PIRect(10, 20, 50, 40), [&regions](const PIRect& region) { regions.push_back(region); });
    ASSERT_EQ(regions.size(), 2);
    EXPECT_EQ(regions[0], PIRect(14, 29, 22, 37));
    EXPECT_EQ(regions[1], PIRect(34, 29, 42, 37));
}

TEST(DisplayBrush, RelativeLayoutRoundingHandlesEmptyViewsAndRejectsOverflow)
{
    PBrushDescription description;
    description.Type = PBrushType::Image;
    description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    description.Image.LayoutSize = PPoint(0.5f, 0.5f);
    EXPECT_EQ(PBrushUtils::TileSize(description, PIPoint(0, 0)), PIPoint(1, 1));
    EXPECT_EQ(PBrushUtils::TileSize(description, PIPoint(5, 7)), PIPoint(3, 4));
    description.Image.LayoutSize = PPoint(std::numeric_limits<float>::max(), 1);
    EXPECT_THROW(PBrushUtils::TileSize(description, PIPoint(2, 2)), std::system_error);
}

TEST(DisplayBrush, LayoutMaximumDoesNotLimitTextureSizesOrPlacement)
{
    PBrushDescription description;
    description.Style = PBrushStyle::TexturePattern;
    description.Image.SourceRect = PIRect(0, 0, 2, 2);
    description.Image.LayoutSize = PPoint(COORD_MAX, COORD_MAX * 2.0f);
    EXPECT_TRUE(PBrushUtils::IsValidScale(description));
    EXPECT_EQ(PBrushUtils::TileSize(description, PIPoint(1, 1)), PIPoint(16000000, 32000000));
    description.Image.LayoutMode = PImageLayoutMode::ViewRelative;
    description.Image.LayoutSize = PPoint(1, 1);
    EXPECT_EQ(PBrushUtils::TileSize(description, PIPoint(16000000, 32000000)), PIPoint(16000000, 32000000));
    EXPECT_EQ(PBrushUtils::TileBounds(PIPoint(32000000, -64000000), PIPoint(2, 2)), PIRect(32000000, -64000000, 32000002, -63999998));
    description.Image.LayoutSize.x = float(std::numeric_limits<int32_t>::max());
    EXPECT_THROW(PBrushUtils::TileSize(description, PIPoint(1, 1)), std::system_error);
    description.Image.LayoutMode = PImageLayoutMode::Absolute;
    EXPECT_FALSE(PBrushUtils::IsValidScale(description));
    const int32_t maximum = std::numeric_limits<int32_t>::max();
    EXPECT_EQ(PBrushUtils::TileBounds(PIPoint(maximum - 2, 0), PIPoint(2, 2)), PIRect(maximum - 2, 0, maximum, 2));
    EXPECT_THROW(PBrushUtils::TileBounds(PIPoint(maximum - 1, 0), PIPoint(2, 2)), std::system_error);
}

TEST(DisplayBrush, SourcePixelBoundsMustRemainExactlyRepresentable)
{
    PImageDescription image;
    image.SourceRect = PIRect(0, 0, 16777216, 1);
    image.SliceRect = PRect(image.SourceRect);
    image.LayoutSize = PPoint(1, 1);
    EXPECT_TRUE(PImageUtils::IsValid(image));
    image.SourceRect.right = 16777217;
    image.SliceRect = PRect(image.SourceRect);
    EXPECT_FALSE(PImageUtils::IsValid(image));
}

TEST_F(DisplayBrushTest, NoneContributesNoPixelsAndSolidStyleRetainsTheTexture)
{
    m_Layer.Description.Style = PBrushStyle::None;
    EXPECT_EQ(Sample(12, 22), PColor(0, 0, 0, 0));
    m_Layer.Description.Style = PBrushStyle::SolidPattern;
    m_Layer.Description.Color = PColor(80, 100, 120);
    EXPECT_EQ(Sample(12, 22), m_Layer.Description.Color);
    m_Layer.Description.Style = PBrushStyle::TexturePattern;
    EXPECT_EQ(Sample(10, 20).GetColor32(), m_Pixels[0]);
}

} // namespace kernel
