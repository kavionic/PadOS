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

static std::array<KDisplayBrushAxisMapping, 3> MakeBrushAxisMappings(
    PBrushType type,
    int32_t destinationStart,
    int32_t destinationSize,
    int32_t sourceStart,
    int32_t sourceSize,
    int32_t leading,
    int32_t trailing,
    int32_t bitmapSize)
{
    if (type != PBrushType::NineSlice) {
        return {MakeBrushAxisMapping(destinationStart, destinationSize, sourceStart, sourceSize, bitmapSize), {}, {}};
    }
    const std::array<int32_t, 4> boundaries = PBrushUtils::SliceAxis(destinationSize, leading, trailing);
    const int32_t destinationLeading = boundaries[1];
    const int32_t destinationTrailing = destinationSize - boundaries[2];
    return
    {
        MakeBrushAxisMapping(destinationStart, destinationLeading, sourceStart, leading, bitmapSize),
        MakeBrushAxisMapping(
            destinationStart + destinationLeading,
            destinationSize - destinationLeading - destinationTrailing,
            sourceStart + leading,
            sourceSize - leading - trailing,
            bitmapSize),
        MakeBrushAxisMapping(
            destinationStart + destinationSize - destinationTrailing,
            destinationTrailing,
            sourceStart + sourceSize - trailing,
            trailing,
            bitmapSize)
    };
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
    const PIRect& source = layer.Description.SourceRect;
    const int32_t sourceY = source.top + ((y - layer.Bounds.top) % source.Height() + source.Height()) % source.Height();
    int32_t sourceX = source.left + ((x - layer.Bounds.left) % source.Width() + source.Width()) % source.Width();
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

KDisplayBrushSampler::KDisplayBrushSampler(const PDisplayBrushLayer& layer)
    : m_Layer(layer), m_AxisCount((layer.Description.Type == PBrushType::NineSlice) ? 3 : 1)
{
    if ((layer.Description.Type == PBrushType::BitmapStretch || layer.Description.Type == PBrushType::NineSlice) && layer.Bounds.IsValid())
    {
        const PIRect& source = layer.Description.SourceRect;
        const PBrushBorders& borders = layer.Description.Borders;
        m_HorizontalMappings = MakeBrushAxisMappings(
            layer.Description.Type,
            layer.Bounds.left,
            layer.Bounds.Width(),
            source.left,
            source.Width(),
            borders.Left,
            borders.Right,
            layer.Bitmap.Size.x);
        m_VerticalMappings = MakeBrushAxisMappings(
            layer.Description.Type,
            layer.Bounds.top,
            layer.Bounds.Height(),
            source.top,
            source.Height(),
            borders.Top,
            borders.Bottom,
            layer.Bitmap.Size.y);
    }
}

void KDisplayBrushSampler::SampleSpan(int32_t x, int32_t y, std::span<PColor> pixels) const
{
    if (m_Layer.Description.Type == PBrushType::Solid)
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
    const PIRect& bounds = m_Layer.Bounds;
    if (m_Layer.Description.Type == PBrushType::BitmapTile)
    {
        SampleTiledBrushSpan<COLOR_SPACE>(m_Layer, x, y, pixels);
        return;
    }
    const int32_t first = std::max<int32_t>(x, bounds.left);
    const int32_t last = std::min<int32_t>(x + int32_t(pixels.size()), bounds.right);
    if (!bounds.IsValid() || y < bounds.top || y >= bounds.bottom || first >= last)
    {
        std::fill(pixels.begin(), pixels.end(), PColor(0, 0, 0, 0));
        return;
    }
    std::fill(pixels.begin(), pixels.begin() + (first - x), PColor(0, 0, 0, 0));
    std::fill(pixels.begin() + (last - x), pixels.end(), PColor(0, 0, 0, 0));
    size_t verticalIndex = 0;
    while (verticalIndex + 1 < m_AxisCount && y >= m_VerticalMappings[verticalIndex].DestinationEnd) {
        ++verticalIndex;
    }
    const KBrushAxisSample vertical = GetBrushRowSample(m_VerticalMappings[verticalIndex], y);
    const uint8_t* firstRow = m_Layer.Bitmap.Raster + size_t(vertical.First) * m_Layer.Bitmap.BytesPerLine;
    const uint8_t* secondRow = m_Layer.Bitmap.Raster + size_t(vertical.Second) * m_Layer.Bitmap.BytesPerLine;
    for (size_t column = 0; column < m_AxisCount; ++column)
    {
        const KDisplayBrushAxisMapping& mapping = m_HorizontalMappings[column];
        const int32_t start = std::max(first, mapping.DestinationStart);
        const int32_t end = std::min(last, mapping.DestinationEnd);
        if (start >= end) {
            continue;
        }
        const std::span<PColor> run = pixels.subspan(size_t(start - x), size_t(end - start));
        if constexpr (COLOR_SPACE == PEColorSpace::RGBA32)
        {
            const size_t slice = verticalIndex * m_AxisCount + column;
            const bool opaque = m_Layer.Description.IsOpaque || (m_Layer.Description.OpaqueSlices & (0x01 << slice)) != 0;
            if (opaque) {
                DispatchBrushRun<PEColorSpace::RGB32>(mapping, firstRow, secondRow, vertical.Fraction, start, run);
            } else {
                DispatchBrushRun<COLOR_SPACE>(mapping, firstRow, secondRow, vertical.Fraction, start, run);
            }
        }
        else
        {
            DispatchBrushRun<COLOR_SPACE>(mapping, firstRow, secondRow, vertical.Fraction, start, run);
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
