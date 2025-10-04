// This file is part of PadOS.
//
// Copyright (C) 2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 01.12.2024 21:30

#pragma once

#include <limits>
#include <time.h>
#include <sys/pados_types.h>
#include <System/Sections.h>

template<typename T, uint64_t TICKS_PER_SECOND>
struct TimeValue
{
    // Constants:
    static constexpr T TicksPerSecond = TICKS_PER_SECOND;
    static constexpr T TicksPerMillisecond = TicksPerSecond / 1000;
    static constexpr T TicksPerMicrosecond = TicksPerMillisecond / 1000;
    static constexpr T TicksPerNanosecond = TicksPerMicrosecond / 1000;

    // Conversions from misc time domains to TimeValue:
    static constexpr TimeValue FromNative(T value) { return TimeValue(value); }

    static constexpr TimeValue FromSeconds(time_t value) { return TimeValue(value * TicksPerSecond); }
    static constexpr TimeValue FromSeconds(float value)  { return TimeValue(T(value * double(TicksPerSecond))); }
    static constexpr TimeValue FromSeconds(double value) { return TimeValue(T(value * double(TicksPerSecond))); }
    static constexpr TimeValue FromMilliseconds(T value) { return TimeValue(value * TicksPerMillisecond); }
    static constexpr TimeValue FromMicroseconds(T value)
    {
        if constexpr (TicksPerMicrosecond > 0) {
            return TimeValue(value * TicksPerMicrosecond);
        } else {
            return TimeValue(value / (1000000 / TicksPerSecond));
        }
    }
    static constexpr TimeValue FromNanoseconds(T value)
    {
        if constexpr (TicksPerNanosecond > 0) {
            return TimeValue(value * TicksPerNanosecond);
        } else {
            return TimeValue(value / (1000000000 / TicksPerSecond));
        }
    }
    static constexpr TimeValue FromTimespec(const timespec& time)
    {
        return FromSeconds(time.tv_sec) + FromNanoseconds(time.tv_nsec);
    }
    // Constructors:
    constexpr TimeValue() : m_Value(0) {}
    constexpr TimeValue(float value) : m_Value(T(value * double(TicksPerSecond))) {}
    constexpr TimeValue(double value) : m_Value(T(value * double(TicksPerSecond))) {}

    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    constexpr TimeValue(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& value)
    {
        if (value.IsInfinit()) {
            m_Value = infinit.m_Value;
            return;
        }
        if constexpr (value.TicksPerSecond > TicksPerSecond) {
            m_Value = value.AsNative() / (value.TicksPerSecond / TicksPerSecond);
        } else {
            m_Value = value.AsNative() * (TicksPerSecond / value.TicksPerSecond);
        }
    }

