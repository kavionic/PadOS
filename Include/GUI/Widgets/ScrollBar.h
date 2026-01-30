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

#pragma once

#include <limits.h>

#include <GUI/Widgets/Control.h>
#include <Threads/EventTimer.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PScrollBar : public PControl
{
public:
    PScrollBar(const PString& name = PString::zero, Ptr<PView> parent = nullptr, float min = 0.0f, float max = std::numeric_limits<float>::max(), POrientation orientation = POrientation::Vertical, uint32_t flags = 0);
    ~PScrollBar();

    void  SetScrollTarget(Ptr<PView> target);
    Ptr<PView> GetScrollTarget();


    // From View
    virtual void    OnFrameSized(const PPoint& delta) override;

    virtual bool    OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;

//    virtual void	WheelMoved(const Point& cDelta);

    virtual void	OnPaint(const PRect& updateRect) override;

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    void SlotTimerTick();

    // From ScrollBar:
    void    SetValue(float value);
    float   GetValue() const { return m_Value; }

    void	SetSteps(float small, float big);
    void	GetSteps(float* small, float* big) const;

    void	SetMinMax(float min, float max);

    void	SetProportion(float proportion);
    float	GetProportion() const;

    Signal<void, float, bool, Ptr<PScrollBar>>	SignalValueChanged;

private:
    enum { HIT_NONE, HIT_KNOB, HIT_ARROW };
    PRect	GetKnobFrame() const;
    float	PosToVal(PPoint pos) const;

    PEventTimer m_RepeatTimer;

    PView*       m_Target = nullptr;
    float       m_Min = 0.0f;
    float       m_Max = 1.0f;
    float       m_Value = 0.0f;
    float       m_Proportion = 0.1f;
    float       m_SmallStep = 1.0f;
    float       m_BigStep = 10.0f;
    POrientation m_Orientation = POrientation::Vertical;
    PRect        m_ArrowRects[4];
    bool        m_ArrowStates[4];
    PRect        m_KnobArea;
    bool        m_Changed = false;
    bool        m_FirstTick = true;
    PPoint       m_HitPos;
    int         m_HitButton = 0;
    int         m_HitState = HIT_NONE;
};
