// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>
#include <GUI/Styles/Skinned/ViewRendererGroupViewSkinned.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleGroupView.h>

struct PViewRendererGroupViewSkinned::Context : PViewRendererContext<PViewStyleGroupView>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}

    PSkinnedImage Frame;
    PPoint LabelSize;
    PPoint MinimumSize;
    float LabelAscender = 0.0f;
    float HeaderHeight = 0.0f;
    bool MetricsValid = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererGroupViewSkinned::PViewRendererGroupViewSkinned(const PGroupViewSkin& skin) : m_GroupViewSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererGroupViewSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewSkinned::Paint(PView& target, const PRect& componentFrame, const PRect& updateRect, PtrTarget* context) const
{
    const PGroupViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetGroupViewStyleData();
    Context& state = GetGroupViewStyleData(target, data, context);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBrush(nullptr);
    const PRect bounds = target.GetBounds();
    PRect frame = PInsetSkinnedFrame(bounds, m_GroupViewSkin.FrameInsets);
    frame.top = std::min(frame.bottom, frame.top + std::floor(state.HeaderHeight * 0.5f));
    PaintGroupViewFrame(target, updateRect, frame, state);
    if (!data.Label.empty())
    {
        const PRect& padding = m_GroupViewSkin.LabelPadding;
        const PPoint position = (bounds.TopLeft() + PPoint(padding.left, padding.top + state.LabelAscender)).GetRounded();
        const PRect gap(
            position.x - m_GroupViewSkin.LabelGap,
            position.y - state.LabelAscender,
            position.x + state.LabelSize.x + m_GroupViewSkin.LabelGap,
            position.y - state.LabelAscender + state.LabelSize.y);
        const PRect damage = gap & bounds & updateRect;
        if (damage.IsValid()) {
            target.EraseRect(damage);
        }
        target.SetFgColor(m_GroupViewSkin.LabelColor);
        target.DrawString(data.Label, position, state.LabelSize.x);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PGroupViewStyleData& data = static_cast<Context*>(context)->GetTarget().GetGroupViewStyleData();
    *minSize = GetGroupViewStyleData(target, data, context).MinimumSize;
    *maxSize = PPoint(LAYOUT_MAX_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewSkinned::PropertyChanged(PView& target, PGroupViewProperties properties, PtrTarget* context) const
{
    const bool sizeChanged = properties.HasAny(PGroupViewProperties(PGroupViewProperty::Label, PGroupViewProperty::Font));
    if (sizeChanged) {
        static_cast<Context*>(context)->MetricsValid = false;
    }
    target.Invalidate();
    if (sizeChanged) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererGroupViewSkinned::Context& PViewRendererGroupViewSkinned::GetGroupViewStyleData(
    const PView& target,
    const PGroupViewStyleData& data,
    PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    if (!state.MetricsValid)
    {
        state.LabelSize = PPoint();
        state.LabelAscender = 0.0f;
        state.HeaderHeight = 0.0f;
        const PRect& padding = m_GroupViewSkin.LabelPadding;
        if (!data.Label.empty())
        {
            const PFontHeight fontHeight = target.GetFontHeight();
            state.LabelSize = PPoint(target.GetStringWidth(data.Label), fontHeight.ascender + fontHeight.descender);
            state.LabelAscender = fontHeight.ascender;
            state.HeaderHeight = std::max(0.0f, state.LabelSize.y + padding.top + padding.bottom);
        }
        const PImageDescription& description = m_GroupViewSkin.Frame->GetDescription();
        const PPoint frameBorders = PPoint(description.SourceRect.Size()) - description.SliceRect.Size();
        const PRect& insets = m_GroupViewSkin.FrameInsets;
        state.MinimumSize = PPoint(
            std::max(m_GroupViewSkin.MinimumSize.x, frameBorders.x + insets.left + insets.right),
            std::max(m_GroupViewSkin.MinimumSize.y, frameBorders.y + insets.top + insets.bottom + std::floor(state.HeaderHeight * 0.5f)));
        if (!data.Label.empty())
        {
            state.MinimumSize.x = std::max(state.MinimumSize.x, state.LabelSize.x + padding.left + padding.right);
            state.MinimumSize.y = std::max(state.MinimumSize.y, state.HeaderHeight);
        }
        state.MetricsValid = true;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewSkinned::PaintGroupViewFrame(PView& target, const PRect& updateRect, const PRect& frame, Context& state) const
{
    const PRect bounds = target.GetBounds();
    const PRect visible = frame & bounds;
    if (!visible.IsValid())
    {
        target.EraseRect(updateRect);
        return;
    }
    const Ptr<const PBrush> brush = PGetSkinnedBrush(state.Frame, *m_GroupViewSkin.Frame, frame.Size());
    target.SetBrushOrigin(frame.TopLeft());
    const PRect frameDamage = visible & updateRect;
    if (frameDamage.IsValid()) {
        target.FillRect(frameDamage, brush);
    }
    const std::array<PRect, 4> outside =
    {
        PRect(bounds.left, bounds.top, bounds.right, visible.top),
        PRect(bounds.left, visible.top, visible.left, visible.bottom),
        PRect(visible.right, visible.top, bounds.right, visible.bottom),
        PRect(bounds.left, visible.bottom, bounds.right, bounds.bottom)
    };
    for (const PRect& strip : outside)
    {
        const PRect damage = strip & updateRect;
        if (damage.IsValid()) {
            target.EraseRect(damage);
        }
    }
}
