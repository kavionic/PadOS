// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

#include <ApplicationServer/ServerImage.h>
#include <GUI/BrushTypes.h>

class PServerBrush
{
public:
    PServerBrush() noexcept;
    explicit PServerBrush(PColor color) noexcept;
    explicit PServerBrush(const PBrushData& data, Ptr<PServerImage> image = nullptr);

    const PBrushData& GetData() const noexcept { return m_Data; }
    const Ptr<PServerImage>& GetImage() const noexcept { return m_Image; }
    bool IsOpaque() const noexcept { return m_Data.IsOpaque((m_Image != nullptr) ? &m_Image->GetDescription() : nullptr); }
    bool operator==(const PServerBrush&) const = default;

private:
    PBrushData m_Data;
    Ptr<PServerImage> m_Image;
};

struct PServerBrushBinding
{
    PServerBrush Brush;
    PPoint Origin;
};

struct PServerBrushLayer
{
    PServerBrush Brush;
    PIRect Bounds;
    PIPoint ReferenceSize;

    bool CanTranslateFrom(const PServerBrushLayer& previous, const PIPoint& movement) const noexcept
    {
        return Brush == previous.Brush && PBrushUtils::CanTranslate(Brush.GetData(), previous.Bounds, Bounds, movement);
    }
    bool operator==(const PServerBrushLayer&) const = default;
};

// A constant composition needs no layer storage and uses the native solid paths.
struct PServerBrushState
{
    std::vector<PServerBrushLayer> Layers;
    PColor Color = PColor(255, 255, 255);
    bool IsSolid() const noexcept { return Layers.empty(); }
};
