// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <utility>

#include <GUI/Styles/Skinned/SkinnedImage.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PBrush PGetSkinnedBrush(PSkinnedImage& cache, const PImage& source, const PPoint& size)
{
    if (!cache.Image.IsValid() || cache.Image.GetLayoutSize() != size)
    {
        PImage image = cache.Image.IsValid() ? cache.Image : source;
        image.SetLayoutSize(size, PImageLayoutMode::Absolute);
        PBrush brush(image);
        cache.Image = std::move(image);
        cache.Brush = std::move(brush);
    }
    return cache.Brush;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PInsetSkinnedFrame(const PRect& frame, const PRect& insets)
{
    PRect result = frame.GetResized(insets.left, insets.top, -insets.right, -insets.bottom);
    result.left = std::min(result.left, frame.right);
    result.top = std::min(result.top, frame.bottom);
    result.right = std::max(result.left, result.right);
    result.bottom = std::max(result.top, result.bottom);
    return result;
}

