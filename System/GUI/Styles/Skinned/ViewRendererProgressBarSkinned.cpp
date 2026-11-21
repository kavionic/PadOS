// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleProgressBar.h>
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererProgressBarSkinned.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

struct PViewRendererProgressBarSkinned::Context : PViewRendererContext<PViewStyleProgressBar>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}

    std::array<PSkinnedImage, 2> Tracks;
    std::array<PSkinnedImage, 2> Fills;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererProgressBarSkinned::PViewRendererProgressBarSkinned(const PProgressBarSkin& skin) : m_ProgressBarSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererProgressBarSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererProgressBarSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PProgressBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetProgressBarStyleData();
    const PRect bounds = frame.GetFloored();
    if (!bounds.IsValid()) {
        return;
    }
    const PProgressBarOrientationSkin& skin = GetProgressBarOrientation(data.Orientation);
    Context& state = *static_cast<Context*>(context);
    const size_t orientationIndex = (data.Orientation == POrientation::Horizontal) ? 0 : 1;
    const Ptr<const PBrush> track = PGetSkinnedBrush(state.Tracks[orientationIndex], *skin.Track, bounds.Size());
    const Ptr<const PBrush> previousBackground = target.GetBackgroundBrush();
    const PPoint previousBackgroundOrigin = target.GetBackgroundBrushOrigin();
    const PPoint previousDrawingOrigin = target.GetDrawingBrushOrigin();
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreState([&target, previousBackground, previousBackgroundOrigin, previousDrawingOrigin, previousMode]
    {
        target.SetBackgroundBrush(previousBackground);
        target.SetBackgroundBrushOrigin(previousBackgroundOrigin);
        target.SetBrushOrigin(previousDrawingOrigin);
        target.SetDrawingMode(previousMode);
    });
    // Resolve the fill's transparent edges against the track, then restore the
    // view's background so this skin does not outlive a later style change.
    target.SetBackgroundBrush(track);
    target.SetBackgroundBrushOrigin(bounds.TopLeft());
    target.SetDrawingMode(PDrawingMode::Copy);
    const PRect fillFrame = GetProgressBarFillFrame(bounds, skin, data);
    const PRect visibleFill = fillFrame & bounds;
    if (visibleFill.IsValid())
    {
        const std::array<PRect, 4> exposedTrack =
        {
            PRect(bounds.left, bounds.top, bounds.right, visibleFill.top),
            PRect(bounds.left, visibleFill.bottom, bounds.right, bounds.bottom),
            PRect(bounds.left, visibleFill.top, visibleFill.left, visibleFill.bottom),
            PRect(visibleFill.right, visibleFill.top, bounds.right, visibleFill.bottom)
        };
        for (const PRect& section : exposedTrack)
        {
            if (section.IsValid()) {
                target.EraseRect(section);
            }
        }
        const Ptr<const PBrush> fill = PGetSkinnedBrush(state.Fills[orientationIndex], *skin.Fill, fillFrame.Size());
        target.SetBrushOrigin(fillFrame.TopLeft());
        target.FillRect(visibleFill, fill);
    }
    else
    {
        target.EraseRect(bounds);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererProgressBarSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PProgressBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetProgressBarStyleData();
    const PProgressBarOrientationSkin& skin = GetProgressBarOrientation(data.Orientation);
    const bool horizontal = data.Orientation == POrientation::Horizontal;
    *minSize = horizontal ? PPoint(skin.MinimumLength, skin.Thickness) : PPoint(skin.Thickness, skin.MinimumLength);
    *maxSize = horizontal ? PPoint(LAYOUT_MAX_SIZE, skin.Thickness) : PPoint(skin.Thickness, LAYOUT_MAX_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererProgressBarSkinned::PropertyChanged(PView& target, PProgressBarProperties properties, PtrTarget* context) const
{
    const PProgressBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetProgressBarStyleData();
    if (properties == PProgressBarProperties(PProgressBarProperty::Value))
    {
        const PRect bounds = target.GetNormalizedBounds().GetFloored();
        const PRect damage = PInsetSkinnedFrame(bounds, GetProgressBarOrientation(data.Orientation).FillInsets).GetFloored() & bounds;
        if (damage.IsValid()) {
            target.Invalidate(damage);
        }
    }
    else
    {
        target.Invalidate();
    }
    if (properties.Has(PProgressBarProperty::Font)) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PProgressBarOrientationSkin& PViewRendererProgressBarSkinned::GetProgressBarOrientation(POrientation orientation) const
{
    return (orientation == POrientation::Horizontal) ? m_ProgressBarSkin.Horizontal : m_ProgressBarSkin.Vertical;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererProgressBarSkinned::GetProgressBarFillFrame(const PRect& bounds, const PProgressBarOrientationSkin& skin, const PProgressBarStyleData& data)
{
    PRect fillFrame = PInsetSkinnedFrame(bounds, skin.FillInsets).GetFloored();
    const float value = PMath::is_finite(data.Value) ? std::clamp(data.Value, 0.0f, 1.0f) : 0.0f;
    if (data.Orientation == POrientation::Horizontal) {
        fillFrame.right = fillFrame.left + std::ceil(fillFrame.Width() * value);
    } else {
        fillFrame.top = fillFrame.bottom - std::ceil(fillFrame.Height() * value);
    }
    return fillFrame;
}
