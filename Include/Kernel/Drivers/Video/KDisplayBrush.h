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

#include <Kernel/Drivers/Video/KDisplayBrushSampler.h>

namespace kernel
{

struct KDisplayBrushLayer
{
    Ptr<const KDisplayBrushSampler> Sampler;
    PIPoint Origin;
    PColor Color;
    PBrushStyle Style = PBrushStyle::SolidPattern;
};

class KDisplayBrush
{
public:
    void SetLayers(std::span<const KDisplayBrushLayer> layers);
    void Clear() noexcept { m_Layers.clear(); }
    bool IsEmpty() const noexcept { return m_Layers.empty(); }
    void RenderSpan(int32_t x, int32_t y, std::span<PColor> pixels) const;

    static PColor Composite(PColor foreground, PColor background);
    static void SampleSpan(const PDisplayBrushLayer& layer, int32_t x, int32_t y, std::span<PColor> pixels);
    static uint16_t FindTransparencyKey(std::span<const PColor> pixels);

private:
    std::vector<KDisplayBrushLayer> m_Layers;
    mutable std::array<PColor, DISPLAY_BRUSH_BLOCK_PIXELS> m_LayerPixels;
};

} // namespace kernel
