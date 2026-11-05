// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <span>
#include <vector>
#include <DeviceControl/DisplayBrush.h>

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

class KDisplayBrushSampler
{
public:
    explicit KDisplayBrushSampler(const PDisplayBrushLayer& layer);
    void SampleSpan(int32_t x, int32_t y, std::span<PColor> pixels) const;

private:
    template<PEColorSpace COLOR_SPACE>
    void SampleBitmapSpan(int32_t x, int32_t y, std::span<PColor> pixels) const;

    PDisplayBrushLayer m_Layer;
    std::array<KDisplayBrushAxisMapping, 3> m_HorizontalMappings{};
    std::array<KDisplayBrushAxisMapping, 3> m_VerticalMappings{};
    size_t m_AxisCount;
};

class KDisplayBrush
{
public:
    void SetLayers(std::span<const PDisplayBrushLayer> layers);
    void Clear() noexcept { m_Layers.clear(); }
    bool IsEmpty() const noexcept { return m_Layers.empty(); }
    void RenderSpan(int32_t x, int32_t y, std::span<PColor> pixels) const;

    static PColor Composite(PColor foreground, PColor background);
    static void SampleSpan(const PDisplayBrushLayer& layer, int32_t x, int32_t y, std::span<PColor> pixels);
    static uint16_t FindTransparencyKey(std::span<const PColor> pixels);

private:
    std::vector<KDisplayBrushSampler> m_Layers;
    mutable std::array<PColor, DISPLAY_BRUSH_BLOCK_PIXELS> m_LayerPixels;
};

} // namespace kernel
