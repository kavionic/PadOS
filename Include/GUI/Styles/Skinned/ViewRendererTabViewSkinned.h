// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererTabView.h>
#include <GUI/Styles/SkinnableGUIStyle.h>

class PViewRendererTabViewSkinned : public PViewRendererTabView
{
public:
    explicit PViewRendererTabViewSkinned(const PTabViewSkin& skin, PBrush backgroundBrush);
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
    struct SkinnedTabViewTab;
    struct SkinnedTabViewGeometry;

    Context& GetTabViewStyleData(const PView& target, const PTabViewStyleData& data, PtrTarget* context) const;
    SkinnedTabViewGeometry GetSkinnedTabViewGeometry(const PView& target, const PTabViewStyleData& data, PtrTarget* context) const;
    PRect GetSkinnedTabFrame(const Context& state, const SkinnedTabViewGeometry& geometry, size_t index, bool selected) const;
    static PTabSkinState GetTabViewState(const PTabViewStyleData& data, size_t index);
    void PaintSkinnedTab(PView& target, const PTabViewStyleData& data, Context& state, const SkinnedTabViewGeometry& geometry, size_t index) const;
    static void PaintTabViewOverflow(PView& target, const PRect& frame, const Ptr<const PBitmap>& bitmap);

    const PTabViewSkin m_TabViewSkin;
    const PBrush m_BackgroundBrush;
};
