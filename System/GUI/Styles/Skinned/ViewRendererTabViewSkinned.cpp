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
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererTabViewSkinned.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

struct PViewRendererTabViewSkinned::SkinnedTabViewTab
{
    std::array<PSkinnedImage, std::to_underlying(PTabSkinState::Count) * 2> Appearances;
    float Left = 0.0f;
    float Width = 0.0f;
    float LabelWidth = 0.0f;
};

struct PViewRendererTabViewSkinned::Context : PViewRendererContext<PViewStyleTabView>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}

    std::vector<SkinnedTabViewTab> Tabs;
    PSkinnedImage Frame;
    PFontHeight FontHeight;
    PRect PreviousBounds;
    PRect PreviousTabsFrame;
    size_t PreviousSelection = INVALID_INDEX;
    size_t PreviousHighlight = INVALID_INDEX;
    size_t PreviousPressed = INVALID_INDEX;
    float TabHeight = 0.0f;
    float TotalWidth = 0.0f;
    bool MetricsValid = false;
    bool HasVisualState = false;
};

struct PViewRendererTabViewSkinned::SkinnedTabViewGeometry
{
    PTabViewFrames Frames;
    PRect Bounds;
    PRect Panel;
    PRect OverflowLeft;
    PRect OverflowRight;
    float HeaderHeight = 0.0f;
    bool Overflow = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererTabViewSkinned::PViewRendererTabViewSkinned(const PTabViewSkin& skin, Ptr<const PBrush> backgroundBrush)
    : m_TabViewSkin(skin)
    , m_BackgroundBrush(std::move(backgroundBrush))
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererTabViewSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PTabViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetTabViewStyleData();
    PView& tabsTarget = static_cast<Context*>(context)->GetTarget().GetTabsView();
    Context& state = GetTabViewStyleData(target, data, context);
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data, context);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); target.SetBrush(nullptr); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_BackgroundBrush);
    target.SetBrush(nullptr);
    target.EraseRect(PRect(geometry.Bounds.left, geometry.Bounds.top, geometry.Bounds.right, geometry.Panel.top) & updateRect);
    if (geometry.Panel.IsValid())
    {
        const Ptr<const PBrush> brush = PGetSkinnedBrush(state.Frame, *m_TabViewSkin.Frame, geometry.Panel.Size());
        target.SetBrushOrigin(geometry.Panel.TopLeft());
        target.FillRect(geometry.Panel & updateRect, brush);
    }
    if (geometry.Overflow)
    {
        if (tabsTarget.GetBounds().left > 0.0f) {
            PaintTabViewOverflow(target, geometry.OverflowLeft, m_TabViewSkin.OverflowLeft);
        }
        if (tabsTarget.GetBounds().right < state.TotalWidth) {
            PaintTabViewOverflow(target, geometry.OverflowRight, m_TabViewSkin.OverflowRight);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewSkinned::PaintTabs(PView& target, const PView& tabView, const PRect& updateRect, PtrTarget* context) const
{
    const PTabViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetTabViewStyleData();
    Context& state = GetTabViewStyleData(tabView, data, context);
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(tabView, data, context);
    const PRect bounds = target.GetBounds();
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); target.SetBrush(nullptr); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_BackgroundBrush);
    target.SetBrush(nullptr);
    target.EraseRect(updateRect);
    if (geometry.Panel.IsValid())
    {
        const Ptr<const PBrush> brush = PGetSkinnedBrush(state.Frame, *m_TabViewSkin.Frame, geometry.Panel.Size());
        target.SetBrushOrigin(geometry.Panel.TopLeft() - geometry.Frames.TabsFrame.TopLeft() - target.GetScrollOffset());
        target.FillRect(PRect(bounds.left, geometry.HeaderHeight, bounds.right, bounds.bottom) & updateRect, brush);
    }
    for (size_t index = 0; index < state.Tabs.size(); ++index)
    {
        if (index != data.Selection && GetSkinnedTabFrame(state, geometry, index, false).DoIntersect(updateRect)) {
            PaintSkinnedTab(target, data, state, geometry, index);
        }
    }
    if (data.Selection < state.Tabs.size() && GetSkinnedTabFrame(state, geometry, data.Selection, true).DoIntersect(updateRect)) {
        PaintSkinnedTab(target, data, state, geometry, data.Selection);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PTabViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetTabViewStyleData();
    PPoint contentMinSize;
    PPoint contentMaxSize;
    static_cast<Context*>(context)->GetTarget().GetTabContentSizeLimits(&contentMinSize, &contentMaxSize);
    const Context& state = GetTabViewStyleData(target, data, context);
    const PRect& insets = m_TabViewSkin.FrameInsets;
    const float headerHeight = std::max(state.TabHeight, data.TopBarMinSize.y);
    const float topBarSpacing = (data.HasTopBarClient && !data.Titles.empty()) ? m_TabViewSkin.TabSpacing : 0.0f;
    const float horizontalBorder = insets.left + insets.right;
    const float verticalBorder = headerHeight + insets.top + insets.bottom;
    minSize->x = std::max(contentMinSize.x, state.TotalWidth + topBarSpacing + data.TopBarMinSize.x) + horizontalBorder;
    minSize->y = contentMinSize.y + verticalBorder;
    maxSize->x = std::max(contentMaxSize.x, state.TotalWidth + topBarSpacing + data.TopBarMaxSize.x) + horizontalBorder;
    maxSize->y = contentMaxSize.y + verticalBorder;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewSkinned::PropertyChanged(PView& target, PTabViewProperties properties, size_t tabIndex, PtrTarget* context) const
{
    const PTabViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetTabViewStyleData();
    PView& tabsTarget = static_cast<Context*>(context)->GetTarget().GetTabsView();
    Context& state = *static_cast<Context*>(context);
    const PTabViewProperties metricProperties(PTabViewProperty::TabInserted, PTabViewProperty::TabDeleted, PTabViewProperty::Title, PTabViewProperty::Font);
    const bool sizeChanged = properties.HasAny(metricProperties | PTabViewProperty::TopBar);
    if (properties.HasAny(metricProperties)) {
        state.MetricsValid = false;
    }
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data, context);
    const bool frameChanged = geometry.Bounds != state.PreviousBounds || geometry.Frames.TabsFrame != state.PreviousTabsFrame;
    if (!state.HasVisualState || sizeChanged || frameChanged)
    {
        target.Invalidate();
        tabsTarget.Invalidate();
    }
    else
    {
        const auto invalidateTab = [this, &tabsTarget, &state, &geometry](size_t index)
        {
            if (index < state.Tabs.size()) {
                tabsTarget.Invalidate(GetSkinnedTabFrame(state, geometry, index, true));
            }
        };
        if (properties.Has(PTabViewProperty::Selection))
        {
            invalidateTab(state.PreviousSelection);
            invalidateTab(data.Selection);
        }
        if (properties.Has(PTabViewProperty::Highlight))
        {
            invalidateTab(state.PreviousHighlight);
            invalidateTab(data.HighlightedTab);
            if (state.PreviousPressed != state.PreviousHighlight) {
                invalidateTab(state.PreviousPressed);
            }
            if (data.PressedTab != data.HighlightedTab) {
                invalidateTab(data.PressedTab);
            }
        }
        if (properties.Has(PTabViewProperty::Scroll) && geometry.Overflow)
        {
            target.Invalidate(geometry.OverflowLeft);
            target.Invalidate(geometry.OverflowRight);
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousHighlight = data.HighlightedTab;
    state.PreviousPressed = data.PressedTab;
    // Scroll notifications can arrive before the container finishes layout.
    if (!properties.Has(PTabViewProperty::Scroll))
    {
        state.PreviousBounds = geometry.Bounds;
        state.PreviousTabsFrame = geometry.Frames.TabsFrame;
        state.HasVisualState = true;
    }
    // Notifications can replace the renderer binding, so finish all context updates first.
    if (sizeChanged)
    {
        target.PreferredSizeChanged();
        tabsTarget.ContentSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabViewFrames PViewRendererTabViewSkinned::GetFrames(const PView& target, PtrTarget* context) const
{
    const PTabViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetTabViewStyleData();
    return GetSkinnedTabViewGeometry(target, data, context).Frames;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererTabViewSkinned::GetTabsContentSize(const PView& target, PtrTarget* context) const
{
    const PTabViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetTabViewStyleData();
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data, context);
    return PPoint(static_cast<Context*>(context)->TotalWidth, geometry.Frames.TabsFrame.Height());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PViewRendererTabViewSkinned::GetTabFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    const PTabViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetTabViewStyleData();
    PView& tabsTarget = static_cast<Context*>(context)->GetTarget().GetTabsView();
    if (!tabsTarget.GetBounds().DoIntersect(position)) {
        return INVALID_INDEX;
    }
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data, context);
    const Context& state = *static_cast<Context*>(context);
    if (data.Selection < state.Tabs.size() && GetSkinnedTabFrame(state, geometry, data.Selection, true).DoIntersect(position)) {
        return data.Selection;
    }
    for (size_t index = state.Tabs.size(); index > 0; --index)
    {
        if (GetSkinnedTabFrame(state, geometry, index - 1, false).DoIntersect(position)) {
            return index - 1;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererTabViewSkinned::Context& PViewRendererTabViewSkinned::GetTabViewStyleData(
    const PView& target,
    const PTabViewStyleData& data,
    PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    if (!state.MetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        const PRect& padding = m_TabViewSkin.LabelPadding;
        const float labelHeight = state.FontHeight.ascender + state.FontHeight.descender;
        state.TabHeight = std::ceil(std::max(m_TabViewSkin.MinimumTabSize.y, labelHeight + padding.top + padding.bottom));
        state.Tabs.resize(data.Titles.size());
        state.TotalWidth = 0.0f;
        float left = 0.0f;
        for (size_t index = 0; index < data.Titles.size(); ++index)
        {
            SkinnedTabViewTab& tab = state.Tabs[index];
            tab.LabelWidth = target.GetStringWidth(data.Titles[index]);
            tab.Width = std::ceil(std::max(m_TabViewSkin.MinimumTabSize.x, tab.LabelWidth + padding.left + padding.right));
            tab.Left = left;
            state.TotalWidth = std::max(state.TotalWidth, left + tab.Width);
            left += std::max(0.0f, tab.Width + m_TabViewSkin.TabSpacing);
        }
        state.MetricsValid = true;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererTabViewSkinned::SkinnedTabViewGeometry PViewRendererTabViewSkinned::GetSkinnedTabViewGeometry(
    const PView& target,
    const PTabViewStyleData& data,
    PtrTarget* context) const
{
    const Context& state = GetTabViewStyleData(target, data, context);
    const PRect& insets = m_TabViewSkin.FrameInsets;
    SkinnedTabViewGeometry geometry;
    geometry.Bounds = target.GetNormalizedBounds();
    geometry.HeaderHeight = std::min(geometry.Bounds.bottom, std::max(state.TabHeight, data.TopBarMinSize.y));
    geometry.Panel = PRect(0.0f, geometry.HeaderHeight, geometry.Bounds.right, geometry.Bounds.bottom);
    const float left = std::min(insets.left, geometry.Bounds.right);
    const float right = std::max(left, geometry.Bounds.right - insets.right);
    float tabsRight = right;
    if (data.HasTopBarClient)
    {
        const float spacing = data.Titles.empty() ? 0.0f : m_TabViewSkin.TabSpacing;
        const float availableWidth = right - left;
        const float extraWidth = availableWidth - state.TotalWidth - spacing;
        const float width = std::min(availableWidth, std::clamp(extraWidth, data.TopBarMinSize.x, data.TopBarMaxSize.x));
        const float height = std::min(geometry.HeaderHeight, data.TopBarMaxSize.y);
        geometry.Frames.TopBarClientFrame = PRect(right - width, 0.0f, right, height);
        tabsRight = std::clamp(geometry.Frames.TopBarClientFrame.left - spacing, left, right);
    }
    geometry.Overflow = state.TotalWidth > tabsRight - left;
    const float margin = geometry.Overflow ? std::min(m_TabViewSkin.OverflowWidth, (tabsRight - left) * 0.5f) : 0.0f;
    const float clientTop = std::min(geometry.HeaderHeight + insets.top, geometry.Bounds.bottom);
    const float clientBottom = std::max(clientTop, geometry.Bounds.bottom - insets.bottom);
    geometry.Frames.ClientFrame = PRect(left, clientTop, right, clientBottom);
    geometry.Frames.TabsFrame = PRect(left + margin, 0.0f, tabsRight - margin, clientTop);
    geometry.OverflowLeft = PRect(left, 0.0f, left + margin, geometry.HeaderHeight);
    geometry.OverflowRight = PRect(tabsRight - margin, 0.0f, tabsRight, geometry.HeaderHeight);
    return geometry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererTabViewSkinned::GetSkinnedTabFrame(const Context& state, const SkinnedTabViewGeometry& geometry, size_t index, bool selected) const
{
    const SkinnedTabViewTab& tab = state.Tabs[index];
    const float top = selected ? 0.0f : std::clamp(m_TabViewSkin.InactiveTopInset, 0.0f, geometry.HeaderHeight);
    const float bottom = selected ? geometry.Frames.TabsFrame.bottom : geometry.HeaderHeight;
    return PRect(tab.Left, top, tab.Left + tab.Width, bottom);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabSkinState PViewRendererTabViewSkinned::GetTabViewState(const PTabViewStyleData& data, size_t index)
{
    if (index == data.PressedTab) {
        return PTabSkinState::Pressed;
    } else if (index == data.HighlightedTab) {
        return PTabSkinState::Hovered;
    } else {
        return PTabSkinState::Normal;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewSkinned::PaintSkinnedTab(
    PView& target,
    const PTabViewStyleData& data,
    Context& state,
    const SkinnedTabViewGeometry& geometry,
    size_t index) const
{
    const bool selected = index == data.Selection;
    const PRect frame = GetSkinnedTabFrame(state, geometry, index, selected);
    if (!frame.IsValid()) {
        return;
    }
    const std::array<PTabSkinAppearance, std::to_underlying(PTabSkinState::Count)>& appearances
        = selected ? m_TabViewSkin.SelectedStates : m_TabViewSkin.States;
    const size_t stateIndex = std::to_underlying(GetTabViewState(data, index));
    const PTabSkinAppearance& appearance = (appearances[stateIndex].Image != nullptr) ? appearances[stateIndex] : appearances[0];
    const size_t cacheIndex = stateIndex + (selected ? std::to_underlying(PTabSkinState::Count) : 0);
    SkinnedTabViewTab& tab = state.Tabs[index];
    const Ptr<const PBrush> brush = PGetSkinnedBrush(tab.Appearances[cacheIndex], *appearance.Image, frame.Size());
    const PRect& padding = m_TabViewSkin.LabelPadding;
    const float labelHeight = state.FontHeight.ascender + state.FontHeight.descender;
    const PPoint baseline = (PPoint(
        frame.left + padding.left + (frame.Width() - padding.left - padding.right - tab.LabelWidth) * 0.5f,
        padding.top + (geometry.HeaderHeight - padding.top - padding.bottom - labelHeight) * 0.5f + state.FontHeight.ascender)
        + appearance.LabelOffset).GetRounded();
    target.SetBrushOrigin(frame.TopLeft());
    target.SetFgColor(target.GetStyle()->GetColor(GetLabelColorID(data, index)));
    const PRect labelBounds(baseline.x, baseline.y - state.FontHeight.ascender, baseline.x + tab.LabelWidth, baseline.y + state.FontHeight.descender);
    if ((labelBounds & frame) == labelBounds)
    {
        target.DrawString(brush, data.Titles[index], baseline, frame);
    }
    else
    {
        // Only unusual padding/offsets or a very short view need extra clipping.
        const PIRect clip = PIRect(frame + target.GetScrollOffset()) & PIRect(target.GetNormalizedBounds());
        const Ptr<const PRegion> previousRegion = target.GetDrawingRegion();
        const Ptr<const PRegion> region = (previousRegion != nullptr) ? ptr_new<PRegion>(*previousRegion, clip, false) : ptr_new<PRegion>(clip);
        PScopeExit restoreRegion([&target, previousRegion] { target.SetDrawingRegion(previousRegion); });
        target.SetDrawingRegion(region);
        target.DrawString(brush, data.Titles[index], baseline, frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTabViewSkinned::PaintTabViewOverflow(PView& target, const PRect& frame, const Ptr<const PBitmap>& bitmap)
{
    if (frame.IsValid() && bitmap != nullptr)
    {
        const PRect source(bitmap->GetBounds());
        const PPoint position = (frame.TopLeft() + (frame.Size() - source.Size()) * 0.5f).GetRounded();
        const PRect visible = PRect(position, position + source.Size()) & frame;
        if (visible.IsValid()) {
            target.DrawBitmap(bitmap, visible - position + source.TopLeft(), visible.TopLeft());
        }
    }
}
