// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/BrushTypes.h>
#include <GUI/Image.h>
#include <Ptr/WeakPtr.h>

// Brushes own their source, style and scale. Views supply only an origin.
// Changes are queued with drawing commands. Hold the owning application's
// mutex when creating, changing, binding or releasing brushes and images.
class PBrush : public PtrTarget, public SignalTarget
{
public:
    explicit PBrush(PColor color, PApplication* application = nullptr);
    explicit PBrush(PBrushStyle style, PApplication* application = nullptr);
    explicit PBrush(Ptr<const PBitmap> bitmap);
    PBrush(Ptr<const PBitmap> bitmap, const PIRect& sourceRect);
    explicit PBrush(Ptr<const PImage> image);

    ~PBrush();
    PBrush(const PBrush&) = delete;
    PBrush& operator=(const PBrush&) = delete;

    PApplication* GetApplication() const;
    handle_id GetHandle() const noexcept { return m_Handle; }
    const PBrushDescription& GetDescription() const noexcept { return m_Description; }
    PBrushType GetType() const noexcept { return m_Description.Type; }
    PBrushStyle GetStyle() const noexcept { return m_Description.Style; }
    PPoint GetScale() const noexcept { return m_Description.Scale; }
    Ptr<const PBitmap> GetBitmap() const noexcept { return m_Bitmap; }
    Ptr<const PImage> GetImage() const noexcept { return m_Image; }

    void SetColor(PColor color);
    void SetBitmap(Ptr<const PBitmap> bitmap);
    void SetBitmap(Ptr<const PBitmap> bitmap, const PIRect& sourceRect);
    void SetImage(Ptr<const PImage> image);
    void SetStyle(PBrushStyle style);
    void SetScale(const PPoint& scale);

private:
    void Create(PApplication* application);
    void ImageChanged();
    void SetProperties(const PBrushDescription& description, Ptr<const PBitmap> bitmap, Ptr<const PImage> image);

    Ptr<const PBitmap> m_Bitmap;
    Ptr<const PImage> m_Image;
    PBrushDescription m_Description;
    WeakPtr<PApplication> m_Application;
    handle_id m_Handle = INVALID_HANDLE;
};
