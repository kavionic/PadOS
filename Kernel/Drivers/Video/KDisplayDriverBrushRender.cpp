// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <Kernel/Drivers/Video/KDisplayDriver.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// Sample the resolved background in destination coordinates, preserving the
/// brush mapping when clipped. Render into fixed-capacity blocks and pass each
/// completed block to the destination-specific writer.
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::FillBrushRect(PDisplayBitmap* bitmap, const PIRect& bounds)
{
    const PIRect clipped = bounds & PIRect(PIPoint(0, 0), bitmap->Size);
    if (m_Brush.IsEmpty() || !clipped.IsValid()) {
        return;
    }
    for (int32_t top = clipped.top; top < clipped.bottom;)
    {
        const int32_t width = std::min<int32_t>(clipped.Width(), int32_t(DISPLAY_BRUSH_BLOCK_PIXELS));
        const int32_t height = std::min<int32_t>(clipped.bottom - top, int32_t(DISPLAY_BRUSH_BLOCK_PIXELS / size_t(width)));
        for (int32_t left = clipped.left; left < clipped.right; left += width)
        {
            const PIRect block(left, top, std::min<int32_t>(left + width, clipped.right), top + height);
            const size_t rowWidth = size_t(block.Width());
            for (int32_t y = block.top; y < block.bottom; ++y) {
                m_Brush.RenderSpan(block.left, y, std::span(m_BrushPixels).subspan(size_t(y - block.top) * rowWidth, rowWidth));
            }
            WriteBrushBlock(bitmap, block, std::span(m_BrushPixels).first(rowWidth * size_t(height)));
        }
        top += height;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Write an already-composited block to RAM using a compile-time pixel format.
/// Convert visible colors without reading destination pixels; zero-alpha
/// entries are skip markers and leave the destination unchanged.
///////////////////////////////////////////////////////////////////////////////

template<PEColorSpace COLOR_SPACE>
static void WriteBrushPixels(PDisplayBitmap& bitmap, const PIRect& bounds, std::span<const PColor> pixels)
{
    size_t index = 0;
    for (int32_t y = bounds.top; y < bounds.bottom; ++y)
    {
        uint8_t* row = bitmap.Raster + size_t(y) * bitmap.BytesPerLine;
        for (int32_t x = bounds.left; x < bounds.right; ++x)
        {
            const PColor color = pixels[index++];
            if (color.GetAlpha() != 0)
            {
                if constexpr (COLOR_SPACE == PEColorSpace::RGB15) {
                    reinterpret_cast<uint16_t*>(row)[x] = color.GetColor15();
                } else if constexpr (COLOR_SPACE == PEColorSpace::RGB16) {
                    reinterpret_cast<uint16_t*>(row)[x] = color.GetColor16();
                } else {
                    reinterpret_cast<uint32_t*>(row)[x] = color.GetColor32();
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Dispatch a prepared block to the RAM writer for the bitmap's color space.
/// Format selection occurs once per block, outside the pixel loops. Display
/// drivers override this method to transfer blocks to video memory.
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::WriteBrushBlock(PDisplayBitmap* bitmap, const PIRect& bounds, std::span<const PColor> pixels)
{
    if (bitmap->VideoMemory) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    switch (bitmap->ColorSpace)
    {
        case PEColorSpace::RGB15:
            WriteBrushPixels<PEColorSpace::RGB15>(*bitmap, bounds, pixels);
            break;
        case PEColorSpace::RGB16:
            WriteBrushPixels<PEColorSpace::RGB16>(*bitmap, bounds, pixels);
            break;
        case PEColorSpace::RGB32:
        case PEColorSpace::RGBA32:
            WriteBrushPixels<PEColorSpace::RGB32>(*bitmap, bounds, pixels);
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Rasterize clipped text spans in fixed-capacity blocks, sampling the brush
/// at each destination position. Glyph coverage scales the foreground alpha
/// before compositing over the sampled background. Overlay skips zero-alpha
/// glyph pixels; Copy writes the background there, without display readback.
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::WriteBrushString(
    PDisplayBitmap* bitmap,
    const PIPoint& position,
    std::string_view text,
    const PDisplayFontCache& font,
    std::span<const int32_t> kernings,
    std::span<const PIRect> clipRects,
    PColor foreground,
    PPoint advance,
    PDrawingMode mode,
    float width)
{
    if (mode == PDrawingMode::Overlay && foreground.GetAlpha() == 0) {
        return;
    }
    const PIRect bitmapBounds(PIPoint(0, 0), bitmap->Size);
    m_FontText.Begin(font, text, position, bitmapBounds, kernings, advance, mode, width);
    PScopeExit releaseText([this] { m_FontText.End(); });
    while (m_FontText.NextSpan())
    {
        for (const PIRect& clip : clipRects)
        {
            const PIRect bounds = m_FontText.GetSpan().Bounds & clip & bitmapBounds;
            if (!bounds.IsValid()) {
                continue;
            }
            for (int32_t top = bounds.top; top < bounds.bottom;)
            {
                const int32_t blockWidth = std::min<int32_t>(bounds.Width(), int32_t(DISPLAY_BRUSH_BLOCK_PIXELS));
                const int32_t height = std::min<int32_t>(bounds.bottom - top, int32_t(DISPLAY_BRUSH_BLOCK_PIXELS / size_t(blockWidth)));
                for (int32_t left = bounds.left; left < bounds.right; left += blockWidth)
                {
                    const PIRect block(left, top, std::min<int32_t>(left + blockWidth, bounds.right), top + height);
                    const size_t rowWidth = size_t(block.Width());
                    for (int32_t y = block.top; y < block.bottom; ++y)
                    {
                        const std::span<PColor> row = std::span(m_BrushPixels).subspan(size_t(y - block.top) * rowWidth, rowWidth);
                        m_Brush.RenderSpan(block.left, y, row);
                        const KDisplayTextGlyph* single = m_FontText.GetSpan().SingleGlyph;
                        const uint8_t* source = nullptr;
                        std::span<const uint8_t*> sources;
                        if (single != nullptr)
                        {
                            source = single->Glyph->Raster + size_t(y - single->Bounds.top) * size_t(single->Bounds.Width())
                                + size_t(block.left - single->Bounds.left);
                        }
                        else if (m_FontText.GetSpan().HasGlyphs)
                        {
                            sources = m_FontText.GetRowSources(block.left, y);
                        }
                        for (PColor& pixel : row)
                        {
                            const uint8_t coverage = (source != nullptr) ? *source++ : KDisplayText::ReadCoverage(sources);
                            const uint8_t alpha = uint8_t((uint32_t(coverage) * foreground.GetAlpha() + 127) / 255);
                            if (mode == PDrawingMode::Overlay && alpha == 0)
                            {
                                pixel = PColor(0, 0, 0, 0);
                            }
                            else
                            {
                                const PColor covered(foreground.GetRed(), foreground.GetGreen(), foreground.GetBlue(), alpha);
                                pixel = KDisplayBrush::Composite(covered, pixel);
                            }
                        }
                    }
                    WriteBrushBlock(bitmap, block, std::span(m_BrushPixels).first(rowWidth * size_t(height)));
                }
                top += height;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Prepare the source mapping once, then sample source and background rows in
/// fixed-capacity blocks using destination coordinates. Composite each source
/// sample over the background; Overlay leaves transparent source pixels
/// unwritten, while Copy writes the background through them.
///////////////////////////////////////////////////////////////////////////////

void KDisplayDriver::DrawBrushBitmap(PDisplayBitmap* destination, const PDisplayBrushLayer& source, const PIRect& bounds, PDrawingMode mode)
{
    if (mode != PDrawingMode::Copy && mode != PDrawingMode::Overlay) {
        PERROR_THROW_CODE(PErrorCode::NOSYS);
    }
    const PIRect clipped = bounds & PIRect(PIPoint(0, 0), destination->Size);
    if (!clipped.IsValid()) {
        return;
    }
    const KDisplayBrushSampler sourceSampler(source);
    for (int32_t top = clipped.top; top < clipped.bottom;)
    {
        const int32_t width = std::min<int32_t>(clipped.Width(), int32_t(DISPLAY_BRUSH_BLOCK_PIXELS));
        const int32_t height = std::min<int32_t>(clipped.bottom - top, int32_t(DISPLAY_BRUSH_BLOCK_PIXELS / size_t(width)));
        for (int32_t left = clipped.left; left < clipped.right; left += width)
        {
            const PIRect block(left, top, std::min<int32_t>(left + width, clipped.right), top + height);
            const size_t rowWidth = size_t(block.Width());
            for (int32_t y = block.top; y < block.bottom; ++y)
            {
                const std::span<PColor> row = std::span(m_BrushPixels).subspan(size_t(y - block.top) * rowWidth, rowWidth);
                const std::span<PColor> sourceRow = std::span(m_SourcePixels).first(rowWidth);
                m_Brush.RenderSpan(block.left, y, row);
                sourceSampler.SampleSpan(block.left, y, sourceRow);
                for (size_t i = 0; i < rowWidth; ++i)
                {
                    if (mode == PDrawingMode::Overlay && sourceRow[i].GetAlpha() == 0) {
                        row[i] = PColor(0, 0, 0, 0);
                    } else {
                        row[i] = KDisplayBrush::Composite(sourceRow[i], row[i]);
                    }
                }
            }
            WriteBrushBlock(destination, block, std::span(m_BrushPixels).first(rowWidth * size_t(height)));
        }
        top += height;
    }
}

} // namespace kernel
