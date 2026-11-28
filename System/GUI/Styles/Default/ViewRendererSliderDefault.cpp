// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/SliderStyleLayout.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleSlider.h>
#include <GUI/Styles/Default/ViewRendererSliderDefault.h>
#include <GUI/View.h>

struct PViewRendererSliderDefault::Context : PViewRendererContext<PViewStyleSlider>
{
    using PViewRendererContext<PViewStyleSlider>::PViewRendererContext;

    PSliderStyleLayout Layout;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererSliderDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PSliderStyleData& data = state.GetTarget().GetSliderStyleData();
    target.SetBackgroundBrush(PStandardColorID::SliderTrackNormal);

    PSliderStyleLayout& layout = GetSliderLayout(data, state);
    const PRect& bounds = frame;

    const PRect sliderFrame = layout.GetTrackFrame(target, bounds, data);
    const PPoint knobPosition = PSliderStyleLayout::GetPosition(sliderFrame, data.Orientation, data.Value);
    const PRect knobBounds = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame);
    const PRect knobFrame = knobBounds + knobPosition;

    layout.ClearBackground(target, bounds, sliderFrame, knobFrame, data);
    const PRect labelFrame = layout.GetValueLabelFrame(target, data);

    PSliderStyleLayout::DrawTicks(target, sliderFrame, data, pget_standard_color(PStandardColorID::Shadow), pget_standard_color(PStandardColorID::Shine));
    layout.DrawLabels(target, bounds, sliderFrame, data, PColor(0, 0, 0));

    DrawSliderTrack(target, sliderFrame, knobFrame, data);
    for (float value : data.ShadowValues) {
        DrawSliderKnob(target, sliderFrame, data, PStandardColorID::SliderKnobShadow, value);
    }

    const PStandardColorID knobColor = data.IsPressed ? PStandardColorID::SliderKnobPressed : PStandardColorID::SliderKnobNormal;
    DrawSliderKnob(target, sliderFrame, data, knobColor, data.Value);
    if (labelFrame.IsValid())
    {
        target.SetFgColor(0, 0, 0);
        target.SetBrush(PBrush::zero);
        target.DrawString(data.ValueLabel, labelFrame.TopLeft() + PPoint(0.0f, target.GetFontHeight().ascender), labelFrame);
    }
    if (!layout.HasVisualState()) {
        layout.UpdateDamage(target, data, PSliderProperties(), false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PSliderStyleData& data = state.GetTarget().GetSliderStyleData();
    GetSliderLayout(data, state).GetPreferredSize(target, data, minSize, maxSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderDefault::PropertyChanged(PView& target, PSliderProperties properties, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PSliderStyleData& data = state.GetTarget().GetSliderStyleData();
    GetSliderLayout(data, state).PropertyChanged(target, data, properties);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererSliderDefault::GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PSliderStyleData& data = state.GetTarget().GetSliderStyleData();
    return GetSliderLayout(data, state).GetValuePosition(target, data, relativeValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PViewRendererSliderDefault::GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PSliderStyleData& data = state.GetTarget().GetSliderStyleData();
    return GetSliderLayout(data, state).GetValueFromPosition(target, data, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderStyleLayout& PViewRendererSliderDefault::GetSliderLayout(const PSliderStyleData& data, Context& state)
{
    PSliderStyleLayout& layout = state.Layout;
    layout.SetKnobBounds(GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame));
    return layout;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererSliderDefault::GetSliderKnobBounds(const PSliderStyleData& data, POrientation orientation, SliderKnobFrameMode mode)
{
    PRect solidFrame(-7.0f, -10.0f, 8.0f, 11.0f);

    if (data.KnobPointUp || data.KnobPointDown)
    {
        solidFrame.top      += 4.0f;
        solidFrame.bottom   -= 4.0f;
    }
    if (data.KnobPointUp)
    {
        if (mode == SliderKnobFrameMode::FullFrame) {
            solidFrame.top -= 10;
        } else {
            solidFrame.top -= 2;
        }
    }
    if (data.KnobPointDown)
    {
        if (mode == SliderKnobFrameMode::FullFrame) {
            solidFrame.bottom += 10;
        } else {
            solidFrame.bottom += 2;
        }
    }
    if (orientation == POrientation::Vertical)
    {
        std::swap(solidFrame.left, solidFrame.top);
        std::swap(solidFrame.right, solidFrame.bottom);
    }
    return solidFrame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderDefault::DrawSliderTrack(PView& target, const PRect& sliderFrame, const PRect& knobFrame, const PSliderStyleData& data)
{
    if (sliderFrame.IsValid())
    {
        const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
        const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);

        if (data.Orientation == POrientation::Horizontal)
        {
            PRect leftSliderFrame(sliderFrame.left, sliderFrame.top, knobFrame.left, sliderFrame.bottom);
            PRect rightSliderFrame(knobFrame.right - 1.0f, sliderFrame.top, sliderFrame.right, sliderFrame.bottom);
            if (leftSliderFrame.IsValid())
            {
                PRect leftSliderCenter = leftSliderFrame;
                leftSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                leftSliderCenter.Resize(1.0f, 1.0f, 0.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(leftSliderFrame.BottomLeft());
                target.DrawLine(leftSliderFrame.TopLeft());
                target.DrawLine(leftSliderFrame.TopRight());

                target.SetFgColor(shineColor);
                target.MovePenTo(leftSliderFrame.BottomRight());
                target.DrawLine(PPoint(leftSliderFrame.left + 1, leftSliderFrame.bottom));

                target.FillRect(leftSliderCenter, data.SliderColor1);
            }
            if (rightSliderFrame.IsValid())
            {
                PRect rightSliderCenter = rightSliderFrame;
                rightSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                rightSliderCenter.Resize(0.0f, 1.0f, -1.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(rightSliderFrame.TopLeft());
                target.DrawLine(rightSliderFrame.TopRight());

                target.SetFgColor(shineColor);
                target.MovePenTo(PPoint(rightSliderFrame.right, rightSliderFrame.top + 1.0f));
                target.DrawLine(rightSliderFrame.BottomRight());
                target.DrawLine(rightSliderFrame.BottomLeft() + PPoint(1.0f, 0.0f));

                target.FillRect(rightSliderCenter, data.SliderColor2);
            }
        }
        else
        {
            PRect topSliderFrame(sliderFrame.left, sliderFrame.top, sliderFrame.right, knobFrame.top);
            PRect bottomSliderFrame(sliderFrame.left, knobFrame.bottom - 1.0f, sliderFrame.right, sliderFrame.bottom);
            if (topSliderFrame.IsValid())
            {
                PRect topSliderCenter = topSliderFrame;
                topSliderCenter.Resize(1.0f, 1.0f, -1.0f, 0.0f);
                topSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(topSliderFrame.BottomLeft());
                target.DrawLine(topSliderFrame.TopLeft());
                target.DrawLine(topSliderFrame.TopRight());

                target.SetFgColor(shineColor);
                target.MovePenTo(topSliderFrame.BottomRight());
                target.DrawLine(PPoint(topSliderFrame.right, topSliderFrame.top + 1.0f));

                target.FillRect(topSliderCenter, data.SliderColor1);
            }
            if (bottomSliderFrame.IsValid())
            {
                PRect bottomSliderCenter = bottomSliderFrame;
                bottomSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                bottomSliderCenter.Resize(1.0f, 0.0f, -1.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(bottomSliderFrame.TopLeft());
                target.DrawLine(bottomSliderFrame.BottomLeft());

                target.SetFgColor(shineColor);
                target.MovePenTo(PPoint(bottomSliderFrame.left + 1.0f, bottomSliderFrame.bottom));
                target.DrawLine(bottomSliderFrame.BottomRight());
                target.DrawLine(bottomSliderFrame.TopRight() + PPoint(0.0f, 1.0f));

                target.FillRect(bottomSliderCenter, data.SliderColor2);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderDefault::DrawSliderKnob(
    PView& target,
    const PRect& trackFrame,
    const PSliderStyleData& data,
    PStandardColorID knobColor,
    float relativeValue)
{
    const PPoint knobPosition = PSliderStyleLayout::GetPosition(trackFrame, data.Orientation, relativeValue);
    PRect knobFrame = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::SquareFrame) + knobPosition;

    if (!data.KnobPointUp && !data.KnobPointDown)
    {
        PRect center(knobFrame);
        center.Resize(1.0f, 1.0f, -1.0f, -1.0f);
        knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

        target.SetFgColor(PStandardColorID::Shine);
        target.MovePenTo(knobFrame.BottomLeft());
        target.DrawLine(knobFrame.TopLeft());
        target.DrawLine(knobFrame.TopRight());

        target.SetFgColor(PStandardColorID::Shadow);
        target.DrawLine(knobFrame.BottomRight());
        target.DrawLine(knobFrame.BottomLeft() + PPoint(1.0f, 0.0f));
        target.FillRect(center, knobColor);
    }
    else
    {
        PRect knobFullFrame = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame) + knobPosition;

        knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
        knobFullFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

        if (data.Orientation == POrientation::Horizontal) {
            DrawHorizontalSliderKnob(target, knobFrame, knobFullFrame, data, knobColor);
        } else {
            DrawVerticalSliderKnob(target, knobFrame, knobFullFrame, data, knobColor);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderDefault::DrawHorizontalSliderKnob(
    PView& target,
    const PRect& knobFrame,
    const PRect& knobFullFrame,
    const PSliderStyleData& data,
    PStandardColorID knobColor)
{
    const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
    const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);
    const float center = std::floor(knobFrame.left + (knobFrame.right - knobFrame.left) * 0.5f);

    target.SetFgColor(knobColor);
    float offset = 0.0f;
    float direction = 1.0f;
    for (float x = knobFrame.left; x <= knobFrame.right; x += 1.0f)
    {
        float y1 = knobFrame.top;
        float y2 = knobFrame.bottom;

        if (data.KnobPointUp)     y1 -= offset;
        if (data.KnobPointDown)   y2 += offset;

        if (data.KnobPointUp) {
            target.EraseRect(PRect(x, knobFullFrame.top, x + 1.0f, y1));
        }
        if (data.KnobPointDown) {
            target.EraseRect(PRect(x, y2 + 1.0f, x + 1.0f, knobFullFrame.bottom + 1.0f));
        }
        target.DrawLine(x, y1, x, y2);
        if (direction > 0.0f && x >= center) {
            direction = -1.0f;
        }
        offset += direction;
    }

    target.SetFgColor(shadowColor);

    if (data.KnobPointDown) {
        target.MovePenTo(center, knobFullFrame.bottom);
    } else {
        target.MovePenTo(knobFrame.BottomLeft());
    }
    target.DrawLine(knobFrame.BottomRight());
    target.DrawLine(knobFrame.TopRight());
    if (data.KnobPointUp) {
        target.DrawLine(center, knobFullFrame.top);
    }
    target.SetFgColor(shineColor);
    target.DrawLine(knobFrame.TopLeft());
    target.DrawLine(knobFrame.BottomLeft());
    if (data.KnobPointDown) {
        target.DrawLine(center, knobFullFrame.bottom);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderDefault::DrawVerticalSliderKnob(
    PView& target,
    const PRect& knobFrame,
    const PRect& knobFullFrame,
    const PSliderStyleData& data,
    PStandardColorID knobColor)
{
    const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
    const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);
    const float center = std::floor(knobFrame.top + (knobFrame.bottom - knobFrame.top) * 0.5f);

    target.SetFgColor(knobColor);
    float offset = 0.0f;
    float direction = 1.0f;
    for (float y = knobFrame.top; y <= knobFrame.bottom; y += 1.0f)
    {
        float x1 = knobFrame.left;
        float x2 = knobFrame.right;

        if (data.KnobPointUp)  x1 -= offset;
        if (data.KnobPointDown) x2 += offset;

        if (data.KnobPointUp) {
            target.EraseRect(PRect(knobFullFrame.left, y, x1, y + 1.0f));
        }
        if (data.KnobPointDown) {
            target.EraseRect(PRect(x2 + 1.0f, y, knobFullFrame.right + 1.0f, y + 1.0f));
        }
        target.DrawLine(x1, y, x2, y);
        if (direction > 0.0f && y >= center) {
            direction = -1.0f;
        }
        offset += direction;
    }

    target.SetFgColor(shineColor);
    if (data.KnobPointUp) {
        target.MovePenTo(knobFullFrame.left, center);
    } else {
        target.MovePenTo(knobFrame.BottomLeft());
    }
    target.DrawLine(knobFrame.TopLeft());
    target.DrawLine(knobFrame.TopRight());
    if (data.KnobPointDown) {
        target.DrawLine(knobFullFrame.right, center);
    }
    target.SetFgColor(shadowColor);
    target.DrawLine(knobFrame.BottomRight());
    target.DrawLine(knobFrame.BottomLeft());
    if (data.KnobPointUp) {
        target.DrawLine(knobFullFrame.left, center);
    }
}
