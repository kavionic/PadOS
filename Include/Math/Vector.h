// This file is part of PadOS.
//
// Copyright (C) 2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 15.05.2021 17:00

#pragma once

#include <cmath>

#include <Math/Misc.h>


template<typename T, int N>
class PVectorN
{
public:
    static constexpr size_t AxisCount = N;

    template<typename ...ARGS>
    PVectorN(ARGS ...args) : m_Axis{ args... }
    {
        for (size_t i = sizeof...(ARGS); i < AxisCount; ++i) m_Axis[i] = 0.0f;
    }
    PVectorN(const PVectorN&) = default;

    T LengthSqr() const
    {
        T length = 0.0f;
        for (size_t i = 0; i < AxisCount; ++i) length += PMath::square(m_Axis[i]);
        return length;
    }
    T Length() const { return T(sqrt(double(LengthSqr()))); }

    PVectorN     GetNormalized() const { return *this * (1.0f / Length()); }
    PVectorN&    Normalize() { return *this *= (1.0f / Length()); }

    T& operator[](size_t index) { return m_Axis[index]; }
    const T& operator[](size_t index) const { return m_Axis[index]; }

    PVectorN      operator-(void) const
    {
        PVectorN result;
        for (size_t i = 0; i < AxisCount; ++i)
        {
            result.m_Axis[i] = -m_Axis[i];
        }
        return result;
    }
    PVectorN        operator+(const PVectorN& rhs) const
    {
        PVectorN result;
        for (size_t i = 0; i < AxisCount; ++i) {
            result.m_Axis[i] = m_Axis[i] + rhs.m_Axis[i];
        }
        return result;
    }
    PVectorN        operator-(const PVectorN& rhs) const
    {
        PVectorN result;
        for (size_t i = 0; i < AxisCount; ++i) {
            result.m_Axis[i] = m_Axis[i] - rhs.m_Axis[i];
        }
        return result;
    }

    PVectorN& operator+=(const PVectorN& rhs)
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            m_Axis[i] += rhs.m_Axis[i];
        }
        return *this;
    }
    PVectorN& operator-=(const PVectorN& rhs)
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            m_Axis[i] -= rhs.m_Axis[i];
        }
        return *this;
    }


    PVectorN operator*(const PVectorN& rhs) const
    {
        PVectorN result;
        for (size_t i = 0; i < AxisCount; ++i) {
            result.m_Axis[i] = m_Axis[i] * rhs.m_Axis[i];
        }
        return result;
    }
    PVectorN operator*(float rhs) const
    {
        PVectorN result;
        for (size_t i = 0; i < AxisCount; ++i) {
            result.m_Axis[i] = m_Axis[i] * rhs;
        }
        return result;
    }

    PVectorN operator/(const PVectorN& rhs) const
    {
        PVectorN result;
        for (size_t i = 0; i < AxisCount; ++i) {
            result.m_Axis[i] = m_Axis[i] / rhs.m_Axis[i];
        }
        return result;
    }
    PVectorN operator/(float rhs) const
    {
        PVectorN result;
        for (size_t i = 0; i < AxisCount; ++i) {
            result.m_Axis[i] = m_Axis[i] / rhs;
        }
        return result;
    }

    PVectorN& operator*=(const PVectorN& rhs)
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            m_Axis[i] *= rhs.m_Axis[i];
        }
        return *this;
    }
    PVectorN& operator*=(float rhs)
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            m_Axis[i] *= rhs;
        }
        return *this;
    }

    PVectorN& operator/=(const PVectorN& rhs)
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            m_Axis[i] /= rhs.m_Axis[i];
        }
        return *this;
    }

    PVectorN& operator/=(float rhs)
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            m_Axis[i] /= rhs;
        }
        return *this;
    }

    //    bool         operator<(const VectorN& rhs) const { return(y < rhs.y || (y == rhs.y && x < rhs.x)); }
    //    bool         operator>(const VectorN& rhs) const { return(y > rhs.y || (y == rhs.y && x > rhs.x)); }
    bool operator==(const PVectorN& rhs) const
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            if (m_Axis[i] != rhs.m_Axis[i]) return false;
        }
        return true;
    }
    bool operator!=(const PVectorN& rhs) const
    {
        for (size_t i = 0; i < AxisCount; ++i) {
            if (m_Axis[i] != rhs.m_Axis[i]) return true;
        }
        return false;
    }


private:
    T m_Axis[N];
};

using PVector2 = PVectorN<float, 2>;
using PVector3 = PVectorN<float, 3>;
using PVector4 = PVectorN<float, 4>;
