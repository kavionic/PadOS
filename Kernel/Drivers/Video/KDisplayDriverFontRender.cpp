// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <Kernel/Drivers/Video/KDisplayDriver.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

namespace kernel
{

template<typename PixelType, bool OVERLAY, typename CoverageReader>
static void WriteFontBitmapRow(
    PixelType* destination,
    int width,
    const std::array<uint32_t, 256>& palette,
    CoverageReader readCoverage)
{
    for (int x = 0; x < width; ++x)
    {
        const uint8_t coverage = readCoverage();
        if (!OVERLAY || coverage != 0) {
            *destination = PixelType(palette[coverage]);
        }
        ++destination;
    }
}

template<typename PixelType, bool OVERLAY>
static void WriteFontBitmapSpan(
    PDisplayBitmap& bitmap,
    KDisplayText& text,
    const PIRect& bounds,
    const std::array<uint32_t, 256>& palette)
{
    uint8_t* row = bitmap.Raster + size_t(bounds.top) * bitmap.BytesPerLine;
    const KDisplayTextGlyph* singleGlyph = text.GetSpan().SingleGlyph;
    if (singleGlyph != nullptr)
    {
        const size_t stride = size_t(singleGlyph->Bounds.Width());
        const size_t column = size_t(bounds.left - singleGlyph->Bounds.left);
        const uint8_t* sourceRow = singleGlyph->Glyph->Raster + size_t(bounds.top - singleGlyph->Bounds.top) * stride;
        for (int y = bounds.top; y < bounds.bottom; ++y)
        {
            PixelType* destination = reinterpret_cast<PixelType*>(row) + bounds.left;
            const uint8_t* source = sourceRow + column;
            WriteFontBitmapRow<PixelType, OVERLAY>(
                destination,
                bounds.Width(),
                palette,
                [source]() mutable { return *source++; });
            sourceRow += stride;
            row += bitmap.BytesPerLine;
        }
        return;
    }
    for (int y = bounds.top; y < bounds.bottom; ++y)
    {
        PixelType* destination = reinterpret_cast<PixelType*>(row) + bounds.left;
        if (text.GetSpan().HasGlyphs)
        {
            const std::span<const uint8_t*> sources = text.GetRowSources(bounds.left, y);
            if (sources.size() == 1)
            {
                const uint8_t* source = sources[0];
                WriteFontBitmapRow<PixelType, OVERLAY>(
                    destination,
                    bounds.Width(),
                    palette,
                    [source]() mutable { return *source++; });
            }
            else
            {
                WriteFontBitmapRow<PixelType, OVERLAY>(
                    destination,
                    bounds.Width(),
                    palette,
                    [sources] { return KDisplayText::ReadCoverage(sources); });
            }
        }
        else if constexpr (!OVERLAY)
        {
            std::fill_n(destination, size_t(bounds.Width()), PixelType(palette[0]));
        }
        row += bitmap.BytesPerLine;
    }
}

void KDisplayDriver::WriteString(
    PDisplayBitmap* bitmap,
    const PIPoint& position,
    std::string_view text,
    const PDisplayFontCache& font,
    std::span<const int32_t> kernings,
    std::span<const PIRect> clipRects,
    PColor background,
    PColor foreground,
    PPoint advance,
    PDrawingMode mode,
    float width)
{
    kassert(m_Mutex.IsLocked());
    if (mode != PDrawingMode::Copy && mode != PDrawingMode::Overlay) {
        PERROR_THROW_CODE(PErrorCode::NOSYS);
    }
    if (bitmap->VideoMemory) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if ((text.empty() && !PMath::is_finite(width)) || clipRects.empty()) {
        return;
    }
    if (mode == PDrawingMode::Overlay && foreground.GetAlpha() == 0) {
        return;
    }
    const std::array<uint32_t, 256>& palette = PrepareFontPalette(bitmap->ColorSpace, background, foreground);
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
            switch (bitmap->ColorSpace)
            {
                case PEColorSpace::RGB15:
                case PEColorSpace::RGB16:
                    if (mode == PDrawingMode::Overlay) {
                        WriteFontBitmapSpan<uint16_t, true>(*bitmap, m_FontText, bounds, palette);
                    } else {
                        WriteFontBitmapSpan<uint16_t, false>(*bitmap, m_FontText, bounds, palette);
                    }
                    break;
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                    if (mode == PDrawingMode::Overlay) {
                        WriteFontBitmapSpan<uint32_t, true>(*bitmap, m_FontText, bounds, palette);
                    } else {
                        WriteFontBitmapSpan<uint32_t, false>(*bitmap, m_FontText, bounds, palette);
                    }
                    break;
                default:
                    PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        }
    }
}

const std::array<uint32_t, 256>& KDisplayDriver::PrepareFontPalette(
    PEColorSpace colorSpace,
    PColor background,
    PColor foreground,
    std::optional<uint32_t> colorKey)
{
    kassert(m_Mutex.IsLocked());
    if (!m_FontPaletteValid || m_FontColorSpace != colorSpace
        || m_FontBackground != background || m_FontForeground != foreground)
    {
        for (size_t coverage = 0; coverage < m_FontPalette.size(); ++coverage)
        {
            const uint32_t alpha = (uint32_t(coverage) * foreground.GetAlpha() + 127) / 255;
            const uint32_t inverse = 255 - alpha;
            const PColor color(
                uint8_t((foreground.GetRed() * alpha + background.GetRed() * inverse + 127) / 255),
                uint8_t((foreground.GetGreen() * alpha + background.GetGreen() * inverse + 127) / 255),
                uint8_t((foreground.GetBlue() * alpha + background.GetBlue() * inverse + 127) / 255));
            switch (colorSpace)
            {
                case PEColorSpace::RGB15:
                    m_FontPalette[coverage] = color.GetColor15();
                    break;
                case PEColorSpace::RGB16:
                    m_FontPalette[coverage] = color.GetColor16();
                    break;
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                    m_FontPalette[coverage] = color.GetColor32();
                    break;
                default:
                    PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        }
        m_FontBackgroundPixel = m_FontPalette[0];
        m_FontBackground = background;
        m_FontForeground = foreground;
        m_FontColorSpace = colorSpace;
        m_FontPaletteValid = true;
    }
    m_FontPalette[0] = colorKey.value_or(m_FontBackgroundPixel);
    return m_FontPalette;
}

} // namespace kernel
