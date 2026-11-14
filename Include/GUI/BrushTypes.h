// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <GUI/Color.h>
#include <GUI/GUIDefines.h>
#include <GUI/ImageTypes.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

enum class PBrushType : uint8_t
{
    Solid,
    Bitmap,
    Image
};

enum class PBrushStyle : uint8_t
{
    None,
    SolidPattern,
    TexturePattern
};

// Image metadata is published with brush resource updates, never with draws.
// Bitmap brushes use an unsliced image description at the bitmap's native size.
struct PBrushDescription
{
    PImageDescription Image;
    PPoint Scale = PPoint(1.0f, 1.0f);
    PColor Color = PColor(255, 255, 255);
    PBrushType Type = PBrushType::Solid;
    PBrushStyle Style = PBrushStyle::SolidPattern;

    PBrushDescription() noexcept = default;
    PBrushDescription(const PBrushDescription&) noexcept = default;
    PBrushDescription& operator=(const PBrushDescription&) noexcept = default;
    bool operator==(const PBrushDescription&) const = default;
    bool IsOpaque() const noexcept
    {
        return (Style == PBrushStyle::SolidPattern) ? Color.GetAlpha() == 255
            : Style == PBrushStyle::TexturePattern && Image.IsOpaque;
    }

    // Selecting a color restores solid painting, even after selecting None.
    void SetColor(PColor color) noexcept
    {
        if (Type != PBrushType::Solid) {
            *this = PBrushDescription();
        }
        Style = PBrushStyle::SolidPattern;
        Color = color;
    }
};

