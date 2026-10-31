// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Style/SliderStyleData.h>
#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Bitmap.h>
#include <GUI/View.h>
#include <GUI/Region.h>

// Measurement queries never advance the appearance snapshot used for damage.
struct PDefaultGUIStyle::SliderStyleData : PtrTarget
{
    PPoint  ValueLabelSize;
    float   MinLabelWidth = 0.0f;
    float   MaxLabelWidth = 0.0f;
    PRect   KnobFrame;
    PRect   ValueLabelFrame;
    std::vector<PRect> ShadowKnobFrames;
    bool    IsValueLabelSizeValid = false;
    bool    AreLimitLabelWidthsValid = false;
    bool    HasVisualState = false;
};

struct PDefaultGUIStyle::ScrollBarStyleData : PtrTarget
{
    Ptr<PBitmap> DecrementArrowBitmap;
    Ptr<PBitmap> IncrementArrowBitmap;
    PScrollBarPart PressedPart = PScrollBarPart::None;
    bool HasVisualState = false;
};

struct PDefaultGUIStyle::ScrollBarGeometry
{
    PRect ArrowFrames[4];
    PRect TrackFrame;
    PRect KnobFrame;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const
{
    UpdateButtonMetrics(target, data);

    const PStandardColorID backgroundColor =
        (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::ButtonBackground;
    target.SetEraseColor(backgroundColor);

    if (isEnabled) {
        target.DrawFrame(frame, (data.IsPressed ? FRAME_RECESSED : FRAME_RAISED) | FRAME_TRANSPARENT);
    } else {
        target.DrawFrame(frame, FRAME_DISABLED | FRAME_TRANSPARENT);
    }
    PPoint labelPosition(std::round((frame.Width() - data.LabelSize.x) * 0.5f), std::round((frame.Height() - data.LabelSize.y) * 0.5f));

    if (data.IsPressed) {
        labelPosition += PPoint(1.0f, 1.0f);
    }

    labelPosition.y += target.GetFontHeight().ascender;
    target.SetFgColor(isEnabled ? PStandardColorID::ButtonLabelNormal : PStandardColorID::ButtonLabelDisabled);
    target.SetBgColor(backgroundColor);
    target.DrawString(data.Label, labelPosition, frame.GetFloored().GetResized(2.0f, 2.0f, -2.0f, -2.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    UpdateButtonMetrics(target, data);
    const PPoint size(data.LabelSize.x + 16.0f, data.LabelSize.y + 8.0f);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::CheckBoxPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const
{
    const PStandardColorID backgroundColor =
        (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    const PFontHeight fontHeight = target.GetFontHeight();
    const float baseline = frame.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender;
    target.SetFgColor(0, 0, 0);
    target.SetBgColor(PStandardColorID::DefaultBackground);
    target.DrawString(data.Label, PPoint(CHECK_BOX_SIZE + CHECK_BOX_LABEL_SPACING, baseline), frame);

    target.SetEraseColor(backgroundColor);

    const float buttonTop = (frame.Height() - CHECK_BOX_SIZE) * 0.5f;
    const PRect buttonFrame(0.0f, buttonTop, CHECK_BOX_SIZE, buttonTop + CHECK_BOX_SIZE);
    target.DrawFrame(buttonFrame, FRAME_RECESSED);

    if (data.IsChecked)
    {
        target.SetFgColor(0, 0, 0);
        PPoint point1(buttonFrame.left + CHECK_BOX_CHECK_BORDER, buttonFrame.top + CHECK_BOX_CHECK_BORDER);
        PPoint point2(buttonFrame.right - CHECK_BOX_CHECK_BORDER, buttonFrame.bottom - CHECK_BOX_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            target.DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
        point1 = PPoint(buttonFrame.left + CHECK_BOX_CHECK_BORDER, buttonFrame.bottom - CHECK_BOX_CHECK_BORDER);
        point2 = PPoint(buttonFrame.right - CHECK_BOX_CHECK_BORDER, buttonFrame.top + CHECK_BOX_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            target.DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::CheckBoxGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    PPoint size(CHECK_BOX_SIZE, CHECK_BOX_SIZE);
    if (!data.Label.empty())
    {
        UpdateButtonMetrics(target, data);
        const PFontHeight fontHeight = target.GetFontHeight();
        size.x += CHECK_BOX_LABEL_SPACING + data.LabelSize.x;
        size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender);
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::RadioButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const
{
    const PStandardColorID backgroundColor =
        (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    const PRect buttonFrame = DrawRadioButtonLabel(target, frame, data);

    target.SetEraseColor(PStandardColorID::DefaultBackground);
    target.SetFgColor(PStandardColorID::Shadow);

    float radius = std::floor(RADIO_BUTTON_SIZE * 0.5f);
    const PPoint center(std::floor(buttonFrame.left + buttonFrame.Width() * 0.5f), std::floor(buttonFrame.top + buttonFrame.Height() * 0.5f));

    radius -= 1.0f;
    target.FillCircle(center, radius);

    target.SetFgColor(backgroundColor);
    radius -= 3.0f;
    target.FillCircle(center, radius);

    if (data.IsChecked)
    {
        target.SetFgColor(PStandardColorID::Shadow);
        radius -= 3.0f;
        target.FillCircle(center, radius);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::RadioButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    PPoint size(RADIO_BUTTON_SIZE, RADIO_BUTTON_SIZE);
    if (!data.Label.empty())
    {
        UpdateButtonMetrics(target, data);
        const PFontHeight fontHeight = target.GetFontHeight();
        if (data.LabelAlignment == PAlignment::Top || data.LabelAlignment == PAlignment::Bottom)
        {
            size.x = std::max(size.x, data.LabelSize.x);
            size.y += RADIO_BUTTON_LABEL_VERTICAL_SPACING + fontHeight.ascender + fontHeight.descender;
        }
        else
        {
            size.x += RADIO_BUTTON_LABEL_HORIZONTAL_SPACING + data.LabelSize.x;
            size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender);
        }
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::SliderPaint(PView& target, const PRect& frame, const PSliderStyleData& data) const
{
    target.SetEraseColor(PStandardColorID::SliderTrackNormal);

    const PRect& bounds = frame;

    const PRect sliderFrame = GetSliderTrackFrame(target, bounds, data);
    const PPoint knobPosition = GetSliderPosition(sliderFrame, data.Orientation, data.Value);
    const PRect knobBounds = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame);
    const PRect knobFrame = knobBounds + knobPosition;

    // Clear areas not fully overwritten by the slider track and the knob.
    PRegion clearRegion(bounds);
    clearRegion.Exclude(sliderFrame);
    clearRegion.Exclude(knobFrame);
    const PRect labelFrame = GetSliderValueLabelFrame(target, data);
    if (labelFrame.IsValid()) {
        clearRegion.Exclude(labelFrame);
    }
    if (data.Orientation == POrientation::Horizontal)
    {
        if (!data.MinLabel.empty() && !data.MaxLabel.empty())
        {
            const float top = sliderFrame.bottom + SLIDER_TICK_SPACING + SLIDER_TICK_LENGTH + SLIDER_HORIZONTAL_LABEL_SPACING;
            clearRegion.Exclude(PRect(bounds.left, top, bounds.right, bounds.bottom));
        }
    }
    else
    {
        const PRect knobBounds = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame);
        const float offset = std::ceil(knobBounds.Height() * 0.5f) + SLIDER_VERTICAL_LABEL_SPACING;
        if (!data.MaxLabel.empty()) {
            clearRegion.Exclude(PRect(bounds.left, bounds.top, bounds.right, sliderFrame.top - offset));
        }
        if (!data.MinLabel.empty()) {
            clearRegion.Exclude(PRect(bounds.left, sliderFrame.bottom + offset, bounds.right, bounds.bottom));
        }
    }
    clearRegion.Optimize();
    for (const PIRect& rect : clearRegion.m_Rects) {
        target.EraseRect(rect);
    }

    DrawSliderTicks(target, sliderFrame, data);
    DrawSliderLabels(target, bounds, sliderFrame, data);

    DrawSliderTrack(target, sliderFrame, knobFrame, data);
    for (float value : data.ShadowValues) {
        DrawSliderKnob(target, sliderFrame, data, PStandardColorID::SliderKnobShadow, value);
    }

    const PStandardColorID knobColor = data.IsPressed ? PStandardColorID::SliderKnobPressed : PStandardColorID::SliderKnobNormal;
    DrawSliderKnob(target, sliderFrame, data, knobColor, data.Value);
    if (labelFrame.IsValid())
    {
        target.SetFgColor(0, 0, 0);
        target.SetBgColor(target.GetEraseColor());
        target.DrawString(data.ValueLabel, labelFrame.TopLeft() + PPoint(0.0f, target.GetFontHeight().ascender), labelFrame);
    }
    if (!data.GetOrCreateStyleData<SliderStyleData>().HasVisualState) {
        UpdateSliderDamage(target, data, PSliderProperties(), false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::SliderGetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    PRect totalFrame;
    float minimumLength;
    GetSliderTrackFrame(target, target.GetNormalizedBounds(), data, &totalFrame, &minimumLength);
    if (data.Orientation == POrientation::Horizontal)
    {
        *minSize = PPoint(minimumLength, totalFrame.Height());
        *maxSize = PPoint(2000.0f, minSize->y);
    }
    else
    {
        *minSize = PPoint(totalFrame.Width(), minimumLength);
        *maxSize = PPoint(minSize->x, 2000.0f);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::SliderPropertyChanged(PView& target, PSliderStyleData& data, PSliderProperties properties) const
{
    SliderStyleData& state = data.GetOrCreateStyleData<SliderStyleData>();
    if (properties.HasAny(PSliderProperties(PSliderProperty::ValueLabel, PSliderProperty::Font))) {
        state.IsValueLabelSizeValid = false;
    }
    if (properties.HasAny(PSliderProperties(PSliderProperty::MinLabel, PSliderProperty::MaxLabel, PSliderProperty::Font))) {
        state.AreLimitLabelWidthsValid = false;
    }
    const bool labelHeightChanged = state.ValueLabelFrame.Height() != GetSliderValueLabelFrame(target, data).Height();
    UpdateSliderDamage(target, data, properties, true);

    const PSliderProperties sizeProperties(
        PSliderProperty::Label,
        PSliderProperty::MinLabel,
        PSliderProperty::MaxLabel,
        PSliderProperty::Font,
        PSliderProperty::SliderSize,
        PSliderProperty::TickCount,
        PSliderProperty::Appearance);
    if (labelHeightChanged || properties.HasAny(sizeProperties)) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PDefaultGUIStyle::SliderGetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue) const
{
    const PRect trackFrame = GetSliderTrackFrame(target, target.GetNormalizedBounds(), data);
    return GetSliderPosition(trackFrame, data.Orientation, relativeValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PDefaultGUIStyle::SliderGetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position) const
{
    const PRect trackFrame = GetSliderTrackFrame(target, target.GetNormalizedBounds(), data);
    const bool isHorizontal = data.Orientation == POrientation::Horizontal;
    const float length = isHorizontal ? trackFrame.Width() : trackFrame.Height();
    if (length <= 0.0f) {
        return data.Value;
    }
    const float relativeValue = isHorizontal ? (position.x - trackFrame.left) / length : (trackFrame.bottom - position.y) / length;
    return std::clamp(relativeValue, 0.0f, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ScrollBarPaint(PView& target, const PRect& frame, const PScrollBarStyleData& data) const
{
    const ScrollBarGeometry geometry = GetScrollBarGeometry(frame, data, data.Value);
    DrawScrollBarKnob(target, geometry.KnobFrame, data.Orientation);
    DrawScrollBarTrack(target, geometry, data.Orientation);
    DrawScrollBarArrows(target, geometry, data);
    ScrollBarStyleData& state = data.GetOrCreateStyleData<ScrollBarStyleData>();
    state.PressedPart = data.PressedPart;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ScrollBarGetPreferredSize(const PView& target, const PScrollBarStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
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

void PDefaultGUIStyle::ScrollBarPropertyChanged(PView& target, PScrollBarStyleData& data, PScrollBarProperties properties) const
{
    ScrollBarStyleData& state = data.GetOrCreateStyleData<ScrollBarStyleData>();

    const PScrollBarProperties  trackProperties(PScrollBarProperty::Value, PScrollBarProperty::Proportion, PScrollBarProperty::Range);
    const PScrollBarProperties  incrementalProperties = trackProperties | PScrollBarProperty::Pressed;
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

PScrollBarPart PDefaultGUIStyle::ScrollBarHitTest(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const
{
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
        const bool isBeforeKnob = (data.Orientation == POrientation::Horizontal)
            ? position.x < geometry.KnobFrame.left : position.y < geometry.KnobFrame.top;
        return isBeforeKnob ? PScrollBarPart::PageDecrement : PScrollBarPart::PageIncrement;
    }
    return PScrollBarPart::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PDefaultGUIStyle::ScrollBarGetValuePosition(const PView& target, const PScrollBarStyleData& data, float relativeValue) const
{
    return GetScrollBarGeometry(target.GetNormalizedBounds(), data, relativeValue).KnobFrame.Center();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PDefaultGUIStyle::ScrollBarGetValueFromPosition(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const
{
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

void PDefaultGUIStyle::UpdateButtonMetrics(const PView& target, PButtonStyleData& data)
{
    if (!data.IsLabelSizeValid)
    {
        data.LabelSize.x = target.GetStringWidth(data.Label);
        const PFontHeight fontHeight = target.GetFontHeight();
        data.LabelSize.y = fontHeight.descender + fontHeight.ascender + fontHeight.line_gap;
        data.IsLabelSizeValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::DrawRadioButtonLabel(PView& target, const PRect& frame, PButtonStyleData& data)
{
    PRect buttonFrame = frame;
    if (!data.Label.empty())
    {
        UpdateButtonMetrics(target, data);
        const PFontHeight fontHeight = target.GetFontHeight();
        const float labelWidth = data.LabelSize.x;
        const PAlignment labelAlignment = data.LabelAlignment;
        if (labelAlignment == PAlignment::Top || labelAlignment == PAlignment::Bottom)
        {
            const float labelLeft = std::floor((frame.Width() - labelWidth) * 0.5f);
            if (labelAlignment == PAlignment::Top)
            {
                target.MovePenTo(labelLeft, fontHeight.ascender);
                buttonFrame.top += fontHeight.ascender + fontHeight.descender + RADIO_BUTTON_LABEL_VERTICAL_SPACING;
            }
            else
            {
                target.MovePenTo(labelLeft, RADIO_BUTTON_SIZE + RADIO_BUTTON_LABEL_VERTICAL_SPACING + fontHeight.ascender);
                buttonFrame.bottom -= fontHeight.ascender + fontHeight.descender + RADIO_BUTTON_LABEL_VERTICAL_SPACING;
            }
        }
        else
        {
            const float baseline = std::floor(frame.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender);
            if (labelAlignment == PAlignment::Left)
            {
                target.MovePenTo(0.0f, baseline);
                buttonFrame.left += labelWidth + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING;
            }
            else
            {
                target.MovePenTo(RADIO_BUTTON_SIZE + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING, baseline);
                buttonFrame.right -= labelWidth + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING;
            }
        }
        target.SetFgColor(0, 0, 0);
        target.SetBgColor(PStandardColorID::DefaultBackground);
        target.DrawString(data.Label, target.GetPenPosition(), frame);
    }
    else
    {
        target.FillRect(frame, pget_standard_color(PStandardColorID::DefaultBackground));
    }
    return buttonFrame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateSliderMetrics(const PView& target, const PSliderStyleData& data)
{
    SliderStyleData& state = data.GetOrCreateStyleData<SliderStyleData>();
    if (!state.IsValueLabelSizeValid)
    {
        const PFontHeight fontHeight = target.GetFontHeight();
        state.ValueLabelSize = PPoint(target.GetStringWidth(data.ValueLabel), fontHeight.ascender + fontHeight.descender);
        state.IsValueLabelSizeValid = true;
    }
    if (!state.AreLimitLabelWidthsValid)
    {
        state.MinLabelWidth = target.GetStringWidth(data.MinLabel);
        state.MaxLabelWidth = target.GetStringWidth(data.MaxLabel);
        state.AreLimitLabelWidthsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateSliderDamage(PView& target, const PSliderStyleData& data, PSliderProperties properties, bool invalidate)
{
    SliderStyleData& state = data.GetOrCreateStyleData<SliderStyleData>();
    const PRect trackFrame = GetSliderTrackFrame(target, target.GetNormalizedBounds(), data);
    const PRect knobBounds = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame);
    const PRect knobFrame = knobBounds + GetSliderPosition(trackFrame, data.Orientation, data.Value);
    const PRect labelFrame = GetSliderValueLabelFrame(target, data);
    const PSliderProperties incrementalProperties(
        PSliderProperty::Value,
        PSliderProperty::ValueLabel,
        PSliderProperty::ShadowValues,
        PSliderProperty::Pressed);
    const bool propertiesRequireFullUpdate = !(properties & ~incrementalProperties).Empty();
    const bool fullUpdate = !state.HasVisualState || propertiesRequireFullUpdate || labelFrame.Height() != state.ValueLabelFrame.Height();
    if (invalidate)
    {
        if (fullUpdate)
        {
            target.Invalidate();
        }
        else
        {
            if (properties.HasAny(PSliderProperties(PSliderProperty::Value, PSliderProperty::Pressed))) {
                InvalidateSliderFrames(target, state.KnobFrame, knobFrame);
            }
            if (properties.Has(PSliderProperty::ValueLabel)) {
                InvalidateSliderFrames(target, state.ValueLabelFrame, labelFrame);
            }
        }
    }
    if (fullUpdate || properties.Has(PSliderProperty::ShadowValues)) {
        UpdateSliderShadowDamage(target, data, trackFrame, knobBounds, invalidate && !fullUpdate);
    }

    // Advance only after queuing damage. Repeated changes before a paint then
    // accumulate all affected regions, including any intermediate positions.
    state.KnobFrame = knobFrame;
    state.ValueLabelFrame = labelFrame;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateSliderShadowDamage(
    PView& target,
    const PSliderStyleData& data,
    const PRect& trackFrame,
    const PRect& knobBounds,
    bool invalidate)
{
    SliderStyleData& state = data.GetOrCreateStyleData<SliderStyleData>();
    const size_t previousCount = state.ShadowKnobFrames.size();
    if (invalidate)
    {
        for (size_t i = data.ShadowValues.size(); i < previousCount; ++i) {
            InvalidateSliderFrames(target, state.ShadowKnobFrames[i], PRect());
        }
    }
    state.ShadowKnobFrames.resize(data.ShadowValues.size());
    for (size_t i = 0; i < data.ShadowValues.size(); ++i)
    {
        const PRect frame = knobBounds + GetSliderPosition(trackFrame, data.Orientation, data.ShadowValues[i]);
        if (invalidate && (i >= previousCount || frame != state.ShadowKnobFrames[i])) {
            InvalidateSliderFrames(target, state.ShadowKnobFrames[i], frame);
        }
        state.ShadowKnobFrames[i] = frame;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::InvalidateSliderFrames(PView& target, const PRect& previousFrame, const PRect& currentFrame)
{
    if (previousFrame.IsValid()) {
        target.Invalidate(currentFrame.IsValid() ? previousFrame | currentFrame : previousFrame);
    } else if (currentFrame.IsValid()) {
        target.Invalidate(currentFrame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::GetSliderValueLabelFrame(const PView& target, const PSliderStyleData& data)
{
    if (data.Orientation == POrientation::Horizontal && data.HasValueLabel)
    {
        UpdateSliderMetrics(target, data);
        return PRect::FromSize(data.GetOrCreateStyleData<SliderStyleData>().ValueLabelSize);
    }
    else
    {
        return PRect();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::GetSliderTrackFrame(
    const PView& target,
    const PRect& bounds,
    const PSliderStyleData& data,
    PRect* outTotalFrame,
    float* minimumLength)
{
    const SliderStyleData& state = data.GetOrCreateStyleData<SliderStyleData>();
    UpdateSliderMetrics(target, data);
    PRect  knobFrame = GetSliderKnobBounds(data, POrientation::Horizontal, SliderKnobFrameMode::FullFrame);

    float center = std::ceil(data.SliderSize * 0.5f);
    PRect sliderFrame(0.0f, -center, 0.0f, data.SliderSize - center);
    PRect totalFrame = sliderFrame;

    if (data.TicksAbove) {
        totalFrame.top -= SLIDER_TICK_SPACING + SLIDER_TICK_LENGTH;
    }
    if (data.TicksBelow) {
        totalFrame.bottom += SLIDER_TICK_SPACING + SLIDER_TICK_LENGTH;
    }
    if (knobFrame.top < totalFrame.top)         totalFrame.top = knobFrame.top;
    if (knobFrame.bottom > totalFrame.bottom)   totalFrame.bottom = knobFrame.bottom;

    if (data.Orientation == POrientation::Vertical)
    {
        std::swap(knobFrame.left,   knobFrame.top);
        std::swap(knobFrame.right,  knobFrame.bottom);
        std::swap(sliderFrame.left, sliderFrame.top);
        std::swap(sliderFrame.right, sliderFrame.bottom);
        std::swap(totalFrame.left, totalFrame.top);
        std::swap(totalFrame.right, totalFrame.bottom);
    }

    if (data.Orientation == POrientation::Horizontal)
    {
        if (data.HasValueLabel) {
            totalFrame.top -= state.ValueLabelSize.y;
        }
        if (!data.MinLabel.empty() || !data.MaxLabel.empty())
        {
            const PFontHeight    fontHeight = target.GetFontHeight();
            totalFrame.bottom += fontHeight.ascender + fontHeight.descender;
        }
        const float knobCenter = std::ceil(knobFrame.Width() * 0.5f);
        sliderFrame.left = knobCenter;
        sliderFrame.right = std::max(sliderFrame.left, bounds.right - knobCenter);
        totalFrame.left = bounds.left;
        totalFrame.right = bounds.right;
        if (minimumLength != nullptr) {
            *minimumLength = knobCenter * 2.0f;
        }
    }
    else
    {
        const PFontHeight    fontHeight = target.GetFontHeight();

        const float minLabelWidth = state.MinLabelWidth;
        const float minLabelFontHeight = data.MinLabel.empty() ? 0.0f : fontHeight.ascender + fontHeight.descender + SLIDER_VERTICAL_LABEL_SPACING;

        const float maxLabelWidth = state.MaxLabelWidth;
        const float maxLabelFontHeight = data.MaxLabel.empty() ? 0.0f : fontHeight.ascender + fontHeight.descender + SLIDER_VERTICAL_LABEL_SPACING;

        const float labelCenter = std::ceil(std::max(minLabelWidth, maxLabelWidth) * 0.5f);

        if (-labelCenter < totalFrame.left) totalFrame.left = -labelCenter;
        if (labelCenter > totalFrame.right) totalFrame.right = labelCenter;

        const float knobCenter = std::ceil(knobFrame.Height() * 0.5f);
        sliderFrame.top     = knobCenter;
        sliderFrame.bottom  = bounds.bottom - knobCenter;

        sliderFrame.top     += maxLabelFontHeight;
        sliderFrame.bottom  -= minLabelFontHeight;
        sliderFrame.bottom = std::max(sliderFrame.top, sliderFrame.bottom);

        totalFrame.top    = bounds.top;
        totalFrame.bottom = bounds.bottom;
        if (minimumLength != nullptr) {
            *minimumLength = knobCenter * 2.0f + minLabelFontHeight + maxLabelFontHeight;
        }
    }
    sliderFrame.left -= totalFrame.left;
    sliderFrame.right -= totalFrame.left;
    totalFrame.right -= totalFrame.left;
    totalFrame.left = 0.0f;

    sliderFrame.top -= totalFrame.top;
    sliderFrame.bottom -= totalFrame.top;
    totalFrame.bottom -= totalFrame.top;
    totalFrame.top = 0.0f;

    PPoint centerOffset;
    if (data.Orientation == POrientation::Horizontal) {
        centerOffset.y = std::floor((bounds.Height() - totalFrame.Height()) * 0.5f);
    } else {
        centerOffset.x = std::floor((bounds.Width() - totalFrame.Width()) * 0.5f);
    }
    sliderFrame += centerOffset;
    totalFrame += centerOffset;
    if (outTotalFrame != nullptr) {
        *outTotalFrame = totalFrame;
    }
    return sliderFrame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::GetSliderKnobBounds(const PSliderStyleData& data, POrientation orientation, SliderKnobFrameMode mode)
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

PPoint PDefaultGUIStyle::GetSliderPosition(const PRect& trackFrame, POrientation orientation, float relativeValue)
{
    if (orientation == POrientation::Horizontal)
    {
        return PPoint(std::round(trackFrame.left + trackFrame.Width() * relativeValue), std::round(trackFrame.top + trackFrame.Height() * 0.5f));
    }
    else
    {
        return PPoint(
            std::round(trackFrame.left + trackFrame.Width() * 0.5f),
            std::round(trackFrame.top + trackFrame.Height() * (1.0f - relativeValue)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawSliderTrack(PView& target, const PRect& sliderFrame, const PRect& knobFrame, const PSliderStyleData& data)
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

void PDefaultGUIStyle::DrawSliderKnob(
    PView& target,
    const PRect& trackFrame,
    const PSliderStyleData& data,
    PStandardColorID knobColor,
    float relativeValue)
{
    const PPoint knobPosition = GetSliderPosition(trackFrame, data.Orientation, relativeValue);
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
        target.FillRect(center, pget_standard_color(knobColor));
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

void PDefaultGUIStyle::DrawHorizontalSliderKnob(
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

        target.SetFgColor(target.GetEraseColor());
        if (data.KnobPointUp)     target.DrawLine(x, knobFullFrame.top, x, y1 - 1.0f);
        if (data.KnobPointDown)   target.DrawLine(x, y2 + 1.0f, x, knobFullFrame.bottom);
        target.SetFgColor(knobColor);
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

void PDefaultGUIStyle::DrawVerticalSliderKnob(
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

        target.SetFgColor(target.GetEraseColor());
        if (data.KnobPointUp)  target.DrawLine(knobFullFrame.left, y, x1 - 1.0f, y);
        if (data.KnobPointDown) target.DrawLine(x2 + 1.0f, y, knobFullFrame.right, y);
        target.SetFgColor(knobColor);
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawSliderLabels(PView& target, const PRect& bounds, const PRect& sliderFrame, const PSliderStyleData& data)
{
    const SliderStyleData& state = data.GetOrCreateStyleData<SliderStyleData>();
    const PPoint         center(bounds.Width() * 0.5f, bounds.Height() * 0.5f);
    const PFontHeight    fontHeight = target.GetFontHeight();

    target.SetBgColor(target.GetEraseColor());
    target.SetFgColor(0, 0, 0);
    if (data.Orientation == POrientation::Horizontal)
    {
        if (!data.MinLabel.empty() && !data.MaxLabel.empty())
        {
            const float top = sliderFrame.bottom + SLIDER_TICK_SPACING + SLIDER_TICK_LENGTH + SLIDER_HORIZONTAL_LABEL_SPACING;
            const float rightLabelX = bounds.right - state.MaxLabelWidth;
            const float split = std::clamp(rightLabelX, bounds.left, bounds.right);
            target.DrawString(data.MinLabel, PPoint(0.0f, top + fontHeight.ascender), PRect(bounds.left, top, split, bounds.bottom));
            target.DrawString(data.MaxLabel, PPoint(rightLabelX, top + fontHeight.ascender), PRect(split, top, bounds.right, bounds.bottom));
        }
    }
    else
    {
        const PRect knobBounds = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame);
        const float offset = std::ceil(knobBounds.Height() * 0.5f) + SLIDER_VERTICAL_LABEL_SPACING;
        if (!data.MaxLabel.empty())
        {
            target.DrawString(
                data.MaxLabel,
                PPoint(center.x - state.MaxLabelWidth * 0.5f, sliderFrame.top - fontHeight.descender - offset),
                PRect(bounds.left, bounds.top, bounds.right, sliderFrame.top - offset));
        }
        if (!data.MinLabel.empty())
        {
            target.DrawString(
                data.MinLabel,
                PPoint(center.x - state.MinLabelWidth * 0.5f, sliderFrame.bottom + fontHeight.ascender + offset),
                PRect(bounds.left, sliderFrame.bottom + offset, bounds.right, bounds.bottom));
        }
    }
}

void PDefaultGUIStyle::DrawSliderTicks(PView& target, const PRect& sliderFrame, const PSliderStyleData& data)
{
    if (data.TickCount > 0 && sliderFrame.IsValid())
    {
        const float scale = (data.TickCount > 1) ? 1.0f / float(data.TickCount - 1) : 0.0f;

        if (data.Orientation == POrientation::Horizontal)
        {
            const float width = std::floor(sliderFrame.Width() - 2.0f);

            const float y1 = std::floor(sliderFrame.top - SLIDER_TICK_SPACING);
            const float y2 = y1 - SLIDER_TICK_LENGTH;
            const float y4 = std::floor(sliderFrame.bottom + SLIDER_TICK_SPACING);
            const float y3 = y4 + SLIDER_TICK_LENGTH;

            target.SetFgColor(pget_standard_color(PStandardColorID::Shadow));
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float x = std::floor(sliderFrame.left + width * float(i) * scale);
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x, y1), PPoint(x, y2));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x, y3), PPoint(x, y4));
                }
            }
            target.SetFgColor(pget_standard_color(PStandardColorID::Shine));
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float x = std::floor(sliderFrame.left + width * float(i) * scale) + 1.0f;
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x, y1), PPoint(x, y2));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x, y3), PPoint(x, y4));
                }
            }
        }
        else
        {
            const float height = sliderFrame.Height() - 2.0f;

            const float x1 = std::floor(sliderFrame.left - SLIDER_TICK_SPACING);
            const float x2 = x1 - SLIDER_TICK_LENGTH;
            const float x4 = std::floor(sliderFrame.right + SLIDER_TICK_SPACING - 1.0f);
            const float x3 = x4 + SLIDER_TICK_LENGTH;

            target.SetFgColor(pget_standard_color(PStandardColorID::Shadow));
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float y = std::floor(sliderFrame.top + height * float(i) * scale);
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x1, y), PPoint(x2, y));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x3, y), PPoint(x4, y));
                }
            }
            target.SetFgColor(pget_standard_color(PStandardColorID::Shine));
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float y = std::floor(sliderFrame.top + height * float(i) * scale) + 1.0f;
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x1, y), PPoint(x2, y));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x3, y), PPoint(x4, y));
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDefaultGUIStyle::ScrollBarGeometry PDefaultGUIStyle::GetScrollBarGeometry(const PRect& frame, const PScrollBarStyleData& data, float relativeValue)
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

PRect PDefaultGUIStyle::GetScrollBarFrame(POrientation orientation, float start, float end, float crossStart, float crossEnd)
{
    return (orientation == POrientation::Horizontal) ? PRect(start, crossStart, end, crossEnd) : PRect(crossStart, start, crossEnd, end);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarPart PDefaultGUIStyle::GetScrollBarArrowPart(size_t index)
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

PColor PDefaultGUIStyle::GetScrollBarTint(PColor color, float tint)
{
    const int red = std::clamp(int(float(color.GetRed()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    const int green = std::clamp(int(float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    const int blue = std::clamp(int(float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    return PColor(uint8_t(red), uint8_t(green), uint8_t(blue), color.GetAlpha());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawScrollBarKnob(PView& target, const PRect& knobFrame, POrientation orientation)
{
    if (!knobFrame.IsValid()) {
        return;
    }
    target.SetEraseColor(PStandardColorID::ScrollBarKnob);
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

void PDefaultGUIStyle::DrawScrollBarTrack(PView& target, const ScrollBarGeometry& geometry, POrientation orientation)
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
    target.SetFgColor(PStandardColorID::ScrollBarBackground);
    if (beforeKnob.IsValid()) {
        target.FillRect(beforeKnob);
    }
    if (afterKnob.IsValid()) {
        target.FillRect(afterKnob);
    }
    target.SetFgColor(GetScrollBarTint(pget_standard_color(PStandardColorID::Shadow), 0.5f));
    if (isHorizontal) {
        target.DrawLine(PPoint(trackFrame.left, trackFrame.top - 1.0f), PPoint(trackFrame.right, trackFrame.top - 1.0f));
    } else {
        target.DrawLine(PPoint(trackFrame.left - 1.0f, trackFrame.top), PPoint(trackFrame.left - 1.0f, trackFrame.bottom));
    }
    target.SetFgColor(GetScrollBarTint(pget_standard_color(PStandardColorID::Shine), 0.6f));
    if (isHorizontal) {
        target.DrawLine(PPoint(trackFrame.left, trackFrame.bottom + 1.0f), PPoint(trackFrame.right, trackFrame.bottom + 1.0f));
    } else {
        target.DrawLine(PPoint(trackFrame.right + 1.0f, trackFrame.top), PPoint(trackFrame.right + 1.0f, trackFrame.bottom));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawScrollBarArrows(PView& target, const ScrollBarGeometry& geometry, const PScrollBarStyleData& data)
{
    ScrollBarStyleData& state = data.GetOrCreateStyleData<ScrollBarStyleData>();
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
    target.SetEraseColor(PStandardColorID::ScrollBarKnob);
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
