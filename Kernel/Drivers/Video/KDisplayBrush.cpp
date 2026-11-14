// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <bitset>
#include <type_traits>
#include <Kernel/Drivers/Video/KDisplayBrush.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

struct KBrushAxisSample
{
    int32_t First;
    int32_t Second;
    uint32_t Fraction;
};

static float GetBrushCoordinate(const KDisplayBrushAxisMapping& mapping, int32_t position)
{
    return float(mapping.SourceStart) + (float(position - mapping.DestinationStart) + 0.5f) * mapping.Scale - 0.5f;
}

// Use the sampling expression itself so boundary rounding agrees with partial
// redraws. This search runs during preparation, never in a pixel loop.
static int32_t FindBrushAxisBoundary(const KDisplayBrushAxisMapping& mapping, int32_t boundary)
{
    if (GetBrushCoordinate(mapping, mapping.DestinationStart) >= float(boundary)) {
        return mapping.DestinationStart;
    }
    if (GetBrushCoordinate(mapping, mapping.DestinationEnd - 1) < float(boundary)) {
        return mapping.DestinationEnd;
    }
    int32_t first = mapping.DestinationStart;
    int32_t last = mapping.DestinationEnd;
    while (first < last)
    {
        const int32_t middle = first + (last - first) / 2;
        if (GetBrushCoordinate(mapping, middle) < float(boundary)) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return first;
}

static KDisplayBrushAxisMapping MakeBrushAxisMapping(
    int32_t destinationStart,
    int32_t destinationSize,
    int32_t sourceStart,
    int32_t sourceSize,
    int32_t bitmapSize)
{
    KDisplayBrushAxisMapping mapping
    {
        (destinationSize > 0) ? float(sourceSize) / float(destinationSize) : 0.0f,
        destinationStart, destinationStart + destinationSize, sourceStart,
        std::max<int32_t>(sourceStart, 0), std::min(sourceStart + sourceSize - 1, bitmapSize - 1),
        destinationStart, destinationStart, sourceSize == destinationSize
    };
    if (destinationSize > 0)
    {
        mapping.InteriorStart = FindBrushAxisBoundary(mapping, mapping.SampleMinimum);
        mapping.InteriorEnd = FindBrushAxisBoundary(mapping, mapping.SampleMaximum);
    }
    return mapping;
}

static KBrushAxisSample GetBrushRowSample(const KDisplayBrushAxisMapping& mapping, int32_t position)
{
    const float clamped = std::clamp(GetBrushCoordinate(mapping, position), float(mapping.SampleMinimum), float(mapping.SampleMaximum));
    const int32_t first = int32_t(clamped);
    return {first, std::min(first + 1, mapping.SampleMaximum), uint32_t((clamped - float(first)) * 256.0f)};
}

template<PEColorSpace COLOR_SPACE>
using KBrushRasterPixel = std::conditional_t<COLOR_SPACE == PEColorSpace::RGB16, uint16_t, uint32_t>;

template<PEColorSpace COLOR_SPACE>
static inline PALWAYS_INLINE PColor ReadBrushPixel(KBrushRasterPixel<COLOR_SPACE> pixel)
{
    if constexpr (COLOR_SPACE == PEColorSpace::RGB16) {
        return PColor::FromRGB16(pixel);
    } else if constexpr (COLOR_SPACE == PEColorSpace::RGB32) {
        return PColor::FromRGB32(pixel);
    } else {
        return PColor::FromRGB32A(pixel);
    }
}

template<PEColorSpace COLOR_SPACE, uint32_t WEIGHT_BITS>
static inline PALWAYS_INLINE PColor FinishBrushSample(uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha)
{
    constexpr uint32_t ROUNDING = uint32_t(1) << (WEIGHT_BITS - 1);
    if constexpr (COLOR_SPACE != PEColorSpace::RGBA32)
    {
        return PColor(
            uint8_t((red + ROUNDING) >> WEIGHT_BITS),
            uint8_t((green + ROUNDING) >> WEIGHT_BITS),
            uint8_t((blue + ROUNDING) >> WEIGHT_BITS));
    }
    else
    {
        if (alpha == 0) {
            return PColor(0, 0, 0, 0);
        }
        return PColor(
            uint8_t((red + alpha / 2) / alpha),
            uint8_t((green + alpha / 2) / alpha),
            uint8_t((blue + alpha / 2) / alpha),
            uint8_t((alpha + ROUNDING) >> WEIGHT_BITS));
    }
}

template<PEColorSpace COLOR_SPACE>
static inline PALWAYS_INLINE PColor FilterBrushPair(PColor first, PColor second, uint32_t firstWeight, uint32_t secondWeight)
{
    uint32_t alpha = 0;
    if constexpr (COLOR_SPACE == PEColorSpace::RGBA32)
    {
        firstWeight *= first.GetAlpha();
        secondWeight *= second.GetAlpha();
        alpha = firstWeight + secondWeight;
    }
    return FinishBrushSample<COLOR_SPACE, 8>(
        firstWeight * first.GetRed() + secondWeight * second.GetRed(),
        firstWeight * first.GetGreen() + secondWeight * second.GetGreen(),
        firstWeight * first.GetBlue() + secondWeight * second.GetBlue(),
        alpha);
}

template<PEColorSpace COLOR_SPACE>
static inline PALWAYS_INLINE PColor FilterBrushQuad(
    PColor topLeft,
    PColor topRight,
    PColor bottomLeft,
    PColor bottomRight,
    uint32_t horizontal,
    uint32_t topWeight,
    uint32_t bottomWeight)
{
    const uint32_t inverse = 256 - horizontal;
    uint32_t topLeftWeight = inverse * topWeight;
    uint32_t topRightWeight = horizontal * topWeight;
    uint32_t bottomLeftWeight = inverse * bottomWeight;
    uint32_t bottomRightWeight = horizontal * bottomWeight;
    uint32_t alpha = 0;
    if constexpr (COLOR_SPACE == PEColorSpace::RGBA32)
    {
        topLeftWeight *= topLeft.GetAlpha();
        topRightWeight *= topRight.GetAlpha();
        bottomLeftWeight *= bottomLeft.GetAlpha();
        bottomRightWeight *= bottomRight.GetAlpha();
        alpha = topLeftWeight + topRightWeight + bottomLeftWeight + bottomRightWeight;
    }
    return FinishBrushSample<COLOR_SPACE, 16>(
        topLeftWeight * topLeft.GetRed() + topRightWeight * topRight.GetRed()
            + bottomLeftWeight * bottomLeft.GetRed() + bottomRightWeight * bottomRight.GetRed(),
        topLeftWeight * topLeft.GetGreen() + topRightWeight * topRight.GetGreen()
            + bottomLeftWeight * bottomLeft.GetGreen() + bottomRightWeight * bottomRight.GetGreen(),
        topLeftWeight * topLeft.GetBlue() + topRightWeight * topRight.GetBlue()
            + bottomLeftWeight * bottomLeft.GetBlue() + bottomRightWeight * bottomRight.GetBlue(),
        alpha);
}

template<PEColorSpace COLOR_SPACE, bool VERTICAL>
static inline PALWAYS_INLINE PColor SampleBrushColumn(
    const KBrushRasterPixel<COLOR_SPACE>* firstRow,
    const KBrushRasterPixel<COLOR_SPACE>* secondRow,
    int32_t sourceX,
    uint32_t topWeight,
    uint32_t bottomWeight)
{
    const PColor first = ReadBrushPixel<COLOR_SPACE>(firstRow[sourceX]);
    if constexpr (VERTICAL) {
        return FilterBrushPair<COLOR_SPACE>(first, ReadBrushPixel<COLOR_SPACE>(secondRow[sourceX]), topWeight, bottomWeight);
    } else {
        return (first.GetAlpha() == 0) ? PColor(0, 0, 0, 0) : first;
    }
}

// Each run belongs to one slice. Only its interior loop samples horizontally;
// clamped edge runs reuse a single result, and unscaled runs skip filtering.
template<PEColorSpace COLOR_SPACE, bool VERTICAL>
static void RenderBrushRun(
    const KDisplayBrushAxisMapping& mapping,
    const uint8_t* firstRaster,
    const uint8_t* secondRaster,
    uint32_t topWeight,
    uint32_t bottomWeight,
    int32_t start,
    std::span<PColor> pixels)
{
    const KBrushRasterPixel<COLOR_SPACE>* firstRow = reinterpret_cast<const KBrushRasterPixel<COLOR_SPACE>*>(firstRaster);
    const KBrushRasterPixel<COLOR_SPACE>* secondRow = reinterpret_cast<const KBrushRasterPixel<COLOR_SPACE>*>(secondRaster);
    const int32_t end = start + int32_t(pixels.size());
    const int32_t firstInterior = std::min(end, mapping.InteriorStart);
    if (start < firstInterior)
    {
        const PColor pixel = SampleBrushColumn<COLOR_SPACE, VERTICAL>(firstRow, secondRow, mapping.SampleMinimum, topWeight, bottomWeight);
        const size_t count = size_t(firstInterior - start);
        std::fill_n(pixels.begin(), count, pixel);
        pixels = pixels.subspan(count);
        start = firstInterior;
    }
    const int32_t lastInterior = std::min(end, mapping.InteriorEnd);
    if (start < lastInterior)
    {
        const size_t count = size_t(lastInterior - start);
        if (mapping.Direct)
        {
            int32_t sourceX = mapping.SourceStart + start - mapping.DestinationStart;
            for (PColor& pixel : pixels.first(count)) {
                pixel = SampleBrushColumn<COLOR_SPACE, VERTICAL>(firstRow, secondRow, sourceX++, topWeight, bottomWeight);
            }
        }
        else
        {
            const float sourceStart = float(mapping.SourceStart);
            const float scale = mapping.Scale;
            int32_t position = start - mapping.DestinationStart;
            for (PColor& pixel : pixels.first(count))
            {
                const float coordinate = sourceStart + (float(position++) + 0.5f) * scale - 0.5f;
                const int32_t sourceX = int32_t(coordinate);
                const uint32_t fraction = uint32_t((coordinate - float(sourceX)) * 256.0f);
                const PColor first = ReadBrushPixel<COLOR_SPACE>(firstRow[sourceX]);
                const PColor second = ReadBrushPixel<COLOR_SPACE>(firstRow[sourceX + 1]);
                if constexpr (VERTICAL)
                {
                    pixel = FilterBrushQuad<COLOR_SPACE>(
                        first,
                        second,
                        ReadBrushPixel<COLOR_SPACE>(secondRow[sourceX]),
                        ReadBrushPixel<COLOR_SPACE>(secondRow[sourceX + 1]),
                        fraction,
                        topWeight,
                        bottomWeight);
                }
                else
                {
                    pixel = FilterBrushPair<COLOR_SPACE>(first, second, 256 - fraction, fraction);
                }
            }
        }
        pixels = pixels.subspan(count);
    }
    if (!pixels.empty())
    {
        const PColor pixel = SampleBrushColumn<COLOR_SPACE, VERTICAL>(firstRow, secondRow, mapping.SampleMaximum, topWeight, bottomWeight);
        std::fill(pixels.begin(), pixels.end(), pixel);
    }
}

template<PEColorSpace COLOR_SPACE>
static void DispatchBrushRun(
    const KDisplayBrushAxisMapping& mapping,
    const uint8_t* firstRow,
    const uint8_t* secondRow,
    uint32_t fraction,
    int32_t start,
    std::span<PColor> pixels)
{
    if (fraction == 0) {
        RenderBrushRun<COLOR_SPACE, false>(mapping, firstRow, secondRow, 256, 0, start, pixels);
    } else {
        RenderBrushRun<COLOR_SPACE, true>(mapping, firstRow, secondRow, 256 - fraction, fraction, start, pixels);
    }
}

template<PEColorSpace COLOR_SPACE>
static void SampleTiledBrushSpan(const PDisplayBrushLayer& layer, int32_t x, int32_t y, std::span<PColor> pixels)
{
    const PIRect& source = layer.Description.Image.SourceRect;
    const int32_t sourceY = source.top + PBrushUtils::WrapCoordinate(y - layer.Bounds.top, source.Height());
    int32_t sourceX = source.left + PBrushUtils::WrapCoordinate(x - layer.Bounds.left, source.Width());
    const uint8_t* raster = layer.Bitmap.Raster + size_t(sourceY) * layer.Bitmap.BytesPerLine;
    const KBrushRasterPixel<COLOR_SPACE>* row = reinterpret_cast<const KBrushRasterPixel<COLOR_SPACE>*>(raster);
    while (!pixels.empty())
    {
        const size_t count = std::min(pixels.size(), size_t(source.right - sourceX));
        for (PColor& pixel : pixels.first(count)) {
            pixel = ReadBrushPixel<COLOR_SPACE>(row[sourceX++]);
        }
        pixels = pixels.subspan(count);
        sourceX = source.left;
    }
}

// Prepare every slice's stretch or tile mapping once. Repetition is split into
// contiguous runs, preserving the existing direct and bilinear inner loops.
KDisplayBrushSampler::KDisplayBrushSampler(const PDisplayBrushLayer& layer) : m_Layer(layer)
{
    if (layer.Description.Style != PBrushStyle::TexturePattern) {
        return;
    }
    const PImageDescription& image = layer.Description.Image;
    const PIRect& source = image.SourceRect;
    const PRect& slice = image.SliceRect;
    const PIPoint size = PBrushUtils::TileSize(layer.Description, layer.ReferenceSize);
    m_Layer.Bounds = PIRect(layer.Bounds.TopLeft(), layer.Bounds.TopLeft() + size);
    m_DirectBitmap = slice == PRect(source) && size == source.Size()
        && (!image.Tiling.Has(PImageTilingFlag::CenterHorizontal) || layer.Description.Scale.x == 1.0f)
        && (!image.Tiling.Has(PImageTilingFlag::CenterVertical) || layer.Description.Scale.y == 1.0f)
        && (source & PIRect(PIPoint(0, 0), layer.Bitmap.Size)) == source;
    if (m_DirectBitmap) {
        return;
    }
    const PPoint layoutSize = PBrushUtils::LayoutSize(layer.Description, size);
    const std::array<int32_t, 4> sourceHorizontal = PImageUtils::SourceAxis(source.left, source.right, slice.left, slice.right);
    const std::array<int32_t, 4> sourceVertical = PImageUtils::SourceAxis(source.top, source.bottom, slice.top, slice.bottom);
    m_HorizontalBoundaries = PImageUtils::LayoutAxis(
        layoutSize.x,
        size.x,
        int32_t(slice.left) - source.left,
        source.right - int32_t(slice.right));
    m_VerticalBoundaries = PImageUtils::LayoutAxis(
        layoutSize.y,
        size.y,
        int32_t(slice.top) - source.top,
        source.bottom - int32_t(slice.bottom));
    for (size_t index = 0; index < m_Slices.size(); ++index)
    {
        const size_t column = index % 3;
        const size_t row = index / 3;
        KDisplayBrushSlice& mapping = m_Slices[index];
        mapping.Bounds = PIRect(
            m_HorizontalBoundaries[column],
            m_VerticalBoundaries[row],
            m_HorizontalBoundaries[column + 1],
            m_VerticalBoundaries[row + 1]);
        if (!mapping.Bounds.IsValid()) {
            continue;
        }
        const int32_t sourceWidth = sourceHorizontal[column + 1] - sourceHorizontal[column];
        const int32_t sourceHeight = sourceVertical[row + 1] - sourceVertical[row];
        const bool tileHorizontal = PImageUtils::TileHorizontal(image.Tiling, row, column);
        const bool tileVertical = PImageUtils::TileVertical(image.Tiling, row, column);
        if (tileHorizontal) {
            mapping.Repeat.x = std::max<int32_t>(1, int32_t(std::round(float(sourceWidth) * layer.Description.Scale.x)));
        }
        if (tileVertical) {
            mapping.Repeat.y = std::max<int32_t>(1, int32_t(std::round(float(sourceHeight) * layer.Description.Scale.y)));
        }
        mapping.Horizontal = MakeBrushAxisMapping(
            mapping.Bounds.left,
            tileHorizontal ? mapping.Repeat.x : mapping.Bounds.Width(),
            sourceHorizontal[column],
            sourceWidth,
            layer.Bitmap.Size.x);
        mapping.Vertical = MakeBrushAxisMapping(
            mapping.Bounds.top,
            tileVertical ? mapping.Repeat.y : mapping.Bounds.Height(),
            sourceVertical[row],
            sourceHeight,
            layer.Bitmap.Size.y);
    }
}

void KDisplayBrushSampler::SampleSpan(int32_t x, int32_t y, std::span<PColor> pixels) const
{
    if (m_Layer.Description.Style == PBrushStyle::None)
    {
        std::fill(pixels.begin(), pixels.end(), PColor(0, 0, 0, 0));
        return;
    }
    if (m_Layer.Description.Style == PBrushStyle::SolidPattern)
    {
        std::fill(pixels.begin(), pixels.end(), m_Layer.Description.Color);
        return;
    }
    switch (m_Layer.Bitmap.ColorSpace)
    {
        case PEColorSpace::RGB16:
            SampleBitmapSpan<PEColorSpace::RGB16>(x, y, pixels);
            break;
        case PEColorSpace::RGB32:
            SampleBitmapSpan<PEColorSpace::RGB32>(x, y, pixels);
            break;
        case PEColorSpace::RGBA32:
            SampleBitmapSpan<PEColorSpace::RGBA32>(x, y, pixels);
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

template<PEColorSpace COLOR_SPACE>
void KDisplayBrushSampler::SampleBitmapSpan(int32_t x, int32_t y, std::span<PColor> pixels) const
{
    if (m_DirectBitmap)
    {
        SampleTiledBrushSpan<COLOR_SPACE>(m_Layer, x, y, pixels);
        return;
    }
    const PIRect& bounds = m_Layer.Bounds;
    const int32_t localY = PBrushUtils::WrapCoordinate(y - bounds.top, bounds.Height());
    int32_t localX = PBrushUtils::WrapCoordinate(x - bounds.left, bounds.Width());
    size_t row = 0;
    while (row < 2 && localY >= m_VerticalBoundaries[row + 1]) {
        ++row;
    }
    while (!pixels.empty())
    {
        size_t column = 0;
        while (column < 2 && localX >= m_HorizontalBoundaries[column + 1]) {
            ++column;
        }
        const size_t index = row * 3 + column;
        const KDisplayBrushSlice& mapping = m_Slices[index];
        int32_t sampleX = localX;
        int32_t sampleY = localY;
        size_t count = std::min(pixels.size(), size_t(mapping.Bounds.right - localX));
        if (mapping.Repeat.x > 0)
        {
            sampleX = mapping.Bounds.left + (localX - mapping.Bounds.left) % mapping.Repeat.x;
            count = std::min(count, size_t(mapping.Horizontal.DestinationEnd - sampleX));
        }
        if (mapping.Repeat.y > 0) {
            sampleY = mapping.Bounds.top + (localY - mapping.Bounds.top) % mapping.Repeat.y;
        }
        const KBrushAxisSample vertical = GetBrushRowSample(mapping.Vertical, sampleY);
        const uint8_t* firstRow = m_Layer.Bitmap.Raster + size_t(vertical.First) * m_Layer.Bitmap.BytesPerLine;
        const uint8_t* secondRow = m_Layer.Bitmap.Raster + size_t(vertical.Second) * m_Layer.Bitmap.BytesPerLine;
        const std::span<PColor> run = pixels.first(count);
        if constexpr (COLOR_SPACE == PEColorSpace::RGBA32)
        {
            const PImageDescription& image = m_Layer.Description.Image;
            const bool opaque = image.IsOpaque || (image.OpaqueSlices & (0x01 << index)) != 0;
            if (opaque) {
                DispatchBrushRun<PEColorSpace::RGB32>(mapping.Horizontal, firstRow, secondRow, vertical.Fraction, sampleX, run);
            } else {
                DispatchBrushRun<COLOR_SPACE>(mapping.Horizontal, firstRow, secondRow, vertical.Fraction, sampleX, run);
            }
        }
        else
        {
            DispatchBrushRun<COLOR_SPACE>(mapping.Horizontal, firstRow, secondRow, vertical.Fraction, sampleX, run);
        }
        pixels = pixels.subspan(count);
        localX += int32_t(count);
        if (localX == bounds.Width()) {
            localX = 0;
        }
    }
}

void KDisplayBrush::SetLayers(std::span<const PDisplayBrushLayer> layers)
{
    m_Layers.clear();
    m_Layers.reserve(layers.size());
    for (const PDisplayBrushLayer& layer : layers) {
        m_Layers.emplace_back(layer);
    }
}

void KDisplayBrush::RenderSpan(int32_t x, int32_t y, std::span<PColor> pixels) const
{
    if (pixels.size() > m_LayerPixels.size()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    std::fill(pixels.begin(), pixels.end(), PColor(255, 255, 255));
    const std::span<PColor> layerPixels(m_LayerPixels.data(), pixels.size());
    for (const KDisplayBrushSampler& layer : m_Layers)
    {
        layer.SampleSpan(x, y, layerPixels);
        for (size_t i = 0; i < pixels.size(); ++i) {
            pixels[i] = Composite(layerPixels[i], pixels[i]);
        }
    }
}

PColor KDisplayBrush::Composite(PColor foreground, PColor background)
{
    return PBrushUtils::Composite(foreground, background);
}

void KDisplayBrush::SampleSpan(const PDisplayBrushLayer& layer, int32_t x, int32_t y, std::span<PColor> pixels)
{
    KDisplayBrushSampler(layer).SampleSpan(x, y, pixels);
}

uint16_t KDisplayBrush::FindTransparencyKey(std::span<const PColor> pixels)
{
    if (pixels.size() > DISPLAY_BRUSH_BLOCK_PIXELS) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    std::bitset<DISPLAY_BRUSH_BLOCK_PIXELS + 1> used;
    for (PColor color : pixels)
    {
        const uint16_t pixel = color.GetColor16();
        if (color.GetAlpha() != 0 && pixel < used.size()) {
            used.set(pixel);
        }
    }
    size_t key = 0;
    while (used.test(key)) {
        ++key;
    }
    return uint16_t(key);
}

} // namespace kernel
