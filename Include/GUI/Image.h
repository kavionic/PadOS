// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/ImageData.h>

// Image values share their definition until modified. Brushes retain snapshots.
// Source pixels must remain unchanged while referenced by images. Hold the
// owning application's mutex when changing, publishing or releasing images.
class PImage
{
public:
    PImage() noexcept = default;
    // Bitmap and source rectangle constructors fill the selecting view.
    explicit PImage(Ptr<const PBitmap> bitmap);
    PImage(Ptr<const PBitmap> bitmap, const PIRect& sourceRect);
    PImage(Ptr<const PBitmap> bitmap, const PImageDescription& description);

    bool IsValid() const noexcept { return m_Data != nullptr; }
    bool operator==(const PImage&) const noexcept = default;
    PApplication* GetApplication() const noexcept { return IsValid() ? m_Data->GetBitmap()->GetApplication() : nullptr; }
    Ptr<const PBitmap> GetBitmap() const noexcept { return IsValid() ? m_Data->GetBitmap() : nullptr; }
    const PImageDescription& GetDescription() const noexcept;
    PImageID GetImageID() const;
    // Absolute sizes are scaled with the brush. View-relative sizes specify
    // the final tile size; brush scale still controls borders and repeated texels.
    PPoint GetLayoutSize() const noexcept { return GetDescription().LayoutSize; }
    PImageLayoutMode GetLayoutMode() const noexcept { return GetDescription().LayoutMode; }
    PRect GetSliceRect() const noexcept { return GetDescription().SliceRect; }
    PImageTiling GetTiling() const noexcept { return GetDescription().Tiling; }

    // The one-argument setter preserves the current mode. In ViewRelative
    // mode, (1, 1) fills the selecting view and (0.5, 0.5) repeats twice per axis.
    void SetLayoutSize(const PPoint& size);
    void SetLayoutSize(const PPoint& size, PImageLayoutMode mode);
    void SetSliceRect(const PRect& rectangle);
    void SetTiling(PImageTiling tiling);

    static PImageDescription DescribeBitmap(const PBitmap& bitmap, const PIRect& sourceRect);

private:
    static void ClassifyOpacity(const PBitmap& bitmap, PImageDescription& description);
    void SetDescription(const PImageDescription& description);

    Ptr<PImageData> m_Data;
};
