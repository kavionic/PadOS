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

#pragma once

#include <limits.h>

#include "GUI/Control.h"
#include "Threads/EventTimer.h"

namespace os
{

enum	{ SB_MINSIZE = 12 };

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class ScrollBar : public Control
{
public:
    ScrollBar(const String& name, Ptr<View> parent = nullptr, float min = 0.0f, float max = std::numeric_limits<float>::max(), Orientation orientation = Orientation::Vertical, uint32_t flags = 0);
    ~ScrollBar();

    void  SetScrollTarget(Ptr<View> target);
    Ptr<View> GetScrollTarget();


    // From View
    virtual void    FrameSized(const Point& delta) override;

    virtual bool    OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;

//    virtual void	WheelMoved(const Point& cDelta);

    virtual void	Paint(const Rect& updateRect) override;

    virtual void    CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;

    void SlotTimerTick();

    // From ScrollBar:
    void    SetValue(float value);
    float   GetValue() const { return m_Value; }

    void	SetSteps(float small, float big);
    void	GetSteps(float* small, float* big) const;

    void	SetMinMax(float min, float max);

    void	SetProportion(float proportion);
    float	GetProportion() const;

    Signal<void, float, bool, Ptr<ScrollBar>>	SignalValueChanged;

private:
    enum { HIT_NONE, HIT_KNOB, HIT_ARROW };
    Rect	GetKnobFrame() const;
    float	PosToVal(Point pos) const;

    EventTimer m_RepeatTimer;

    View*       m_Target = nullptr;
    float       m_Min = 0.0f;
    float       m_Max = 1.0f;
    float       m_Value = 0.0f;
    float       m_Proportion = 0.1f;
    float       m_SmallStep = 1.0f;
    float       m_BigStep = 10.0f;
    Orientation m_Orientation = Orientation::Vertical;
    Rect        m_ArrowRects[4];
    bool        m_ArrowStates[4];
    Rect        m_KnobArea;
    bool        m_Changed = false;
    bool        m_FirstTick = true;
    Point       m_HitPos;
    int         m_HitButton = 0;
    int         m_HitState = HIT_NONE;
};

}
