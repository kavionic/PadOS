// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <assert.h>
#include <cmath>

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/ScrollBar.h>
#include <Threads/Looper.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBar::PScrollBar(const PString& name, float min, float max, POrientation orientation, uint32_t flags)
    : PControl(name, flags | PViewFlags::WillDraw)
    , m_Min(min)
    , m_Max(max)
{
    m_ScrollBarStyleData.Orientation = orientation;
    m_ScrollBarStyleData.HasRange = m_Max > m_Min;
    m_ScrollBarStyleData.Value = GetRelativeValue();
    m_RepeatTimer.SignalTrigged.Connect(this, &PScrollBar::SlotTimerTick);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBar::~PScrollBar()
{
    if (m_Target != nullptr)
    {
        if (m_ScrollBarStyleData.Orientation == POrientation::Horizontal) {
            m_Target->SetHScrollBar(nullptr);
        } else {
            m_Target->SetVScrollBar(nullptr);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SetScrollTarget(Ptr<PView> target)
{
    if (m_Target != nullptr)
    {
        if (m_ScrollBarStyleData.Orientation == POrientation::Horizontal) {
            assert(m_Target->GetHScrollBar() == this);
            m_Target->SetHScrollBar(nullptr);
        } else {
            assert(m_Target->GetVScrollBar() == this);
            m_Target->SetVScrollBar(nullptr);
        }
    }
    m_Target = ptr_raw_pointer_cast(target);
    if (m_Target != nullptr)
    {
        if (m_ScrollBarStyleData.Orientation == POrientation::Horizontal)
        {
            assert(m_Target->GetHScrollBar() == nullptr);
            m_Target->SetHScrollBar(this);
            SetValue(m_Target->GetScrollOffset().x);
        }
        else
        {
            assert(m_Target->GetVScrollBar() == nullptr);
            m_Target->SetVScrollBar(this);
            SetValue(m_Target->GetScrollOffset().y);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PScrollBar::GetScrollTarget()
{
    return ptr_tmp_cast(m_Target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnFrameSized(const PPoint& delta)
{
	PControl::OnFrameSized(delta);
	NotifyStylePropertyChanged(PScrollBarProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnFontChanged(Ptr<const PFont> newFont)
{
    NotifyStylePropertyChanged(PScrollBarProperty::Font);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnStyleChanged()
{
    m_ScrollBarStyleData.StyleData.Reset();
    EndInteraction();
    PControl::OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnEnableStatusChanged(bool isEnabled)
{
    m_ScrollBarStyleData.IsEnabled = isEnabled;
    if (!isEnabled) {
        EndInteraction();
    }
    NotifyStylePropertyChanged(PScrollBarProperty::Enabled);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
	if (phase != PEventPhase::Target || !IsEnabled() || m_HitPointerID != PInvalidPointerID) {
		return;
	}
	const PGUIStyle* style = GetStyle();
	if (style == nullptr) {
		return;
	}
	const PScrollBarPart part = style->ScrollBarHitTest(*this, m_ScrollBarStyleData, position);
	if (part == PScrollBarPart::None || !SetPointerCapture(pointerID)) {
		return;
	}
	m_HitPointerID = pointerID;
	m_HitPart = part;
	m_Changed = false;
	m_ScrollBarStyleData.PressedPart = part;
	if (part == PScrollBarPart::Knob)
	{
		m_HitPos = position - style->ScrollBarGetValuePosition(*this, m_ScrollBarStyleData, m_ScrollBarStyleData.Value);
	}
	else if (IsArrowPart(part))
	{
		m_RepeatTimer.Set(TimeValNanos::FromMilliseconds(300));
		m_RepeatTimer.Start(true, GetLooper());
	}
	else
	{
		StepValue(part);
	}
	NotifyStylePropertyChanged(PScrollBarProperty::Pressed);
	Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
	if (phase != PEventPhase::Target || pointerID != m_HitPointerID) {
		return;
	}
	const PGUIStyle* style = GetStyle();
	if (style != nullptr && IsArrowPart(m_HitPart) && m_FirstTick)
	{
		if (style->ScrollBarHitTest(*this, m_ScrollBarStyleData, position) == m_HitPart) {
			StepValue(m_HitPart);
		}
	}
	else if (m_HitPart == PScrollBarPart::Knob && m_Changed)
	{
		SignalValueChanged(GetValue(), true, ptr_tmp_cast(this)); // Send a 'final' event.
	}
	EndInteraction();
	Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
	if (phase != PEventPhase::Target || pointerID != m_HitPointerID) {
		return;
	}
	const PGUIStyle* style = GetStyle();
	if (style == nullptr) {
		return;
	}
	if (IsArrowPart(m_HitPart))
	{
		const bool isHit = style->ScrollBarHitTest(*this, m_ScrollBarStyleData, position) == m_HitPart;
		const PScrollBarPart pressedPart = isHit ? m_HitPart : PScrollBarPart::None;
		if (pressedPart != m_ScrollBarStyleData.PressedPart)
		{
			if (isHit)
			{
				m_RepeatTimer.Set(TimeValNanos::FromMilliseconds(300));
				m_RepeatTimer.Start(true, GetLooper());
			}
			else
			{
				m_RepeatTimer.Stop();
			}
			m_ScrollBarStyleData.PressedPart = pressedPart;
			NotifyStylePropertyChanged(PScrollBarProperty::Pressed);
			Flush();
		}
	}
	else if (m_HitPart == PScrollBarPart::Knob)
	{
		const float relativeValue = style->ScrollBarGetValueFromPosition(*this, m_ScrollBarStyleData, position - m_HitPos);
		SetValue(m_Min + (m_Max - m_Min) * std::clamp(relativeValue, 0.0f, 1.0f));
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnPointerCancel(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Target && pointerID == m_HitPointerID)
    {
        EndInteraction();
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason)
{
    if (pointerID == m_HitPointerID)
    {
        EndInteraction();
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void ScrollBar::WheelMoved(const Point& delta)
//{
//	float value = GetValue();
//
//	if (m_Orientation == Orientation::Vertical && delta.y != 0.0f) {
//		value += delta.y * m_SmallStep;
//	} else if (m_Orientation == Orientation::Horizontal && delta.x != 0.0f) {
//		value += delta.y * m_SmallStep;
//	}
//	if (value < m_Min) {
//		value = m_Min;
//	} else if (value > m_Max) {
//		value = m_Max;
//	}
//
//	SetValue(value);
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnPaint(const PRect& updateRect)
{
	const PGUIStyle* style = GetStyle();
	if (style != nullptr) {
		style->ScrollBarPaint(*this, GetNormalizedBounds(), m_ScrollBarStyleData);
	} else {
		PView::OnPaint(updateRect);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->ScrollBarGetPreferredSize(*this, m_ScrollBarStyleData, minSize, maxSize);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SlotTimerTick()
{
	if (!IsEnabled() || GetStyle() == nullptr || !IsArrowPart(m_ScrollBarStyleData.PressedPart)) {
		return;
	}
	StepValue(m_HitPart);
	if (m_RepeatTimer.IsSingleshot())
	{
		m_RepeatTimer.Set(TimeValNanos::FromMilliseconds(30));
		m_RepeatTimer.Start(false, GetLooper());
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SetValue(float value)
{
    if (value == m_Value) {
        return;
    }
    m_Value = value;
    m_ScrollBarStyleData.Value = GetRelativeValue();
    if (m_Target != nullptr)
    {
        PPoint pos = m_Target->GetScrollOffset();

        if (m_ScrollBarStyleData.Orientation == POrientation::Horizontal) {
            pos.x = -std::floor(value);
        } else {
            pos.y = -std::floor(value);
        }
        m_Target->ScrollTo(pos);
        m_Target->Flush();
    }
    NotifyStylePropertyChanged(PScrollBarProperty::Value);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SetSteps(float small, float big)
{
    m_SmallStep = small;
    m_BigStep   = big;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::GetSteps(float* small, float* big) const
{
    *small = m_SmallStep;
    *big   = m_BigStep;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SetMinMax(float min, float max)
{
    m_Min = min;
    m_Max = max;
    m_ScrollBarStyleData.HasRange = m_Max > m_Min;
    m_ScrollBarStyleData.Value = GetRelativeValue();
    NotifyStylePropertyChanged(PScrollBarProperties(PScrollBarProperty::Range, PScrollBarProperty::Value));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SetProportion(float proportion)
{
    m_ScrollBarStyleData.Proportion = proportion;
    NotifyStylePropertyChanged(PScrollBarProperty::Proportion);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PScrollBar::GetProportion() const
{
    return m_ScrollBarStyleData.Proportion;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollBar::IsArrowPart(PScrollBarPart part)
{
    return part == PScrollBarPart::DecrementStart || part == PScrollBarPart::IncrementStart ||
        part == PScrollBarPart::DecrementEnd || part == PScrollBarPart::IncrementEnd;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::StepValue(PScrollBarPart part)
{
    float step = 0.0f;
    switch (part)
    {
        case PScrollBarPart::DecrementStart:
        case PScrollBarPart::DecrementEnd:
            step = -m_SmallStep;
            break;
        case PScrollBarPart::IncrementStart:
        case PScrollBarPart::IncrementEnd:
            step = m_SmallStep;
            break;
        case PScrollBarPart::PageDecrement:
            step = -m_BigStep;
            break;
        case PScrollBarPart::PageIncrement:
            step = m_BigStep;
            break;
        default:
            return;
    }
    SetValue(std::clamp(m_Value + step, m_Min, m_Max));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::EndInteraction()
{
    m_RepeatTimer.Stop();
    const PPointerID pointerID = m_HitPointerID;
    m_HitPointerID = PInvalidPointerID;
    m_HitPart = PScrollBarPart::None;
    m_Changed = false;
    if (pointerID != PInvalidPointerID && HasPointerCapture(pointerID)) {
        ReleasePointerCapture(pointerID);
    }
    if (m_ScrollBarStyleData.PressedPart != PScrollBarPart::None)
    {
        m_ScrollBarStyleData.PressedPart = PScrollBarPart::None;
        NotifyStylePropertyChanged(PScrollBarProperty::Pressed);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PScrollBar::GetRelativeValue() const
{
    return (m_Max > m_Min) ? std::clamp((m_Value - m_Min) / (m_Max - m_Min), 0.0f, 1.0f) : 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::NotifyStylePropertyChanged(PScrollBarProperties properties)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        style->ScrollBarPropertyChanged(*this, m_ScrollBarStyleData, properties);
    }
    else
    {
        PreferredSizeChanged();
        Invalidate();
    }
}

