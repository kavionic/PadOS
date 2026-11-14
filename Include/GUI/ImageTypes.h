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
#include <Math/Misc.h>
#include <Math/Rect.h>
#include <Utils/EnumBitmask.h>

// Unset bits stretch. Edge bits repeat along the edge's length; the center
// repeats independently on each selected axis. Partial repeats are clipped.
enum class PImageTilingFlag : uint8_t
{
    TopEdge          = 0x01,
    BottomEdge       = 0x02,
    LeftEdge         = 0x04,
    RightEdge        = 0x08,
    CenterHorizontal = 0x10,
    CenterVertical   = 0x20,
    Edges = TopEdge | BottomEdge | LeftEdge | RightEdge,
    Center = CenterHorizontal | CenterVertical,
    All = Edges | Center
};

PDEFINE_ENUM_BITMASK(PImageTiling, PImageTilingFlag);

// Absolute layout sizes use pixels before brush scaling. View-relative sizes
// specify the final tile as a fraction of the view selecting the brush.
enum class PImageLayoutMode : uint8_t
{
    Absolute,
    ViewRelative
};

// Source and slice rectangles use bitmap coordinates. Slice boundaries are
// texel-aligned; LayoutMode defines the units of LayoutSize.
struct PImageDescription
{
    PIRect SourceRect;
    PRect SliceRect;
    PPoint LayoutSize;
    uint16_t OpaqueSlices = 0x01ff;
    PImageTiling Tiling;
    PImageLayoutMode LayoutMode = PImageLayoutMode::Absolute;
    bool IsOpaque = true;

    bool operator==(const PImageDescription&) const = default;
};

namespace PImageUtils
{
// Slice and filter coordinates use float. Keep source boundaries in its exact
// integer range so rounding cannot select a pixel beyond the source raster.
static constexpr int32_t MAX_EXACT_SOURCE_COORDINATE = 0x01 << std::numeric_limits<float>::digits;

inline bool IsValid(const PImageDescription& image)
{
    const PIRect& source = image.SourceRect;
    const PRect& slice = image.SliceRect;
    return source.IsValid() && source.right <= MAX_EXACT_SOURCE_COORDINATE && source.bottom <= MAX_EXACT_SOURCE_COORDINATE
        && slice.IsValid() && (slice & PRect(source)) == slice
        && slice.GetFloored() == slice && PMath::is_finite(image.LayoutSize.x) && PMath::is_finite(image.LayoutSize.y)
        && image.LayoutSize.x > 0.0f && image.LayoutSize.y > 0.0f;
}

inline std::array<int32_t, 4> SourceAxis(int32_t start, int32_t end, float centerStart, float centerEnd)
{
    return {start, int32_t(centerStart), int32_t(centerEnd), end};
}

// Arrange borders at their native size, compress them proportionally if needed,
// then rasterize the scaled layout. The final boundary always matches the tile.
inline std::array<int32_t, 4> LayoutAxis(float size, int32_t pixels, int32_t leading, int32_t trailing)
{
    const int32_t borderSize = leading + trailing;
    if (borderSize == 0) {
        return {0, 0, pixels, pixels};
    }
    if (float(borderSize) >= size)
    {
        const int32_t first = int32_t(double(leading) * pixels / borderSize);
        return {0, first, first, pixels};
    }
    const float rasterScale = float(pixels) / size;
    const int32_t first = std::min(pixels, int32_t(std::round(float(leading) * rasterScale)));
    const int32_t last = std::max(first, pixels - int32_t(std::round(float(trailing) * rasterScale)));
    return {0, first, last, pixels};
}

inline bool TileHorizontal(PImageTiling tiling, size_t row, size_t column)
{
    if (column != 1) {
        return false;
    }
    return tiling.Has((row == 0) ? PImageTilingFlag::TopEdge
        : (row == 2) ? PImageTilingFlag::BottomEdge : PImageTilingFlag::CenterHorizontal);
}

inline bool TileVertical(PImageTiling tiling, size_t row, size_t column)
{
    if (row != 1) {
        return false;
    }
    return tiling.Has((column == 0) ? PImageTilingFlag::LeftEdge
        : (column == 2) ? PImageTilingFlag::RightEdge : PImageTilingFlag::CenterVertical);
}
}
