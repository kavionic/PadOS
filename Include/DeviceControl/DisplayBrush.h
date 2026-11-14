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
    PIPoint ReferenceSize;
};

// An invalid brush ID supplies a constant layer produced by composition.
// Resource descriptions are transferred only at creation or explicit updates.
// Retain the selecting view's size so queued image changes can resolve again;
// the driver calculates tile bounds from this size and the current resource.
struct PDisplayBrushBinding
{
    PDriverResourceID DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    PColor Color;
    PIPoint Origin;
    PIPoint ReferenceSize;

    bool operator==(const PDisplayBrushBinding&) const = default;
};

static constexpr size_t DISPLAY_BRUSH_BLOCK_PIXELS = 1024;

namespace PBrushUtils
{
// Validate only the selected sampler's inputs. Raster access rights are checked
// once when the backing bitmap is registered.
inline bool IsValidResource(const PBrushDescription& description, const PDisplayBitmap* bitmap)
{
    switch (description.Style)
    {
        case PBrushStyle::None:
        case PBrushStyle::SolidPattern:
            return true;
        case PBrushStyle::TexturePattern:
            break;
        default:
            return false;
    }
    if (bitmap == nullptr || bitmap->VideoMemory
        || (bitmap->ColorSpace != PEColorSpace::RGB16 && bitmap->ColorSpace != PEColorSpace::RGB32 && bitmap->ColorSpace != PEColorSpace::RGBA32)) {
        return false;
    }
    const PIRect& source = description.Image.SourceRect;
    return source.left >= 0 && source.top >= 0 && source.right <= bitmap->Size.x && source.bottom <= bitmap->Size.y
        && PImageUtils::IsValid(description.Image) && IsValidScale(description);
}
}
