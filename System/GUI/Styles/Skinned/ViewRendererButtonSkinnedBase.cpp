// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Styles/Skinned/ViewRendererButtonSkinnedBase.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererButtonSkinnedBase::Setup(PView& target)
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonSkinnedBase::PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context)
{
    if (properties.HasAny(PButtonProperties(PButtonProperty::Label, PButtonProperty::Font))) {
        static_cast<Context*>(context)->IsLabelSizeValid = false;
    }
    target.Invalidate();
    if (properties.HasAny(PButtonProperties(PButtonProperty::Label, PButtonProperty::Font, PButtonProperty::LabelAlignment))) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonSkinnedBase::UpdateMetrics(const PView& target, const PButtonStyleData& data, Context& context)
{
    if (!context.IsLabelSizeValid)
    {
        context.LabelSize.x = target.GetStringWidth(data.Label);
        const PFontHeight fontHeight = target.GetFontHeight();
        context.LabelSize.y = fontHeight.descender + fontHeight.ascender + fontHeight.line_gap;
        context.IsLabelSizeValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinState PViewRendererButtonSkinnedBase::GetButtonState(bool isEnabled, const PButtonStyleData& data)
{
    PButtonSkinState state = PButtonSkinState::Normal;
    if (!isEnabled) {
        state = PButtonSkinState::Disabled;
    } else if (data.IsPressed) {
        state = PButtonSkinState::Pressed;
    } else if (data.IsPointerOver) {
        state = PButtonSkinState::Hovered;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonSkinnedBase::PaintIndicatorButton(
    PView& target,
    const PRect& frame,
    bool isEnabled,
    const PButtonStyleData& data,
    const PButtonIndicatorSkin& skin,
    const PPoint& indicatorPosition,
    const PPoint& labelPosition,
    bool labelOverlapsIndicator)
{
    const PButtonSkinState state = GetButtonState(isEnabled, data);
    const PButtonSkinAppearance& appearance = data.IsChecked ? skin.CheckedStates[std::to_underlying(state)] : skin.States[std::to_underlying(state)];
    const Ptr<const PBrush>& normal = skin.States[std::to_underlying(PButtonSkinState::Normal)].Background;
    const Ptr<const PBrush>& brush = (appearance.Background != nullptr) ? appearance.Background : normal;
    target.SetBackgroundBrush(nullptr);
    target.SetBrush(nullptr);
    target.SetFgColor(appearance.LabelColor);
    target.SetBrushOrigin(indicatorPosition);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    const PRect indicatorFrame(indicatorPosition, indicatorPosition + PPoint(skin.Size));
    if (labelOverlapsIndicator)
    {
        PaintOverlappingIndicator(target, frame, data.Label, labelPosition, indicatorFrame, brush);
    }
    else
    {
        target.DrawString(data.Label, labelPosition, frame);
        target.FillRect(indicatorFrame, brush);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonSkinnedBase::PaintOverlappingIndicator(
    PView& target,
    const PRect& frame,
    const PString& label,
    const PPoint& labelPosition,
    const PRect& indicatorFrame,
    Ptr<const PBrush> brush)
{
    const PPoint scrollOffset = target.GetScrollOffset();
    const PIRect paintFrame = PIRect(frame + scrollOffset) & PIRect(target.GetNormalizedBounds());
    if (!paintFrame.IsValid()) {
        return;
    }
    const PIRect imageFrame(indicatorFrame + scrollOffset);
    const Ptr<const PRegion> previousRegion = target.GetDrawingRegion();
    Ptr<PRegion> outside = (previousRegion != nullptr) ? ptr_new<PRegion>(*previousRegion, paintFrame, false) : ptr_new<PRegion>(paintFrame);
    const Ptr<const PRegion> inside = ptr_new<PRegion>(*outside, imageFrame, false);
    outside->Exclude(imageFrame);
    PScopeExit restoreRegion([&target, previousRegion] { target.SetDrawingRegion(previousRegion); });

    // Composite text against the image only within its bounds; image brushes repeat.
    target.SetDrawingRegion(outside);
    target.DrawString(label, labelPosition, frame);
    target.SetDrawingRegion(inside);
    target.DrawString(std::move(brush), label, labelPosition, frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PViewRendererButtonSkinnedBase::GetSpacedExtent(float firstSize, float secondSize, float spacing)
{
    return std::max({firstSize, secondSize, firstSize + spacing + secondSize, -spacing});
}
