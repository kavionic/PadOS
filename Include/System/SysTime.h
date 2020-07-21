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
// Created: 19.07.2020 20:19

#pragma once

#include "System/Types.h"


struct TimeValMicros
{
    static constexpr bigtime_t TicksPerSecond = 1000000LL;
    static constexpr bigtime_t TicksPerMillisecond = TicksPerSecond / 1000;
    static constexpr bigtime_t TicksPerMicrosecond = 1;

    static TimeValMicros FromSeconds(bigtime_t value)       { return TimeValMicros(value * TicksPerSecond); }
    static TimeValMicros FromSeconds(float value)           { return TimeValMicros(bigtime_t(value * float(TicksPerSecond))); }
    static TimeValMicros FromSeconds(double value)          { return TimeValMicros(bigtime_t(value * double(TicksPerSecond))); }
    static TimeValMicros FromMilliseconds(bigtime_t value)  { return TimeValMicros(value * TicksPerMillisecond); }
    static TimeValMicros FromNanoseconds(bigtime_t value)   { return TimeValMicros(value / 1000); }

    TimeValMicros() : m_Value(0) {}
    TimeValMicros(bigtime_t value) : m_Value(value) {}
    TimeValMicros& operator=(bigtime_t value) { m_Value = value; return *this; }
    TimeValMicros& operator=(const TimeValMicros& value) = default;

    TimeValMicros operator+(const TimeValMicros& rhs) const { return TimeValMicros(m_Value + rhs.m_Value); }
    TimeValMicros operator-(const TimeValMicros& rhs) const { return TimeValMicros(m_Value - rhs.m_Value); }
    TimeValMicros& operator+=(const TimeValMicros& rhs) { m_Value += rhs.m_Value; return *this; }
    TimeValMicros& operator-=(const TimeValMicros& rhs) { m_Value -= rhs.m_Value; return *this; }

    operator bigtime_t() const { return m_Value; }
    bigtime_t m_Value;
};

struct TimeValNanos
{
    static constexpr bigtime_t TicksPerSecond = 1000000000LL;
    static constexpr bigtime_t TicksPerMillisecond = TicksPerSecond / 1000;
    static constexpr bigtime_t TicksPerMicrosecond = TicksPerMillisecond / 1000;
    static constexpr bigtime_t TicksPerNanosecond = 1;

    static TimeValNanos FromSeconds(bigtime_t value)        { return TimeValNanos(value * TicksPerSecond); }
    static TimeValNanos FromSeconds(float value)            { return TimeValNanos(bigtime_t(value * double(TicksPerSecond))); }
    static TimeValNanos FromSeconds(double value)           { return TimeValNanos(bigtime_t(value * double(TicksPerSecond))); }
    static TimeValNanos FromMilliseconds(bigtime_t value)   { return TimeValNanos(value * TicksPerMillisecond); }
    static TimeValNanos FromMicroseconds(bigtime_t value)   { return TimeValNanos(value * TicksPerMicrosecond); }

    TimeValNanos() : m_Value(0) {}
    TimeValNanos(bigtime_t value) : m_Value(value) {}
    TimeValNanos& operator=(bigtime_t value) { m_Value = value; return *this; }
    TimeValNanos& operator=(const TimeValNanos& value) = default;

    TimeValNanos operator+(const TimeValNanos& rhs) const { return TimeValNanos(m_Value + rhs.m_Value); }
    TimeValNanos operator-(const TimeValNanos& rhs) const { return TimeValNanos(m_Value - rhs.m_Value); }
    TimeValNanos& operator+=(const TimeValNanos& rhs) { m_Value += rhs.m_Value; return *this; }
    TimeValNanos& operator-=(const TimeValNanos& rhs) { m_Value -= rhs.m_Value; return *this; }

    operator bigtime_t() const { return m_Value; }
    bigtime_t m_Value;
};

constexpr bigtime_t bigtime_from_s(bigtime_t s) { return s * 1000000; }
inline bigtime_t bigtime_from_sf(double s) { return bigtime_t(s * 1000000.0); }
inline bigtime_t bigtime_from_ms(bigtime_t ms) { return ms * 1000; }

bigtime_t get_system_time();
TimeValNanos get_system_time_hires();
TimeValNanos get_idle_time();
uint64_t    get_core_clock_cycles();

bigtime_t get_real_time();
