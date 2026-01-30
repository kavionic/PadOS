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

#include <GUI/Widgets/ScrollBar.h>
#include <GUI/Color.h>
#include <Threads/Looper.h>

//#include "stdbitmaps.h"


enum { SB_MINSIZE = 12 };

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PColor Tint(const PColor& color, float tint)
{
    int r = int((float(color.GetRed()) * tint + 127.0f * (1.0f - tint)));
    int g = int((float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)));
    int b = int((float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)));
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    return PColor(uint8_t(r), uint8_t(g), uint8_t(b), color.GetAlpha());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBar::PScrollBar(const PString& name, Ptr<PView> parent, float min, float max, POrientation orientation, uint32_t flags)
    : PControl(name, parent, flags | PViewFlags::WillDraw)
{
    memset(m_ArrowStates, 0, sizeof(m_ArrowStates));
    m_Orientation  = orientation;
    m_Min          = min;
    m_Max          = max;
    OnFrameSized(PPoint(0.0f, 0.0f));

    m_RepeatTimer.SignalTrigged.Connect(this, &PScrollBar::SlotTimerTick);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBar::~PScrollBar()
{
    if (m_Target != nullptr)
    {
        if (m_Orientation == POrientation::Horizontal) {
            m_Target->SetHScrollBar(nullptr);
        } else {
            m_Target->SetVScrollBar(nullptr);
        }
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
    if (m_Target != nullptr)
    {
        PPoint pos = m_Target->GetScrollOffset();

        if (m_Orientation == POrientation::Horizontal) {
            pos.x = -std::floor(value);
        } else {
            pos.y = -std::floor(value);
        }
        m_Target->ScrollTo(pos);
        m_Target->Flush();
    }
    Invalidate(m_KnobArea);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SetProportion(float proportion)
{
    m_Proportion = proportion;
    Invalidate(m_KnobArea);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PScrollBar::GetProportion() const
{
    return m_Proportion;
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
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SetScrollTarget(Ptr<PView> target)
{
    if (m_Target != nullptr)
    {
        if (m_Orientation == POrientation::Horizontal) {
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
        if (m_Orientation == POrientation::Horizontal)
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

void PScrollBar::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const float w = 16.0f;

    if (m_Orientation == POrientation::Horizontal) {
        *minSize = PPoint(0.0f, w);
        *maxSize = PPoint(COORD_MAX, w);
    } else {
        *minSize = PPoint(w, 0.0f);
        *maxSize = PPoint(w, COORD_MAX);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::SlotTimerTick()
{
	float value = GetValue();
	if (m_HitButton & 0x01) {
		value += m_SmallStep;
	} else {
		value -= m_SmallStep;
	}
	if (value < m_Min) {
		value = m_Min;
	} else if (value > m_Max) {
		value = m_Max;
	}
	SetValue(value);

	if (m_RepeatTimer.IsSingleshot()) {
        m_RepeatTimer.Set(TimeValNanos::FromMilliseconds(30));
		GetLooper()->AddTimer(&m_RepeatTimer, false);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollBar::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
	MakeFocus(button, true);
	m_HitState = HIT_NONE;
	m_Changed = false;

	for (int i = 0; i < 4; ++i)
	{
		if (m_ArrowRects[i].DoIntersect(position))
		{
			m_ArrowStates[i] = true;
			m_HitButton = i;
			m_HitState = HIT_ARROW;
			Invalidate(m_ArrowRects[i]);
			Flush();
            m_RepeatTimer.Set(TimeValNanos::FromMilliseconds(300));
            GetLooper()->AddTimer(&m_RepeatTimer, true);
			return true;
		}
	}
	if (m_KnobArea.DoIntersect(position))
	{
		PRect cKnobFrame = GetKnobFrame();
		if (cKnobFrame.DoIntersect(position)) {
			m_HitPos = position - cKnobFrame.TopLeft();
			m_HitState = HIT_KNOB;
			return true;
		}
		float vValue = GetValue();
		if (m_Orientation == POrientation::Horizontal)
		{
			if (position.x < cKnobFrame.left) {
				vValue -= m_BigStep;
			} else if (position.x > cKnobFrame.right) {
				vValue += m_BigStep;
			}
		}
		else
		{
			if (position.y < cKnobFrame.top) {
				vValue -= m_BigStep;
			} else if (position.y > cKnobFrame.bottom) {
				vValue += m_BigStep;
			}
		}
		if (vValue < m_Min) {
			vValue = m_Min;
		} else if (vValue > m_Max) {
			vValue = m_Max;
		}
		SetValue(vValue);
		return true;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollBar::OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
	if (m_HitState == HIT_ARROW)
	{
		float value = GetValue();
		bool changed = false;
		if (m_FirstTick)
		{
			for (int i = 0; i < 4; ++i)
			{
				if (m_ArrowRects[i].DoIntersect(position))
				{
					if (i == m_HitButton)
					{
						changed = true;
						if (i & 0x01) {
							value += m_SmallStep;
						} else {
							value -= m_SmallStep;
						}
					}
					break;
				}
			}
		}
		if (m_ArrowStates[m_HitButton])
		{
			m_RepeatTimer.Stop();
			m_ArrowStates[m_HitButton] = false;
		}
		Invalidate(m_ArrowRects[m_HitButton]);
		if (changed)
		{
			if (value < m_Min) {
				value = m_Min;
			} else if (value > m_Max) {
				value = m_Max;
			}
			SetValue(value);
		}
		Flush();
	}
	else if (m_HitState == HIT_KNOB)
	{
		if (m_Changed) {
			SignalValueChanged(GetValue(), true, ptr_tmp_cast(this)); // Send a 'final' event.
			m_Changed = false;
		}
	}

	m_HitState = HIT_NONE;
	MakeFocus(button, false);

	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollBar::OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
	if (m_HitState == HIT_ARROW)
	{
		int i;
		for (i = 0; i < 4; ++i)
		{
			if (m_ArrowRects[i].DoIntersect(position))
			{
				break;
			}
		}
		const bool bHit = m_HitButton == i;
		if (bHit != m_ArrowStates[m_HitButton])
		{
			if (m_ArrowStates[m_HitButton])
			{
                m_RepeatTimer.Stop();
			}
			else
			{
				if (!m_RepeatTimer.IsRunning())
				{
					m_RepeatTimer.Set(TimeValNanos::FromMilliseconds(300));
					GetLooper()->AddTimer(&m_RepeatTimer, true);
				}
			}
			m_ArrowStates[m_HitButton] = bHit;
			Invalidate(m_ArrowRects[m_HitButton]);
			Flush();
		}

	}
	else if (m_HitState == HIT_KNOB)
	{
		SetValue(PosToVal(position));
	}
	return true;
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

void PScrollBar::OnPaint(const PRect& cUpdateRect)
{
	PRect cBounds = GetBounds();
	PRect cKnobFrame = GetKnobFrame();

	//  SetEraseColor( get_default_color( COL_SCROLLBAR_BG ) );

	//    DrawFrame( cBounds, FRAME_RECESSED | FRAME_TRANSPARENT | FRAME_THIN );

	SetEraseColor(PStandardColorID::ScrollBarKnob);
	DrawFrame(cKnobFrame, FRAME_RAISED);

	if (m_Orientation == POrientation::Horizontal)
	{
		if (cKnobFrame.Width() > 30.0f)
		{
			const float vDist = 5.0f;
			float vCenter = cKnobFrame.left + (cKnobFrame.Width() + 1.0f) * 0.5f;
			SetFgColor(PStandardColorID::Shine);
			DrawLine(PPoint(vCenter - vDist, cKnobFrame.top + 4.0f), PPoint(vCenter - vDist, cKnobFrame.bottom - 4.0f));
			DrawLine(PPoint(vCenter, cKnobFrame.top + 4.0f), PPoint(vCenter, cKnobFrame.bottom - 4.0f));
			DrawLine(PPoint(vCenter + vDist, cKnobFrame.top + 4.0f), PPoint(vCenter + vDist, cKnobFrame.bottom - 4.0f));

			SetFgColor(PStandardColorID::Shadow);
			vCenter -= 1.0f;
			DrawLine(PPoint(vCenter - vDist, cKnobFrame.top + 4.0f), PPoint(vCenter - vDist, cKnobFrame.bottom - 4.0f));
			DrawLine(PPoint(vCenter, cKnobFrame.top + 4.0f), PPoint(vCenter, cKnobFrame.bottom - 4.0f));
			DrawLine(PPoint(vCenter + vDist, cKnobFrame.top + 4.0f), PPoint(vCenter + vDist, cKnobFrame.bottom - 4.0f));
		}
	}
	else
	{
		if (cKnobFrame.Height() > 30.0f)
		{
			const float vDist = 5.0f;
			float vCenter = cKnobFrame.top + (cKnobFrame.Height() + 1.0f) * 0.5f;
			SetFgColor(PStandardColorID::Shine);
			DrawLine(PPoint(cKnobFrame.left + 4.0f, vCenter - vDist), PPoint(cKnobFrame.right - 4.0f, vCenter - vDist));
			DrawLine(PPoint(cKnobFrame.left + 4.0f, vCenter), PPoint(cKnobFrame.right - 4.0f, vCenter));
			DrawLine(PPoint(cKnobFrame.left + 4.0f, vCenter + vDist), PPoint(cKnobFrame.right - 4.0f, vCenter + vDist));

			SetFgColor(PStandardColorID::Shadow);
			vCenter -= 1.0f;
			DrawLine(PPoint(cKnobFrame.left + 4.0f, vCenter - vDist), PPoint(cKnobFrame.right - 4.0f, vCenter - vDist));
			DrawLine(PPoint(cKnobFrame.left + 4.0f, vCenter), PPoint(cKnobFrame.right - 4.0f, vCenter));
			DrawLine(PPoint(cKnobFrame.left + 4.0f, vCenter + vDist), PPoint(cKnobFrame.right - 4.0f, vCenter + vDist));
		}
	}

	SetFgColor(PStandardColorID::ScrollBarBackground);

	cBounds.left += 1;
	cBounds.top += 1;
	cBounds.right -= 1;
	cBounds.bottom -= 1;

	PRect cFrame(m_KnobArea);
	if (m_Orientation == POrientation::Horizontal) {
		cFrame.right = cKnobFrame.left - 1.0f;
	} else {
		cFrame.bottom = cKnobFrame.top - 1.0f;
	}
	FillRect(cFrame);

	cFrame = m_KnobArea;
	if (m_Orientation == POrientation::Horizontal) {
		cFrame.left = cKnobFrame.right + 1.0f;
	} else {
		cFrame.top = cKnobFrame.bottom + 1.0f;
	}
	FillRect(cFrame);
	if (m_Orientation == POrientation::Horizontal)
	{
		SetFgColor(Tint(pget_standard_color(PStandardColorID::Shadow), 0.5f));
		DrawLine(PPoint(m_KnobArea.left, m_KnobArea.top - 1.0f), PPoint(m_KnobArea.right, m_KnobArea.top - 1.0f));
		SetFgColor(Tint(pget_standard_color(PStandardColorID::Shine), 0.6f));
		DrawLine(PPoint(m_KnobArea.left, m_KnobArea.bottom + 1.0f), PPoint(m_KnobArea.right, m_KnobArea.bottom + 1.0f));
	}
	else
	{
		SetFgColor(Tint(pget_standard_color(PStandardColorID::Shadow), 0.5f));
		DrawLine(PPoint(m_KnobArea.left - 1.0f, m_KnobArea.top), PPoint(m_KnobArea.left - 1.0f, m_KnobArea.bottom));
		SetFgColor(Tint(pget_standard_color(PStandardColorID::Shine), 0.6f));
		DrawLine(PPoint(m_KnobArea.right + 1.0f, m_KnobArea.top), PPoint(m_KnobArea.right + 1.0f, m_KnobArea.bottom));
	}
	for (int i = 0; i < 4; ++i)
	{
		if (m_ArrowRects[i].IsValid())
		{
//			Rect cBmRect;
//			Bitmap* pcBitmap;
//
//			if (m_Orientation == Orientation::Horizontal) {
//				pcBitmap = get_std_bitmap(((i & 0x01) ? BMID_ARROW_RIGHT : BMID_ARROW_LEFT), 0, &cBmRect);
//			} else {
//				pcBitmap = get_std_bitmap(((i & 0x01) ? BMID_ARROW_DOWN : BMID_ARROW_UP), 0, &cBmRect);
//			}
			DrawFrame(m_ArrowRects[i], (m_ArrowStates[i] ? FRAME_RECESSED : FRAME_RAISED) | FRAME_THIN);

			SetDrawingMode(PDrawingMode::Overlay);

//			DrawBitmap(pcBitmap, cBmRect, cBmRect.Bounds() + Point((m_acArrowRects[i].Width() + 1.0f) * 0.5 - (cBmRect.Width() + 1.0f) * 0.5,
//				(m_acArrowRects[i].Height() + 1.0f) * 0.5 - (cBmRect.Height() + 1.0f) * 0.5) +
//				m_acArrowRects[i].LeftTop());

			SetDrawingMode(PDrawingMode::Copy);

		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PScrollBar::PosToVal(PPoint pos) const
{
	float value = m_Min;

	pos -= m_HitPos;

	if (m_Orientation == POrientation::Horizontal)
	{
		float	size   = m_KnobArea.Width() * m_Proportion;
		float	length = m_KnobArea.Width() - size;

		if (length > 0.0f) {
			float relativePos = (pos.x - m_KnobArea.left) / length;
			value = m_Min + (m_Max - m_Min) * relativePos;
		}
	} else {
		float size   = m_KnobArea.Height() * m_Proportion;
		float length = m_KnobArea.Height() - size;

		if (length > 0.0f) {
			float relativePos = (pos.y - m_KnobArea.top) / length;
			value = m_Min + (m_Max - m_Min) * relativePos;
		}
	}
	return std::clamp(value, m_Min, m_Max);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollBar::OnFrameSized(const PPoint& delta)
{
	const PRect bounds = GetBounds();
	PRect arrowRect = bounds;

	m_KnobArea = bounds;
	if (m_Orientation == POrientation::Horizontal)
	{
		arrowRect.right = std::ceil(arrowRect.Height() * 0.7f);
		const float width = arrowRect.Width();

		m_ArrowRects[0] = arrowRect;
		m_ArrowRects[1] = arrowRect + PPoint(width, 0.0f);
		m_ArrowRects[2] = arrowRect + PPoint(bounds.Width() - width * 2.0f, 0.0f);
		m_ArrowRects[3] = arrowRect + PPoint(bounds.Width() - width, 0.0f);

		if (m_ArrowRects[0].right > m_ArrowRects[3].left)
        {
			m_ArrowRects[0].right = std::floor(bounds.Width() * 0.5f);
			m_ArrowRects[3].left  = m_ArrowRects[0].right;
			m_ArrowRects[1].left  = m_ArrowRects[0].right;
			m_ArrowRects[1].right = m_ArrowRects[1].left;
			m_ArrowRects[2].right = m_ArrowRects[3].left;
			m_ArrowRects[2].left  = m_ArrowRects[2].right;
		}
        else if (m_ArrowRects[1].right + 15.0f > m_ArrowRects[2].left)
        {
			m_ArrowRects[1].right = m_ArrowRects[1].left;
			m_ArrowRects[2].left = m_ArrowRects[2].right;
		}
		m_KnobArea.left = m_ArrowRects[1].right;
		m_KnobArea.right = m_ArrowRects[2].left;
		m_KnobArea.top += 1.0f;
		m_KnobArea.bottom -= 1.0f;
	}
    else
    {
		arrowRect.bottom = std::ceil(arrowRect.Width() * 0.7f);

		const float height = arrowRect.Height();

		m_ArrowRects[0] = arrowRect;
		m_ArrowRects[1] = arrowRect + PPoint(0.0f, height);
		m_ArrowRects[2] = arrowRect + PPoint(0.0f, bounds.Height() - height * 2.0f);
		m_ArrowRects[3] = arrowRect + PPoint(0.0f, bounds.Height() - height);

		if (m_ArrowRects[0].bottom > m_ArrowRects[3].top) {
			m_ArrowRects[0].bottom = std::floor(bounds.Width() * 0.5f);
			m_ArrowRects[3].top = m_ArrowRects[0].bottom;
			m_ArrowRects[1].top = m_ArrowRects[0].bottom;
			m_ArrowRects[1].bottom = m_ArrowRects[1].top;
			m_ArrowRects[2].bottom = m_ArrowRects[3].top;
			m_ArrowRects[2].top = m_ArrowRects[2].bottom;
		} else if (m_ArrowRects[1].bottom + 15.0f > m_ArrowRects[2].top) {
			m_ArrowRects[1].bottom = m_ArrowRects[1].top;
			m_ArrowRects[2].top = m_ArrowRects[2].bottom;
		}
		m_KnobArea.top = m_ArrowRects[1].bottom;
		m_KnobArea.bottom = m_ArrowRects[2].top;
		m_KnobArea.left += 1.0f;
		m_KnobArea.right -= 1.0f;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PScrollBar::GetKnobFrame(void) const
{
	PRect bounds = GetBounds();

    bounds.Resize(1.0f, 1.0f, -1.0f, -1.0f);

    PRect rect = m_KnobArea;

	float value = std::clamp(GetValue(), m_Min, m_Max);

    if (m_Orientation == POrientation::Horizontal)
	{
		float viewLength = m_KnobArea.Width() + 1.0f;
		float size = std::max(float(SB_MINSIZE), viewLength * m_Proportion);
		float length = viewLength - size;
		float delta = m_Max - m_Min;

		if (delta > 0.0f)
		{
			rect.left = rect.left + length * (value - m_Min) / delta;
			rect.right = rect.left + size - 1.0f;
		}
	}
	else
	{
		float viewLength = m_KnobArea.Height() + 1.0f;
		float size = std::max(float(SB_MINSIZE), viewLength * m_Proportion);
		float length = viewLength - size;
		float delta = m_Max - m_Min;

		if (delta > 0.0f) {
			rect.top = rect.top + length * (value - m_Min) / delta;
			rect.bottom = rect.top + size - 1.0f;
		}
	}
	return rect & m_KnobArea;
}

