// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <array>
#include <GUI/Color.h>
#include <Math/Rect.h>

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
}

enum class PBrushType : uint8_t
{
    Solid,
    BitmapStretch,
    BitmapTile,
    NineSlice
};

enum class PBrushSource : uint8_t
{
    Brush,
    Inherit
};

struct PBrushBorders
{
    int32_t Left = 0;
    int32_t Top = 0;
    int32_t Right = 0;
    int32_t Bottom = 0;

    bool operator==(const PBrushBorders&) const = default;
};

struct PBrushDescription
{
    PIRect          SourceRect;
    PBrushBorders   Borders;
    PColor          Color           = PColor(255, 255, 255);
    uint16_t        OpaqueSlices    = 0x01ff;
    PBrushType      Type            = PBrushType::Solid;
    bool            IsOpaque        = true;

    PBrushDescription() noexcept = default;
    PBrushDescription(const PBrushDescription&) noexcept = default;
    PBrushDescription& operator=(const PBrushDescription&) noexcept = default;
    bool operator==(const PBrushDescription&) const = default;

    // Selecting a color converts bitmap brushes to solid brushes.
    void SetColor(PColor color) noexcept
    {
        if (Type != PBrushType::Solid) {
            *this = PBrushDescription();
        }
        Color = color;
        IsOpaque = color.GetAlpha() == 255;
    }
};

namespace PBrushUtils
{
inline std::array<int32_t, 4> SliceAxis(int32_t size, int32_t leading, int32_t trailing)
{
    if (int64_t(leading) + trailing > size)
    {
        leading = int32_t(int64_t(size) * leading / (int64_t(leading) + trailing));
        trailing = size - leading;
    }
    return {0, leading, size - trailing, size};
}

// Map a clipped source boundary to the first destination pixel whose centered
// source coordinate reaches it. Keep the original scale for partial redraws.
inline int32_t MapBitmapSourceBoundary(int32_t offset, int32_t sourceSize, int32_t destinationSize)
{
    const int64_t numerator = (int64_t(offset) * 2 + 1) * destinationSize - sourceSize;
    const int64_t denominator = int64_t(sourceSize) * 2;
    const int64_t boundary = numerator / denominator + ((numerator % denominator > 0) ? 1 : 0);
    return int32_t(std::clamp<int64_t>(boundary, 0, destinationSize));
}

// Opaque source slices are classified once when an immutable brush is created.
// Sampling clamps within each slice, so opacity holds through interpolation.
inline std::array<PIRect, 9> OpaqueRegions(const PBrushDescription& description, const PIRect& bounds, const PIRect& footprint)
{
    std::array<PIRect, 9> regions;
    if (description.IsOpaque)
    {
        regions[0] = (description.Type == PBrushType::Solid || description.Type == PBrushType::BitmapTile) ? footprint : bounds;
    }
    else if (description.Type == PBrushType::NineSlice && bounds.IsValid())
    {
        const std::array<int32_t, 4> horizontal = SliceAxis(bounds.Width(), description.Borders.Left, description.Borders.Right);
        const std::array<int32_t, 4> vertical = SliceAxis(bounds.Height(), description.Borders.Top, description.Borders.Bottom);
        for (size_t index = 0; index < regions.size(); ++index)
        {
            if ((description.OpaqueSlices & (0x01 << index)) != 0)
            {
                const size_t column = index % 3;
                const size_t row = index / 3;
                regions[index] = PIRect(horizontal[column], vertical[row], horizontal[column + 1], vertical[row + 1]) + bounds.TopLeft();
            }
        }
    }
    return regions;
}

// Compare sampling transforms, including the period of repeated artwork.
// Resource and description identity are checked by the caller.
inline bool CanTranslate(const PBrushDescription& description, const PIRect& previous, const PIRect& current, const PIPoint& movement)
{
    if (description.Type == PBrushType::Solid) {
        return true;
    }
    const PIPoint relative = current.TopLeft() - previous.TopLeft() - movement;
    if (description.Type == PBrushType::BitmapTile) {
        return relative.x % description.SourceRect.Width() == 0 && relative.y % description.SourceRect.Height() == 0;
    }
    return relative == PIPoint(0, 0) && current.Size() == previous.Size();
}
}

