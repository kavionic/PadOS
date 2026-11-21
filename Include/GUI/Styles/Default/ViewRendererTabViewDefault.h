// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererTabView.h>

class PViewRendererTabViewDefault : public PViewRendererTabView
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void PaintTabs(PView& target, const PView& tabView, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PTabViewProperties properties, size_t tabIndex, PtrTarget* context) const override;
    virtual PTabViewFrames GetFrames(const PView& target, PtrTarget* context) const override;
    virtual PPoint GetTabsContentSize(const PView& target, PtrTarget* context) const override;
    virtual size_t GetTabFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;
    struct TabViewGeometry;
    static constexpr float TAB_VIEW_CORNER_SIZE = 10.0f;
    static constexpr float TAB_VIEW_ARROW_SPACE = 16.0f;
    static constexpr float TAB_VIEW_BORDER = 2.0f;
    static constexpr float TAB_VIEW_TITLE_PADDING = 4.0f;
    static constexpr float TAB_VIEW_TITLE_SCALE = 1.1f;
    static void UpdateTabViewMetrics(const PView& target, const PTabViewStyleData& data, Context& state);
    static void UpdateTabViewTitle(const PView& target, const PTabViewStyleData& data, PTabViewProperties properties, size_t tabIndex, Context& state);
    static TabViewGeometry GetTabViewGeometry(const PView& target, const PTabViewStyleData& data, Context& state);
    static void InvalidateTabViewResize(PView& target, PView& tabsTarget, const PTabViewStyleData& data, const TabViewGeometry& geometry, Context& state);
    static PColor GetTabViewTint(PColor color, float tint);
    static void DrawTabViewBorder(PView& target, const PRect& bounds);
    static void DrawTabViewSelection(PView& target, const PView& tabsTarget, const PTabViewStyleData& data, const TabViewGeometry& geometry, Context& state);
    static void DrawTabViewArrows(PView& target, const TabViewGeometry& geometry);
    static void DrawTabViewTitles(PView& target, const PTabViewStyleData& data, const TabViewGeometry& geometry, Context& state);
    static void DrawTabViewHighlight(PView& target, const PTabViewStyleData& data, PRect frame, const PPoint& baseline);
    static void DrawTabViewTabs(PView& target, const PTabViewStyleData& data, const TabViewGeometry& geometry, Context& state);
};