    // Compare operators:
    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    constexpr bool operator==(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs) const
    {
        if (IsInfinit() || rhs.IsInfinit()) {
            return IsInfinit() == rhs.IsInfinit();
        }
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            return m_Value * (rhs.TicksPerSecond / TicksPerSecond) == rhs.AsNative();
        } else {
            return m_Value == rhs.AsNative() * (TicksPerSecond / rhs.TicksPerSecond);
        }
    }
    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    constexpr bool operator!=(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs) const
    {
        if (IsInfinit() || rhs.IsInfinit()) {
            return IsInfinit() != rhs.IsInfinit();
        }
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            return m_Value * (rhs.TicksPerSecond / TicksPerSecond) != rhs.AsNative();
        } else {
            return m_Value != rhs.AsNative() * (TicksPerSecond / rhs.TicksPerSecond);
        }
    }
    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    constexpr bool operator>(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs) const
    {
        if (IsInfinit() || rhs.IsInfinit()) {
            return IsInfinit() > rhs.IsInfinit();
        }
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            return m_Value * (rhs.TicksPerSecond / TicksPerSecond) > rhs.AsNative();
        } else {
            return m_Value > rhs.AsNative() * (TicksPerSecond / rhs.TicksPerSecond);
        }
    }
    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    constexpr bool operator<(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs) const
    {
        if (IsInfinit() || rhs.IsInfinit()) {
            return IsInfinit() < rhs.IsInfinit();
        }
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            return m_Value * (rhs.TicksPerSecond / TicksPerSecond) < rhs.AsNative();
        } else {
            return m_Value < rhs.AsNative()* (TicksPerSecond / rhs.TicksPerSecond);
        }
    }
    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    constexpr bool operator>=(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs) const
    {
        if (IsInfinit() || rhs.IsInfinit()) {
            return IsInfinit() >= rhs.IsInfinit();
        }
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            return m_Value * (rhs.TicksPerSecond / TicksPerSecond) >= rhs.AsNative();
        } else {
            return m_Value >= rhs.AsNative() * (TicksPerSecond / rhs.TicksPerSecond);
        }
    }
    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    constexpr bool operator<=(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs) const
    {
        if (IsInfinit() || rhs.IsInfinit()) {
            return IsInfinit() <= rhs.IsInfinit();
        }
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            return m_Value * (rhs.TicksPerSecond / TicksPerSecond) <= rhs.AsNative();
        } else {
            return m_Value <= rhs.AsNative() * (TicksPerSecond / rhs.TicksPerSecond);
        }
    }

    // Arithmetic operators:
    IFLASHC TimeValue& operator=(const TimeValue& value) = default;

    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    TimeValue& operator+=(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs)
    {
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            m_Value += rhs.AsNative() / (rhs.TicksPerSecond / TicksPerSecond);
        } else {
            m_Value += rhs.AsNative() * (TicksPerSecond / rhs.TicksPerSecond);
        }
        return *this;
    }
    template<typename VALUE_T, uint64_t VALUE_TICKS_PER_SECOND>
    TimeValue& operator-=(const TimeValue<VALUE_T, VALUE_TICKS_PER_SECOND>& rhs)
    {
        if constexpr (rhs.TicksPerSecond > TicksPerSecond) {
            m_Value -= rhs.AsNative() / (rhs.TicksPerSecond / TicksPerSecond);
        } else {
            m_Value -= rhs.AsNative() * (TicksPerSecond / rhs.TicksPerSecond);
        }
        return *this;
    }

    constexpr inline TimeValue operator+(float rhs) const { return TimeValue(m_Value + FromSeconds(rhs).AsNative()); }
    constexpr inline TimeValue operator-(float rhs) const { return TimeValue(m_Value - FromSeconds(rhs).AsNative()); }
    inline TimeValue& operator+=(float rhs) { m_Value += FromSeconds(rhs).AsNative(); return *this; }
    inline TimeValue& operator-=(float rhs) { m_Value -= FromSeconds(rhs).AsNative(); return *this; }

    constexpr inline TimeValue operator+(double rhs) const { return TimeValue(m_Value + FromSeconds(rhs).AsNative()); }
    constexpr inline TimeValue operator-(double rhs) const { return TimeValue(m_Value - FromSeconds(rhs).AsNative()); }
    inline TimeValue& operator+=(double rhs) { m_Value += FromSeconds(rhs).AsNative(); return *this; }
    inline TimeValue& operator-=(double rhs) { m_Value -= FromSeconds(rhs).AsNative(); return *this; }
    
    inline TimeValue operator-() const { return TimeValue::FromNative(-m_Value); }

    constexpr inline TimeValue operator*(float multiplier) const { return TimeValue::FromSeconds(AsSecondsF() * multiplier); }
    inline TimeValue& operator*=(float multiplier) { *this = (*this) * multiplier; return *this; }

    constexpr inline TimeValue operator*(double multiplier) const { return TimeValue::FromSeconds(AsSeconds() * multiplier); }
    inline TimeValue& operator*=(double multiplier) { *this = (*this) * multiplier; return *this; }

    constexpr inline TimeValue operator*(T multiplier) const { return TimeValue(m_Value * multiplier); }
    inline TimeValue& operator*=(T multiplier) { m_Value *= multiplier; return *this; }

    // Getters for the different time domains:
    constexpr inline T AsMilliseconds() const { return m_Value / (TicksPerSecond / 1000); }
    constexpr inline T AsMicroseconds() const
    {
        if constexpr (TicksPerSecond >= 1000000) {
            return m_Value / (TicksPerSecond / 1000000);
        } else {
            return m_Value * (1000000 / TicksPerSecond);
        }
    }
    constexpr inline T AsNanoseconds() const
    {
        if constexpr (TicksPerSecond >= 1000000000) {
            return m_Value / (TicksPerSecond / 1000000000);
        } else {
            return m_Value * (1000000000 / TicksPerSecond);
        }
    }
    constexpr inline double      AsSeconds() const { return double(m_Value) / double(TicksPerSecond); }
    constexpr inline float       AsSecondsF() const { return float(AsSeconds()); }
    constexpr inline bigtime_t   AsSecondsI() const { return m_Value / TicksPerSecond; }
    constexpr inline timespec    AsTimespec() const { return { .tv_sec = m_Value / TicksPerSecond, .tv_nsec = long(AsNanoseconds() % 1000000000) }; }

    constexpr inline T AsNative() const { return m_Value; }

    // Misc:
    constexpr inline bool IsZero() const { return m_Value == 0; }
    constexpr inline bool IsInfinit() const { return m_Value == infinit.m_Value; }

    static constexpr TimeValue zero = FromNative(0);
    static constexpr TimeValue infinit = FromNative(std::numeric_limits<T>::max());

