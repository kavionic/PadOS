// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Bitmap.h>
#include <GUI/BrushTypes.h>
#include <Ptr/Ptr.h>
#include <Ptr/WeakPtr.h>

// Property changes are ordered with drawing commands. Bitmap rasters remain
// immutable while referenced; use SetBitmap() to publish different artwork.
class PBrush : public PtrTarget
{
public:
    // Solid brushes default to the current application; bitmap brushes use its owner.
    // Hold the application mutex while creating, binding, or releasing a brush.
    explicit PBrush(PColor color, PApplication* application = nullptr);
    PBrush(Ptr<const PBitmap> bitmap, const PIRect& sourceRect, PBrushType type = PBrushType::BitmapStretch, PBrushBorders borders = {});

    ~PBrush();
    PBrush(const PBrush&) = delete;
    PBrush& operator=(const PBrush&) = delete;

    PApplication*               GetApplication() const;
    handle_id                   GetHandle() const noexcept { return m_Handle; }
    const PBrushDescription&    GetDescription() const noexcept { return m_Description; }
    Ptr<const PBitmap>          GetBitmap() const noexcept { return m_Bitmap; }

    // Hold the owning application mutex, as for view drawing commands.
    void SetColor(PColor color);
    void SetBitmap(Ptr<const PBitmap> bitmap, const PIRect& sourceRect, PBrushType type = PBrushType::BitmapStretch, PBrushBorders borders = {});

private:
    static PBrushDescription DescribeBitmap(const PBitmap& bitmap, const PIRect& sourceRect, PBrushType type, PBrushBorders borders);
    void SetProperties(const PBrushDescription& description, Ptr<const PBitmap> bitmap);

    Ptr<const PBitmap>      m_Bitmap;
    PBrushDescription       m_Description;
    WeakPtr<PApplication>   m_Application;
    handle_id               m_Handle = INVALID_HANDLE;
};

struct PViewBrushBinding
{
    Ptr<const PBrush>   Brush;
    PRect               Bounds;
    PBrushSource        Source = PBrushSource::Inherit;
    bool                AutomaticBounds = true;
};
