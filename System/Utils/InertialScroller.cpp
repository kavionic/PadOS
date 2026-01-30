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

#include <PadOS/Time.h>
#include <Math/Misc.h>
#include <Utils/InertialScroller.h>


float PInertialScroller::s_SpringConstant  = 20.0f;
float PInertialScroller::s_MinSpeed        = 40.0f;
float PInertialScroller::s_OverscrollSlip  = 0.25f;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PInertialScroller::PInertialScroller(const PPoint& initialValue, float frameRate, int ticksPerUpdate)
    : m_TicksPerUpdate(ticksPerUpdate)
    , m_TargetPosition(initialValue)
    , m_CurrentPosition(initialValue)
{
    m_LastTickTime = get_monotonic_time();

    m_Timer.Set(TimeValNanos::FromSeconds(1.0f / frameRate));
    m_Timer.SignalTrigged.Connect(this, &PInertialScroller::SlotTick);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PInertialScroller::BeginDrag(const PPoint& scrollOffset, const PPoint& dragPosition, PLooper* looper)
{
    m_BeginDragTime     = get_monotonic_time();
    
    m_BeginDragPosition      = scrollOffset;
    m_CurrentPosition     = scrollOffset;
    m_TargetPosition   = scrollOffset;

    m_StaticOffset      = dragPosition - scrollOffset;

    if (m_State == PInertialScroller::State::Idle)
    {
        if (m_StartScrollThreshold == 0.0f)
        {
            m_LastTickTime = m_BeginDragTime;
            m_Timer.Start(false, looper);
            m_State = PInertialScroller::State::Dragging;
        }
        else
        {
            m_State = PInertialScroller::State::WaitForThreshold;
        }
    }
    else
    {
        m_State = PInertialScroller::State::Dragging;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PInertialScroller::EndDrag()
{
    if (m_State == State::WaitForThreshold)
    {
        m_State = State::Idle;
        return;
    }

    const float deltaTime = (get_monotonic_time() - m_BeginDragTime).AsSecondsF();
    if (deltaTime > 0.0f && deltaTime < 0.2f) {
        m_Velocity = (m_TargetPosition - m_BeginDragPosition) / deltaTime;
    }
    m_TargetPosition = m_CurrentPosition;

    // Calculate stop time based on current velocity and default friction.
    const PPoint speed(std::abs(m_Velocity.x), std::abs(m_Velocity.y));
    const PPoint stopTime(std::max(s_MinSpeed, speed.x - s_MinSpeed) / s_DefaultFriction, std::max(s_MinSpeed, speed.y - s_MinSpeed) / s_DefaultFriction);

    // Calculate stopping distance based on current velocity and default friction.
    m_TargetPosition.x += std::copysign(std::abs(m_Velocity.x) * stopTime.x - s_DefaultFriction * stopTime.x * stopTime.x * 0.5f, m_Velocity.x);
    m_TargetPosition.y += std::copysign(std::abs(m_Velocity.y) * stopTime.y - s_DefaultFriction * stopTime.y * stopTime.y * 0.5f, m_Velocity.y);

    // Calculate distance between natural stopping point and nearest detention.
    m_TargetPosition = GetClosestIndention(m_TargetPosition);
    const PPoint targetDistance = m_TargetPosition - m_CurrentPosition;

    // Calculate friction needed to stop at the selected detention.
    if (targetDistance.x != 0.0f) {
        m_Friction.x = (m_Velocity.x * m_Velocity.x - s_MinSpeed * s_MinSpeed) / (std::abs(targetDistance.x) * 2.0f);
    } else {
        m_Friction.x = s_DefaultFriction;
        m_Velocity.x = 0.0f;
    }
    if (targetDistance.y != 0.0f) {
        m_Friction.y = (m_Velocity.y * m_Velocity.y - s_MinSpeed * s_MinSpeed) / (std::abs(targetDistance.y) * 2.0f);
    } else {
        m_Friction.y = s_DefaultFriction;
        m_Velocity.y = 0.0f;
    }
    m_State = PInertialScroller::State::Coasting;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PInertialScroller::AddUpdate(const PPoint& value, PLooper* looper)
{
    m_TargetPosition = value - m_StaticOffset;
    if (m_State == State::WaitForThreshold)
    {
        if ((value - (m_StaticOffset + m_BeginDragPosition)).LengthSqr() >= PMath::square(m_StartScrollThreshold))
        {
            m_LastTickTime = m_BeginDragTime;
            m_Timer.Start(false, looper);
            m_State = State::Dragging;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PInertialScroller::GetValue() const
{
    return m_CurrentPosition;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PInertialScroller::GetVelocity() const
{
    return m_Velocity;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PInertialScroller::GetClosestIndention(const PPoint& position) const
{
    PPoint nearest;

    nearest.x = (m_DetentSpacing.x > 1.0f) ? std::round(position.x / m_DetentSpacing.x) * m_DetentSpacing.x : position.x;
    nearest.y = (m_DetentSpacing.y > 1.0f) ? std::round(position.y / m_DetentSpacing.y) * m_DetentSpacing.y : position.y;

    return nearest;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PInertialScroller::SlotTick()
{
    const TimeValNanos curTime = get_monotonic_time();
    if (curTime == m_LastTickTime) {
        return;
    }
    const float deltaTime = (curTime - m_LastTickTime).AsSecondsF();

    m_LastTickTime = curTime;

    if (m_State == State::Dragging)
    {
        PPoint nearestIndent = GetClosestIndention(m_CurrentPosition);
        PPoint detentOffset = nearestIndent - m_CurrentPosition;
        PPoint detentPull((m_DetentSpacing.x != 0.0f) ? detentOffset.x / (m_DetentSpacing.x * 0.5f) : 0.0f,
                         (m_DetentSpacing.y != 0.0f) ? detentOffset.y / (m_DetentSpacing.y * 0.5f) : 0.0f);
        detentPull *= m_DetentAttraction;
        PPoint deltaMove = (m_TargetPosition - m_CurrentPosition + detentPull) * std::min(1.0f, deltaTime * s_SpringConstant);
        m_Velocity = deltaMove / deltaTime;
        m_CurrentPosition += deltaMove;
    }
    else
    {
        if (m_Velocity.x != 0.0f)
        {
            bool xWasNegative = m_Velocity.x < 0.0f;

            if (xWasNegative) m_Velocity.x *= -1.0f;

            m_Velocity.x -= m_Friction.x * deltaTime;

            if (m_Velocity.x < s_MinSpeed) m_Velocity.x = s_MinSpeed;
            if (xWasNegative) m_Velocity.x *= -1.0f;
        }

        if (m_Velocity.y != 0.0f)
        {
            const bool yWasNegative = m_Velocity.y < 0.0f;

            if (yWasNegative) m_Velocity.y = -m_Velocity.y;

            m_Velocity.y -= m_Friction.y * deltaTime;

            if (m_Velocity.y < s_MinSpeed) m_Velocity.y = s_MinSpeed;
            if (yWasNegative) m_Velocity.y = -m_Velocity.y;
        }
        bool xIdle = false;
        bool yIdle = false;

        m_CurrentPosition += m_Velocity * deltaTime;

        if ((m_Velocity.x < 0.0f) ? (m_CurrentPosition.x <= m_TargetPosition.x) : (m_CurrentPosition.x >= m_TargetPosition.x))
        {
            m_CurrentPosition.x = m_TargetPosition.x;
            m_Velocity.x = 0.0f;
            xIdle = true;
        }
        if ((m_Velocity.y < 0.0f) ? (m_CurrentPosition.y <= m_TargetPosition.y) : (m_CurrentPosition.y >= m_TargetPosition.y))
        {
            m_CurrentPosition.y = m_TargetPosition.y;
            m_Velocity.y = 0.0f;
            yIdle = true;
        }
        if (xIdle && yIdle) {
            m_State = PInertialScroller::State::Idle;
        }
    }
    m_TicksSinceUpdate++;
    if (m_State == PInertialScroller::State::Idle || m_TicksSinceUpdate >= m_TicksPerUpdate)
    {
        m_TicksSinceUpdate = 0;

        PPoint scrollOffset;

        if (m_State == State::Dragging)
        {
            scrollOffset = m_CurrentPosition;
            if (scrollOffset.x < m_ScrollBounds.left) {
                scrollOffset.x = m_ScrollBounds.left - std::min((m_ScrollBounds.left - scrollOffset.x) * s_OverscrollSlip, m_MaxHOverscroll);
            } else if (scrollOffset.x > m_ScrollBounds.right) {
                scrollOffset.x = m_ScrollBounds.right + std::min((scrollOffset.x - m_ScrollBounds.right) * s_OverscrollSlip, m_MaxHOverscroll);
            }
            if (scrollOffset.y < m_ScrollBounds.top) {
                scrollOffset.y = m_ScrollBounds.top - std::min((m_ScrollBounds.top - scrollOffset.y) * s_OverscrollSlip, m_MaxVOverscroll);
            } else if (scrollOffset.y > m_ScrollBounds.bottom) {
                scrollOffset.y = m_ScrollBounds.bottom + std::min((scrollOffset.y - m_ScrollBounds.bottom) * s_OverscrollSlip, m_MaxVOverscroll);
            }
        }
        else
        {
            scrollOffset.x = std::clamp(m_CurrentPosition.x, m_ScrollBounds.left, m_ScrollBounds.right);
            scrollOffset.y = std::clamp(m_CurrentPosition.y, m_ScrollBounds.top, m_ScrollBounds.bottom);
        }

        SignalUpdate(PPoint(std::round(scrollOffset.x), std::round(scrollOffset.y)), this);
    }
    if (m_State == PInertialScroller::State::Idle) {
        m_Timer.Stop();
    }
}

void PInertialScroller::ScrollTo(const PPoint& scrollOffset, const PPoint& velocity, PLooper* looper)
{
    const PPoint prevPosition = m_CurrentPosition;

    m_TargetPosition = scrollOffset;
    if (velocity.x == 0.0f) {
        m_CurrentPosition.x = m_TargetPosition.x;
    }
    if (velocity.y == 0.0f) {
        m_CurrentPosition.y = m_TargetPosition.y;
    }
    if (m_CurrentPosition != m_TargetPosition)
    {
        m_Velocity.x = std::copysign(std::abs(velocity.x), m_TargetPosition.x - m_CurrentPosition.x);
        m_Velocity.y = std::copysign(std::abs(velocity.y), m_TargetPosition.y - m_CurrentPosition.y);

        if (m_State == PInertialScroller::State::Idle)
        {
            m_LastTickTime = get_monotonic_time();
            m_Timer.Start(false, looper);
        }
        m_State = PInertialScroller::State::Coasting;
    }
    else
    {
        m_Velocity = PPoint(0.0f, 0.0f);
        m_State = PInertialScroller::State::Idle;
        m_Timer.Stop();
        if (m_CurrentPosition != prevPosition)
        {
            PPoint scrollOffset;
            scrollOffset.x = std::clamp(m_CurrentPosition.x, m_ScrollBounds.left, m_ScrollBounds.right);
            scrollOffset.y = std::clamp(m_CurrentPosition.y, m_ScrollBounds.top, m_ScrollBounds.bottom);
            SignalUpdate(PPoint(std::round(scrollOffset.x), std::round(scrollOffset.y)), this);
        }
    }
}
