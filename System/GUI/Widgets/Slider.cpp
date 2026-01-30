// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdio.h>
#include <GUI/Widgets/Slider.h>
#include <GUI/Widgets/TextView.h>
#include <GUI/Region.h>
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

static constexpr float TICK_LENGTH      = 6.0f;
static constexpr float TICK_SPACING     = 3.0f;
static constexpr float VLABEL_SPACING   = 3.0f;
static constexpr float HLABEL_SPACING   = 2.0f;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSlider::PSlider(const PString& name, Ptr<PView> parent, uint32_t flags, int tickCount, POrientation orientation)
    : PControl(name, parent, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize )
{
    m_Orientation   = orientation;
    m_NumTicks      = tickCount;

    m_SliderColor1  = pget_standard_color(PStandardColorID::ScrollBarBackground);;
    m_SliderColor2  = m_SliderColor1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSlider::PSlider(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PControl(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, PSliderFlags::FlagMap, "flags", PSliderFlags::TicksBelow | PSliderFlags::KnobPointDown) | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize);

    SetFont(ptr_new<PFont>(PFontID::e_FontSmall));

    m_Orientation   = context.GetAttribute(xmlData, "orientation", POrientation::Horizontal);
    m_NumTicks      = context.GetAttribute(xmlData, "num_ticks", 10);

    m_MinLabel = context.GetAttribute(xmlData, "min_label", PString::zero);
    m_MaxLabel = context.GetAttribute(xmlData, "max_label", PString::zero);

    m_Min           = context.GetAttribute(xmlData, "min", 0.0f);
    m_Max           = context.GetAttribute(xmlData, "max", 1.0f);
    m_Resolution    = context.GetAttribute(xmlData, "resolution", m_Resolution);
    m_Value         = context.GetAttribute(xmlData, "value", m_Min);

    m_DragScale = context.GetAttribute(xmlData, "drag_scale", 1.0f);
    m_DragScaleRange = context.GetAttribute(xmlData, "drag_scale_range", 100.0f);

    m_SliderColor1  = pget_standard_color(PStandardColorID::ScrollBarBackground);;
    m_SliderColor2  = m_SliderColor1;

    SetValueStringFormat(context.GetAttribute(xmlData, "value_format", PString::zero), context.GetAttribute(xmlData, "value_scale", 1.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSlider::~PSlider()
{
    if (m_ValueView != nullptr)
    {
        RemoveChild(m_ValueView);
        m_ValueView = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Sets the background color to the one of the parent view
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::AttachedToScreen()
{
    SetEraseColor(GetParent()->GetEraseColor());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnFrameSized(const PPoint& delta)
{
    LayoutValueView();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PRect frame;
    float minLength;
    GetSliderFrame(&frame, &minLength);

    if (m_Orientation == POrientation::Horizontal)
    {
        minSize->x = minLength;
        minSize->y = frame.Height();

        maxSize->x = 2000.0f;
        maxSize->y = minSize->y;
    }
    else
    {
        minSize->x = frame.Width();
        minSize->y = minLength;

        maxSize->x = minSize->x;
        maxSize->y = 2000.0f;
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
    m_SliderColor1 = color1;
    m_SliderColor2 = color2;
    RefreshDisplay();
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
        *outColor1 = m_SliderColor1;
    }
    if ( outColor2 != nullptr ) {
        *outColor2 = m_SliderColor2;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Set the thickness of the slider bar.
/// \par Description:
///     Set the size in slider bar in pixels. For a horizontal slider it
///     sets the height and for a vertical it sets the width.
/// \par Note:
/// \par Warning:
/// \param vSize - Slider-bar thickness in pixels.
/// \sa GetSliderSize(), GetSliderFrame()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetSliderSize(float size)
{
    m_SliderSize = size;
    RefreshDisplay();
}

///////////////////////////////////////////////////////////////////////////////
/// Get the current slider thickness.
/// \par Description:
///     Returns the width for a horizontal and the height for a vertical
///     slider of the slider-bar.
/// \return The thickness of the slider-bar in pixels.
/// \sa SetSliderSize(), GetSliderFrame()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSlider::GetSliderSize() const
{
    return m_SliderSize;
}

///////////////////////////////////////////////////////////////////////////////
/// Set format string for the value label.
/// \par Description:
///     Set a printf() style string that will be used to format the
///     "value" label printed above the slider. The format function
///     will be passed the sliders value as a parameter so if you throw
///     in an %f somewhere it will be replaced with the current value.
/// \par Note:
///     If you need some more advanced formatting you can inherit a new
///     class from os::Slider and overload the GetValueString() and
///     return the string to be used as a value label.
/// \param cFormat - The printf() style format string used to generate the
///         value label.
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
    if (m_Resolution != 0.0f)
    {
        value = std::round(value / m_Resolution) * m_Resolution;
    }
    if (value != m_Value)
    {
        m_Changed = true;
        PRect knobFrame = GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(m_Value);
        m_Value = value;
        knobFrame |= GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(m_Value);
        Invalidate(knobFrame);
        UpdateValueView();
        Sync();
        if (sendEvent) {
            SignalValueChanged(m_Value, m_HitButton == PMouseButton::None, this);
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
/// \param nCound - Number of possible knob positions. 0 to disable snapping.
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
    m_NumTicks = count;
    RefreshDisplay();
}

///////////////////////////////////////////////////////////////////////////////
/// Obtain the tick count as set with SetTickCount()
/// \return The number of ticks to be rendered
/// \sa SetTickCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PSlider::GetTickCount() const
{
    return m_NumTicks;
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
    m_MinLabel = minLabel;
    m_MaxLabel = maxLabel;
    PreferredSizeChanged();
    RefreshDisplay();
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
        *minLabel = m_MinLabel;
    }
    if (maxLabel != nullptr) {
        *maxLabel = m_MaxLabel;
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
        if (m_Resolution != 0.0f)
        {
            SetValue(GetValue());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetShadowKnobsCount(size_t count)
{
    m_ShadowArrows.resize(count);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PSlider::GetShadowKnobsCount() const
{
    return m_ShadowArrows.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::SetShadowKnobValue(size_t index, float value)
{
    if (index >= m_ShadowArrows.size() || value == m_ShadowArrows[index]) {
        return;
    }
    PRect knobFrame = GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(m_ShadowArrows[index]);
    m_ShadowArrows[index] = value;
    knobFrame |= GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(value);
    Invalidate(knobFrame);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnEnableStatusChanged(bool isEnabled)
{
    if (m_HitButton != PMouseButton::None)
    {
        MakeFocus(m_HitButton, false);
        m_HitButton = PMouseButton::None;
        if (m_Changed)
        {
            SignalValueChanged(m_Value, true, this);
            m_Changed = false;
        }
    }
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PSlider::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (!IsEnabled()) return false;
    if (m_HitButton == PMouseButton::None)
    {
        m_HitButton = button;
        m_HitValue = GetValue();
        m_HitPos = position; // (position - ValToPos(m_HitValue));
        m_SmoothedPos = position;
        m_Changed = false;
        MakeFocus(button, true);
        Invalidate(GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(m_Value));

        SignalBeginDrag(m_Value, this, button);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PSlider::OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (!IsEnabled()) return false;
    if (button == m_HitButton)
    {
        MakeFocus(m_HitButton, false);
        m_HitButton = PMouseButton::None;
        if (m_Changed)
        {
            SignalValueChanged(m_Value, true, this);
            m_Changed = false;
        }
        Invalidate(GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(m_Value));

        SignalEndDrag(m_Value, this, button);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PSlider::OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (!IsEnabled()) return false;
    if (button == m_HitButton)
    {
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
        float interpolatedScale = m_DragScale + (1.0f - m_DragScale) * proportion;  // Grows from m_DragScale -> 1 as proportion go from 0 -> 1.
        scaledPos.x *= interpolatedScale;

        proportion = std::min(1.0f, fabsf(scaledPos.y) / m_DragScaleRange);     // Grows from 0->1 over m_DragScaleRange.
        proportion *= proportion;                                               // Exponential growth.
        interpolatedScale = m_DragScale + (1.0f - m_DragScale) * proportion;    // Grows from m_DragScale -> 1 as proportion go from 0 -> 1.
        scaledPos.y *= interpolatedScale;

        SetValue(PosToVal(scaledPos + ValToPos(m_HitValue)));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnLabelChanged(const PString& label)
{
    if (m_Orientation == POrientation::Horizontal && (!m_ValueFormat.empty() || !label.empty()))
    {
        if (m_ValueView == nullptr)
        {
            m_ValueView = ptr_new<PTextView>("value", PString::zero, ptr_tmp_cast(this), PViewFlags::IgnoreMouse);
            m_ValueView->SignalPreferredSizeChanged.Connect(this, &PSlider::LayoutValueView);
        }
        UpdateValueView();
        LayoutValueView();
    }
    else
    {
        if (m_ValueView != nullptr)
        {
            RemoveChild(m_ValueView);
            m_ValueView = nullptr;
        }
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::RenderSlider()
{
    SetEraseColor(PStandardColorID::SliderTrackNormal);

    PRect bounds = GetNormalizedBounds();

    PRect sliderFrame = GetSliderFrame();
    PRect knobFrame = GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(GetValue());

    // Clear areas not fully overwritten by the slider track and the knob.
    PRegion clearRegion(bounds);
    clearRegion.Exclude(sliderFrame);
    clearRegion.Exclude(knobFrame);
    clearRegion.Optimize();
    for (const PIRect& rect : clearRegion.m_Rects) {
        EraseRect(rect);
    }

    RenderTicks();
    RenderLabels();

    if (sliderFrame.IsValid())
    {
        const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
        const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);

        if (m_Orientation == POrientation::Horizontal)
        {
            PRect leftSliderFrame(sliderFrame.left, sliderFrame.top, knobFrame.left, sliderFrame.bottom);
            PRect rightSliderFrame(knobFrame.right - 1.0f, sliderFrame.top, sliderFrame.right, sliderFrame.bottom);
            if (leftSliderFrame.IsValid())
            {
                PRect leftSliderCenter = leftSliderFrame;
                leftSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                leftSliderCenter.Resize(1.0f, 1.0f, 0.0f, -1.0f);

                SetFgColor(shadowColor);
                MovePenTo(leftSliderFrame.BottomLeft());
                DrawLine(leftSliderFrame.TopLeft());
                DrawLine(leftSliderFrame.TopRight());

                SetFgColor(shineColor);
                MovePenTo(leftSliderFrame.BottomRight());
                DrawLine(PPoint(leftSliderFrame.left + 1, leftSliderFrame.bottom));

                FillRect(leftSliderCenter, m_SliderColor1);
            }
            if (rightSliderFrame.IsValid())
            {
                PRect rightSliderCenter = rightSliderFrame;
                rightSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                rightSliderCenter.Resize(0.0f, 1.0f, -1.0f, -1.0f);

                SetFgColor(shadowColor);
                MovePenTo(rightSliderFrame.TopLeft());
                DrawLine(rightSliderFrame.TopRight());

                SetFgColor(shineColor);
                MovePenTo(PPoint(rightSliderFrame.right, rightSliderFrame.top + 1.0f));
                DrawLine(rightSliderFrame.BottomRight());
                DrawLine(rightSliderFrame.BottomLeft() + PPoint(1.0f, 0.0f));

                FillRect(rightSliderCenter, m_SliderColor2);
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

                SetFgColor(shadowColor);
                MovePenTo(topSliderFrame.BottomLeft());
                DrawLine(topSliderFrame.TopLeft());
                DrawLine(topSliderFrame.TopRight());

                SetFgColor(shineColor);
                MovePenTo(topSliderFrame.BottomRight());
                DrawLine(PPoint(topSliderFrame.right, topSliderFrame.top + 1.0f));

                FillRect(topSliderCenter, m_SliderColor1);
            }
            if (bottomSliderFrame.IsValid())
            {
                PRect bottomSliderCenter = bottomSliderFrame;
                bottomSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                bottomSliderCenter.Resize(1.0f, 0.0f, -1.0f, -1.0f);

                SetFgColor(shadowColor);
                MovePenTo(bottomSliderFrame.TopLeft());
                DrawLine(bottomSliderFrame.BottomLeft());

                SetFgColor(shineColor);
                MovePenTo(PPoint(bottomSliderFrame.left + 1.0f, bottomSliderFrame.bottom));
                DrawLine(bottomSliderFrame.BottomRight());
                DrawLine(bottomSliderFrame.TopRight() + PPoint(0.0f, 1.0f));

                FillRect(bottomSliderCenter, m_SliderColor2);
            }
        }
    }
    for (float value : m_ShadowArrows)
    {
        RenderKnob(PStandardColorID::SliderKnobShadow, value);
    }

    RenderKnob((IsBeingDragged()) ? PStandardColorID::SliderKnobPressed : PStandardColorID::SliderKnobNormal, GetValue());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::RenderKnob(PStandardColorID knobColor, float value)
{
    PRect knobFrame = GetKnobFrame(m_Orientation, GetKnobFrameMode::SquareFrame) + ValToPos(value);

    if (!HasFlags(PSliderFlags::KnobPointUp | PSliderFlags::KnobPointDown))
    {
        PRect center(knobFrame);
        center.Resize(1.0f, 1.0f, -1.0f, -1.0f);
        knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

        SetFgColor(PStandardColorID::Shine);
        MovePenTo(knobFrame.BottomLeft());
        DrawLine(knobFrame.TopLeft());
        DrawLine(knobFrame.TopRight());

        SetFgColor(PStandardColorID::Shadow);
        DrawLine(knobFrame.BottomRight());
        DrawLine(knobFrame.BottomLeft() + PPoint(1.0f, 0.0f));
        FillRect(center, pget_standard_color(knobColor));
    }
    else
    {
        const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
        const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);

        PRect knobFullFrame = GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame) + ValToPos(value);

        knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
        knobFullFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

        if (m_Orientation == POrientation::Horizontal)
        {
            const float center = std::floor(knobFrame.left + (knobFrame.right - knobFrame.left) * 0.5f);

            SetFgColor(knobColor);
            float offset = 0.0f;
            float direction = 1.0f;
            for (float x = knobFrame.left; x <= knobFrame.right; x += 1.0f)
            {
                float y1 = knobFrame.top;
                float y2 = knobFrame.bottom;

                if (HasFlags(PSliderFlags::KnobPointUp))     y1 -= offset;
                if (HasFlags(PSliderFlags::KnobPointDown))   y2 += offset;

                SetFgColor(GetEraseColor());
                if (HasFlags(PSliderFlags::KnobPointUp))     DrawLine(x, knobFullFrame.top, x, y1 - 1.0f);
                if (HasFlags(PSliderFlags::KnobPointDown))   DrawLine(x, y2 + 1.0f, x, knobFullFrame.bottom);
                SetFgColor(knobColor);
                DrawLine(x, y1, x, y2);
                if (direction > 0.0f && x >= center) {
                    direction = -1.0f;
                }
                offset += direction;
            }

            SetFgColor(shadowColor);

            if (HasFlags(PSliderFlags::KnobPointDown)) {
                MovePenTo(center, knobFullFrame.bottom);
            } else {
                MovePenTo(knobFrame.BottomLeft());
            }
            DrawLine(knobFrame.BottomRight());
            DrawLine(knobFrame.TopRight());
            if (HasFlags(PSliderFlags::KnobPointUp)) {
                DrawLine(center, knobFullFrame.top);
            }
            SetFgColor(shineColor);
            DrawLine(knobFrame.TopLeft());
            DrawLine(knobFrame.BottomLeft());
            if (HasFlags(PSliderFlags::KnobPointDown)) {
                DrawLine(center, knobFullFrame.bottom);
            }
        }
        else
        {
            const float center = std::floor(knobFrame.top + (knobFrame.bottom - knobFrame.top) * 0.5f);

            SetFgColor(knobColor);
            float offset = 0.0f;
            float direction = 1.0f;
            for (float y = knobFrame.top; y <= knobFrame.bottom; y += 1.0f)
            {
                float x1 = knobFrame.left;
                float x2 = knobFrame.right;

                if (HasFlags(PSliderFlags::KnobPointLeft))  x1 -= offset;
                if (HasFlags(PSliderFlags::KnobPointRight)) x2 += offset;

                SetFgColor(GetEraseColor());
                if (HasFlags(PSliderFlags::KnobPointLeft))  DrawLine(knobFullFrame.left, y, x1 - 1.0f, y);
                if (HasFlags(PSliderFlags::KnobPointRight)) DrawLine(x2 + 1.0f, y, knobFullFrame.right, y);
                SetFgColor(knobColor);
                DrawLine(x1, y, x2, y);
                if (direction > 0.0f && y >= center) {
                    direction = -1.0f;
                }
                offset += direction;
            }

            SetFgColor(shineColor);
            if (HasFlags(PSliderFlags::KnobPointLeft)) {
                MovePenTo(knobFullFrame.left, center);
            } else {
                MovePenTo(knobFrame.BottomLeft());
            }
            DrawLine(knobFrame.TopLeft());
            DrawLine(knobFrame.TopRight());
            if (HasFlags(PSliderFlags::KnobPointRight)) {
                DrawLine(knobFullFrame.right, center);
            }
            SetFgColor(shadowColor);
            DrawLine(knobFrame.BottomRight());
            DrawLine(knobFrame.BottomLeft());
            if (HasFlags(PSliderFlags::KnobPointLeft)) {
                DrawLine(knobFullFrame.left, center);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::RenderLabels()
{
    const PRect          bounds = GetNormalizedBounds();
    const PRect          sliderFrame = GetSliderFrame();
    const PPoint         center(bounds.Width() * 0.5f, bounds.Height() * 0.5f);
    const PFontHeight    fontHeight = GetFontHeight();

    SetBgColor(GetEraseColor());
    SetFgColor(0, 0, 0);
    if (m_Orientation == POrientation::Horizontal)
    {
        if (!m_MinLabel.empty() && !m_MaxLabel.empty())
        {
            MovePenTo(0.0f, sliderFrame.bottom + fontHeight.ascender + TICK_SPACING + TICK_LENGTH + HLABEL_SPACING);
            DrawString(m_MinLabel);
            MovePenTo(bounds.right - GetStringWidth(m_MaxLabel.c_str()),
                sliderFrame.bottom + fontHeight.ascender + TICK_SPACING + TICK_LENGTH + HLABEL_SPACING);
            DrawString(m_MaxLabel);
        }
    }
    else
    {
        const float offset = std::ceil(GetKnobFrame(m_Orientation, GetKnobFrameMode::FullFrame).Height() * 0.5f) + VLABEL_SPACING;
        if (!m_MaxLabel.empty())
        {
            MovePenTo(center.x - GetStringWidth(m_MaxLabel) * 0.5f, sliderFrame.top - fontHeight.descender - offset);
            DrawString(m_MaxLabel);
        }
        if (!m_MinLabel.empty()) {
            MovePenTo(center.x - GetStringWidth(m_MinLabel) * 0.5f, sliderFrame.bottom + fontHeight.ascender + offset);
            DrawString(m_MinLabel);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::RenderTicks()
{
    if (m_NumTicks > 0)
    {
        const PRect sliderFrame(GetSliderFrame());
        const float scale = 1.0f / float(m_NumTicks - 1);

        if (m_Orientation == POrientation::Horizontal)
        {
            const float width = std::floor(sliderFrame.Width() - 2.0f);

            const float y1 = std::floor(sliderFrame.top - TICK_SPACING);
            const float y2 = y1 - TICK_LENGTH;
            const float y4 = std::floor(sliderFrame.bottom + TICK_SPACING);
            const float y3 = y4 + TICK_LENGTH;

            SetFgColor(pget_standard_color(PStandardColorID::Shadow));
            for (int i = 0; i < m_NumTicks; ++i)
            {
                const float x = std::floor(sliderFrame.left + width * float(i) * scale);
                if (HasFlags(PSliderFlags::TicksAbove)) {
                    DrawLine(PPoint(x, y1), PPoint(x, y2));
                }
                if (HasFlags(PSliderFlags::TicksBelow)) {
                    DrawLine(PPoint(x, y3), PPoint(x, y4));
                }
            }
            SetFgColor(pget_standard_color(PStandardColorID::Shine));
            for (int i = 0; i < m_NumTicks; ++i)
            {
                const float x = std::floor(sliderFrame.left + width * float(i) * scale) + 1.0f;
                if (HasFlags(PSliderFlags::TicksAbove)) {
                    DrawLine(PPoint(x, y1), PPoint(x, y2));
                }
                if (HasFlags(PSliderFlags::TicksBelow)) {
                    DrawLine(PPoint(x, y3), PPoint(x, y4));
                }
            }
        }
        else
        {
            const float height = sliderFrame.Height() - 2.0f;

            const float x1 = std::floor(sliderFrame.left - TICK_SPACING);
            const float x2 = x1 - TICK_LENGTH;
            const float x4 = std::floor(sliderFrame.right + TICK_SPACING - 1.0f);
            const float x3 = x4 + TICK_LENGTH;

            SetFgColor(pget_standard_color(PStandardColorID::Shadow));
            for (int i = 0; i < m_NumTicks; ++i)
            {
                const float y = std::floor(sliderFrame.top + height * float(i) * scale);
                if (HasFlags(PSliderFlags::TicksLeft)) {
                    DrawLine(PPoint(x1, y), PPoint(x2, y));
                }
                if (HasFlags(PSliderFlags::TicksRight)) {
                    DrawLine(PPoint(x3, y), PPoint(x4, y));
                }
            }
            SetFgColor(pget_standard_color(PStandardColorID::Shine));
            for (int i = 0; i < m_NumTicks; ++i)
            {
                const float y = std::floor(sliderFrame.top + height * float(i) * scale) + 1.0f;
                if (HasFlags(PSliderFlags::TicksLeft)) {
                    DrawLine(PPoint(x1, y), PPoint(x2, y));
                }
                if (HasFlags(PSliderFlags::TicksRight)) {
                    DrawLine(PPoint(x3, y), PPoint(x4, y));
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSlider::PosToVal(const PPoint& position) const
{
    const PRect sliderFrame = GetSliderFrame();
    float value;

    PPoint trackPosition = position - sliderFrame.TopLeft();

    if (m_Orientation == POrientation::Horizontal)
    {
        if (m_NumSteps > 1) {
            float vAlign = sliderFrame.Width() / float(m_NumSteps - 1);
            trackPosition.x = std::floor((trackPosition.x + vAlign * 0.5f) / vAlign) * vAlign;
        }
        value = m_Min + (trackPosition.x / sliderFrame.Width()) * (m_Max - m_Min);
    }
    else
    {
        if (m_NumSteps > 1) {
            float vAlign = sliderFrame.Height() / float(m_NumSteps - 1);
            trackPosition.y = std::floor((trackPosition.y + vAlign * 0.5f) / vAlign) * vAlign;
        }
        value = m_Max + (trackPosition.y / sliderFrame.Height()) * (m_Min - m_Max);
    }
    return std::clamp(value, m_Min, m_Max);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSlider::ValToPos(float value) const
{
    const PRect  bounds = GetSliderFrame();
    const PRect  sliderFrame = GetSliderFrame();
    const float position = (value - m_Min) / (m_Max - m_Min);

    if (m_Orientation == POrientation::Horizontal) {
        return PPoint(std::round(sliderFrame.left + (sliderFrame.Width()) * position), std::round(bounds.top + bounds.Height() * 0.5f));
    } else {
        return PPoint(std::round(bounds.left + bounds.Width() * 0.5f), std::round(sliderFrame.top + sliderFrame.Height() * (1.0f-position)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSlider::GetKnobFrame(POrientation orientation, GetKnobFrameMode mode) const
{
    PRect solidFrame(-7.0f, -10.0f, 8.0f, 11.0f);

    if (HasFlags(PSliderFlags::KnobPointUp) || HasFlags(PSliderFlags::KnobPointDown))
    {
        solidFrame.top      += 4.0f;
        solidFrame.bottom   -= 4.0f;
    }
    if (HasFlags(PSliderFlags::KnobPointUp))
    {
        if (mode == GetKnobFrameMode::FullFrame) {
            solidFrame.top -= 10;
        } else {
            solidFrame.top -= 2;
        }
    }
    if (HasFlags(PSliderFlags::KnobPointDown))
    {
        if (mode == GetKnobFrameMode::FullFrame) {
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

PRect PSlider::GetSliderFrame(PRect* outTotalFrame, float* minimumLength) const
{
    const PRect  bounds(GetNormalizedBounds());
    PRect  knobFrame = GetKnobFrame(POrientation::Horizontal, GetKnobFrameMode::FullFrame);

    float center = std::ceil(GetSliderSize() * 0.5f);
    PRect sliderFrame(0.0f, -center, 0.0f, GetSliderSize() - center);
    PRect totalFrame = sliderFrame;

    if (HasFlags(PSliderFlags::TicksAbove)) {
        totalFrame.top -= TICK_SPACING + TICK_LENGTH;
    }
    if (HasFlags(PSliderFlags::TicksAbove)) {
        totalFrame.bottom += TICK_SPACING + TICK_LENGTH;
    }
    if (knobFrame.top < totalFrame.top)         totalFrame.top = knobFrame.top;
    if (knobFrame.bottom > totalFrame.bottom)   totalFrame.bottom = knobFrame.bottom;

    if (m_Orientation == POrientation::Vertical)
    {
        std::swap(knobFrame.left,   knobFrame.top);
        std::swap(knobFrame.right,  knobFrame.bottom);
        std::swap(sliderFrame.left, sliderFrame.top);
        std::swap(sliderFrame.right, sliderFrame.bottom);
        std::swap(totalFrame.left, totalFrame.top);
        std::swap(totalFrame.right, totalFrame.bottom);
    }

    if (m_Orientation == POrientation::Horizontal)
    {
        if (m_ValueView != nullptr)
        {
            totalFrame.top -= m_ValueView->GetPreferredSize(PPrefSizeType::Smallest).y;
        }
        if (!m_MinLabel.empty() || !m_MaxLabel.empty())
        {
            const PFontHeight    fontHeight = GetFontHeight();
            totalFrame.bottom += fontHeight.ascender + fontHeight.descender;
        }
        const float knobCenter = std::ceil(knobFrame.Width() * 0.5f);
        sliderFrame.left = knobCenter;
        sliderFrame.right = bounds.right - knobCenter;
        totalFrame.left = bounds.left;
        totalFrame.right = bounds.right;
        if (minimumLength != nullptr) {
            *minimumLength = knobCenter * 2.0f;
        }
    }
    else
    {
        const PFontHeight    fontHeight = GetFontHeight();

        const float minLabelWidth = GetStringWidth(m_MinLabel);
        const float minLabelFontHeight = (m_MinLabel.empty()) ? 0.0f : (fontHeight.ascender + fontHeight.descender + VLABEL_SPACING);

        const float maxLabelWidth = GetStringWidth(m_MaxLabel);
        const float maxLabelFontHeight = (m_MaxLabel.empty()) ? 0.0f : (fontHeight.ascender + fontHeight.descender + VLABEL_SPACING);

        const float labelCenter = std::ceil(std::max(minLabelWidth, maxLabelWidth) * 0.5f);

        if (-labelCenter < totalFrame.left) totalFrame.left = -labelCenter;
        if (labelCenter > totalFrame.right) totalFrame.right = labelCenter;

        const float knobCenter = std::ceil(knobFrame.Height() * 0.5f);
        sliderFrame.top     = knobCenter;
        sliderFrame.bottom  = bounds.bottom - knobCenter;

        sliderFrame.top     += maxLabelFontHeight;
        sliderFrame.bottom  -= minLabelFontHeight;

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
    if (m_Orientation == POrientation::Horizontal) {
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

//  const float size = std::ceil(GetSliderSize() * 0.5f);
//
//  if (m_Orientation == Orientation::Horizontal)
//  {
//      sliderBounds.left = std::ceil(knobFrame.Width() * 0.5f);
//      sliderBounds.right = bounds.right - std::ceil(knobFrame.Width() * 0.5f);
//      sliderBounds.top = -std::ceil(GetSliderSize() * 0.5f);
//
//      if (knobFrame.top < sliderBounds.top) sliderBounds.top = knobFrame.top;
//
//      if (m_ValueView != nullptr)
//      {
//          const float textOffset = std::max(TICK_SPACING + TICK_LENGTH, -knobFrame.top);
//          const float height = -(m_ValueView->GetPreferredSize(PrefSizeType::Smallest).y + textOffset);
//          if (height < sliderBounds.top) {
//              sliderBounds.top = height;
//          }
//      }
//      else
//      {
//          if (HasFlags(SliderFlags::TicksAbove)) {
//              if (-(TICK_SPACING + TICK_LENGTH) < sliderBounds.top) sliderBounds.top = -(TICK_SPACING + TICK_LENGTH);
//          }
//      }
//      sliderBounds.bottom = GetSliderSize() - sliderBounds.top;
//      sliderBounds.top = sliderBounds.bottom - GetSliderSize();
//  }
//  else
//  {
//      const Point         center(bounds.Width() * 0.5f, bounds.Height() * 0.5f);
//      const FontHeight    fontHeight = GetFontHeight();
//      const float         knobHeight = std::ceil((knobFrame.Height() + 1.0f) * 0.5f);
//
//      const float minLabelFontHeight = (m_MinLabel.empty()) ? 0.0f : (fontHeight.ascender + fontHeight.descender + VLABEL_SPACING);
//      const float maxLabelFontHeight = (m_MaxLabel.empty()) ? 0.0f : (fontHeight.ascender + fontHeight.descender + VLABEL_SPACING);
//
//      sliderBounds = Rect(center.x - size, knobHeight + maxLabelFontHeight, center.x + size, bounds.bottom - knobHeight - minLabelFontHeight);
//  }
//  sliderBounds.Floor();
//  return sliderBounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::OnPaint(const PRect& updateRect)
{
    RenderSlider();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::UpdateValueView()
{
    if (m_ValueView != nullptr)
    {
        m_ValueView->SetBgColor(PStandardColorID::SliderTrackNormal);
        m_ValueView->SetText(GetLabel() + GetValueString());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::LayoutValueView()
{
    if (m_ValueView != nullptr)
    {
        if (m_Orientation == POrientation::Horizontal)
        {
            const PPoint size = m_ValueView->GetPreferredSize(PPrefSizeType::Smallest);
            const PRect  frame(0.0f, 0.0f, size.x, size.y);
            m_ValueView->SetFrame(frame);
        }
        else
        {
            const PRect  bounds = GetBounds();
            const PPoint size = m_ValueView->GetPreferredSize(PPrefSizeType::Smallest);
            const PRect  frame(0.0f, 0.0f, size.x, size.y);
            m_ValueView->SetFrame(frame + PPoint(bounds.Width() - size.x, std::round((bounds.Height() - size.y) * 0.5f)));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSlider::RefreshDisplay()
{
    Invalidate();
    Flush();
}

