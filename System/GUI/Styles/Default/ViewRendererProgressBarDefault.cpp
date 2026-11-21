// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Default/ViewRendererProgressBarDefault.h>

#include <algorithm>
#include <cmath>

#include <GUI/View.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleProgressBar.h>

struct PViewRendererProgressBarDefault::Context : PViewRendererContext<PViewStyleProgressBar>
{
    using PViewRendererContext<PViewStyleProgressBar>::PViewRendererContext;

    PRect PreviousFrame;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererProgressBarDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererProgressBarDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PProgressBarStyleData& data = state.GetTarget().GetProgressBarStyleData();
    const PRect bounds = frame.GetFloored();
    target.DrawFrame(bounds, FRAME_RECESSED | FRAME_TRANSPARENT);

    const PRect trackFrame = bounds.GetResized(PROGRESS_BAR_BORDER, PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER);
    if (trackFrame.IsValid())
    {
        const float trackLength = (data.Orientation == POrientation::Horizontal) ? trackFrame.Width() : trackFrame.Height();
        const float barLength = std::ceil(trackLength * std::clamp(data.Value, 0.0f, 1.0f));
        PRect barFrame = trackFrame;
        PRect restFrame = trackFrame;
        if (data.Orientation == POrientation::Horizontal)
        {
            barFrame.right = barFrame.left + barLength;
            restFrame.left = barFrame.right;
        }
        else
        {
            barFrame.top = barFrame.bottom - barLength;
            restFrame.bottom = barFrame.top;
        }
        if (barFrame.IsValid()) {
            target.FillRect(barFrame, PColor(0x66, 0x88, 0xbb));
        }
        if (restFrame.IsValid()) {
            target.EraseRect(restFrame);
        }
    }
    state.PreviousFrame = frame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererProgressBarDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PProgressBarStyleData& data = state.GetTarget().GetProgressBarStyleData();
    if (data.Orientation == POrientation::Horizontal)
    {
        *minSize = PPoint(PROGRESS_BAR_MIN_LENGTH, PROGRESS_BAR_THICKNESS);
        *maxSize = PPoint(LAYOUT_MAX_SIZE, PROGRESS_BAR_THICKNESS);
    }
    else
    {
        *minSize = PPoint(PROGRESS_BAR_THICKNESS, PROGRESS_BAR_MIN_LENGTH);
        *maxSize = PPoint(PROGRESS_BAR_THICKNESS, LAYOUT_MAX_SIZE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererProgressBarDefault::PropertyChanged(PView& target, PProgressBarProperties properties, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PProgressBarStyleData& data = state.GetTarget().GetProgressBarStyleData();
    if (properties == PProgressBarProperties(PProgressBarProperty::Frame) && state.PreviousFrame.IsValid()) {
        InvalidateProgressBarResize(target, data, state.PreviousFrame);
    } else {
        target.Invalidate();
    }
    state.PreviousFrame = target.GetNormalizedBounds();
    if (properties.Has(PProgressBarProperty::Font)) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererProgressBarDefault::InvalidateProgressBarResize(PView& target, const PProgressBarStyleData& data, const PRect& previousFrame)
{
    const PRect bounds = target.GetNormalizedBounds();
    const PPoint delta = bounds.Size() - previousFrame.Size();
    if ((data.Orientation == POrientation::Horizontal && delta.x != 0.0f) || (data.Orientation == POrientation::Vertical && delta.y != 0.0f)) {
        target.Invalidate(bounds.GetResized(PROGRESS_BAR_BORDER, PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER));
    }
    if (delta.x != 0.0f)
    {
        PRect damage = bounds;
        damage.left = damage.right - std::max(PROGRESS_BAR_BORDER, delta.x + PROGRESS_BAR_BORDER);
        target.Invalidate(damage);
    }
    if (delta.y != 0.0f)
    {
        PRect damage = bounds;
        damage.top = damage.bottom - std::max(PROGRESS_BAR_BORDER, delta.y + PROGRESS_BAR_BORDER);
        target.Invalidate(damage);
    }
}
