// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <DeviceControl/Display.h>
#include <GUI/BrushTypes.h>

// Resolved image data is local to the kernel samplers. Selection commands carry
// compact brush values and refer to images registered separately.
struct PDisplayBrushLayer
{
    PDisplayBitmap Bitmap;
    PImageDescription Image;
    PBrushData Data;
    PIRect Bounds;
    PIPoint ReferenceSize;
};

// Layers are ordered from the opaque base towards the drawing surface.
// Data.ImageID names an immutable driver image in this protocol.
struct PDisplayBrushBinding
{
    PBrushData Data;
    PIPoint Origin;
    PIPoint ReferenceSize;

    bool operator==(const PDisplayBrushBinding&) const = default;
};

static constexpr size_t DISPLAY_BRUSH_BLOCK_PIXELS = 1024;

namespace PImageUtils
{
// Raster access rights are checked when the backing bitmap is registered.
inline bool IsValidResource(const PImageDescription& image, const PDisplayBitmap* bitmap)
{
    if (bitmap == nullptr || bitmap->VideoMemory
        || (bitmap->ColorSpace != PEColorSpace::RGB16 && bitmap->ColorSpace != PEColorSpace::RGB32 && bitmap->ColorSpace != PEColorSpace::RGBA32)) {
        return false;
    }
    const PIRect& source = image.SourceRect;
    return source.left >= 0 && source.top >= 0 && source.right <= bitmap->Size.x && source.bottom <= bitmap->Size.y && IsValid(image);
}
}
