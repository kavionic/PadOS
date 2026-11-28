// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/BrushSharedData.h>

// Copies share brush data until modified. Views retain the assigned
// snapshot; changing a value requires assigning it again to update a view.
// Hold the application's mutex when changing, binding or releasing values.
class PBrush
{
public:
    static const PBrush zero;

    PBrush() noexcept = default;
    explicit PBrush(PColor color, PApplication* application = nullptr);
    explicit PBrush(PBrushStyle style, PApplication* application = nullptr);
    explicit PBrush(Ptr<const PBitmap> bitmap);
    PBrush(Ptr<const PBitmap> bitmap, const PIRect& sourceRect);
    explicit PBrush(const PImage& image);

    bool IsValid() const noexcept { return m_Data != nullptr; }
    bool operator==(const PBrush& rhs) const noexcept;
    PApplication* GetApplication() const;
    PBrushType GetType() const noexcept { return (m_Data != nullptr) ? m_Data->GetData().Type : PBrushType::Solid; }
    PBrushStyle GetStyle() const noexcept { return (m_Data != nullptr) ? m_Data->GetData().Style : PBrushStyle::None; }
    PColor GetColor() const noexcept { return (m_Data != nullptr) ? m_Data->GetData().Color : PColor(255, 255, 255); }
    PPoint GetScale() const noexcept { return (m_Data != nullptr) ? m_Data->GetData().Scale : PPoint(1.0f, 1.0f); }
    Ptr<const PBitmap> GetBitmap() const noexcept { return (m_Data != nullptr) ? m_Data->GetImage().GetBitmap() : nullptr; }
    PImage GetImage() const noexcept { return (m_Data != nullptr) ? m_Data->GetImage() : PImage(); }
    bool IsOpaque() const noexcept;

    void SetColor(PColor color);
    void SetBitmap(Ptr<const PBitmap> bitmap);
    void SetBitmap(Ptr<const PBitmap> bitmap, const PIRect& sourceRect);
    void SetImage(const PImage& image);
    void SetStyle(PBrushStyle style);
    void SetScale(const PPoint& scale);

private:
    friend class PApplication;
    friend class PView;

    // Publish an image once, in queue order before the command using it.
    PBrushData GetServerData() const;
    void Create(PApplication* application, const PBrushData& data, const PImage& image = PImage());
    void SetProperties(const PBrushData& data, const PImage& image);

    Ptr<PBrushSharedData> m_Data;
};
