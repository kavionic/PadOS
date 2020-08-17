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

#include "GUI/ScrollBar.h"
#include "GUI/Color.h"
#include "Threads/Looper.h"

//#include "stdbitmaps.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static Color Tint(const Color& color, float tint)
{
    int r = int((float(color.GetRed()) * tint + 127.0f * (1.0f - tint)));
    int g = int((float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)));
    int b = int((float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)));
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    return Color(uint8_t(r), uint8_t(g), uint8_t(b), color.GetAlpha());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ScrollBar::ScrollBar(const String& name, Ptr<View> parent, float min, float max, Orientation orientation, uint32_t flags)
    : Control(name, parent, flags | ViewFlags::WillDraw)
{
    memset(m_ArrowStates, 0, sizeof(m_ArrowStates));
    m_Orientation  = orientation;
    m_Min          = min;
    m_Max          = max;
    FrameSized(Point(0.0f, 0.0f));

    m_RepeatTimer.SignalTrigged.Connect(this, &ScrollBar::SlotTimerTick);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ScrollBar::~ScrollBar()
{
    if (m_Target != nullptr)
    {
        if (m_Orientation == Orientation::Horizontal) {
            m_Target->SetHScrollBar(nullptr);
        } else {
            m_Target->SetVScrollBar(nullptr);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollBar::SetValue(float value)
{
    if (value == m_Value) {
        return;
    }
    m_Value = value;
    if (m_Target != nullptr)
    {
        Point pos = m_Target->GetScrollOffset();

        if (m_Orientation == Orientation::Horizontal) {
            pos.x = -floor(value);
        } else {
            pos.y = -floor(value);
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

void ScrollBar::SetProportion(float proportion)
{
    m_Proportion = proportion;
    Invalidate(m_KnobArea);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float ScrollBar::GetProportion() const
{
    return m_Proportion;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollBar::SetSteps(float small, float big)
{
    m_SmallStep = small;
    m_BigStep   = big;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollBar::GetSteps(float* small, float* big) const
{
    *small = m_SmallStep;
    *big   = m_BigStep;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollBar::SetMinMax(float min, float max)
{
    m_Min = min;
    m_Max = max;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollBar::SetScrollTarget(Ptr<View> target)
{
    if (m_Target != nullptr)
    {
        if (m_Orientation == Orientation::Horizontal) {
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
        if (m_Orientation == Orientation::Horizontal)
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

Ptr<View> ScrollBar::GetScrollTarget()
{
    return ptr_tmp_cast(m_Target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollBar::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    const float w = 16.0f;

    if (m_Orientation == Orientation::Horizontal) {
        *minSize = Point(0.0f, w);
        *maxSize = Point(COORD_MAX, w);
    } else {
        *minSize = Point(w, 0.0f);
        *maxSize = Point(w, COORD_MAX);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollBar::SlotTimerTick()
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
        m_RepeatTimer.Set(TimeValMicros::FromMilliseconds(30));
		GetLooper()->AddTimer(&m_RepeatTimer, false);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ScrollBar::OnMouseDown(MouseButton_e button, const Point& position)
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
            m_RepeatTimer.Set(TimeValMicros::FromMilliseconds(300));
            GetLooper()->AddTimer(&m_RepeatTimer, true);
			return true;
		}
	}
	if (m_KnobArea.DoIntersect(position))
	{
		Rect cKnobFrame = GetKnobFrame();
		if (cKnobFrame.DoIntersect(position)) {
			m_HitPos = position - cKnobFrame.TopLeft();
			m_HitState = HIT_KNOB;
			return true;
		}
		float vValue = GetValue();
		if (m_Orientation == Orientation::Horizontal)
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

bool ScrollBar::OnMouseUp(MouseButton_e button, const Point& position)
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

bool ScrollBar::OnMouseMove(MouseButton_e button, const Point& position)
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
					m_RepeatTimer.Set(TimeValMicros::FromMilliseconds(300));
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

void ScrollBar::Paint(const Rect& cUpdateRect)
{
	Rect cBounds = GetBounds();
	Rect cKnobFrame = GetKnobFrame();

	//  SetEraseColor( get_default_color( COL_SCROLLBAR_BG ) );

	//    DrawFrame( cBounds, FRAME_RECESSED | FRAME_TRANSPARENT | FRAME_THIN );

	SetEraseColor(StandardColorID::SCROLLBAR_KNOB);
	DrawFrame(cKnobFrame, FRAME_RAISED);

	if (m_Orientation == Orientation::Horizontal)
	{
		if (cKnobFrame.Width() > 30.0f)
		{
			const float vDist = 5.0f;
			float vCenter = cKnobFrame.left + (cKnobFrame.Width() + 1.0f) * 0.5f;
			SetFgColor(StandardColorID::SHINE);
			DrawLine(Point(vCenter - vDist, cKnobFrame.top + 4.0f), Point(vCenter - vDist, cKnobFrame.bottom - 4.0f));
			DrawLine(Point(vCenter, cKnobFrame.top + 4.0f), Point(vCenter, cKnobFrame.bottom - 4.0f));
			DrawLine(Point(vCenter + vDist, cKnobFrame.top + 4.0f), Point(vCenter + vDist, cKnobFrame.bottom - 4.0f));

			SetFgColor(StandardColorID::SHADOW);
			vCenter -= 1.0f;
			DrawLine(Point(vCenter - vDist, cKnobFrame.top + 4.0f), Point(vCenter - vDist, cKnobFrame.bottom - 4.0f));
			DrawLine(Point(vCenter, cKnobFrame.top + 4.0f), Point(vCenter, cKnobFrame.bottom - 4.0f));
			DrawLine(Point(vCenter + vDist, cKnobFrame.top + 4.0f), Point(vCenter + vDist, cKnobFrame.bottom - 4.0f));
		}
	}
	else
	{
		if (cKnobFrame.Height() > 30.0f)
		{
			const float vDist = 5.0f;
			float vCenter = cKnobFrame.top + (cKnobFrame.Height() + 1.0f) * 0.5f;
			SetFgColor(StandardColorID::SHINE);
			DrawLine(Point(cKnobFrame.left + 4.0f, vCenter - vDist), Point(cKnobFrame.right - 4.0f, vCenter - vDist));
			DrawLine(Point(cKnobFrame.left + 4.0f, vCenter), Point(cKnobFrame.right - 4.0f, vCenter));
			DrawLine(Point(cKnobFrame.left + 4.0f, vCenter + vDist), Point(cKnobFrame.right - 4.0f, vCenter + vDist));

			SetFgColor(StandardColorID::SHADOW);
			vCenter -= 1.0f;
			DrawLine(Point(cKnobFrame.left + 4.0f, vCenter - vDist), Point(cKnobFrame.right - 4.0f, vCenter - vDist));
			DrawLine(Point(cKnobFrame.left + 4.0f, vCenter), Point(cKnobFrame.right - 4.0f, vCenter));
			DrawLine(Point(cKnobFrame.left + 4.0f, vCenter + vDist), Point(cKnobFrame.right - 4.0f, vCenter + vDist));
		}
	}

	SetFgColor(StandardColorID::SCROLLBAR_BG);

	cBounds.left += 1;
	cBounds.top += 1;
	cBounds.right -= 1;
	cBounds.bottom -= 1;

	Rect cFrame(m_KnobArea);
	if (m_Orientation == Orientation::Horizontal) {
		cFrame.right = cKnobFrame.left - 1.0f;
	} else {
		cFrame.bottom = cKnobFrame.top - 1.0f;
	}
	FillRect(cFrame);

	cFrame = m_KnobArea;
	if (m_Orientation == Orientation::Horizontal) {
		cFrame.left = cKnobFrame.right + 1.0f;
	} else {
		cFrame.top = cKnobFrame.bottom + 1.0f;
	}
	FillRect(cFrame);
	if (m_Orientation == Orientation::Horizontal)
	{
		SetFgColor(Tint(get_standard_color(StandardColorID::SHADOW), 0.5f));
		DrawLine(Point(m_KnobArea.left, m_KnobArea.top - 1.0f), Point(m_KnobArea.right, m_KnobArea.top - 1.0f));
		SetFgColor(Tint(get_standard_color(StandardColorID::SHINE), 0.6f));
		DrawLine(Point(m_KnobArea.left, m_KnobArea.bottom + 1.0f), Point(m_KnobArea.right, m_KnobArea.bottom + 1.0f));
	}
	else
	{
		SetFgColor(Tint(get_standard_color(StandardColorID::SHADOW), 0.5f));
		DrawLine(Point(m_KnobArea.left - 1.0f, m_KnobArea.top), Point(m_KnobArea.left - 1.0f, m_KnobArea.bottom));
		SetFgColor(Tint(get_standard_color(StandardColorID::SHINE), 0.6f));
		DrawLine(Point(m_KnobArea.right + 1.0f, m_KnobArea.top), Point(m_KnobArea.right + 1.0f, m_KnobArea.bottom));
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

			SetDrawingMode(DM_OVER);

//			DrawBitmap(pcBitmap, cBmRect, cBmRect.Bounds() + Point((m_acArrowRects[i].Width() + 1.0f) * 0.5 - (cBmRect.Width() + 1.0f) * 0.5,
//				(m_acArrowRects[i].Height() + 1.0f) * 0.5 - (cBmRect.Height() + 1.0f) * 0.5) +
//				m_acArrowRects[i].LeftTop());

			SetDrawingMode(DM_COPY);

		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float ScrollBar::PosToVal(Point pos) const
{
	float value = m_Min;

	pos -= m_HitPos;

	if (m_Orientation == Orientation::Horizontal)
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

void ScrollBar::FrameSized(const Point& delta)
{
	const Rect bounds = GetBounds();
	Rect arrowRect = bounds;

	m_KnobArea = bounds;
	if (m_Orientation == Orientation::Horizontal)
	{
		arrowRect.right = ceil(arrowRect.Height() * 0.7f);
		const float width = arrowRect.Width();

		m_ArrowRects[0] = arrowRect;
		m_ArrowRects[1] = arrowRect + Point(width, 0.0f);
		m_ArrowRects[2] = arrowRect + Point(bounds.Width() - width * 2.0f, 0.0f);
		m_ArrowRects[3] = arrowRect + Point(bounds.Width() - width, 0.0f);

		if (m_ArrowRects[0].right > m_ArrowRects[3].left)
        {
			m_ArrowRects[0].right = floor(bounds.Width() * 0.5f);
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
		arrowRect.bottom = ceil(arrowRect.Width() * 0.7f);

		const float height = arrowRect.Height();

		m_ArrowRects[0] = arrowRect;
		m_ArrowRects[1] = arrowRect + Point(0.0f, height);
		m_ArrowRects[2] = arrowRect + Point(0.0f, bounds.Height() - height * 2.0f);
		m_ArrowRects[3] = arrowRect + Point(0.0f, bounds.Height() - height);

		if (m_ArrowRects[0].bottom > m_ArrowRects[3].top) {
			m_ArrowRects[0].bottom = floor(bounds.Width() * 0.5f);
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

Rect ScrollBar::GetKnobFrame(void) const
{
	Rect bounds = GetBounds();

    bounds.Resize(1.0f, 1.0f, -1.0f, -1.0f);

    Rect rect = m_KnobArea;

	float value = std::clamp(GetValue(), m_Min, m_Max);

    if (m_Orientation == Orientation::Horizontal)
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

