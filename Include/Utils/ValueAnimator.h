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
// Created: 08.09.2020 22:30

#pragma once


template<typename VALUE_TYPE, typename EASING_TYPE>
class PValueAnimator
{
public:
    template<typename... EASE_ARGS>
    PValueAnimator(EASE_ARGS&& ...easeArgs) : m_EasingCurve(std::forward<EASE_ARGS>(easeArgs)...) {}

    void            Start() { m_StartTime = get_monotonic_time(); }
    void            Reverse()
    {
        TimeValNanos curTime = get_monotonic_time();

        std::swap(m_StartValue, m_EndValue);

        if (m_StartTime.AsNative() != 0)
        {
            TimeValNanos currentRuntime = curTime - m_StartTime;
            if (currentRuntime < m_Period) {
                m_StartTime = curTime - m_Period + currentRuntime;
            }
            else {
                m_StartTime = curTime - m_Period;
            }
        }
    }
    bool            IsRunning() { return m_StartTime.AsNative() != 0 && (get_monotonic_time() - m_StartTime) < m_Period; }
    void            SetPeriod(TimeValNanos period) { m_Period = period; }
    TimeValNanos    GetPeriod() const { return m_Period; }

    void        SetRange(const VALUE_TYPE& startValue, const VALUE_TYPE& endValue) { m_StartValue = startValue; m_EndValue = endValue; }

    void        SetStartValue(const VALUE_TYPE& value) { m_StartValue = value; }
    VALUE_TYPE  GetStartValue() const { return m_StartValue; }

    void        SetEndValue(const VALUE_TYPE& value) { m_EndValue = value; }
    VALUE_TYPE  GetEndValue() const { return m_EndValue; }

    float GetProgress() const
    {
        if (m_StartTime.AsNative() == 0)
        {
            return 0.0f;
        }
        else
        {
            TimeValNanos curTime = get_monotonic_time();
            if ((curTime - m_StartTime) < m_Period) {
                return (curTime - m_StartTime).AsSecondsF() / m_Period.AsSecondsF();
            }
            else {
                return 1.0f;
            }
        }
    }

    VALUE_TYPE  GetValue(float progress) const
    {
        float easedProgress = m_EasingCurve.GetValue(progress);
        return m_StartValue + (m_EndValue - m_StartValue) * easedProgress;
    }

    VALUE_TYPE  GetValue() const
    {
        return GetValue(GetProgress());
    }
private:
    EASING_TYPE     m_EasingCurve;
    VALUE_TYPE      m_StartValue;
    VALUE_TYPE      m_EndValue;

    TimeValNanos    m_StartTime;
    TimeValNanos    m_Period;
};
