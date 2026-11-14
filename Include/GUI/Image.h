// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Bitmap.h>
#include <GUI/ImageTypes.h>
#include <Ptr/Ptr.h>
#include <Signals/Signal.h>

// Describes an image without allocating another raster. Source pixels must
// remain unchanged while used by brushes. Hold the application's mutex when
// changing an image referenced by brushes, as for brush and view operations.
class PImage : public PtrTarget
{
public:
    // The bitmap and source rectangle constructors default to a view-relative
    // layout of (1, 1), filling the view that selects the image brush.
    explicit PImage(Ptr<const PBitmap> bitmap);
    PImage(Ptr<const PBitmap> bitmap, const PIRect& sourceRect);
    PImage(Ptr<const PBitmap> bitmap, const PImageDescription& description);
    PImage(const PImage& source);
    PImage& operator=(const PImage&) = delete;

    PApplication* GetApplication() const noexcept { return m_Bitmap->GetApplication(); }
    Ptr<const PBitmap> GetBitmap() const noexcept { return m_Bitmap; }
    const PImageDescription& GetDescription() const noexcept { return m_Description; }
    // Absolute sizes are scaled with the brush. View-relative sizes specify
    // the final tile size; brush scale still controls borders and repeated texels.
    PPoint GetLayoutSize() const noexcept { return m_Description.LayoutSize; }
    PImageLayoutMode GetLayoutMode() const noexcept { return m_Description.LayoutMode; }
    PRect GetSliceRect() const noexcept { return m_Description.SliceRect; }
    PImageTiling GetTiling() const noexcept { return m_Description.Tiling; }

    // The one-argument setter preserves the current mode. In ViewRelative
    // mode, (1, 1) fills the selecting view and (0.5, 0.5) repeats twice per axis.
    void SetLayoutSize(const PPoint& size);
    void SetLayoutSize(const PPoint& size, PImageLayoutMode mode);
    void SetSliceRect(const PRect& rectangle);
    void SetTiling(PImageTiling tiling);

    // Describe a bitmap without requiring an image object for bitmap brushes.
    static PImageDescription DescribeBitmap(const PBitmap& bitmap, const PIRect& sourceRect);

    Signal<void> SignalChanged;

private:
    static void ClassifyOpacity(const PBitmap& bitmap, PImageDescription& description);
    void SetDescription(PImageDescription description);

    Ptr<const PBitmap> m_Bitmap;
    PImageDescription m_Description;
};
