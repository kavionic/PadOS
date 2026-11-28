// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <span>

#include <DeviceControl/DisplayBrush.h>
#include <Ptr/Ptr.h>

namespace kernel
{

struct KDisplayBrushAxisMapping
{
    float Scale;
    int32_t DestinationStart;
    int32_t DestinationEnd;
    int32_t SourceStart;
    int32_t SampleMinimum;
    int32_t SampleMaximum;
    int32_t InteriorStart;
    int32_t InteriorEnd;
    bool Direct;
};

struct KDisplayBrushSlice
{
    KDisplayBrushAxisMapping Horizontal{};
    KDisplayBrushAxisMapping Vertical{};
    PIRect Bounds;
    PIPoint Repeat = PIPoint(0, 0);
};

class KDisplayBrushSampler : public PtrTarget
{
public:
    explicit KDisplayBrushSampler(const PDisplayBrushLayer& layer);
    PIPoint GetTileSize() const noexcept { return m_Layer.Bounds.Size(); }
    bool IsOpaque() const noexcept { return m_Layer.Data.IsOpaque(&m_Layer.Image); }
    void SampleSpan(int32_t x, int32_t y, std::span<PColor> pixels) const;

private:
    template<PEColorSpace COLOR_SPACE>
    void SampleBitmapSpan(int32_t x, int32_t y, std::span<PColor> pixels) const;

    PDisplayBrushLayer m_Layer;
    std::array<KDisplayBrushSlice, 9> m_Slices;
    std::array<int32_t, 4> m_HorizontalBoundaries{};
    std::array<int32_t, 4> m_VerticalBoundaries{};
    bool m_DirectBitmap = false;
};

} // namespace kernel
