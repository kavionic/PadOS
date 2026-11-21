// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererScrollBar.h>
#include <GUI/Styles/SkinnableGUIStyle.h>

class PViewRendererScrollBarSkinned : public PViewRendererScrollBar
{
public:
    explicit PViewRendererScrollBarSkinned(const PScrollBarSkin& skin);
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PScrollBarProperties properties, PtrTarget* context) const override;
    virtual PScrollBarPart HitTest(const PView& target, const PPoint& position, PtrTarget* context) const override;
    virtual PPoint GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const override;
    virtual float GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;
    struct SkinnedScrollBarPart;
    struct SkinnedScrollBarGeometry;
    static constexpr std::array<PScrollBarPart, 4> SCROLL_BAR_ARROW_PARTS =
    {
        PScrollBarPart::DecrementStart, PScrollBarPart::IncrementStart, PScrollBarPart::DecrementEnd, PScrollBarPart::IncrementEnd
    };

    const PScrollBarOrientationSkin& GetScrollBarOrientation(POrientation orientation) const;
    SkinnedScrollBarGeometry GetScrollBarGeometry(const PRect& frame, const PScrollBarStyleData& data, float relativeValue) const;
    static PRect GetScrollBarPartFrame(const SkinnedScrollBarGeometry& geometry, PScrollBarPart part);
    static PButtonSkinState GetScrollBarState(const PScrollBarStyleData& data, PScrollBarPart part);
    static Ptr<const PBrush> GetScrollBarBrush(SkinnedScrollBarPart& cache, const PScrollBarSkinPart& part, PButtonSkinState state, const PPoint& size);
    static void PaintScrollBarPart(PView& target, const PRect& frame, const PScrollBarSkinPart& part, SkinnedScrollBarPart& cache, PButtonSkinState state);
    static void ClearScrollBarTrack(PView& target, const SkinnedScrollBarGeometry& geometry);

    const PScrollBarSkin m_ScrollBarSkin;
};
