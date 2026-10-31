// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <cmath>
#include <stdio.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/Slider.h>
#include <Utils/XMLFactory.h>
#include <Utils/XMLObjectParser.h>


const std::map<PString, uint32_t> PSliderFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, TicksAbove),
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, TicksBelow),
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, TicksLeft),
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, TicksRight),
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, KnobPointUp),
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, KnobPointDown),
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, KnobPointLeft),
    DEFINE_FLAG_MAP_ENTRY(PSliderFlags, KnobPointRight)
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSlider::PSlider(const PString& name, uint32_t flags, int tickCount, POrientation orientation)
    : PControl(name, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    UpdateStyleFlags();
    m_SliderStyleData.Orientation = orientation;
    m_SliderStyleData.TickCount = tickCount;

    m_SliderStyleData.SliderColor1 = pget_standard_color(PStandardColorID::ScrollBarBackground);
    m_SliderStyleData.SliderColor2 = m_SliderStyleData.SliderColor1;
    m_SliderStyleData.IsEnabled = IsEnabled();
    m_SliderStyleData.Value = GetRelativeValue(m_Value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSlider::PSlider(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PControl(context, xmlData)
{
    m_SliderStyleData.Label = context.GetAttribute(xmlData, "label", PString::zero);
    const uint32_t flags = context.GetFlagsAttribute<uint32_t>(
        xmlData,
        PSliderFlags::FlagMap,
        "flags",
        PSliderFlags::TicksBelow | PSliderFlags::KnobPointDown);
    MergeFlags(flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize);
    UpdateStyleFlags();

    m_SliderStyleData.Orientation = context.GetAttribute(xmlData, "orientation", POrientation::Horizontal);
    m_SliderStyleData.TickCount = context.GetAttribute(xmlData, "num_ticks", 10);

    m_SliderStyleData.MinLabel = context.GetAttribute(xmlData, "min_label", PString::zero);
    m_SliderStyleData.MaxLabel = context.GetAttribute(xmlData, "max_label", PString::zero);

    m_Min = context.GetAttribute(xmlData, "min", 0.0f);
    m_Max = context.GetAttribute(xmlData, "max", 1.0f);
    m_Resolution    = context.GetAttribute(xmlData, "resolution", m_Resolution);
    m_Value = context.GetAttribute(xmlData, "value", m_Min);

    m_DragScale = context.GetAttribute(xmlData, "drag_scale", 1.0f);
    m_DragScaleRange = context.GetAttribute(xmlData, "drag_scale_range", 100.0f);

    m_SliderStyleData.SliderColor1 = pget_standard_color(PStandardColorID::ScrollBarBackground);
    m_SliderStyleData.SliderColor2 = m_SliderStyleData.SliderColor1;
    m_SliderStyleData.IsEnabled = IsEnabled();
    m_SliderStyleData.Value = GetRelativeValue(m_Value);

    SetValueStringFormat(context.GetAttribute(xmlData, "value_format", PString::zero), context.GetAttribute(xmlData, "value_scale", 1.0f));
    SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnFlagsChanged(uint32_t changedFlags)
{
    PControl::OnFlagsChanged(changedFlags);
    constexpr uint32_t STYLE_FLAGS = PSliderFlags::TicksAbove | PSliderFlags::TicksBelow | PSliderFlags::KnobPointUp | PSliderFlags::KnobPointDown;
    if ((changedFlags & STYLE_FLAGS) != 0)
    {
        UpdateStyleFlags();
        NotifyStylePropertyChanged(PSliderProperty::Appearance);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetLabel(const PString& label)
{
    if (label != m_SliderStyleData.Label)
    {
        m_SliderStyleData.Label = label;
        OnLabelChanged(m_SliderStyleData.Label);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnLabelChanged(const PString& label)
{
    PSliderProperties properties(PSliderProperty::Label);
    if (UpdateValueLabel()) {
        properties |= PSliderProperty::ValueLabel;
    }
    NotifyStylePropertyChanged(properties);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSlider::PosToVal(const PPoint& position) const
{
    const PGUIStyle* style = GetStyle();
    if (style == nullptr) {
        return GetValue();
    }
    float relativeValue = style->SliderGetValueFromPosition(*this, m_SliderStyleData, position);
    if (m_NumSteps > 1)
    {
        // Preserve snapping ties in the direction of increasing screen coordinates.
        const bool isVertical = m_SliderStyleData.Orientation == POrientation::Vertical;
        float stepPosition = isVertical ? 1.0f - relativeValue : relativeValue;
        const float intervalCount = float(m_NumSteps - 1);
        stepPosition = std::floor(stepPosition * intervalCount + 0.5f) / intervalCount;
        relativeValue = isVertical ? 1.0f - stepPosition : stepPosition;
    }
    relativeValue = std::clamp(relativeValue, 0.0f, 1.0f);
    return m_Min + relativeValue * (m_Max - m_Min);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSlider::ValToPos(float value) const
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        return style->SliderGetValuePosition(*this, m_SliderStyleData, GetRelativeValue(value));
    } else {
        return PPoint();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Modify the fill color of the slider bar.
/// \par Description:
///     Set the two colors used to fill the slider bar.
///     The first color is used to fill the "lesser" part
///     of the slider while the second color is used in
///     the "greater" part of the slider.
/// \param color1 - Color used left/below the knob
/// \param color2 - Color used right/above the knob
/// \sa GetSliderColors(), SetSliderSize(), SetLimitLabels(), SetValueStringFormat()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetSliderColors(const PColor& color1, const PColor& color2)
{
    m_SliderStyleData.SliderColor1 = color1;
    m_SliderStyleData.SliderColor2 = color2;
    NotifyStylePropertyChanged(PSliderProperty::SliderColors);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// Get the slider-bar fill color.
/// \par Description:
/// Return the two slider-bar colors. Each of the two pointers might be NULL.
///
/// \param outColor1 - Color used left/below the knob
/// \param outColor2 - Color used right/abow the knob
/// \sa SetSliderColors(), SetLimitLabels(), GetSliderSize()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::GetSliderColors(PColor* outColor1, PColor* outColor2) const
{
    if ( outColor1 != nullptr ) {
        *outColor1 = m_SliderStyleData.SliderColor1;
    }
    if ( outColor2 != nullptr ) {
        *outColor2 = m_SliderStyleData.SliderColor2;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Set the thickness of the slider bar.
/// \par Description:
///     Set the size in slider bar in pixels. For a horizontal slider it
///     sets the height and for a vertical it sets the width.
/// \par Note:
/// \par Warning:
/// \param size - Slider-bar thickness in pixels.
/// \sa GetSliderSize()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetSliderSize(float size)
{
    m_SliderStyleData.SliderSize = size;
    NotifyStylePropertyChanged(PSliderProperty::SliderSize);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// Get the current slider thickness.
/// \par Description:
///     Returns the width for a horizontal and the height for a vertical
///     slider of the slider-bar.
/// \return The thickness of the slider-bar in pixels.
/// \sa SetSliderSize()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSlider::GetSliderSize() const
{
    return m_SliderStyleData.SliderSize;
}

///////////////////////////////////////////////////////////////////////////////
/// Set format string for the value label.
/// \par Description:
///     Set a std::format() style string that will be used to format the
///     "value" label printed above the slider. The slider's current value
///     (multiplied by \p scale) is passed as the single format argument,
///     so a format string of "{:.2f}" will render the value with two decimal places.
/// \par Note:
///     If you need some more advanced formatting you can inherit a new
///     class from PSlider and overload the GetValueString() and
///     return the string to be used as a value label.
/// \param format The std::format() style format string used to generate the value label.
/// \param scale  Scale factor applied to the slider value before formatting. Defaults to 1.0.
/// \sa GetValueStringFormat(), GetValueString(), SetLimitLabels(), GetLimitLabels()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetValueStringFormat(const PString& format, float scale)
{
    m_ValueFormat = format;
    m_ValueScale = scale;
    OnLabelChanged(GetLabel());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PSlider::GetValueStringFormat() const
{
    return m_ValueFormat;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PSlider::GetValueString() const
{
    if (m_ValueFormat.empty()) {
        return PString::zero;
    } else {
        return PString::vformat_string(m_ValueFormat.c_str(), m_Value * m_ValueScale);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetValue(float value, bool sendEvent)
{
    if (m_Resolution != 0.0f) {
        value = std::round(value / m_Resolution) * m_Resolution;
    }
    if (value != m_Value)
    {
        m_Changed = true;
        m_Value = value;
        m_SliderStyleData.Value = GetRelativeValue(value);
        PSliderProperties properties(PSliderProperty::Value);
        if (UpdateValueLabel()) {
            properties |= PSliderProperty::ValueLabel;
        }
        NotifyStylePropertyChanged(properties);
        Sync();
        if (sendEvent) {
            SignalValueChanged(m_Value, m_HitPointerID == PInvalidPointerID, this);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSlider::GetValue() const
{
    return m_Value;
}

///////////////////////////////////////////////////////////////////////////////
/// Set the number of possible knob positions.
/// \par Description:
///     The step count is used to limit the number of possible knob
///     positions. The points are equally spread out over the length
///     of the slider and the knob will "snap" to these points.
///     If set to the same value as SetTickCount() the knob will snap
///     to the ticks.
///
///     If the value is less than 2 the knob is not snapped.
/// \param count - Number of possible knob positions. 0 to disable snapping.
/// \sa GetStepCount(), SetTickCount(), GetTickCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetStepCount(int count)
{
    m_NumSteps = count;
}

///////////////////////////////////////////////////////////////////////////////
/// Obtain the step-count as set by SetStepCount()
/// \return The number of possible knob positions.
/// \sa SetStepCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PSlider::GetStepCount() const
{
    return m_NumSteps;
}

///////////////////////////////////////////////////////////////////////////////
/// Set number of "ticks" rendered along the slider.
/// \par Description:
///     The slider can render "ticks" along both sides of the slider-bar.
///     With SetTickCount() you can set how many such ticks should be rendered
///     and with SetTickFlags() you can set on which side (if any) the ticks
///     should be rendered. To make the knob "snap" to the ticks you can set
///     the same count with SetStepCount().
/// \param count - The number of ticks to be rendered.
/// \sa GetTickCount(), SetTickFlags(), GetTickFlags(), SetStepCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetTickCount(int count)
{
    m_SliderStyleData.TickCount = count;
    NotifyStylePropertyChanged(PSliderProperty::TickCount);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// Obtain the tick count as set with SetTickCount()
/// \return The number of ticks to be rendered
/// \sa SetTickCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PSlider::GetTickCount() const
{
    return m_SliderStyleData.TickCount;
}

///////////////////////////////////////////////////////////////////////////////
/// Set the static labels rendere at each end of the slider.
/// \par Description:
///     The min/max labels are rendered below a horizontal slider
///     and to the right of a vertical slider.
/// \par Note:
///     Both label must be set for the labels to be rendered.
/// \param minLabel - The label to be rendered at the "smaller" end of the slider.
/// \param maxLabel - The label to be rendered at the "greater" end of the slider.
/// \sa GetLimitLabels(), SetValueStringFormat()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetLimitLabels(const PString& minLabel, const PString& maxLabel)
{
    PSliderProperties properties;
    if (minLabel != m_SliderStyleData.MinLabel)
    {
        m_SliderStyleData.MinLabel = minLabel;
        properties |= PSliderProperty::MinLabel;
    }
    if (maxLabel != m_SliderStyleData.MaxLabel)
    {
        m_SliderStyleData.MaxLabel = maxLabel;
        properties |= PSliderProperty::MaxLabel;
    }
    if (!properties.Empty())
    {
        NotifyStylePropertyChanged(properties);
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Obtain the limit-labels as set with SetLimitLabels()
/// \param minLabel - Pointer to a string receiving the min-label or NULL.
/// \param maxLabel - Pointer to a string receiving the max-label or NULL.
/// \sa SetLimitLabels()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::GetLimitLabels(PString* minLabel, PString* maxLabel)
{
    if (minLabel != nullptr) {
        *minLabel = m_SliderStyleData.MinLabel;
    }
    if (maxLabel != nullptr) {
        *maxLabel = m_SliderStyleData.MaxLabel;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetMinMax(float min, float max)
{
    if (min != m_Min || max != m_Max)
    {
        m_Min = min;
        m_Max = max;
        m_SliderStyleData.Value = GetRelativeValue(m_Value);
        for (size_t i = 0; i < m_ShadowValues.size(); ++i) {
            m_SliderStyleData.ShadowValues[i] = GetRelativeValue(m_ShadowValues[i]);
        }
        NotifyStylePropertyChanged(PSliderProperties(PSliderProperty::Value, PSliderProperty::ShadowValues));
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetResolution(float resolution)
{
    if (resolution != m_Resolution)
    {
        m_Resolution = resolution;
        if (m_Resolution != 0.0f) {
            SetValue(GetValue());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetShadowKnobsCount(size_t count)
{
    if (count != m_ShadowValues.size())
    {
        const size_t previousCount = m_ShadowValues.size();
        m_ShadowValues.resize(count);
        m_SliderStyleData.ShadowValues.resize(count);
        for (size_t i = previousCount; i < count; ++i) {
            m_SliderStyleData.ShadowValues[i] = GetRelativeValue(m_ShadowValues[i]);
        }
        NotifyStylePropertyChanged(PSliderProperty::ShadowValues);
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PSlider::GetShadowKnobsCount() const
{
    return m_ShadowValues.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetShadowKnobValue(size_t index, float value)
{
    if (index >= m_ShadowValues.size() || value == m_ShadowValues[index]) {
        return;
    }
    m_ShadowValues[index] = value;
    m_SliderStyleData.ShadowValues[index] = GetRelativeValue(value);
    NotifyStylePropertyChanged(PSliderProperty::ShadowValues);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnEnableStatusChanged(bool isEnabled)
{
    m_SliderStyleData.IsEnabled = isEnabled;
    if (m_HitPointerID != PInvalidPointerID)
    {
        ReleasePointerCapture(m_HitPointerID);
        m_HitPointerID = PInvalidPointerID;
        m_SliderStyleData.IsPressed = false;
        m_PointerCaptureLocked = false;
        if (m_Changed)
        {
            SignalValueChanged(m_Value, true, this);
            m_Changed = false;
        }
    }
    NotifyStylePropertyChanged(PSliderProperties(PSliderProperty::Enabled, PSliderProperty::Pressed));
}

///////////////////////////////////////////////////////////////////////////////
/// Sets the background color to the one of the parent view
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::AttachedToScreen()
{
    Ptr<PView> parent = GetParent();
    if (parent != nullptr) {
        SetEraseColor(parent->GetEraseColor());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase != PEventPhase::Target) return;
    if (!IsEnabled() || GetStyle() == nullptr) return;
    if (m_HitPointerID == PInvalidPointerID)
    {
        if (!SetPointerCapture(pointerID, PPointerCaptureMode::Preemptible)) {
            return;
        }
        m_HitPointerID = pointerID;
        m_SliderStyleData.IsPressed = true;
        m_HitValue = GetValue();
        m_HitPos = position; // (position - ValToPos(m_HitValue));
        m_SmoothedPos = position;
        m_Changed = false;
        m_PointerCaptureLocked = false;
        NotifyStylePropertyChanged(PSliderProperty::Pressed);

        SignalBeginDrag(m_Value, this, pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase != PEventPhase::Target) return;
    if (!IsEnabled()) return;
    if (pointerID == m_HitPointerID)
    {
        m_HitPointerID = PInvalidPointerID;
        m_SliderStyleData.IsPressed = false;
        m_PointerCaptureLocked = false;
        if (m_Changed)
        {
            SignalValueChanged(m_Value, true, this);
            m_Changed = false;
        }
        NotifyStylePropertyChanged(PSliderProperty::Pressed);

        SignalEndDrag(m_Value, this, pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase != PEventPhase::Target) return;
    if (!IsEnabled()) return;
    if (pointerID == m_HitPointerID)
    {
        if (!m_PointerCaptureLocked)
        {
            const PPoint dragDelta = position - m_HitPos;
            const bool isHorizontal = m_SliderStyleData.Orientation == POrientation::Horizontal;
            const float primaryDelta = isHorizontal ? std::abs(dragDelta.x) : std::abs(dragDelta.y);
            const float crossDelta = isHorizontal ? std::abs(dragDelta.y) : std::abs(dragDelta.x);
            if (primaryDelta >= POINTER_CAPTURE_LOCK_THRESHOLD && primaryDelta >= crossDelta && SetPointerCapture(pointerID)) {
                m_PointerCaptureLocked = true;
            }
        }

        PPoint distance(fabsf(m_SmoothedPos.x - position.x), fabsf(m_SmoothedPos.y - position.y));
        PPoint scale = distance * 0.1f;  // Divide by smoothing range.
        scale = scale * scale * 0.5f;   // Exponential growth from 0 -> 0.5

        if (distance.x > 10.0f) {
            m_SmoothedPos.x = position.x;
        } else {
            m_SmoothedPos.x += (position.x - m_SmoothedPos.x) * scale.x;
        }
        if (distance.y > 10.0f) {
            m_SmoothedPos.y = position.y;
        } else {
            m_SmoothedPos.y += (position.y - m_SmoothedPos.y) * scale.y;
        }

        PPoint scaledPos = (m_SmoothedPos - m_HitPos).GetRounded();

        float proportion = std::min(1.0f, fabsf(scaledPos.x) / m_DragScaleRange);   // Grows from 0->1 over m_DragScaleRange.
        proportion *= proportion;                                                   // Exponential growth.
        // Grows from m_DragScale -> 1 as proportion go from 0 -> 1.
        float interpolatedScale = m_DragScale + (1.0f - m_DragScale) * proportion;
        scaledPos.x *= interpolatedScale;

        proportion = std::min(1.0f, fabsf(scaledPos.y) / m_DragScaleRange);     // Grows from 0->1 over m_DragScaleRange.
        proportion *= proportion;                                               // Exponential growth.
        // Grows from m_DragScale -> 1 as proportion go from 0 -> 1.
        interpolatedScale = m_DragScale + (1.0f - m_DragScale) * proportion;
        scaledPos.y *= interpolatedScale;

        SetValue(PosToVal(scaledPos + ValToPos(m_HitValue)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason)
{
    if (pointerID == m_HitPointerID)
    {
        SetValue(m_HitValue, false);
        m_HitPointerID = PInvalidPointerID;
        m_SliderStyleData.IsPressed = false;
        m_PointerCaptureLocked = false;
        m_Changed = false;
        NotifyStylePropertyChanged(PSliderProperty::Pressed);
        SignalEndDrag(m_Value, this, pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnFontChanged(Ptr<const PFont> newFont)
{
    NotifyStylePropertyChanged(PSliderProperty::Font);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnFrameSized(const PPoint& delta)
{
    PControl::OnFrameSized(delta);
    NotifyStylePropertyChanged(PSliderProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnStyleChanged()
{
    m_SliderStyleData.StyleData.Reset();
    PControl::OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->SliderPaint(*this, GetNormalizedBounds(), m_SliderStyleData);
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->SliderGetPreferredSize(*this, m_SliderStyleData, minSize, maxSize);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSlider::GetRelativeValue(float value) const
{
    return (m_Max != m_Min) ? (value - m_Min) / (m_Max - m_Min) : 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::UpdateStyleFlags()
{
    m_SliderStyleData.TicksAbove = HasFlags(PSliderFlags::TicksAbove);
    m_SliderStyleData.TicksBelow = HasFlags(PSliderFlags::TicksBelow);
    m_SliderStyleData.KnobPointUp = HasFlags(PSliderFlags::KnobPointUp);
    m_SliderStyleData.KnobPointDown = HasFlags(PSliderFlags::KnobPointDown);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PSlider::UpdateValueLabel()
{
    const bool hasValueLabel = !m_ValueFormat.empty() || !m_SliderStyleData.Label.empty();
    PString valueLabel = hasValueLabel ? GetLabel() + GetValueString() : PString::zero;
    if (valueLabel != m_SliderStyleData.ValueLabel || hasValueLabel != m_SliderStyleData.HasValueLabel)
    {
        m_SliderStyleData.ValueLabel = std::move(valueLabel);
        m_SliderStyleData.HasValueLabel = hasValueLabel;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::NotifyStylePropertyChanged(PSliderProperties properties)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        style->SliderPropertyChanged(*this, m_SliderStyleData, properties);
    }
    else
    {
        PreferredSizeChanged();
        Invalidate();
    }
}
