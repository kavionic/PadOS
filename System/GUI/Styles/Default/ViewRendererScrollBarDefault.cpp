// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Bitmap.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleScrollBar.h>
#include <GUI/Styles/Default/ViewRendererScrollBarDefault.h>
#include <GUI/View.h>

struct PViewRendererScrollBarDefault::Context : PViewRendererContext<PViewStyleScrollBar>
{
    using PViewRendererContext<PViewStyleScrollBar>::PViewRendererContext;

    Ptr<PBitmap> DecrementArrowBitmap;
    Ptr<PBitmap> IncrementArrowBitmap;
    PScrollBarPart PressedPart = PScrollBarPart::None;
    bool HasVisualState = false;
};

struct PViewRendererScrollBarDefault::ScrollBarGeometry
{
    PRect ArrowFrames[4];
    PRect TrackFrame;
    PRect KnobFrame;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererScrollBarDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PScrollBarStyleData& data = state.GetTarget().GetScrollBarStyleData();
    const ScrollBarGeometry geometry = GetScrollBarGeometry(frame, data, data.Value);
    DrawScrollBarKnob(target, geometry.KnobFrame, data.Orientation);
    DrawScrollBarTrack(target, geometry, data.Orientation);
    DrawScrollBarArrows(target, geometry, data, state);
    state.PressedPart = data.PressedPart;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PScrollBarStyleData& data = state.GetTarget().GetScrollBarStyleData();
    if (data.Orientation == POrientation::Horizontal)
    {
        *minSize = PPoint(0.0f, SCROLL_BAR_THICKNESS);
        *maxSize = PPoint(COORD_MAX, SCROLL_BAR_THICKNESS);
    }
    else
    {
        *minSize = PPoint(SCROLL_BAR_THICKNESS, 0.0f);
        *maxSize = PPoint(SCROLL_BAR_THICKNESS, COORD_MAX);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarDefault::PropertyChanged(PView& target, PScrollBarProperties properties, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PScrollBarStyleData& data = state.GetTarget().GetScrollBarStyleData();

    const PScrollBarProperties  trackProperties(PScrollBarProperty::Value, PScrollBarProperty::Proportion, PScrollBarProperty::Range);
    const PScrollBarProperties  incrementalProperties = trackProperties | PScrollBarProperty::Pressed | PScrollBarProperty::Hovered;
    const bool                  propertiesRequireFullUpdate = !(properties & ~incrementalProperties).Empty();

    if (!state.HasVisualState || propertiesRequireFullUpdate)
    {
        target.Invalidate();
    }
    else
    {
        const ScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
        if (properties.HasAny(trackProperties) && geometry.TrackFrame.IsValid()) {
            target.Invalidate(geometry.TrackFrame);
        }
        if (properties.Has(PScrollBarProperty::Pressed))
        {
            for (size_t i = 0; i < std::size(geometry.ArrowFrames); ++i)
            {
                const PScrollBarPart part = GetScrollBarArrowPart(i);
                if ((part == state.PressedPart || part == data.PressedPart) && geometry.ArrowFrames[i].IsValid()) {
                    target.Invalidate(geometry.ArrowFrames[i]);
                }
            }
        }
    }
    state.PressedPart = data.PressedPart;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarPart PViewRendererScrollBarDefault::HitTest(const PView& target, const PPoint& position, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PScrollBarStyleData& data = state.GetTarget().GetScrollBarStyleData();
    if (!data.IsEnabled) {
        return PScrollBarPart::None;
    }
    const ScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
    for (size_t i = 0; i < std::size(geometry.ArrowFrames); ++i)
    {
        if (geometry.ArrowFrames[i].DoIntersect(position)) {
            return GetScrollBarArrowPart(i);
        }
    }
    if (geometry.KnobFrame.DoIntersect(position)) {
        return PScrollBarPart::Knob;
    }
    if (geometry.TrackFrame.DoIntersect(position))
    {
        const bool isBeforeKnob = (data.Orientation == POrientation::Horizontal) ? position.x < geometry.KnobFrame.left : position.y < geometry.KnobFrame.top;
        return isBeforeKnob ? PScrollBarPart::PageDecrement : PScrollBarPart::PageIncrement;
    }
    return PScrollBarPart::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererScrollBarDefault::GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PScrollBarStyleData& data = state.GetTarget().GetScrollBarStyleData();
    return GetScrollBarGeometry(target.GetNormalizedBounds(), data, relativeValue).KnobFrame.Center();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PViewRendererScrollBarDefault::GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PScrollBarStyleData& data = state.GetTarget().GetScrollBarStyleData();
    const ScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
    if (!geometry.TrackFrame.IsValid()) {
        return data.Value;
    }
    const bool isHorizontal = data.Orientation == POrientation::Horizontal;
    const float trackStart = isHorizontal ? geometry.TrackFrame.left : geometry.TrackFrame.top;
    const float trackLength = isHorizontal ? geometry.TrackFrame.Width() : geometry.TrackFrame.Height();
    const float knobLength = isHorizontal ? geometry.KnobFrame.Width() : geometry.KnobFrame.Height();
    const float travel = trackLength - knobLength;
    const float offset = (isHorizontal ? position.x : position.y) - trackStart - knobLength * 0.5f;
    return (travel > 0.0f) ? std::clamp(offset / travel, 0.0f, 1.0f) : data.Value;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererScrollBarDefault::ScrollBarGeometry PViewRendererScrollBarDefault::GetScrollBarGeometry(
    const PRect& frame,
    const PScrollBarStyleData& data,
    float relativeValue)
{
    ScrollBarGeometry geometry;
    if (!frame.IsValid()) {
        return geometry;
    }
    const bool  isHorizontal    = data.Orientation == POrientation::Horizontal;
    const float start           = isHorizontal ? frame.left : frame.top;
    const float end             = isHorizontal ? frame.right : frame.bottom;
    const float crossStart      = isHorizontal ? frame.top : frame.left;
    const float crossEnd        = isHorizontal ? frame.bottom : frame.right;
    const float arrowLength     = std::ceil((crossEnd - crossStart) * 0.7f);
    float       trackStart      = start + arrowLength;
    float       trackEnd        = end - arrowLength;

    if (trackStart > trackEnd)
    {
        trackStart = start + std::floor((end - start) * 0.5f);
        trackEnd = trackStart;
    }
    geometry.ArrowFrames[0] = GetScrollBarFrame(data.Orientation, start, trackStart, crossStart, crossEnd);
    geometry.ArrowFrames[3] = GetScrollBarFrame(data.Orientation, trackEnd, end, crossStart, crossEnd);
    if (trackEnd - trackStart >= arrowLength * 2.0f + 15.0f)
    {
        geometry.ArrowFrames[1] = GetScrollBarFrame(data.Orientation, trackStart, trackStart + arrowLength, crossStart, crossEnd);
        geometry.ArrowFrames[2] = GetScrollBarFrame(data.Orientation, trackEnd - arrowLength, trackEnd, crossStart, crossEnd);
        trackStart += arrowLength;
        trackEnd -= arrowLength;
    }
    geometry.TrackFrame = GetScrollBarFrame(data.Orientation, trackStart, trackEnd, crossStart + 1.0f, crossEnd - 1.0f);
    if (geometry.TrackFrame.IsValid())
    {
        const float trackLength         = trackEnd - trackStart;
        const float proportionalSize    = (trackLength + 1.0f) * std::clamp(data.Proportion, 0.0f, 1.0f);
        const float knobLength          = data.HasRange ? std::min(trackLength, std::max(SCROLL_BAR_MIN_KNOB_SIZE, proportionalSize) - 1.0f) : trackLength;
        const float knobStart           = trackStart + (trackLength - knobLength) * std::clamp(relativeValue, 0.0f, 1.0f);

        geometry.KnobFrame = GetScrollBarFrame(data.Orientation, knobStart, knobStart + knobLength, crossStart + 1.0f, crossEnd - 1.0f);
    }
    return geometry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererScrollBarDefault::GetScrollBarFrame(POrientation orientation, float start, float end, float crossStart, float crossEnd)
{
    return (orientation == POrientation::Horizontal) ? PRect(start, crossStart, end, crossEnd) : PRect(crossStart, start, crossEnd, end);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarPart PViewRendererScrollBarDefault::GetScrollBarArrowPart(size_t index)
{
    switch (index)
    {
        case 0: return PScrollBarPart::DecrementStart;
        case 1: return PScrollBarPart::IncrementStart;
        case 2: return PScrollBarPart::DecrementEnd;
        case 3: return PScrollBarPart::IncrementEnd;
        default: return PScrollBarPart::None;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PViewRendererScrollBarDefault::GetScrollBarTint(PColor color, float tint)
{
    const int red = std::clamp(int(float(color.GetRed()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    const int green = std::clamp(int(float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    const int blue = std::clamp(int(float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    return PColor(uint8_t(red), uint8_t(green), uint8_t(blue), color.GetAlpha());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarDefault::DrawScrollBarKnob(PView& target, const PRect& knobFrame, POrientation orientation)
{
    if (!knobFrame.IsValid()) {
        return;
    }
    target.SetBackgroundBrush(PStandardColorID::ScrollBarKnob);
    target.DrawFrame(knobFrame, FRAME_RAISED);
    const bool isHorizontal = orientation == POrientation::Horizontal;
    const float length = isHorizontal ? knobFrame.Width() : knobFrame.Height();
    if (length > 30.0f)
    {
        float center = (isHorizontal ? knobFrame.left : knobFrame.top) + (length + 1.0f) * 0.5f;
        for (PStandardColorID color : {PStandardColorID::Shine, PStandardColorID::Shadow})
        {
            target.SetFgColor(color);
            for (int i = -1; i <= 1; ++i)
            {
                const float position = center + float(i) * 5.0f;
                if (isHorizontal) {
                    target.DrawLine(PPoint(position, knobFrame.top + 4.0f), PPoint(position, knobFrame.bottom - 4.0f));
                } else {
                    target.DrawLine(PPoint(knobFrame.left + 4.0f, position), PPoint(knobFrame.right - 4.0f, position));
                }
            }
            center -= 1.0f;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarDefault::DrawScrollBarTrack(PView& target, const ScrollBarGeometry& geometry, POrientation orientation)
{
    const PRect& trackFrame = geometry.TrackFrame;
    if (!trackFrame.IsValid()) {
        return;
    }
    const bool isHorizontal = orientation == POrientation::Horizontal;
    PRect beforeKnob = trackFrame;
    PRect afterKnob = trackFrame;
    if (isHorizontal)
    {
        beforeKnob.right = geometry.KnobFrame.left;
        afterKnob.left = geometry.KnobFrame.right;
    }
    else
    {
        beforeKnob.bottom = geometry.KnobFrame.top;
        afterKnob.top = geometry.KnobFrame.bottom;
    }
    if (beforeKnob.IsValid()) {
        target.FillRect(beforeKnob, PStandardColorID::ScrollBarBackground);
    }
    if (afterKnob.IsValid()) {
        target.FillRect(afterKnob, PStandardColorID::ScrollBarBackground);
    }
    target.SetFgColor(GetScrollBarTint(pget_standard_color(PStandardColorID::Shadow), 0.5f));
    if (isHorizontal) {
        target.DrawLine(PPoint(trackFrame.left, trackFrame.top - 1.0f), PPoint(trackFrame.right, trackFrame.top - 1.0f));
    } else {
        target.DrawLine(PPoint(trackFrame.left - 1.0f, trackFrame.top), PPoint(trackFrame.left - 1.0f, trackFrame.bottom));
    }
    target.SetFgColor(GetScrollBarTint(pget_standard_color(PStandardColorID::Shine), 0.6f));
    if (isHorizontal) {
        target.DrawLine(PPoint(trackFrame.left, trackFrame.bottom), PPoint(trackFrame.right, trackFrame.bottom));
    } else {
        target.DrawLine(PPoint(trackFrame.right, trackFrame.top), PPoint(trackFrame.right, trackFrame.bottom));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarDefault::DrawScrollBarArrows(PView& target, const ScrollBarGeometry& geometry, const PScrollBarStyleData& data, Context& state)
{
    if (state.DecrementArrowBitmap == nullptr)
    {
        const bool isHorizontal = data.Orientation == POrientation::Horizontal;
        const int width = isHorizontal ? 4 : 7;
        const int height = isHorizontal ? 7 : 4;
        const uint32_t* decrementRaster = isHorizontal ? SCROLL_BAR_LEFT_ARROW_BITMAP_RASTER : SCROLL_BAR_UP_ARROW_BITMAP_RASTER;
        const uint32_t* incrementRaster = isHorizontal ? SCROLL_BAR_RIGHT_ARROW_BITMAP_RASTER : SCROLL_BAR_DOWN_ARROW_BITMAP_RASTER;
        state.DecrementArrowBitmap = ptr_new<PBitmap>(width, height, PEColorSpace::MONO1, decrementRaster, sizeof(uint32_t));
        state.IncrementArrowBitmap = ptr_new<PBitmap>(width, height, PEColorSpace::MONO1, incrementRaster, sizeof(uint32_t));
    }
    target.SetBackgroundBrush(PStandardColorID::ScrollBarKnob);
    for (size_t i = 0; i < std::size(geometry.ArrowFrames); ++i)
    {
        const PRect& arrowFrame = geometry.ArrowFrames[i];
        if (arrowFrame.IsValid())
        {
            const Ptr<PBitmap>& arrowBitmap = (i & 0x01) ? state.IncrementArrowBitmap : state.DecrementArrowBitmap;
            const PRect bitmapBounds(arrowBitmap->GetBounds());
            const PPoint arrowPosition = arrowFrame.TopLeft() + (arrowFrame.Size() - bitmapBounds.Size()) * 0.5f;
            const bool isPressed = data.PressedPart == GetScrollBarArrowPart(i);
            target.DrawFrame(arrowFrame, (isPressed ? FRAME_RECESSED : FRAME_RAISED) | FRAME_THIN);
            target.SetDrawingMode(PDrawingMode::Overlay);
            target.SetFgColor(0, 0, 0);
            target.DrawBitmap(arrowBitmap, bitmapBounds, arrowPosition);
            target.SetDrawingMode(PDrawingMode::Copy);
        }
    }
}
