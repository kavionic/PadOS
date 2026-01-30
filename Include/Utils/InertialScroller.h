// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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

#include <Signals/SignalTarget.h>
#include <Threads/EventTimer.h>
#include <Math/Point.h>
#include <Math/Rect.h>
#include <GUI/GUIDefines.h>


class PInertialScroller : public SignalTarget
{
public:
    enum class State : uint8_t
    {
        Idle,
        WaitForThreshold,
        Dragging,
        Coasting
    };

    PInertialScroller(const PPoint& initialValue = PPoint(), float frameRate = 60.0f, int ticksPerUpdate = 2);

    State GetState() const { return m_State; }

    void SetScrollBounds(const PRect& bounds) { m_ScrollBounds = bounds; }
    void SetScrollBounds(float left, float top, float right, float bottom) { SetScrollBounds(PRect(left, top, right, bottom)); }
    void SetScrollHBounds(float left, float right) { m_ScrollBounds.left = left; m_ScrollBounds.right = right; }
    void SetScrollVBounds(float top, float bottom) { m_ScrollBounds.top = top; m_ScrollBounds.bottom = bottom; }
    PRect GetScrollBounds() const { return m_ScrollBounds; }

    void  SetMaxHOverscroll(float value)    { m_MaxHOverscroll = value; }
    float GetMaxHOverscroll() const         { return m_MaxHOverscroll; }

    void  SetMaxVOverscroll(float value)    { m_MaxVOverscroll = value; }
    float GetMaxVOverscroll() const         { return m_MaxVOverscroll; }

    void  SetMaxOverscroll(float horizontal, float vertical)    { m_MaxHOverscroll = horizontal; m_MaxVOverscroll = vertical; }

    void    SetStartScrollThreshold(float threshold) { m_StartScrollThreshold = threshold; }
    float   GetStartScrollThreshold() const { return m_StartScrollThreshold; }

    void SetDetentSpacing(const PPoint& spacing) { m_DetentSpacing = spacing; }
    void SetDetentSpacing(float horizontal, float vertical) { SetDetentSpacing(PPoint(horizontal, vertical)); }
    PPoint GetDetentSpacing() const              { return m_DetentSpacing; }

    void SetFriction(float pixelsPerSecond) { s_DefaultFriction = pixelsPerSecond; }
    float GetFriction() const { return s_DefaultFriction; }

    void BeginDrag(const PPoint& scrollOffset, const PPoint& dragPosition, PLooper* looper = nullptr);
    void EndDrag();
    void AddUpdate(const PPoint& value, PLooper* looper = nullptr);

    PPoint GetValue() const;
    PPoint GetVelocity() const;
    PPoint GetClosestIndention(const PPoint& position) const;

    void ScrollTo(const PPoint& scrollOffset, const PPoint& velocity, PLooper* looper = nullptr);

    Signal<void, const PPoint&, PInertialScroller*> SignalUpdate;

private:
    void SlotTick();

    static float    s_SpringConstant;
    float    s_DefaultFriction = 1000.0f;
    static float    s_MinSpeed;
    static float    s_OverscrollSlip;

    PPoint           m_DetentSpacing = PPoint(0.0f, 0.0f);
    PPoint           m_DetentAttraction = PPoint(20.0f, 20.0f);

    float           m_MaxVOverscroll = 40.0f;
    float           m_MaxHOverscroll = 0.0f;
    PRect            m_ScrollBounds = PRect(-COORD_MAX, -COORD_MAX, COORD_MAX, COORD_MAX);

    float           m_StartScrollThreshold = 0.0f;

    PEventTimer      m_Timer;
    TimeValNanos    m_BeginDragTime;
    TimeValNanos    m_LastTickTime;

    int             m_TicksPerUpdate = 2;
    int             m_TicksSinceUpdate = 0;

    PPoint           m_Friction;

    PPoint           m_Velocity;
    PPoint           m_BeginDragPosition;
    PPoint           m_TargetPosition;
    PPoint           m_CurrentPosition;
    PPoint           m_StaticOffset;
    State           m_State = State::Idle;
};
