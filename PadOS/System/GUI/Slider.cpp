// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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
#include "GUI/Slider.h"
#include "GUI/Region.h"
#include "GUI/TextView.h"
#include "Utils/XMLFactory.h"
#include "Utils/XMLObjectParser.h"


using namespace os;

const std::map<String, uint32_t> SliderFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, TicksAbove),
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, TicksBelow),
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, TicksLeft),
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, TicksRight),
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, KnobPointUp),
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, KnobPointDown),
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, KnobPointLeft),
    DEFINE_FLAG_MAP_ENTRY(SliderFlags, KnobPointRight)
};

static constexpr float TICK_LENGTH	= 6.0f;
static constexpr float TICK_SPACING	= 3.0f;
static constexpr float VLABEL_SPACING	= 3.0f;
static constexpr float HLABEL_SPACING	= 2.0f;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Slider::Slider(const String& name, Ptr<View> parent, uint32_t flags, int tickCount, Orientation orientation)
    : Control(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize )
{
    m_Orientation   = orientation;
    m_NumTicks	    = tickCount;

    m_SliderColor1  = get_standard_color(StandardColorID::SCROLLBAR_BG);;
    m_SliderColor2  = m_SliderColor1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Slider::Slider(Ptr<View> parent, const pugi::xml_node& xmlData) : Control(parent, xmlData)
{
    MergeFlags(xml_object_parser::parse_flags_attribute(xmlData, SliderFlags::FlagMap, "flags", SliderFlags::TicksBelow | SliderFlags::KnobPointDown) | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize);

    m_Orientation   = Orientation::Horizontal; // orientation;
    m_NumTicks	    = xml_object_parser::parse_attribute(xmlData, "num_ticks", 10);

    m_MinLabel = xml_object_parser::parse_attribute(xmlData, "min_label", String::zero);
    m_MaxLabel = xml_object_parser::parse_attribute(xmlData, "max_label", String::zero);

    m_Min   = xml_object_parser::parse_attribute(xmlData, "min", 0.0f);
    m_Max   = xml_object_parser::parse_attribute(xmlData, "max", 1.0f);
    m_Value = xml_object_parser::parse_attribute(xmlData, "value", m_Min);

    m_SliderColor1  = get_standard_color(StandardColorID::SCROLLBAR_BG);;
    m_SliderColor2  = m_SliderColor1;

    SetValueStringFormat(xml_object_parser::parse_attribute(xmlData, "value_format", String::zero), xml_object_parser::parse_attribute(xmlData, "value_scale", 1.0f));
/*    if (!m_ValueFormat.empty())
    {
	UpdateValueView();
	LayoutValueView();
    }*/
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Slider::~Slider()
{
}

///////////////////////////////////////////////////////////////////////////////
/// Sets the background color to the one of the parent view
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::AttachedToScreen()
{
    SetEraseColor(GetParent()->GetEraseColor());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::FrameSized(const Point& delta)
{
    LayoutValueView();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    const FontHeight fontHeight = GetFontHeight();

    if (m_Orientation == Orientation::Horizontal)
    {
	Point size(100.0f, GetSliderSize());
	Rect  sliderFrame = GetSliderFrame();
	const Rect  knobFrame   = GetKnobFrame(GetKnobFrameMode::FullFrame);

	const float center = ceil(sliderFrame.top + sliderFrame.Height() * 0.5f);
	if (!m_MinLabel.empty() && !m_MaxLabel.empty())
	{
	    const float stringWidth = GetStringWidth(m_MinLabel) + GetStringWidth(m_MaxLabel) + 20.0f;
	    if (stringWidth > size.x) {
		size.x = stringWidth;
	    }
	    sliderFrame.bottom += fontHeight.ascender + fontHeight.descender + TICK_SPACING + TICK_LENGTH + HLABEL_SPACING;
	}
	else if (HasFlags(SliderFlags::TicksBelow))
	{
	    sliderFrame.bottom += TICK_SPACING + TICK_LENGTH;
	}
	if (center + knobFrame.bottom > sliderFrame.bottom) {
	    sliderFrame.bottom = center + knobFrame.bottom;
	}
	size.y = sliderFrame.bottom;
	*minSize = Point(ceil(size.x), ceil(size.y));
	maxSize->x = 2000.0f;
	maxSize->y = minSize->y;
    }
    else
    {
	Point size(GetSliderSize(), 100.0f);

	if (HasFlags(SliderFlags::TicksLeft)) {
	    size.x += TICK_SPACING + TICK_LENGTH + 1.0f;
	}
	if (HasFlags(SliderFlags::TicksRight)) {
	    size.x += TICK_SPACING + TICK_LENGTH + 1.0f;
	}

	if (m_MinLabel.size() > 0) {
	    float stringWidth = GetStringWidth(m_MinLabel);
	    if (stringWidth > size.x) {
		size.x = stringWidth;
	    }
	    size.y += fontHeight.ascender + fontHeight.descender + VLABEL_SPACING;
	}
	if (m_MaxLabel.size() > 0) {
	    float stringWidth = GetStringWidth(m_MaxLabel);
	    if (stringWidth > size.x) {
		size.x = stringWidth;
	    }
	    size.y += fontHeight.ascender + fontHeight.descender + VLABEL_SPACING;
	}
	if (size.x < GetKnobFrame(GetKnobFrameMode::FullFrame).Width()) {
	    size.x = GetKnobFrame(GetKnobFrameMode::FullFrame).Width();
	}
	*minSize = Point(ceil(size.x), ceil(size.y));
	maxSize->x = minSize->x;
	maxSize->y = 2000.0f;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Modify the fill color of the slider bar.
/// \par Description:
///		Set the two colors used to fill the slider bar.
///		The first color is used to fill the "lesser" part
///		of the slider while the second color is used in
///		the "greater" part of the slider.
/// \param color1 - Color used left/below the knob
/// \param color2 - Color used right/above the knob
/// \sa GetSliderColors(), SetSliderSize(), SetLimitLabels(), SetValueStringFormat()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::SetSliderColors(const Color& color1, const Color& color2)
{
    m_SliderColor1 = color1;
    m_SliderColor2 = color2;
    RefreshDisplay();
}

///////////////////////////////////////////////////////////////////////////////
/// Get the slider-bar fill color.
/// \par Description:
///	Return the two slider-bar colors. Each of the two pointers might
///	be NULL.
/// \param outColor1 - Color used left/below the knob
/// \param outColor2 - Color used right/abow the knob
/// \sa SetSliderColors(), SetLimitLabels(), GetSliderSize()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::GetSliderColors(Color* outColor1, Color* outColor2) const
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
///	Set the size in slider bar in pixels. For a horizontal slider it
///	sets the height and for a vertical it sets the width.
/// \par Note:
/// \par Warning:
/// \param vSize - Slider-bar thickness in pixels.
/// \sa GetSliderSize(), GetSliderFrame()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::SetSliderSize(float size)
{
    m_SliderSize = size;
    RefreshDisplay();
}

///////////////////////////////////////////////////////////////////////////////
/// Get the current slider thickness.
/// \par Description:
///	Returns the width for a horizontal and the height for a vertical
///	slider of the slider-bar.
/// \return The thickness of the slider-bar in pixels.
/// \sa SetSliderSize(), GetSliderFrame()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Slider::GetSliderSize() const
{
    return m_SliderSize;
}

///////////////////////////////////////////////////////////////////////////////
/// Set format string for the value label.
/// \par Description:
///	Set a printf() style string that will be used to format the
///	"value" label printed above the slider. The format function
///	will be passed the sliders value as a parameter so if you throw
///	in an %f somewhere it will be replaced with the current value.
/// \par Note:
///	If you need some more advanced formatting you can inherit a new
///	class from os::Slider and overload the GetValueString() and
///	return the string to be used as a value label.
/// \param cFormat - The printf() style format string used to generate the
///		    value label.
/// \sa GetValueStringFormat(), GetValueString(), SetLimitLabels(), GetLimitLabels()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::SetValueStringFormat(const String& format, float scale)
{
    m_ValueFormat   = format;
    m_ValueScale    = scale;

    if (m_Orientation == Orientation::Horizontal && !m_ValueFormat.empty())
    {
	if (m_ValueView == nullptr) {
	    m_ValueView = ptr_new<TextView>("value", String::zero, ptr_tmp_cast(this), ViewFlags::IgnoreMouse);
	    m_ValueView->SignalPreferredSizeChanged.Connect(this, &Slider::LayoutValueView);
	}
	UpdateValueView();
	LayoutValueView();
    }
    else
    {
	if (m_ValueView != nullptr) {
	    RemoveChild(m_ValueView);
	    m_ValueView = nullptr;
	}
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String Slider::GetValueStringFormat() const
{
    return m_ValueFormat;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String Slider::GetValueString() const
{
    if (m_ValueFormat.empty()) {
	return String::zero;
    } else {
	return String::format_string(m_ValueFormat.c_str(), m_Value * m_ValueScale);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::SetValue(float value, bool sendEvent)
{
    if (value != m_Value)
    {
	m_Changed = true;
	Rect knobFrame = GetKnobFrame(GetKnobFrameMode::FullFrame) + ValToPos(m_Value);
	m_Value = value;
	knobFrame |= GetKnobFrame(GetKnobFrameMode::FullFrame) + ValToPos(m_Value);
	Invalidate(knobFrame);
	UpdateValueView();
	Sync();
	SignalValueChanged(m_Value, m_HitButton == MouseButton_e::None);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Slider::GetValue() const
{
    return m_Value;
}

///////////////////////////////////////////////////////////////////////////////
/// Set the number of possible knob positions.
/// \par Description:
///	The step count is used to limit the number of possible knob
///	positions. The points are equally spread out over the length
///	of the slider and the knob will "snap" to these points.
///	If set to the same value as SetTickCount() the knob will snap
///	to the ticks.
///
///	If the value is less than 2 the knob is not snapped.
/// \param nCound - Number of possible knob positions. 0 to disable snapping.
/// \sa GetStepCount(), SetTickCount(), GetTickCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::SetStepCount(int count)
{
    m_NumSteps = count;
}

///////////////////////////////////////////////////////////////////////////////
/// Obtain the step-count as set by SetStepCount()
/// \return The number of possible knob positions.
/// \sa SetStepCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Slider::GetStepCount() const
{
    return m_NumSteps;
}

///////////////////////////////////////////////////////////////////////////////
/// Set number of "ticks" rendered along the slider.
/// \par Description:
///	The slider can render "ticks" along both sides of the slider-bar.
///	With SetTickCount() you can set how many such ticks should be rendered
///	and with SetTickFlags() you can set on which side (if any) the ticks
///	should be rendered. To make the knob "snap" to the ticks you can set
///	the same count with SetStepCount().
/// \param count - The number of ticks to be rendered.
/// \sa GetTickCount(), SetTickFlags(), GetTickFlags(), SetStepCount()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::SetTickCount(int count)
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

int Slider::GetTickCount() const
{
    return m_NumTicks;
}

///////////////////////////////////////////////////////////////////////////////
/// Set the static labels rendere at each end of the slider.
/// \par Description:
///	The min/max labels are rendered below a horizontal slider
///	and to the right of a vertical slider.
/// \par Note:
///	Both label must be set for the labels to be rendered.
/// \param minLabel - The label to be rendered at the "smaller" end of the slider.
/// \param maxLabel - The label to be rendered at the "greater" end of the slider.
/// \sa GetLimitLabels(), SetValueStringFormat()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::SetLimitLabels(const String& minLabel, const String& maxLabel)
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

void Slider::GetLimitLabels(String* minLabel, String* maxLabel)
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

void Slider::OnEnableStatusChanged(bool isEnabled)
{
    if (m_HitButton != MouseButton_e::None)
    {
	m_HitButton = MouseButton_e::None;
	if (m_Changed)
	{
	    SignalValueChanged(m_Value, true);
	    m_Changed = false;
	}
	MakeFocus(false);
    }
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Slider::OnMouseDown(MouseButton_e button, const Point& position)
{
    if (!IsEnabled()) return false;
    if (m_HitButton == MouseButton_e::None)
    {
	m_HitButton = button;
	m_HitPos    = position - ValToPos(GetValue());;
	m_Changed   = false;
	MakeFocus(true);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Slider::OnMouseUp(MouseButton_e button, const Point& position)
{
    if (!IsEnabled()) return false;
    if (button == m_HitButton)
    {
	m_HitButton = MouseButton_e::None;
	if (m_Changed)
	{
	    SignalValueChanged(m_Value, true);
	    m_Changed = false;
	}
	MakeFocus(false);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Slider::OnMouseMove(MouseButton_e button, const Point& position)
{
    if (!IsEnabled()) return false;
    if (button == m_HitButton) {
	SetValue(PosToVal(position - m_HitPos));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::RenderSlider()
{
    Rect bounds = GetNormalizedBounds();

    Rect sliderFrame = GetSliderFrame();
    Rect knobFrame = GetKnobFrame(GetKnobFrameMode::FullFrame) + ValToPos(GetValue());

    // Clear areas not fully overwritten by the slider track and the knob.
    Region clearRegion(bounds);
    clearRegion.Exclude(sliderFrame);
    clearRegion.Exclude(knobFrame);
    clearRegion.Optimize();
    for (const IRect& rect : clearRegion.m_Rects) {
	EraseRect(rect);
    }

    RenderTicks();
    RenderLabels();
    RenderKnob();

    if (sliderFrame.IsValid())
    {
	const Color shineColor    = get_standard_color(StandardColorID::SHINE);
	const Color shadowColor   = get_standard_color(StandardColorID::SHADOW);

	if (m_Orientation == Orientation::Horizontal)
	{
	    Rect leftSliderFrame(sliderFrame.left, sliderFrame.top, knobFrame.left, sliderFrame.bottom);
	    Rect rightSliderFrame(knobFrame.right - 1.0f, sliderFrame.top, sliderFrame.right, sliderFrame.bottom);
	    if (leftSliderFrame.IsValid())
	    {
		Rect leftSliderCenter = leftSliderFrame;
		leftSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
		leftSliderCenter.Resize(1.0f, 1.0f, 0.0f, -1.0f);
		
		SetFgColor(shadowColor);
		MovePenTo(leftSliderFrame.BottomLeft());
		DrawLine(leftSliderFrame.TopLeft());
		DrawLine(leftSliderFrame.TopRight());

		SetFgColor(shineColor);
		MovePenTo(leftSliderFrame.BottomRight());
		DrawLine(Point(leftSliderFrame.left + 1, leftSliderFrame.bottom));

		FillRect(leftSliderCenter, m_SliderColor1);
	    }
	    if (rightSliderFrame.IsValid())
	    {
		Rect rightSliderCenter = rightSliderFrame;
		rightSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
		rightSliderCenter.Resize(0.0f, 1.0f, -1.0f, -1.0f);

		SetFgColor(shadowColor);
		MovePenTo(rightSliderFrame.TopLeft());
		DrawLine(rightSliderFrame.TopRight());

		SetFgColor(shineColor);
		MovePenTo(Point(rightSliderFrame.right, rightSliderFrame.top + 1.0f));
		DrawLine(rightSliderFrame.BottomRight());
		DrawLine(rightSliderFrame.BottomLeft() + Point(1.0f, 0.0f));

		FillRect(rightSliderCenter, m_SliderColor2);
	    }
	}
	else
	{
	    Rect topSliderFrame(sliderFrame.left, sliderFrame.top, sliderFrame.right, knobFrame.top);
	    Rect bottomSliderFrame(sliderFrame.left, knobFrame.bottom - 1.0f, sliderFrame.right, sliderFrame.bottom);
	    if (topSliderFrame.IsValid())
	    {
		Rect topSliderCenter = topSliderFrame;
		topSliderCenter.Resize(1.0f, 1.0f, -1.0f, 0.0f);
		topSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

		SetFgColor(shadowColor);
		MovePenTo(topSliderFrame.BottomLeft());
		DrawLine(topSliderFrame.TopLeft());
		DrawLine(topSliderFrame.TopRight());

		SetFgColor(shineColor);
		MovePenTo(topSliderFrame.BottomRight());
		DrawLine(Point(topSliderFrame.right, topSliderFrame.top + 1.0f));

		FillRect(topSliderCenter, m_SliderColor1);
	    }
	    if (bottomSliderFrame.IsValid())
	    {
		Rect bottomSliderCenter = bottomSliderFrame;
		bottomSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
		bottomSliderCenter.Resize(1.0f, 0.0f, -1.0f, -1.0f);

		SetFgColor(shadowColor);
		MovePenTo(bottomSliderFrame.TopLeft());
		DrawLine(bottomSliderFrame.BottomLeft());

		SetFgColor(shineColor);
		MovePenTo(Point(bottomSliderFrame.left + 1.0f, bottomSliderFrame.bottom));
		DrawLine(bottomSliderFrame.BottomRight());
		DrawLine(bottomSliderFrame.TopRight() + Point(0.0f, 1.0f));

		FillRect(bottomSliderCenter, m_SliderColor2);
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::RenderKnob()
{
    Rect knobFrame = GetKnobFrame(GetKnobFrameMode::SquareFrame) + ValToPos(GetValue());

    if (!HasFlags(SliderFlags::KnobPointUp | SliderFlags::KnobPointDown))
    {
	Rect center(knobFrame);
	center.Resize(1.0f, 1.0f, -1.0f, -1.0f);
	knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

	SetFgColor(StandardColorID::SHINE);
	MovePenTo(knobFrame.BottomLeft());
	DrawLine(knobFrame.TopLeft());
	DrawLine(knobFrame.TopRight());

	SetFgColor(StandardColorID::SHADOW);
	DrawLine(knobFrame.BottomRight());
	DrawLine(knobFrame.BottomLeft() + Point(1.0f, 0.0f));
	FillRect(center, get_standard_color(StandardColorID::SCROLLBAR_KNOB));
    }
    else
    {
	const Color shineColor    = get_standard_color(StandardColorID::SHINE);
	const Color shadowColor   = get_standard_color(StandardColorID::SHADOW);

	Rect knobFullFrame = GetKnobFrame(GetKnobFrameMode::FullFrame) + ValToPos(GetValue());

	Rect centerFrame = knobFrame;
	knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
	knobFullFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

	if (m_Orientation == Orientation::Horizontal)
	{
	    const float center = floor(knobFrame.left + (knobFrame.right - knobFrame.left) * 0.5f);

	    SetFgColor(StandardColorID::SCROLLBAR_KNOB);
	    float offset = 0.0f;
	    float direction = 1.0f;
	    for (float x = knobFrame.left; x <= knobFrame.right; x+= 1.0f)
	    {
		float y1 = knobFrame.top;
		float y2 = knobFrame.bottom;

		if (HasFlags(SliderFlags::KnobPointUp))	    y1 -= offset;
		if (HasFlags(SliderFlags::KnobPointDown))   y2 += offset;

		SetFgColor(GetEraseColor());
		if (HasFlags(SliderFlags::KnobPointUp))	    DrawLine(x, knobFullFrame.top, x, y1 - 1.0f);
		if (HasFlags(SliderFlags::KnobPointDown))   DrawLine(x, y2 + 1.0f, x, knobFullFrame.bottom);
		SetFgColor(StandardColorID::SCROLLBAR_KNOB);
		DrawLine(x, y1, x, y2);
		if (direction > 0.0f && x >= center) {
		    direction = -1.0f;
		}
		offset += direction;
	    }

	    SetFgColor(shadowColor);

	    if (HasFlags(SliderFlags::KnobPointDown)) {
		MovePenTo(center, knobFullFrame.bottom);
	    } else {
		MovePenTo(knobFrame.BottomLeft());
	    }
	    DrawLine(knobFrame.BottomRight());
	    DrawLine(knobFrame.TopRight());
	    if (HasFlags(SliderFlags::KnobPointUp)) {
		DrawLine(center, knobFullFrame.top);
	    }
	    SetFgColor(shineColor);
	    DrawLine(knobFrame.TopLeft());
	    DrawLine(knobFrame.BottomLeft());
	    if (HasFlags(SliderFlags::KnobPointDown)) {
		DrawLine(center, knobFullFrame.bottom);
	    }
	}
	else
	{
	    const float center = floor(knobFrame.top + (knobFrame.bottom - knobFrame.top) * 0.5f);

	    SetFgColor(StandardColorID::SCROLLBAR_KNOB);
	    float offset = 0.0f;
	    float direction = 1.0f;
	    for (float y = knobFrame.top; y <= knobFrame.bottom; y+= 1.0f)
	    {
		float x1 = knobFrame.left;
		float x2 = knobFrame.right;

		if (HasFlags(SliderFlags::KnobPointLeft))  x1 -= offset;
		if (HasFlags(SliderFlags::KnobPointRight)) x2 += offset;

		SetFgColor(GetEraseColor());
		if (HasFlags(SliderFlags::KnobPointLeft))  DrawLine(knobFullFrame.left, y, x1 - 1.0f, y);
		if (HasFlags(SliderFlags::KnobPointRight)) DrawLine(x2 + 1.0f, y, knobFullFrame.right, y);
		SetFgColor(StandardColorID::SCROLLBAR_KNOB);
		DrawLine(x1, y, x2, y);
		if (direction > 0.0f && y >= center) {
		    direction = -1.0f;
		}
		offset += direction;
	    }

	    SetFgColor(shineColor);
	    if (HasFlags(SliderFlags::KnobPointLeft)) {
		MovePenTo(knobFullFrame.left, center);
	    } else {
		MovePenTo(knobFrame.BottomLeft());
	    }
	    DrawLine(knobFrame.TopLeft());
	    DrawLine(knobFrame.TopRight());
	    if (HasFlags(SliderFlags::KnobPointRight)) {
		DrawLine(knobFullFrame.right, center);
	    }
	    SetFgColor(shadowColor);
	    DrawLine(knobFrame.BottomRight());
	    DrawLine(knobFrame.BottomLeft());
	    if (HasFlags(SliderFlags::KnobPointLeft)) {
		DrawLine(knobFullFrame.left, center);
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::RenderLabels()
{
    const Rect bounds = GetNormalizedBounds();
    const Rect sliderFrame = GetSliderFrame();
    const Point center(bounds.Width() * 0.5f, bounds.Height() * 0.5f);
    const FontHeight fontHeight = GetFontHeight();

    SetBgColor(GetEraseColor());
    SetFgColor(0, 0, 0);
    if (m_Orientation == Orientation::Horizontal)
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
	const float offset = ceil(GetKnobFrame(GetKnobFrameMode::FullFrame).Height() * 0.5f) + VLABEL_SPACING;
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

void Slider::RenderTicks()
{
    if (m_NumTicks > 0)
    {
	const Rect sliderFrame(GetSliderFrame());
	const float scale = 1.0f / float(m_NumTicks - 1);

	if (m_Orientation == Orientation::Horizontal)
	{
	    const float width = floor(sliderFrame.Width() - 2.0f);

	    const float y1 = floor(sliderFrame.top - TICK_SPACING);
	    const float y2 = y1 - TICK_LENGTH;
	    const float y4 = floor(sliderFrame.bottom + TICK_SPACING);
	    const float y3 = y4 + TICK_LENGTH;

	    SetFgColor(get_standard_color(StandardColorID::SHADOW));
	    for (float i = 0; i < m_NumTicks; i += 1.0f)
	    {
		const float x = floor(sliderFrame.left + width * i * scale);
		if (HasFlags(SliderFlags::TicksAbove)) {
		    DrawLine(Point(x, y1), Point(x, y2));
		}
		if (HasFlags(SliderFlags::TicksBelow)) {
		    DrawLine(Point(x, y3), Point(x, y4));
		}
	    }
	    SetFgColor(get_standard_color(StandardColorID::SHINE));
	    for (float i = 0; i < m_NumTicks; i += 1.0f)
	    {
		const float x = floor(sliderFrame.left + width * i * scale) + 1.0f;
		if (HasFlags(SliderFlags::TicksAbove)) {
		    DrawLine(Point(x, y1), Point(x, y2));
		}
		if (HasFlags(SliderFlags::TicksBelow)) {
		    DrawLine(Point(x, y3), Point(x, y4));
		}
	    }
	}
	else
	{
	    const float height = sliderFrame.Height() - 2.0f;

	    const float x1 = floor(sliderFrame.left - TICK_SPACING);
	    const float x2 = x1 - TICK_LENGTH;
	    const float x4 = floor(sliderFrame.right + TICK_SPACING - 1.0f);
	    const float x3 = x4 + TICK_LENGTH;

	    SetFgColor(get_standard_color(StandardColorID::SHADOW));
	    for (float i = 0; i < m_NumTicks; i += 1.0f)
	    {
		const float y = floor(sliderFrame.top + height * i * scale);
		if (HasFlags(SliderFlags::TicksLeft)) {
		    DrawLine(Point(x1, y), Point(x2, y));
		}
		if (HasFlags(SliderFlags::TicksRight)) {
		    DrawLine(Point(x3, y), Point(x4, y));
		}
	    }
	    SetFgColor(get_standard_color(StandardColorID::SHINE));
	    for (float i = 0; i < m_NumTicks; i += 1.0f)
	    {
		const float y = floor(sliderFrame.top + height * i * scale) + 1.0f;
		if (HasFlags(SliderFlags::TicksLeft)) {
		    DrawLine(Point(x1, y), Point(x2, y));
		}
		if (HasFlags(SliderFlags::TicksRight)) {
		    DrawLine(Point(x3, y), Point(x4, y));
		}
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Slider::PosToVal(const Point& position) const
{
    const Rect sliderFrame = GetSliderFrame();
    float value;

    Point trackPosition = position - sliderFrame.TopLeft();

    if (m_Orientation == Orientation::Horizontal)
    {
	if (m_NumSteps > 1) {
	    float vAlign = sliderFrame.Width() / float(m_NumSteps - 1);
	    trackPosition.x = floor((trackPosition.x + vAlign * 0.5f) / vAlign) * vAlign;
	}
	value = m_Min + (trackPosition.x / sliderFrame.Width()) * (m_Max - m_Min);
    }
    else
    {
	if (m_NumSteps > 1) {
	    float vAlign = sliderFrame.Height() / float(m_NumSteps - 1);
	    trackPosition.y = floor((trackPosition.y + vAlign * 0.5f) / vAlign) * vAlign;
	}
	value = m_Max + (trackPosition.y / sliderFrame.Height()) * (m_Min - m_Max);
    }
    return std::clamp(value, m_Min, m_Max);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point Slider::ValToPos(float value) const
{
    const Rect  bounds = GetSliderFrame();
    const Rect  sliderFrame = GetSliderFrame();
    const float position = (value - m_Min) / (m_Max - m_Min);

    if (m_Orientation == Orientation::Horizontal) {
	return Point(round(sliderFrame.left + (sliderFrame.Width()) * position), round(bounds.top + bounds.Height() * 0.5f));
    } else {
	return Point(round(bounds.left + bounds.Width() * 0.5f), round(sliderFrame.top + sliderFrame.Height() * (1.0f-position)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect Slider::GetKnobFrame(GetKnobFrameMode mode) const
{
    Rect solidFrame(-7.0f, -10.0f, 8.0f, 11.0f);

    if (HasFlags(SliderFlags::KnobPointUp) || HasFlags(SliderFlags::KnobPointDown)) {
	solidFrame.top += 4.0f;
	solidFrame.bottom -= 4.0f;
    }


    if (HasFlags(SliderFlags::KnobPointUp))
    {
	if (mode == GetKnobFrameMode::FullFrame) {
	    solidFrame.top -= 10;
	} else {
	    solidFrame.top -= 2;
	}
    }
    if (HasFlags(SliderFlags::KnobPointDown))
    {
	if (mode == GetKnobFrameMode::FullFrame) {
	    solidFrame.bottom += 10;
	} else {
	    solidFrame.bottom += 2;
	}
    }

    if (m_Orientation == Orientation::Vertical)
    {
	std::swap(solidFrame.left, solidFrame.top);
	std::swap(solidFrame.right, solidFrame.bottom);
    }
    return solidFrame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect Slider::GetSliderFrame() const
{
    const Rect  bounds(GetNormalizedBounds());
    const Rect  knobFrame = GetKnobFrame(GetKnobFrameMode::FullFrame);
    Rect  sliderBounds;

    const float size = ceil(GetSliderSize() * 0.5f);

    if (m_Orientation == Orientation::Horizontal)
    {
	sliderBounds.left   = ceil(knobFrame.Width() * 0.5f);
	sliderBounds.right  = bounds.right - ceil(knobFrame.Width() * 0.5f);
	sliderBounds.top    = -ceil(GetSliderSize() * 0.5f);

	if (knobFrame.top < sliderBounds.top) sliderBounds.top = knobFrame.top;

	if (m_ValueView != nullptr)
	{
	    float textOffset = std::max(TICK_SPACING + TICK_LENGTH, -knobFrame.top);
	    float height = -(m_ValueView->GetPreferredSize(PrefSizeType::Smallest).y + textOffset);
	    if (height < sliderBounds.top) {
		sliderBounds.top = height;
	    }
	}
	else
	{
	    if (HasFlags(SliderFlags::TicksAbove)) {
		if (-(TICK_SPACING + TICK_LENGTH) < sliderBounds.top) sliderBounds.top = -(TICK_SPACING + TICK_LENGTH);
	    }
	}
	sliderBounds.bottom = GetSliderSize() - sliderBounds.top;
	sliderBounds.top = sliderBounds.bottom - GetSliderSize();
    }
    else
    {
	const Point	    center(bounds.Width() * 0.5f, bounds.Height() * 0.5f);
	const FontHeight    fontHeight = GetFontHeight();
	const float	    knobHeight = ceil((knobFrame.Height() + 1.0f) * 0.5f);

	const float minLabelFontHeight = (m_MinLabel.empty()) ? 0.0f : (fontHeight.ascender + fontHeight.descender + VLABEL_SPACING);
	const float maxLabelFontHeight = (m_MaxLabel.empty()) ? 0.0f : (fontHeight.ascender + fontHeight.descender + VLABEL_SPACING);

	sliderBounds = Rect(center.x - size, knobHeight + maxLabelFontHeight, center.x + size, bounds.bottom - knobHeight - minLabelFontHeight);
    }
    sliderBounds.Floor();
    return sliderBounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::Paint(const Rect& updateRect)
{
    RenderSlider();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::UpdateValueView()
{
    if (m_ValueView != nullptr)
    {
	m_ValueView->SetBgColor(GetEraseColor());
	m_ValueView->SetText(GetValueString());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::LayoutValueView()
{
    if (m_ValueView != nullptr)
    {
	if (m_Orientation == Orientation::Horizontal)
	{
	    const Point	size = m_ValueView->GetPreferredSize(PrefSizeType::Smallest);
	    const Rect	frame(0.0f, 0.0f, size.x, size.y);
	    m_ValueView->SetFrame(frame);
	}
	else
	{
	    const Rect	bounds = GetBounds();
	    const Point size = m_ValueView->GetPreferredSize(PrefSizeType::Smallest);
	    const Rect	frame(0.0f, 0.0f, size.x, size.y);
	    m_ValueView->SetFrame(frame + Point(bounds.Width() - size.x, round((bounds.Height() - size.y) * 0.5f)));
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Slider::RefreshDisplay()
{
    Invalidate();
    Flush();
}