namespace PBrushUtils
{
inline PColor Composite(PColor foreground, PColor background)
{
    const uint32_t alpha = foreground.GetAlpha();
    if (alpha == 255) {
        return foreground;
    }
    if (alpha == 0) {
        return background;
    }
    const uint32_t inverse = 255 - alpha;
    return PColor(
        uint8_t((foreground.GetRed() * alpha + background.GetRed() * inverse + 127) / 255),
        uint8_t((foreground.GetGreen() * alpha + background.GetGreen() * inverse + 127) / 255),
        uint8_t((foreground.GetBlue() * alpha + background.GetBlue() * inverse + 127) / 255));
}

// INT32_MAX rounds up to 2^31 as float. Sizes must stay strictly below this
// boundary before conversion to integer sampling coordinates.
static constexpr float RASTER_SIZE_LIMIT = float(std::numeric_limits<int32_t>::max());

inline bool IsValidScale(const PBrushDescription& description)
{
    if (description.Style != PBrushStyle::TexturePattern) {
        return true;
    }
    const PPoint& scale = description.Scale;
    const PPoint size = (description.Image.LayoutMode == PImageLayoutMode::ViewRelative) ? PPoint(0.0f, 0.0f) : description.Image.LayoutSize;
    const PIRect& source = description.Image.SourceRect;
    return PMath::is_finite(scale.x) && PMath::is_finite(scale.y) && scale.x > 0.0f && scale.y > 0.0f
        && std::max(size.x, float(source.Width())) * scale.x < RASTER_SIZE_LIMIT
        && std::max(size.y, float(source.Height())) * scale.y < RASTER_SIZE_LIMIT;
}

// Resolve against the selecting view, never the clip or an inheriting child.
// Rounding here is shared by rendering, opacity regions and copy dependencies.
inline PIPoint TileSize(const PBrushDescription& description, const PIPoint& referenceSize)
{
    PPoint size = description.Image.LayoutSize;
    if (description.Image.LayoutMode == PImageLayoutMode::ViewRelative)
    {
        size *= PPoint(referenceSize);
        if (!PMath::is_finite(size.x) || !PMath::is_finite(size.y) || size.x >= RASTER_SIZE_LIMIT || size.y >= RASTER_SIZE_LIMIT) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
    }
    else
    {
        size *= description.Scale;
    }
    return PIPoint(std::max<int32_t>(1, int32_t(std::round(size.x))), std::max<int32_t>(1, int32_t(std::round(size.y))));
}

// Check the actual coordinate addition before constructing the tile rectangle.
inline PIRect TileBounds(const PIPoint& origin, const PIPoint& size)
{
    PIPoint bottomRight;
    if (__builtin_add_overflow(origin.x, size.x, &bottomRight.x) || __builtin_add_overflow(origin.y, size.y, &bottomRight.y)) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    return PIRect(origin, bottomRight);
}

// Relative sizes already specify the final raster tile. Remove brush scaling
// before arranging native borders; internal repetitions still use that scale.
inline PPoint LayoutSize(const PBrushDescription& description, const PIPoint& tileSize)
{
    return (description.Image.LayoutMode == PImageLayoutMode::ViewRelative) ? PPoint(tileSize) / description.Scale : description.Image.LayoutSize;
}

// Wrap a relative raster coordinate into a positive tile period.
inline int32_t WrapCoordinate(int32_t coordinate, int32_t period)
{
    const int32_t remainder = coordinate % period;
    return (remainder < 0) ? remainder + period : remainder;
}

// Map a clipped source boundary to the first destination pixel whose centered
// source coordinate reaches it. Keep the original scale for partial redraws.
inline int32_t MapBitmapSourceBoundary(int32_t offset, int32_t sourceSize, int32_t destinationSize)
{
    const double numerator = (offset * 2.0 + 1.0) * destinationSize - sourceSize;
    const double denominator = sourceSize * 2.0;
    return int32_t(std::clamp(std::ceil(numerator / denominator), 0.0, double(destinationSize)));
}

// Visit opaque slices in each intersecting tile without allocating a region.
// Slice-local filtering keeps the classified opacity valid after stretching.
template<typename Visitor>
void VisitOpaqueRegions(const PBrushDescription& description, const PIRect& bounds, const PIRect& footprint, Visitor&& visitor)
{
    if (description.IsOpaque())
    {
        visitor(footprint);
        return;
    }
    if (description.Style != PBrushStyle::TexturePattern || !bounds.IsValid() || !footprint.IsValid()) {
        return;
    }
    const PImageDescription& image = description.Image;
    const PRect& slice = image.SliceRect;
    const PIRect& source = image.SourceRect;
    const PPoint layoutSize = LayoutSize(description, bounds.Size());
    const std::array<int32_t, 4> horizontal = PImageUtils::LayoutAxis(
        layoutSize.x,
        bounds.Width(),
        int32_t(slice.left) - source.left,
        source.right - int32_t(slice.right));
    const std::array<int32_t, 4> vertical = PImageUtils::LayoutAxis(
        layoutSize.y,
        bounds.Height(),
        int32_t(slice.top) - source.top,
        source.bottom - int32_t(slice.bottom));
    std::array<PIRect, 9> opaqueSlices;
    size_t opaqueCount = 0;
    for (size_t index = 0; index < opaqueSlices.size(); ++index)
    {
        if ((image.OpaqueSlices & (0x01 << index)) != 0)
        {
            const size_t column = index % 3;
            const size_t row = index / 3;
            const PIRect opaque(horizontal[column], vertical[row], horizontal[column + 1], vertical[row + 1]);
            if (opaque.IsValid()) {
                opaqueSlices[opaqueCount++] = opaque;
            }
        }
    }
    if (opaqueCount == 0) {
        return;
    }
    const int32_t firstX = footprint.left - WrapCoordinate(footprint.left - bounds.left, bounds.Width());
    const int32_t firstY = footprint.top - WrapCoordinate(footprint.top - bounds.top, bounds.Height());
    for (int32_t top = firstY; top < footprint.bottom; top += bounds.Height())
    {
        for (int32_t left = firstX; left < footprint.right; left += bounds.Width())
        {
            for (size_t index = 0; index < opaqueCount; ++index)
            {
                const PIRect& sliceBounds = opaqueSlices[index];
                const PIRect opaque(
                    std::max<int32_t>(left + sliceBounds.left, footprint.left),
                    std::max<int32_t>(top + sliceBounds.top, footprint.top),
                    std::min<int32_t>(left + sliceBounds.right, footprint.right),
                    std::min<int32_t>(top + sliceBounds.bottom, footprint.bottom));
                if (opaque.IsValid()) {
                    visitor(opaque);
                }
            }
        }
    }
}

// Solids are translation-invariant; textures repeat at the rasterized tile size.
// Brush identity and revision are checked by the caller.
inline bool CanTranslate(const PBrushDescription& description, const PIRect& previous, const PIRect& current, const PIPoint& movement)
{
    if (description.Style != PBrushStyle::TexturePattern) {
        return true;
    }
    return current.Size() == previous.Size() && current.IsValid()
        && WrapCoordinate(current.left - previous.left - movement.x, current.Width()) == 0
        && WrapCoordinate(current.top - previous.top - movement.y, current.Height()) == 0;
}
}
