// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.08.2020 18:37

#pragma once

#include "Signals/SignalTarget.h"
#include "Threads/EventTimer.h"
#include "Math/Point.h"
#include "Math/Rect.h"
#include "GUI/GUIDefines.h"

namespace os
{


class InertialScroller : public SignalTarget
{
public:
    enum class State : uint8_t
    {
        Idle,
        Dragging,
        Coasting
    };

    InertialScroller(const Point& initialValue = Point(), float frameRate = 60.0f, int ticksPerUpdate = 2);

    State GetState() const { return m_State; }

    void SetScrollBounds(const Rect& bounds) { m_ScrollBounds = bounds; }
    void SetScrollBounds(float left, float top, float right, float bottom) { SetScrollBounds(Rect(left, top, right, bottom)); }
    void SetScrollHBounds(float left, float right) { m_ScrollBounds.left = left; m_ScrollBounds.right = right; }
    void SetScrollVBounds(float top, float bottom) { m_ScrollBounds.top = top; m_ScrollBounds.bottom = bottom; }
    Rect GetScrollBounds() const { return m_ScrollBounds; }

    void BeginDrag(const Point& scrollOffset, const Point& dragPosition, Looper* looper = nullptr);
    void EndDrag();
    void AddUpdate(const Point& value);

    Point GetValue() const;
    Point GetVelocity() const;
    Point GetClosestIndention(const Point& position) const;

    Signal<void, const Point&, InertialScroller*> SignalUpdate;

private:
    void SlotTick();

    static float    s_SpringConstant;
    static float    s_DefaultFriction;
    static float    s_MinSpeed;
    static float    s_OverscrollSlip;

    Point           m_DetentSpacing = Point(0.0f, 33.0f);
    Point           m_DetentAttraction = Point(20.0f, 20.0f);

    float           m_MaxVOverscroll = 40.0f;
    float           m_MaxHOverscroll = 0.0f;
    Rect            m_ScrollBounds = Rect(-COORD_MAX, -COORD_MAX, COORD_MAX, COORD_MAX);

    EventTimer      m_Timer;
    TimeValMicros   m_BeginDragTime;
    TimeValMicros   m_LastTickTime;

    int             m_TicksPerUpdate = 2;
    int             m_TicksSinceUpdate = 0;

    Point           m_Friction;

    Point           m_Velocity;
    Point           m_BeginDragPosition;
    Point           m_TargetPosition;
    Point           m_CurrentPosition;
    Point           m_StaticOffset;
    State           m_State = State::Idle;
};

} // namespace os