private:
    explicit constexpr TimeValue(T value) : m_Value(value) {}

    T m_Value;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename LHS_T, uint64_t LHS_TICKS_PER_SECOND, typename RHS_T, uint64_t RHS_TICKS_PER_SECOND>
TimeValue<LHS_T, LHS_TICKS_PER_SECOND> operator+(const TimeValue<LHS_T, LHS_TICKS_PER_SECOND>& lhs, const TimeValue<RHS_T, RHS_TICKS_PER_SECOND>& rhs)
{
    if constexpr (rhs.TicksPerSecond > lhs.TicksPerSecond) {
        return TimeValue<LHS_T, LHS_TICKS_PER_SECOND>::FromNative(lhs.AsNative() + rhs.AsNative() / (rhs.TicksPerSecond / lhs.TicksPerSecond));
    } else {
        return TimeValue<LHS_T, LHS_TICKS_PER_SECOND>::FromNative(lhs.AsNative() + rhs.AsNative() * (lhs.TicksPerSecond / rhs.TicksPerSecond));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename LHS_T, uint64_t LHS_TICKS_PER_SECOND, typename RHS_T, uint64_t RHS_TICKS_PER_SECOND>
TimeValue<LHS_T, LHS_TICKS_PER_SECOND> operator-(const TimeValue<LHS_T, LHS_TICKS_PER_SECOND>& lhs, const TimeValue<RHS_T, RHS_TICKS_PER_SECOND>& rhs)
{
    if constexpr (rhs.TicksPerSecond > lhs.TicksPerSecond) {
        return TimeValue<LHS_T, LHS_TICKS_PER_SECOND>::FromNative(lhs.AsNative() - rhs.AsNative() / (rhs.TicksPerSecond / lhs.TicksPerSecond));
    } else {
        return TimeValue<LHS_T, LHS_TICKS_PER_SECOND>::FromNative(lhs.AsNative() - rhs.AsNative() * (lhs.TicksPerSecond / rhs.TicksPerSecond));
    }
}

using TimeValMillis = TimeValue<bigtime_t, 1000LL>;
using TimeValMicros = TimeValue<bigtime_t, 1000000LL>;
using TimeValNanos  = TimeValue<bigtime_t, 1000000000LL>;

