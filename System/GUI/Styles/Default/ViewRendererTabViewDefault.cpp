// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleTabView.h>
#include <GUI/Styles/Default/ViewRendererTabViewDefault.h>
#include <GUI/View.h>

struct PViewRendererTabViewDefault::Context : PViewRendererContext<PViewStyleTabView>
{
    using PViewRendererContext<PViewStyleTabView>::PViewRendererContext;

    std::vector<float> TabWidths;
    PFontHeight FontHeight;
    PRect PreviousFrame;
    PRect PreviousTabsFrame;
    float GlyphHeight = 0.0f;
    float TabHeight = 0.0f;
    float TotalTabsWidth = 0.0f;
    float PreviousTabsRight = 0.0f;
    bool AreMetricsValid = false;
    bool HasVisualState = false;
    bool PreviousOverflow = false;
};

struct PViewRendererTabViewDefault::TabViewGeometry
{
    PTabViewFrames Frames;
    PRect Bounds;
    PRect HeaderFrame;
    float TabHeight = 0.0f;
    float TabsRight = 0.0f;
    bool Overflow = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererTabViewDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PTabViewStyleData& data = state.GetTarget().GetTabViewStyleData();
    PView& tabsTarget = state.GetTarget().GetTabsView();
    const TabViewGeometry geometry = GetTabViewGeometry(target, data, state);
    PRect bounds = geometry.Bounds;
    bounds.top += geometry.TabHeight - TAB_VIEW_BORDER;
    DrawTabViewBorder(target, bounds);
    DrawTabViewSelection(target, tabsTarget, data, geometry, state);
    if (geometry.Overflow) {
        DrawTabViewArrows(target, geometry);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::PaintTabs(PView& target, const PView& tabView, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PTabViewStyleData& data = state.GetTarget().GetTabViewStyleData();
    const TabViewGeometry geometry = GetTabViewGeometry(tabView, data, state);
    DrawTabViewTitles(target, data, geometry, state);
    DrawTabViewTabs(target, data, geometry, state);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PTabViewStyleData& data = state.GetTarget().GetTabViewStyleData();
    PPoint contentMinSize;
    PPoint contentMaxSize;
    state.GetTarget().GetTabContentSizeLimits(&contentMinSize, &contentMaxSize);
    UpdateTabViewMetrics(target, data, state);
    const float tabHeight = std::max(state.TabHeight, data.TopBarMinSize.y + TAB_VIEW_BORDER);
    minSize->x = std::max(contentMinSize.x, state.TotalTabsWidth + data.TopBarMinSize.x) + 5.0f;
    minSize->y = contentMinSize.y + tabHeight + 3.0f;
    maxSize->x = std::max(contentMaxSize.x, state.TotalTabsWidth + data.TopBarMaxSize.x) + 5.0f;
    maxSize->y = contentMaxSize.y + tabHeight + 3.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::PropertyChanged(PView& target, PTabViewProperties properties, size_t tabIndex, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PTabViewStyleData& data = state.GetTarget().GetTabViewStyleData();
    PView& tabsTarget = state.GetTarget().GetTabsView();
    UpdateTabViewTitle(target, data, properties, tabIndex, state);
    const PTabViewProperties sizeProperties(
        PTabViewProperty::TabInserted,
        PTabViewProperty::TabDeleted,
        PTabViewProperty::Title,
        PTabViewProperty::Font,
        PTabViewProperty::TopBar);
    const TabViewGeometry geometry = GetTabViewGeometry(target, data, state);
    if (!state.HasVisualState || properties.HasAny(sizeProperties))
    {
        target.Invalidate();
        tabsTarget.Invalidate();
    }
    else
    {
        if (properties.Has(PTabViewProperty::Frame)) {
            InvalidateTabViewResize(target, tabsTarget, data, geometry, state);
        }
        if (properties.Has(PTabViewProperty::Selection))
        {
            target.Invalidate(PRect(0.0f, 0.0f, geometry.Bounds.right, geometry.TabHeight));
            tabsTarget.Invalidate();
        }
        else if (properties.Has(PTabViewProperty::Scroll))
        {
            target.Invalidate(PRect(0.0f, geometry.TabHeight - TAB_VIEW_BORDER, geometry.TabsRight, geometry.TabHeight));
        }
        if (properties.Has(PTabViewProperty::Highlight))
        {
            target.Invalidate(PRect(0.0f, geometry.TabHeight - TAB_VIEW_BORDER, geometry.TabsRight, geometry.TabHeight));
            tabsTarget.Invalidate();
        }
    }
    // Scroll and highlight changes can occur during layout. Preserve the previous geometry until resize damage has been handled.
    if (properties.HasAny(sizeProperties | PTabViewProperty::Selection | PTabViewProperty::Frame))
    {
        state.PreviousFrame = geometry.Bounds;
        state.PreviousTabsFrame = geometry.Frames.TabsFrame;
        state.PreviousTabsRight = geometry.TabsRight;
        state.PreviousOverflow = geometry.Overflow;
        state.HasVisualState = true;
    }
    // Notifications can replace the renderer binding, so finish all context updates first.
    if (properties.HasAny(sizeProperties))
    {
        target.PreferredSizeChanged();
        tabsTarget.ContentSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabViewFrames PViewRendererTabViewDefault::GetFrames(const PView& target, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PTabViewStyleData& data = state.GetTarget().GetTabViewStyleData();
    return GetTabViewGeometry(target, data, state).Frames;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererTabViewDefault::GetTabsContentSize(const PView& target, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PTabViewStyleData& data = state.GetTarget().GetTabViewStyleData();
    const TabViewGeometry geometry = GetTabViewGeometry(target, data, state);
    // With no overflow, the parent paints the outer borders outside the strip's viewport.
    const float width = geometry.Overflow ? state.TotalTabsWidth : std::max(0.0f, state.TotalTabsWidth - TAB_VIEW_BORDER * 2.0f);
    return PPoint(width, geometry.Frames.TabsFrame.Height());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PViewRendererTabViewDefault::GetTabFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PTabViewStyleData& data = state.GetTarget().GetTabViewStyleData();
    PView& tabsTarget = state.GetTarget().GetTabsView();
    const TabViewGeometry geometry = GetTabViewGeometry(target, data, state);
    if (!tabsTarget.GetBounds().DoIntersect(position)) {
        return INVALID_INDEX;
    }
    float tabLeft = geometry.Overflow ? 0.0f : -TAB_VIEW_BORDER;
    for (size_t index = 0; index < state.TabWidths.size(); ++index)
    {
        const float tabRight = tabLeft + state.TabWidths[index];
        if (position.x >= tabLeft && position.x < tabRight) {
            return index;
        }
        tabLeft = tabRight;
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::UpdateTabViewMetrics(const PView& target, const PTabViewStyleData& data, Context& state)
{
    if (!state.AreMetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        state.GlyphHeight = state.FontHeight.ascender + state.FontHeight.descender + state.FontHeight.line_gap;
        state.TabHeight = std::round(state.GlyphHeight * 1.2f + 6.0f);
        state.TotalTabsWidth = TAB_VIEW_TITLE_PADDING;
        state.TabWidths.resize(data.Titles.size());
        for (size_t index = 0; index < data.Titles.size(); ++index)
        {
            state.TabWidths[index] = std::round(target.GetStringWidth(data.Titles[index]) * TAB_VIEW_TITLE_SCALE) + TAB_VIEW_TITLE_PADDING;
            state.TotalTabsWidth += state.TabWidths[index];
        }
        state.AreMetricsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::UpdateTabViewTitle(
    const PView& target,
    const PTabViewStyleData& data,
    PTabViewProperties properties,
    size_t tabIndex,
    Context& state)
{
    if (properties.Has(PTabViewProperty::Font)) {
        state.AreMetricsValid = false;
    }
    if (state.AreMetricsValid)
    {
        if (properties.Has(PTabViewProperty::TabInserted))
        {
            const float width = std::round(target.GetStringWidth(data.Titles[tabIndex]) * TAB_VIEW_TITLE_SCALE) + TAB_VIEW_TITLE_PADDING;
            state.TabWidths.insert(state.TabWidths.begin() + tabIndex, width);
            state.TotalTabsWidth += width;
        }
        else if (properties.Has(PTabViewProperty::TabDeleted))
        {
            state.TotalTabsWidth -= state.TabWidths[tabIndex];
            state.TabWidths.erase(state.TabWidths.begin() + tabIndex);
        }
        else if (properties.Has(PTabViewProperty::Title))
        {
            const float width = std::round(target.GetStringWidth(data.Titles[tabIndex]) * TAB_VIEW_TITLE_SCALE) + TAB_VIEW_TITLE_PADDING;
            state.TotalTabsWidth += width - state.TabWidths[tabIndex];
            state.TabWidths[tabIndex] = width;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererTabViewDefault::TabViewGeometry PViewRendererTabViewDefault::GetTabViewGeometry(const PView& target, const PTabViewStyleData& data, Context& state)
{
    UpdateTabViewMetrics(target, data, state);
    TabViewGeometry geometry;
    geometry.Bounds = target.GetNormalizedBounds();
    geometry.TabHeight = std::max(state.TabHeight, data.TopBarMinSize.y + TAB_VIEW_BORDER);
    geometry.TabsRight = geometry.Bounds.right;
    const float headerBottom = std::min(geometry.TabHeight - TAB_VIEW_BORDER, geometry.Bounds.bottom);
    if (data.HasTopBarClient)
    {
        const float availableWidth = std::max(0.0f, geometry.Bounds.Width() - TAB_VIEW_BORDER * 2.0f);
        const float extraWidth = availableWidth - state.TotalTabsWidth;
        const float clientWidth = std::min(availableWidth, std::clamp(extraWidth, data.TopBarMinSize.x, data.TopBarMaxSize.x));
        const float clientRight = std::max(0.0f, geometry.Bounds.right - TAB_VIEW_BORDER);
        const float clientHeight = std::min(headerBottom, data.TopBarMaxSize.y);
        geometry.Frames.TopBarClientFrame = PRect(clientRight - clientWidth, 0.0f, clientRight, clientHeight);
        geometry.TabsRight = geometry.Frames.TopBarClientFrame.left;
    }
    geometry.Overflow = state.TotalTabsWidth > geometry.TabsRight;
    const float inset = geometry.Overflow ? TAB_VIEW_ARROW_SPACE : TAB_VIEW_BORDER;
    const float tabsLeft = std::min(inset, geometry.TabsRight);
    const float tabsRight = std::max(tabsLeft, geometry.TabsRight - inset);
    geometry.Frames.TabsFrame = PRect(tabsLeft, 0.0f, tabsRight, headerBottom);
    geometry.HeaderFrame = PRect(0.0f, 0.0f, geometry.TabsRight, headerBottom);
    const float clientLeft = std::min(TAB_VIEW_BORDER, geometry.Bounds.right);
    const float clientTop = std::min(geometry.TabHeight, geometry.Bounds.bottom);
    const float clientRight = std::max(clientLeft, geometry.Bounds.right - TAB_VIEW_BORDER);
    const float clientBottom = std::max(clientTop, geometry.Bounds.bottom - TAB_VIEW_BORDER);
    geometry.Frames.ClientFrame = PRect(clientLeft, clientTop, clientRight, clientBottom);
    return geometry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::InvalidateTabViewResize(
    PView& target,
    PView& tabsTarget,
    const PTabViewStyleData& data,
    const TabViewGeometry& geometry,
    Context& state)
{
    const PPoint delta = geometry.Bounds.Size() - state.PreviousFrame.Size();
    if (delta.x != 0.0f)
    {
        PRect damage = geometry.Bounds;
        damage.left = damage.right - std::max(3.0f, delta.x + 9.0f);
        target.Invalidate(damage);
    }
    if (delta.y != 0.0f)
    {
        PRect damage = geometry.Bounds;
        damage.top = damage.bottom - std::max(3.0f, delta.y + TAB_VIEW_BORDER);
        target.Invalidate(damage);
    }
    if (geometry.Overflow != state.PreviousOverflow || geometry.TabsRight != state.PreviousTabsRight) {
        target.Invalidate(PRect(0.0f, 0.0f, geometry.Bounds.right, geometry.TabHeight));
    }
    if (geometry.Overflow != state.PreviousOverflow || geometry.Frames.TabsFrame.TopLeft() != state.PreviousTabsFrame.TopLeft()) {
        tabsTarget.Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PViewRendererTabViewDefault::GetTabViewTint(PColor color, float tint)
{
    return PColor(uint8_t((float(color.GetRed()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                  uint8_t((float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                  uint8_t((float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                  color.GetAlpha()
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::DrawTabViewBorder(PView& target, const PRect& bounds)
{
    const PDynamicColor fgColor(PStandardColorID::Shine);
    const PDynamicColor bgColor(PStandardColorID::Shadow);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);
    const PColor bgShadowColor = GetTabViewTint(bgColor, 0.8f);

    target.SetFgColor(bgShadowColor);
    target.DrawLine(PPoint(bounds.right - 1.0f, bounds.top + 2.0f), PPoint(bounds.right - 1.0f, bounds.bottom - 1.0f));  // top/right -> bottom/right
    target.DrawLine(PPoint(bounds.left, bounds.bottom - 1.0f));                                                         // bottom/left

    target.SetFgColor(bgColor);
    target.DrawLine(PPoint(bounds.right - 2.0f, bounds.top + 2.0f), PPoint(bounds.right - 2.0f, bounds.bottom - 2.0f));  // top/right -> bottom/right
    target.DrawLine(PPoint(bounds.left + 1.0f, bounds.bottom - 2.0f));                                                  // bottom/left

    target.SetFgColor(fgShadowColor);
    target.DrawLine(PPoint(bounds.left, bounds.top - 1.0f), PPoint(bounds.left, bounds.bottom - 3.0f));                  // top/left -> bottom/left

    target.SetFgColor(fgColor);
    target.DrawLine(PPoint(bounds.left + 1.0f, bounds.top + 1.0f), PPoint(bounds.left + 1.0f, bounds.bottom - 3.0f));    // top/left -> bottom/left
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::DrawTabViewSelection(
    PView& target,
    const PView& tabsTarget,
    const PTabViewStyleData& data,
    const TabViewGeometry& geometry,
    Context& state)
{
    const PRect bounds = geometry.Bounds;
    const PRect tabRect = geometry.HeaderFrame;
    const PDynamicColor fgColor(PStandardColorID::Shine);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);

    float x = tabsTarget.GetScrollOffset().x;

    if (geometry.Overflow) {
        x += TAB_VIEW_ARROW_SPACE;
    }
    if (!geometry.Overflow)
    {
        target.FillRect(PRect(0.0f, 0.0f, 2.0f, geometry.TabHeight - 2.0f), PStandardColorID::DefaultBackground);
        target.FillRect(PRect(tabRect.right - 2.0f, 0.0f, tabRect.right, geometry.TabHeight - 2.0f), PStandardColorID::DefaultBackground);
    }

    for (size_t i = 0; i < data.Titles.size(); ++i)
    {
        float width = state.TabWidths[i];
        PRect tabFrame(x + 2.0f, 2.0f, x + width + 2.0f, geometry.TabHeight);

        if (i == data.Selection) {
            tabFrame.Resize(-2.0f, -2.0f, 2.0f, 0.0f);
        }

        if (!geometry.Overflow)
        {
            if (i != data.Selection + 1)
            {
                target.SetFgColor(fgShadowColor);
                target.DrawLine(PPoint(tabFrame.left, tabFrame.bottom - 3.0f), PPoint(tabFrame.left, tabFrame.top + 2.0f + TAB_VIEW_CORNER_SIZE));
                target.DrawLine(PPoint(tabFrame.left + 3.0f, tabFrame.top));
                target.SetFgColor(fgColor);
                target.DrawLine(PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f), PPoint(tabFrame.left + 1.0f, tabFrame.top + 2.0f + TAB_VIEW_CORNER_SIZE));
                target.DrawLine(PPoint(tabFrame.left + 1.0f + TAB_VIEW_CORNER_SIZE, tabFrame.top + 1.0f));
            }
        }
        else
        {
            if (i == data.Selection) {
                target.DrawLine(PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f), PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f));
            }
        }
        if (i == data.Selection)
        {
            target.SetFgColor((i == data.HighlightedTab) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground);
            float x1 = x + 2.0f;
            float x2 = x + width + 2.0f;

            if (geometry.Overflow)
            {
                if (x1 < TAB_VIEW_ARROW_SPACE) {
                    x1 = TAB_VIEW_ARROW_SPACE;
                }
                if (x2 > geometry.TabsRight - 2.0f - TAB_VIEW_ARROW_SPACE) {
                    x2 = geometry.TabsRight - 2.0f - TAB_VIEW_ARROW_SPACE;
                }
            }
            target.DrawLine(PPoint(x1, tabFrame.bottom - 2.0f), PPoint(x2, tabFrame.bottom - 2.0f));
            target.DrawLine(PPoint(x1, tabFrame.bottom - 1.0f), PPoint(x2, tabFrame.bottom - 1.0f));

            target.SetFgColor(fgShadowColor);

            if (x >= 0.0f) {
                target.DrawLine(PPoint(0, tabFrame.bottom - 2.0f), PPoint(x, tabFrame.bottom - 2.0f));
            }
            target.DrawLine(PPoint(x + width + 2, tabFrame.bottom - 2.0f), PPoint(bounds.right - 1.0f, tabFrame.bottom - 2.0f));

            target.SetFgColor(fgColor);
            if (x >= 1) {
                target.DrawLine(PPoint(1, tabFrame.bottom - 1.0f), PPoint(x + 1.0f, tabFrame.bottom - 1.0f));
            }
            target.DrawLine(PPoint(x + width + 2.0f, tabFrame.bottom - 1.0f), PPoint(bounds.right - 1.0f, tabFrame.bottom - 1.0f));
        }
        x += width;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::DrawTabViewArrows(PView& target, const TabViewGeometry& geometry)
{
    const PRect tabRect = geometry.HeaderFrame;
    const PDynamicColor fgColor(PStandardColorID::Shine);
    const PDynamicColor bgColor(PStandardColorID::Shadow);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);

    target.FillRect(PRect(0.0f, 0.0f, TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 2.0f), PStandardColorID::DefaultBackground);
    target.FillRect(PRect(tabRect.right - TAB_VIEW_ARROW_SPACE, 0.0f, tabRect.right, geometry.TabHeight - 2.0f), PStandardColorID::DefaultBackground);
    target.SetFgColor(bgColor);
    for (float x = 2.0f; x < TAB_VIEW_ARROW_SPACE - 2.0f; x += 1.0f)
    {
        const float y1 = geometry.TabHeight * 0.5f - (x - 2.0f);
        const float y2 = geometry.TabHeight * 0.5f + (x - 2.0f);
        target.DrawLine(PPoint(x, y1), PPoint(x, y2));
        target.DrawLine(PPoint(geometry.TabsRight - x - 1.0f, y1), PPoint(geometry.TabsRight - x - 1.0f, y2));
    }

    target.SetFgColor(fgShadowColor);
    target.DrawLine(PPoint(0, geometry.TabHeight - 2.0f), PPoint(TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 2.0f));
    target.DrawLine(
        PPoint(geometry.TabsRight - 1.0f - TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 2.0f),
        PPoint(geometry.TabsRight - 1.0f, geometry.TabHeight - 2.0f));
    target.SetFgColor(fgColor);
    target.DrawLine(PPoint(1.0f, geometry.TabHeight - 1.0f), PPoint(TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 1.0f));
    target.DrawLine(
        PPoint(geometry.TabsRight - 1.0f - TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 1.0f),
        PPoint(geometry.TabsRight - 1.0f, geometry.TabHeight - 1.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::DrawTabViewTitles(PView& target, const PTabViewStyleData& data, const TabViewGeometry& geometry, Context& state)
{
    const PRect bounds = target.GetBounds();
    const PFontHeight fontHeight = state.FontHeight;
    const float tabHeight = geometry.TabHeight;
    const float glyphHeight = state.GlyphHeight;
    float x = geometry.Overflow ? 0.0f : -TAB_VIEW_BORDER;
    target.SetBrush(PStandardColorID::DefaultBackground);
    target.SetFgColor(0, 0, 0);
    PRect highlightedFrame;
    PPoint highlightedBaseline;
    float backgroundLeft = bounds.left;
    for (size_t i = 0; i < data.Titles.size(); ++i)
    {
        const float width = state.TabWidths[i];
        const float top = (i == data.Selection) ? 0.0f : 2.0f;
        const PPoint baseline(
            std::round(x + (width - 4.0f) * 0.05f + 4.0f),
            std::round(top + tabHeight * 0.5f + fontHeight.ascender - fontHeight.line_gap * 0.5f - glyphHeight * 0.5f + 2.0f));
        const float right = std::clamp(x + width, bounds.left, bounds.right);
        const PRect background(backgroundLeft, bounds.top, right, bounds.bottom);
        if (i == data.HighlightedTab)
        {
            target.FillRect(background, target.GetBrush());
            highlightedFrame = PRect(x + 2.0f, 2.0f, x + width + 2.0f, tabHeight);
            highlightedBaseline = baseline;
        }
        else
        {
            target.DrawString(target.GetBrush(), data.Titles[i], baseline, background);
        }
        backgroundLeft = right;
        x += width;
    }
    if (backgroundLeft < bounds.right) {
        target.FillRect(PRect(backgroundLeft, bounds.top, bounds.right, bounds.bottom), target.GetBrush());
    }
    if (data.HighlightedTab < data.Titles.size()) {
        DrawTabViewHighlight(target, data, highlightedFrame, highlightedBaseline);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::DrawTabViewHighlight(PView& target, const PTabViewStyleData& data, PRect frame, const PPoint& baseline)
{
    target.SetFgColor(PStandardColorID::ButtonBackgroundHover);
    float cornerSize = TAB_VIEW_CORNER_SIZE;
    if (data.HighlightedTab == data.Selection)
    {
        frame.Resize(-TAB_VIEW_BORDER, -TAB_VIEW_BORDER, TAB_VIEW_BORDER, 0.0f);
    }
    else if (data.HighlightedTab + 1 == data.Selection)
    {
        // The preceding tab remains visible underneath the selected tab's chamfer.
        frame.right -= TAB_VIEW_BORDER;
        const float overlap = TAB_VIEW_CORNER_SIZE - TAB_VIEW_BORDER;
        const float cornerOverlap = std::clamp(cornerSize - frame.Width(), 0.0f, overlap);
        const PPoint topLeft = frame.TopRight() + PPoint(cornerOverlap, 0.0f);
        const PPoint bottomLeft = frame.TopRight() + PPoint(0.0f, overlap);
        target.FillTriangle(topLeft, frame.TopRight() + PPoint(overlap, 0.0f), bottomLeft);
        if (cornerOverlap > 0.0f) {
            target.FillTriangle(topLeft, bottomLeft, frame.TopRight() + PPoint(0.0f, cornerOverlap));
        }
    }
    else if (data.Selection != INVALID_INDEX && data.HighlightedTab == data.Selection + 1)
    {
        // The selected tab covers the following tab's left edge.
        frame.left += TAB_VIEW_BORDER;
        cornerSize -= TAB_VIEW_BORDER;
    }
    const float cornerRight = std::min(frame.left + cornerSize, frame.right);
    const float cornerBottom = frame.top + cornerSize;
    const float cornerTop = cornerBottom - (cornerRight - frame.left);
    target.FillRect(PRect(cornerRight, frame.top, frame.right, frame.bottom), PStandardColorID::ButtonBackgroundHover);
    target.FillRect(PRect(frame.left, cornerBottom, cornerRight, frame.bottom), PStandardColorID::ButtonBackgroundHover);
    target.FillTriangle(PPoint(frame.left, cornerBottom), PPoint(cornerRight, cornerTop), PPoint(cornerRight, cornerBottom));

    const PDrawingMode previousMode = target.GetDrawingMode();
    target.SetDrawingMode(PDrawingMode::Overlay);
    target.SetFgColor(0, 0, 0);
    target.DrawString(target.GetBrush(), data.Titles[data.HighlightedTab], baseline, frame.right - baseline.x);
    target.SetDrawingMode(previousMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewDefault::DrawTabViewTabs(PView& target, const PTabViewStyleData& data, const TabViewGeometry& geometry, Context& state)
{
    float x = geometry.Overflow ? 0.0f : -TAB_VIEW_BORDER;
    const PDynamicColor fgColor(PStandardColorID::Shine);
    const PDynamicColor bgColor(PStandardColorID::Shadow);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);
    const PColor bgShadowColor = GetTabViewTint(bgColor, 0.8f);

    for (size_t i = 0; i < data.Titles.size(); ++i)
    {
        float width = state.TabWidths[i];
        PRect tabFrame(x + 2.0f, 2.0f, x + width + 2.0f, geometry.TabHeight);

        if (i == data.Selection) {
            tabFrame.Resize(-2.0f, -2.0f, 2.0f, 0.0f);
        }
        target.SetFgColor(fgShadowColor);
        if (i != data.Selection + 1)
        {
            target.DrawLine(PPoint(tabFrame.left, tabFrame.bottom - 3.0f), PPoint(tabFrame.left, tabFrame.top + TAB_VIEW_CORNER_SIZE));
            target.DrawLine(PPoint(tabFrame.left + TAB_VIEW_CORNER_SIZE, tabFrame.top));
            if (i == data.Selection - 1) {
                target.DrawLine(PPoint(tabFrame.right + TAB_VIEW_CORNER_SIZE - 3.0f, tabFrame.top));
            } else {
                target.DrawLine(PPoint(tabFrame.right - 1.0f, tabFrame.top));
            }

            target.SetFgColor(fgColor);

            target.DrawLine(PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f), PPoint(tabFrame.left + 1.0f, tabFrame.top + 1.0f + TAB_VIEW_CORNER_SIZE));
            target.DrawLine(PPoint(tabFrame.left + 1.0f + TAB_VIEW_CORNER_SIZE, tabFrame.top + 1.0f));
            if (i == data.Selection - 1) {
                target.DrawLine(PPoint(tabFrame.right + TAB_VIEW_CORNER_SIZE - 4.0f, tabFrame.top + 1.0f));
            } else {
                target.DrawLine(PPoint(tabFrame.right - 2.0f, tabFrame.top + 1.0f));
            }
        }
        else
        {
            // Draw the chamfer from the edge exposed beside the selected tab.
            const float visibleLeft = tabFrame.left + TAB_VIEW_BORDER;
            target.DrawLine(
                PPoint(visibleLeft, tabFrame.top + TAB_VIEW_CORNER_SIZE - TAB_VIEW_BORDER),
                PPoint(tabFrame.left + TAB_VIEW_CORNER_SIZE, tabFrame.top));
            target.DrawLine(PPoint(tabFrame.right - 1.0f, tabFrame.top));
            target.SetFgColor(fgColor);
            target.DrawLine(PPoint(visibleLeft, tabFrame.top + TAB_VIEW_CORNER_SIZE), PPoint(tabFrame.left + 1.0f + TAB_VIEW_CORNER_SIZE, tabFrame.top + 1.0f));
            target.DrawLine(PPoint(tabFrame.right - 2.0f, tabFrame.top + 1.0f));
        }

        if (i != data.Selection - 1)
        {
            target.SetFgColor(bgShadowColor);
            target.DrawLine(PPoint(tabFrame.right - 1.0f, tabFrame.top + 2.0f), PPoint(tabFrame.right - 1.0f, tabFrame.bottom - 3.0f));

            target.SetFgColor(bgColor);
            target.DrawLine(PPoint(tabFrame.right - 2.0f, tabFrame.top + 2.0f), PPoint(tabFrame.right - 2.0f, tabFrame.bottom - 2.0f));
        }
        x += width;
    }
}
