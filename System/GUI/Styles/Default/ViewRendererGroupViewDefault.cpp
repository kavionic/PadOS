// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleGroupView.h>
#include <GUI/Styles/Default/ViewRendererGroupViewDefault.h>
#include <GUI/View.h>

struct PViewRendererGroupViewDefault::Context : PViewRendererContext<PViewStyleGroupView>
{
    using PViewRendererContext<PViewStyleGroupView>::PViewRendererContext;

    PPoint LabelSize;
    float LabelAscender = 0.0f;
    bool AreMetricsValid = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererGroupViewDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PGroupViewStyleData& data = state.GetTarget().GetGroupViewStyleData();
    UpdateGroupViewMetrics(target, data, state);
    if (!data.Label.empty())
    {
        target.SetBrush(nullptr);
        target.SetFgColor(PStandardColorID::Shadow);
        target.DrawString(data.Label, PPoint(GROUP_VIEW_LABEL_OFFSET, state.LabelAscender), target.GetBounds());
    }
    else
    {
        target.EraseRect(updateRect);
    }

    PRect bound = target.GetNormalizedBounds();
    bound.Resize(0.0f, std::floor(state.LabelSize.y * 0.5f), -1.0f, -1.0f);

    const float oldPenWidth = target.GetPenWidth();
    target.SetPenWidth(GROUP_VIEW_PEN_WIDTH);
    bound.Resize(1.0f, 1.0f, -1.0f, -1.0f);

    target.SetFgColor(PStandardColorID::Shadow);
    target.MovePenTo(bound.BottomLeft());
    target.DrawLine(bound.TopLeft());

    if (!data.Label.empty())
    {
        target.DrawLine(GROUP_VIEW_LABEL_GAP_START, bound.top);
        target.MovePenTo(GROUP_VIEW_LABEL_OFFSET + state.LabelSize.x + GROUP_VIEW_LABEL_SPACING, bound.top);
    }
    target.DrawLine(bound.TopRight());

    target.SetFgColor(PStandardColorID::Shine);
    target.MovePenTo(bound.TopRight() + PPoint(0.0f, GROUP_VIEW_PEN_WIDTH));
    target.DrawLine(bound.BottomRight());
    target.DrawLine(bound.BottomLeft() + PPoint(GROUP_VIEW_PEN_WIDTH, 0.0f));

    target.SetPenWidth(oldPenWidth);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PGroupViewStyleData& data = state.GetTarget().GetGroupViewStyleData();
    UpdateGroupViewMetrics(target, data, state);
    *minSize = PPoint(GROUP_VIEW_MIN_SIZE);
    if (!data.Label.empty())
    {
        minSize->x = GROUP_VIEW_LABEL_OFFSET + state.LabelSize.x + GROUP_VIEW_LABEL_SPACING + GROUP_VIEW_PEN_WIDTH;
        minSize->y = std::max(GROUP_VIEW_MIN_SIZE, state.LabelSize.y + GROUP_VIEW_PEN_WIDTH);
    }
    *maxSize = PPoint(LAYOUT_MAX_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewDefault::PropertyChanged(PView& target, PGroupViewProperties properties, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const bool sizeChanged = properties.HasAny(PGroupViewProperties(PGroupViewProperty::Label, PGroupViewProperty::Font));
    if (sizeChanged) {
        state.AreMetricsValid = false;
    }
    target.Invalidate();
    if (sizeChanged) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererGroupViewDefault::UpdateGroupViewMetrics(const PView& target, const PGroupViewStyleData& data, Context& state)
{
    if (!state.AreMetricsValid)
    {
        state.LabelSize = PPoint();
        state.LabelAscender = 0.0f;
        if (!data.Label.empty())
        {
            const PFontHeight fontHeight = target.GetFontHeight();
            state.LabelSize = PPoint(target.GetStringWidth(data.Label), fontHeight.ascender + fontHeight.descender);
            state.LabelAscender = fontHeight.ascender;
        }
        state.AreMetricsValid = true;
    }
}
