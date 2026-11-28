// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <bitset>

#include <Kernel/Drivers/Video/KDisplayBrush.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

void KDisplayBrush::SetLayers(std::span<const KDisplayBrushLayer> layers)
{
    m_Layers.assign(layers.begin(), layers.end());
}

void KDisplayBrush::RenderSpan(int32_t x, int32_t y, std::span<PColor> pixels) const
{
    if (pixels.size() > m_LayerPixels.size()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    // Keep uniform backgrounds as a color until a texture needs per-pixel data.
    PColor solidBackground(255, 255, 255);
    bool hasPixelBackground = false;
    const std::span<PColor> layerPixels(m_LayerPixels.data(), pixels.size());
    for (const KDisplayBrushLayer& layer : m_Layers)
    {
        if (layer.Style == PBrushStyle::None) {
            continue;
        }
        if (layer.Style == PBrushStyle::SolidPattern)
        {
            if (!hasPixelBackground || layer.Color.GetAlpha() == 255)
            {
                solidBackground = Composite(layer.Color, solidBackground);
                hasPixelBackground = false;
            }
            else if (layer.Color.GetAlpha() != 0)
            {
                for (PColor& pixel : pixels) {
                    pixel = Composite(layer.Color, pixel);
                }
            }
        }
        else
        {
            const bool isOpaque = layer.Sampler->IsOpaque();
            if (!hasPixelBackground || isOpaque)
            {
                layer.Sampler->SampleSpan(x - layer.Origin.x, y - layer.Origin.y, pixels);
                if (!hasPixelBackground && !isOpaque)
                {
                    for (PColor& pixel : pixels) {
                        pixel = Composite(pixel, solidBackground);
                    }
                }
                hasPixelBackground = true;
            }
            else
            {
                layer.Sampler->SampleSpan(x - layer.Origin.x, y - layer.Origin.y, layerPixels);
                for (size_t i = 0; i < pixels.size(); ++i) {
                    pixels[i] = Composite(layerPixels[i], pixels[i]);
                }
            }
        }
    }
    if (!hasPixelBackground) {
        std::fill(pixels.begin(), pixels.end(), solidBackground);
    }
}

PColor KDisplayBrush::Composite(PColor foreground, PColor background)
{
    return PBrushUtils::Composite(foreground, background);
}

void KDisplayBrush::SampleSpan(const PDisplayBrushLayer& layer, int32_t x, int32_t y, std::span<PColor> pixels)
{
    KDisplayBrushSampler(layer).SampleSpan(x, y, pixels);
}

uint16_t KDisplayBrush::FindTransparencyKey(std::span<const PColor> pixels)
{
    if (pixels.size() > DISPLAY_BRUSH_BLOCK_PIXELS) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    std::bitset<DISPLAY_BRUSH_BLOCK_PIXELS + 1> used;
    for (PColor color : pixels)
    {
        const uint16_t pixel = color.GetColor16();
        if (color.GetAlpha() != 0 && pixel < used.size()) {
            used.set(pixel);
        }
    }
    size_t key = 0;
    while (used.test(key)) {
        ++key;
    }
    return uint16_t(key);
}

} // namespace kernel
