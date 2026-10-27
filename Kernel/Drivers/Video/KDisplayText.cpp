// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <Kernel/Drivers/Video/KDisplayText.h>
#include <Kernel/KAddressValidation.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>
#include <Utils/UTF8Utils.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace kernel
{

void KDisplayText::Begin(
    const PDisplayFontCache& font,
    std::string_view text,
    const PIPoint& position,
    const PIRect& bitmapBounds,
    std::span<const int32_t> kernings,
    PPoint advance,
    PDrawingMode mode,
    float width)
{
    End();
    PScopeFail rollback([this] { End(); });
    m_BitmapBounds = bitmapBounds;
    m_Vertical = font.GetDirection() == PDisplayFontDirection::Up || font.GetDirection() == PDisplayFontDirection::Down;
    m_Copy = mode == PDrawingMode::Copy;
    m_LogicalAxisAligned = true;
    if (PMath::is_nan(width)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    const bool bounded = PMath::is_finite(width);
    if (width <= 0.0f)
    {
        m_BitmapBounds = PIRect();
        m_StripEnd = GetEnd(m_BitmapBounds);
        return;
    }
    if (bounded) {
        InitializeWidthClip(font, position, width);
    }
    m_StripEnd = GetStart(m_BitmapBounds);
    if (!LayoutGlyphs(font, text, position, kernings))
    {
        std::sort(m_GlyphOrder.begin(), m_GlyphOrder.end(), [this](size_t lhs, size_t rhs)
        {
            return GetStart(m_Glyphs[lhs].Bounds) < GetStart(m_Glyphs[rhs].Bounds);
        });
    }
    if (m_Copy && (bounded || !text.empty())) {
        InitializeLogicalArea(font, position, bounded ? font.GetBaselineVector() * width : advance);
    }
}

void KDisplayText::End() noexcept
{
    m_Glyphs.clear();
    m_GlyphOrder.clear();
    m_ActiveGlyphs.clear();
    m_RowSources.clear();
    m_IntervalEdges.clear();
    m_Intervals.clear();
    m_BackgroundIntervals.clear();
    m_NextBackgroundIntervals.clear();
    m_LogicalBounds = PIRect();
    m_NextGlyph = 0;
    m_HasStrip = false;
    m_ObliqueClip = false;
    m_UnclippedSpan = KDisplayTextSpan();
    m_ClipRow = 0;
}

bool KDisplayText::NextSpan()
{
    if (!m_ObliqueClip) {
        return NextUnclippedSpan();
    }
    for (;;)
    {
        if (NextClippedSpan()) {
            return true;
        }
        if (!NextUnclippedSpan()) {
            return false;
        }
        m_UnclippedSpan = m_Span;
        m_ClipRow = m_UnclippedSpan.Bounds.top;
    }
}

std::span<const uint8_t*> KDisplayText::GetRowSources(int x, int y)
{
    m_RowSources.clear();
    for (size_t index : m_ActiveGlyphs)
    {
        const KDisplayTextGlyph& glyph = m_Glyphs[index];
        if (glyph.Bounds.DoIntersect(PIPoint(x, y)))
        {
            const size_t offset = size_t(y - glyph.Bounds.top) * size_t(glyph.Bounds.Width()) + size_t(x - glyph.Bounds.left);
            m_RowSources.push_back(glyph.Glyph->Raster + offset);
        }
    }
    return m_RowSources;
}

void KDisplayText::InitializeWidthClip(const PDisplayFontCache& font, const PIPoint& position, float width)
{
    const PPoint projection = font.GetWidthProjection();
    const float limit = width + projection.x * float(position.x) + projection.y * float(position.y);
    if (projection.y == 0.0f)
    {
        ClipInterval(limit / projection.x, projection.x < 0.0f, m_BitmapBounds.left, m_BitmapBounds.right);
    }
    else if (projection.x == 0.0f)
    {
        ClipInterval(limit / projection.y, projection.y < 0.0f, m_BitmapBounds.top, m_BitmapBounds.bottom);
    }
    else
    {
        const float inverse = 1.0f / projection.x;
        m_ClipOrigin = limit * inverse;
        m_ClipSlope = -projection.y * inverse;
        m_ClipFromStart = projection.x < 0.0f;
        m_ObliqueClip = true;
    }
}

bool KDisplayText::NextUnclippedSpan()
{
    for (;;)
    {
        if (!m_HasStrip && !BeginStrip()) {
            return false;
        }
        if (m_IntervalIndex < m_Intervals.size())
        {
            const Interval& interval = m_Intervals[m_IntervalIndex++];
            m_Span.Bounds = MakeBounds(m_StripStart, m_StripEnd, interval.Start, interval.End);
            m_Span.HasGlyphs = interval.HasGlyphs;
            m_Span.SingleGlyph = (interval.HasGlyphs && m_ActiveGlyphs.size() == 1)
                ? &m_Glyphs[m_ActiveGlyphs.front()] : nullptr;
            return true;
        }
        if (m_Copy && !m_LogicalAxisAligned && NextBackgroundSpan()) {
            return true;
        }
        m_HasStrip = false;
    }
}

bool KDisplayText::NextClippedSpan()
{
    while (m_ClipRow < m_UnclippedSpan.Bounds.bottom)
    {
        PIRect bounds = ClipRow(m_ClipRow++);
        if (!bounds.IsValid()) {
            continue;
        }
        while (m_ClipRow < m_UnclippedSpan.Bounds.bottom)
        {
            const PIRect next = ClipRow(m_ClipRow);
            if (next.left != bounds.left || next.right != bounds.right) {
                break;
            }
            bounds.bottom = ++m_ClipRow;
        }
        m_Span = m_UnclippedSpan;
        m_Span.Bounds = bounds;
        return true;
    }
    return false;
}

PIRect KDisplayText::ClipRow(int row) const noexcept
{
    PIRect bounds = m_UnclippedSpan.Bounds;
    bounds.top = row;
    bounds.bottom = row + 1;
    ClipInterval(m_ClipOrigin + m_ClipSlope * (float(row) + 0.5f), m_ClipFromStart, bounds.left, bounds.right);
    return bounds;
}

void KDisplayText::ClipInterval(float edge, bool fromStart, int& start, int& end) noexcept
{
    // Sample pixel centres. Clamp before converting to keep distant boundaries in range.
    edge = std::clamp(edge, float(start), float(end));
    if (fromStart) {
        start = int(std::floor(edge - 0.5f)) + 1;
    } else {
        end = int(std::ceil(edge - 0.5f));
    }
}

bool KDisplayText::LayoutGlyphs(
    const PDisplayFontCache& font,
    std::string_view text,
    const PIPoint& position,
    std::span<const int32_t> kernings)
{
    PPoint pen(position);
    const bool hasKerning = font.HasKerning();
    bool sorted = true;
    while (!text.empty())
    {
        size_t byteCount = 0;
        const uint32_t character = utf8_to_unicode(text.data(), text.size(), byteCount);
        text.remove_prefix(byteCount);
        const PDisplayFontGlyph* glyph = font.FindGlyph(character);
        if (glyph == nullptr)
        {
            glyph = font.GetMissingGlyph();
            if (glyph == nullptr) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        }
        if (hasKerning)
        {
            if (kernings.empty()) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            pen += font.TransformKerning(kernings.front());
            kernings = kernings.subspan(1);
        }
        if (!AppendGlyph(*glyph, pen)) {
            sorted = false;
        }
        pen += glyph->Advance;
    }
    if (!kernings.empty()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return sorted;
}

bool KDisplayText::AppendGlyph(const PDisplayFontGlyph& glyph, PPoint position)
{
    if (!glyph.Bounds.IsValid()) {
        return true;
    }
    // Clip in floating point before converting a potentially distant pen.
    const PRect bounds = PRect(glyph.Bounds) + PPoint(std::round(position.x), std::round(position.y));
    if (!bounds.DoIntersect(PRect(m_BitmapBounds))) {
        return true;
    }
    constexpr float INTEGER_LIMIT = float(std::numeric_limits<int>::max()) - 256.0f;
    if (bounds.left < -INTEGER_LIMIT || bounds.top < -INTEGER_LIMIT
        || bounds.right > INTEGER_LIMIT || bounds.bottom > INTEGER_LIMIT) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    const int64_t width = int64_t(glyph.Bounds.right) - glyph.Bounds.left;
    const int64_t height = int64_t(glyph.Bounds.bottom) - glyph.Bounds.top;
    if (width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max()
        || uint64_t(width) * uint64_t(height) > std::numeric_limits<size_t>::max()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    validate_user_read_pointer_trw(glyph.Raster, size_t(width) * size_t(height));
    const PIRect integerBounds(int(bounds.left), int(bounds.top), int(bounds.right), int(bounds.bottom));
    const bool sorted = m_Glyphs.empty() || GetStart(m_Glyphs.back().Bounds) <= GetStart(integerBounds);
    m_GlyphOrder.push_back(m_Glyphs.size());
    m_Glyphs.push_back(KDisplayTextGlyph{&glyph, integerBounds});
    return sorted;
}

void KDisplayText::InitializeLogicalArea(const PDisplayFontCache& font, const PIPoint& position, PPoint advance)
{
    const PPoint top = PPoint(position) + font.GetAscenderOffset();
    const PPoint bottom = PPoint(position) + font.GetDescenderOffset();
    const PPoint height = bottom - top;
    if (std::abs(advance.x * height.y - advance.y * height.x) < 1.0e-6f) {
        return;
    }
    std::array<PPoint, 4> corners = {top, top + advance, bottom + advance, bottom};
    if (m_Vertical)
    {
        for (PPoint& corner : corners) {
            std::swap(corner.x, corner.y);
        }
    }
    float first = corners[0].x;
    float last = first;
    float secondaryFirst = corners[0].y;
    float secondaryLast = secondaryFirst;
    for (size_t index = 0; index < corners.size(); ++index)
    {
        const PPoint& corner = corners[index];
        first = std::min(first, corner.x);
        last = std::max(last, corner.x);
        secondaryFirst = std::min(secondaryFirst, corner.y);
        secondaryLast = std::max(secondaryLast, corner.y);
        PPoint lower = corner;
        PPoint upper = corners[(index + 1) % corners.size()];
        if (lower.x > upper.x) {
            std::swap(lower, upper);
        }
        m_LogicalEdges[index] = LogicalEdge{
            lower.x, upper.x, lower.y, (lower.x != upper.x) ? (upper.y - lower.y) / (upper.x - lower.x) : 0.0f};
    }
    first = std::max(first, float(GetStart(m_BitmapBounds)));
    last = std::min(last, float(GetEnd(m_BitmapBounds)));
    secondaryFirst = std::max(secondaryFirst, float(GetSecondaryStart(m_BitmapBounds)));
    secondaryLast = std::min(secondaryLast, float(GetSecondaryEnd(m_BitmapBounds)));
    if (first >= last || secondaryFirst >= secondaryLast) {
        return;
    }
    const int start = int(std::ceil(first - 0.5f));
    const int end = int(std::ceil(last - 0.5f));
    const int secondaryStart = int(std::ceil(secondaryFirst - 0.5f));
    const int secondaryEnd = int(std::ceil(secondaryLast - 0.5f));
    if (start < end && secondaryStart < secondaryEnd)
    {
        m_LogicalBounds = MakeBounds(start, end, secondaryStart, secondaryEnd);
        m_LogicalAxisAligned = (advance.y == 0.0f && height.x == 0.0f) || (advance.x == 0.0f && height.y == 0.0f);
    }
}

bool KDisplayText::BeginStrip()
{
    for (;;)
    {
        const int position = m_StripEnd;
        if (position >= GetEnd(m_BitmapBounds)) {
            return false;
        }
        std::erase_if(
            m_ActiveGlyphs,
            [this, position](size_t index) noexcept { return GetEnd(m_Glyphs[index].Bounds) <= position; });
        while (m_NextGlyph < m_GlyphOrder.size() && GetStart(m_Glyphs[m_GlyphOrder[m_NextGlyph]].Bounds) <= position)
        {
            const size_t index = m_GlyphOrder[m_NextGlyph++];
            if (GetEnd(m_Glyphs[index].Bounds) > position) {
                m_ActiveGlyphs.insert(std::lower_bound(m_ActiveGlyphs.begin(), m_ActiveGlyphs.end(), index), index);
            }
        }
        const bool insideLogicalArea = m_LogicalBounds.IsValid()
            && position >= GetStart(m_LogicalBounds) && position < GetEnd(m_LogicalBounds);
        if (m_ActiveGlyphs.empty() && !insideLogicalArea)
        {
            int next = GetEnd(m_BitmapBounds);
            if (m_NextGlyph < m_GlyphOrder.size()) {
                next = std::min(next, GetStart(m_Glyphs[m_GlyphOrder[m_NextGlyph]].Bounds));
            }
            if (m_LogicalBounds.IsValid() && GetStart(m_LogicalBounds) > position) {
                next = std::min(next, GetStart(m_LogicalBounds));
            }
            if (next >= GetEnd(m_BitmapBounds)) {
                return false;
            }
            m_StripEnd = next;
            continue;
        }
        m_StripStart = position;
        m_StripEnd = GetEnd(m_BitmapBounds);
        if (m_NextGlyph < m_GlyphOrder.size()) {
            m_StripEnd = std::min(m_StripEnd, GetStart(m_Glyphs[m_GlyphOrder[m_NextGlyph]].Bounds));
        }
        for (size_t index : m_ActiveGlyphs) {
            m_StripEnd = std::min(m_StripEnd, GetEnd(m_Glyphs[index].Bounds));
        }
        if (m_LogicalBounds.IsValid())
        {
            if (position < GetStart(m_LogicalBounds)) {
                m_StripEnd = std::min(m_StripEnd, GetStart(m_LogicalBounds));
            } else if (position < GetEnd(m_LogicalBounds)) {
                m_StripEnd = std::min(m_StripEnd, GetEnd(m_LogicalBounds));
            }
        }
        if (m_ActiveGlyphs.size() > 1) {
            m_RowSources.reserve(m_ActiveGlyphs.size());
        }
        BuildIntervals();
        m_IntervalIndex = 0;
        m_BackgroundIntervals.clear();
        m_BackgroundIntervalIndex = 0;
        m_BackgroundPosition = m_StripStart;
        m_HasStrip = true;
        return true;
    }
}

void KDisplayText::BuildIntervals()
{
    m_IntervalEdges.clear();
    m_Intervals.clear();
    const bool includeBackground = m_Copy && m_LogicalAxisAligned && m_LogicalBounds.IsValid()
        && m_StripStart >= GetStart(m_LogicalBounds) && m_StripStart < GetEnd(m_LogicalBounds);
    if (!m_Vertical && m_ActiveGlyphs.size() <= 1)
    {
        BuildSimpleIntervals(includeBackground);
        return;
    }
    for (size_t index : m_ActiveGlyphs)
    {
        const PIRect& bounds = m_Glyphs[index].Bounds;
        m_IntervalEdges.push_back(std::max(GetSecondaryStart(bounds), GetSecondaryStart(m_BitmapBounds)));
        m_IntervalEdges.push_back(std::min(GetSecondaryEnd(bounds), GetSecondaryEnd(m_BitmapBounds)));
    }
    if (includeBackground)
    {
        m_IntervalEdges.push_back(GetSecondaryStart(m_LogicalBounds));
        m_IntervalEdges.push_back(GetSecondaryEnd(m_LogicalBounds));
    }
    std::sort(m_IntervalEdges.begin(), m_IntervalEdges.end());
    m_IntervalEdges.erase(std::unique(m_IntervalEdges.begin(), m_IntervalEdges.end()), m_IntervalEdges.end());
    for (size_t index = 1; index < m_IntervalEdges.size(); ++index)
    {
        const int start = m_IntervalEdges[index - 1];
        const int end = m_IntervalEdges[index];
        bool hasGlyphs = false;
        for (size_t glyphIndex : m_ActiveGlyphs)
        {
            const PIRect& bounds = m_Glyphs[glyphIndex].Bounds;
            if (start >= GetSecondaryStart(bounds) && start < GetSecondaryEnd(bounds))
            {
                hasGlyphs = true;
                break;
            }
        }
        const bool hasBackground = includeBackground
            && start >= GetSecondaryStart(m_LogicalBounds) && start < GetSecondaryEnd(m_LogicalBounds);
        if (hasGlyphs || hasBackground)
        {
            // Horizontal sweeps select sources per row. Vertical sweeps must also split at glyph x-edges,
            // so every selected source covers the full raster row that the driver will stream.
            if (!m_Intervals.empty() && m_Intervals.back().End == start && m_Intervals.back().HasGlyphs == hasGlyphs
                && (!m_Vertical || !hasGlyphs)) {
                m_Intervals.back().End = end;
            } else {
                m_Intervals.push_back(Interval{start, end, hasGlyphs});
            }
        }
    }
}

void KDisplayText::BuildSimpleIntervals(bool includeBackground)
{
    if (m_ActiveGlyphs.empty())
    {
        if (includeBackground) {
            m_Intervals.push_back(Interval{m_LogicalBounds.top, m_LogicalBounds.bottom, false});
        }
    }
    else
    {
        const PIRect& bounds = m_Glyphs[m_ActiveGlyphs.front()].Bounds;
        const int start = std::max(bounds.top, m_BitmapBounds.top);
        const int end = std::min(bounds.bottom, m_BitmapBounds.bottom);
        if (includeBackground && m_LogicalBounds.top < start) {
            m_Intervals.push_back(Interval{m_LogicalBounds.top, std::min(start, m_LogicalBounds.bottom), false});
        }
        m_Intervals.push_back(Interval{start, end, true});
        if (includeBackground && m_LogicalBounds.bottom > end) {
            m_Intervals.push_back(Interval{std::max(end, m_LogicalBounds.top), m_LogicalBounds.bottom, false});
        }
    }
}

bool KDisplayText::NextBackgroundSpan()
{
    for (;;)
    {
        if (m_BackgroundIntervalIndex < m_BackgroundIntervals.size())
        {
            const Interval& interval = m_BackgroundIntervals[m_BackgroundIntervalIndex++];
            m_Span.Bounds = MakeBounds(m_BackgroundStart, m_BackgroundEnd, interval.Start, interval.End);
            m_Span.HasGlyphs = false;
            m_Span.SingleGlyph = nullptr;
            return true;
        }
        if (m_BackgroundPosition >= m_StripEnd) {
            return false;
        }
        m_BackgroundStart = m_BackgroundPosition;
        BuildBackgroundIntervals(m_BackgroundPosition++, m_BackgroundIntervals);
        while (m_BackgroundPosition < m_StripEnd)
        {
            BuildBackgroundIntervals(m_BackgroundPosition, m_NextBackgroundIntervals);
            if (m_NextBackgroundIntervals != m_BackgroundIntervals) {
                break;
            }
            ++m_BackgroundPosition;
        }
        m_BackgroundEnd = m_BackgroundPosition;
        m_BackgroundIntervalIndex = 0;
    }
}

void KDisplayText::BuildBackgroundIntervals(int position, std::vector<Interval>& intervals) const
{
    intervals.clear();
    int start;
    int end;
    if (GetLogicalInterval(position, start, end))
    {
        for (const Interval& interval : m_Intervals)
        {
            if (interval.Start > start) {
                intervals.push_back(Interval{start, std::min(end, interval.Start), false});
            }
            start = std::max(start, interval.End);
            if (start >= end) {
                break;
            }
        }
        if (start < end) {
            intervals.push_back(Interval{start, end, false});
        }
    }
}

bool KDisplayText::GetLogicalInterval(int position, int& start, int& end) const noexcept
{
    if (!m_LogicalBounds.IsValid() || position < GetStart(m_LogicalBounds) || position >= GetEnd(m_LogicalBounds)) {
        return false;
    }
    const float sample = float(position) + 0.5f;
    float minimum = std::numeric_limits<float>::infinity();
    float maximum = -std::numeric_limits<float>::infinity();
    for (const LogicalEdge& edge : m_LogicalEdges)
    {
        if (sample >= edge.Start && sample < edge.End)
        {
            const float crossing = edge.Origin + (sample - edge.Start) * edge.Slope;
            minimum = std::min(minimum, crossing);
            maximum = std::max(maximum, crossing);
        }
    }
    minimum = std::max(minimum, float(GetSecondaryStart(m_BitmapBounds)));
    maximum = std::min(maximum, float(GetSecondaryEnd(m_BitmapBounds)));
    if (minimum >= maximum) {
        return false;
    }
    start = int(std::ceil(minimum - 0.5f));
    end = int(std::ceil(maximum - 0.5f));
    return start < end;
}

int KDisplayText::GetStart(const PIRect& bounds) const noexcept
{
    return m_Vertical ? bounds.top : bounds.left;
}

int KDisplayText::GetEnd(const PIRect& bounds) const noexcept
{
    return m_Vertical ? bounds.bottom : bounds.right;
}

int KDisplayText::GetSecondaryStart(const PIRect& bounds) const noexcept
{
    return m_Vertical ? bounds.left : bounds.top;
}

int KDisplayText::GetSecondaryEnd(const PIRect& bounds) const noexcept
{
    return m_Vertical ? bounds.right : bounds.bottom;
}

PIRect KDisplayText::MakeBounds(int start, int end, int secondaryStart, int secondaryEnd) const noexcept
{
    return m_Vertical ? PIRect(secondaryStart, start, secondaryEnd, end) : PIRect(start, secondaryStart, end, secondaryEnd);
}

} // namespace kernel
