// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <DeviceControl/Display.h>
#include <GUI/BrushTypes.h>

// Layers are ordered from the opaque base towards the drawing surface.
// Resolved rendering data used by the kernel samplers.
struct PDisplayBrushLayer
{
    PDisplayBitmap Bitmap;
    PBrushDescription Description;
    PIRect Bounds;
};

// An invalid brush ID supplies a constant layer produced by composition.
// Resource descriptions are transferred only at creation or explicit updates.
struct PDisplayBrushBinding
{
    PDriverResourceID DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    PColor Color;
    PIRect Bounds;

    bool operator==(const PDisplayBrushBinding&) const = default;
};

static constexpr size_t DISPLAY_BRUSH_BLOCK_PIXELS = 1024;

namespace PBrushUtils
{
// Creation and property updates share metadata validation. Raster access rights
// are checked separately, once when the backing bitmap is registered.
inline bool IsValidResource(const PBrushDescription& description, const PDisplayBitmap* bitmap)
{
    if (description.Type == PBrushType::Solid) {
        return bitmap == nullptr;
    }
    if (bitmap == nullptr || bitmap->VideoMemory
        || (bitmap->ColorSpace != PEColorSpace::RGB16 && bitmap->ColorSpace != PEColorSpace::RGB32 && bitmap->ColorSpace != PEColorSpace::RGBA32)
        || std::to_underlying(description.Type) > std::to_underlying(PBrushType::NineSlice)) {
        return false;
    }
    const PIRect& source = description.SourceRect;
    const PBrushBorders& borders = description.Borders;
    return source.IsValid() && source.left >= 0 && source.top >= 0 && source.right <= bitmap->Size.x && source.bottom <= bitmap->Size.y
        && borders.Left >= 0 && borders.Right >= 0 && borders.Top >= 0 && borders.Bottom >= 0
        && int64_t(borders.Left) + borders.Right < source.Width() && int64_t(borders.Top) + borders.Bottom < source.Height();
}
}
