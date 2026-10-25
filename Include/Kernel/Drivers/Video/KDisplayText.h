// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <DeviceControl/DisplayFont.h>
#include <GUI/GUIDefines.h>

namespace kernel
{

struct KDisplayTextGlyph
{
    const PDisplayFontGlyph* Glyph = nullptr;
    PIRect Bounds;
};

struct KDisplayTextSpan
{
    PIRect Bounds;
    const KDisplayTextGlyph* SingleGlyph = nullptr;
    bool HasGlyphs = false;
};

// Layout retains glyph positions and an index for sweeping their bounds; it
// never stores pixel coverage. Spans partition the text area so every pixel
// receives its final colour once, including pixels shared by several glyphs.
class KDisplayText
{
public:
    void Begin(
        const PDisplayFontCache& font,
        std::string_view text,
        const PIPoint& position,
        const PIRect& bitmapBounds,
        std::span<const int32_t> kernings = {},
        PPoint advance = PPoint(),
        PDrawingMode mode = PDrawingMode::Overlay);
    void End() noexcept;
    bool NextSpan();
    const KDisplayTextSpan& GetSpan() const noexcept { return m_Span; }
    std::span<const KDisplayTextGlyph> GetGlyphs() const noexcept { return m_Glyphs; }
    std::span<const uint8_t*> GetRowSources(int x, int y);

    // Every source covers the full width of the current span. Advance sources
    // together, combining coverage in string order with the existing rounding.
    static uint8_t ReadCoverage(std::span<const uint8_t*> sources) noexcept
    {
        uint32_t coverage = 0;
        for (const uint8_t*& source : sources)
        {
            const uint32_t alpha = *source++;
            coverage += alpha - (coverage * alpha + 127) / 255;
        }
        return uint8_t(coverage);
    }

private:
    struct Interval
    {
        bool operator==(const Interval&) const = default;
        int Start = 0;
        int End = 0;
        bool HasGlyphs = false;
    };

    struct LogicalEdge
    {
        float Start = 0.0f;
        float End = 0.0f;
        float Origin = 0.0f;
        float Slope = 0.0f;
    };

    bool LayoutGlyphs(
        const PDisplayFontCache& font,
        std::string_view text,
        const PIPoint& position,
        std::span<const int32_t> kernings);
    bool AppendGlyph(const PDisplayFontGlyph& glyph, PPoint position);
    void InitializeLogicalArea(const PDisplayFontCache& font, const PIPoint& position, PPoint advance);
    bool BeginStrip();
    void BuildIntervals();
    void BuildSimpleIntervals(bool includeBackground);
    bool NextBackgroundSpan();
    void BuildBackgroundIntervals(int position, std::vector<Interval>& intervals) const;
    bool GetLogicalInterval(int position, int& start, int& end) const noexcept;
    int GetStart(const PIRect& bounds) const noexcept;
    int GetEnd(const PIRect& bounds) const noexcept;
    int GetSecondaryStart(const PIRect& bounds) const noexcept;
    int GetSecondaryEnd(const PIRect& bounds) const noexcept;
    PIRect MakeBounds(int start, int end, int secondaryStart, int secondaryEnd) const noexcept;

    std::vector<KDisplayTextGlyph> m_Glyphs;
    std::vector<size_t> m_GlyphOrder;
    std::vector<size_t> m_ActiveGlyphs;
    std::vector<const uint8_t*> m_RowSources;
    std::vector<int> m_IntervalEdges;
    std::vector<Interval> m_Intervals;
    std::vector<Interval> m_BackgroundIntervals;
    std::vector<Interval> m_NextBackgroundIntervals;
    std::array<LogicalEdge, 4> m_LogicalEdges;
    PIRect m_BitmapBounds;
    PIRect m_LogicalBounds;
    KDisplayTextSpan m_Span;
    size_t m_NextGlyph = 0;
    size_t m_IntervalIndex = 0;
    size_t m_BackgroundIntervalIndex = 0;
    int m_StripStart = 0;
    int m_StripEnd = 0;
    int m_BackgroundStart = 0;
    int m_BackgroundEnd = 0;
    int m_BackgroundPosition = 0;
    bool m_Vertical = false;
    bool m_Copy = false;
    bool m_LogicalAxisAligned = true;
    bool m_HasStrip = false;
};

} // namespace kernel
