// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
#include <cmath>

namespace os
{

class IPoint;

/**
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Point
{
public:
    float x;
    float y;

    constexpr Point() : x(0.0f), y(0.0f) {}
    constexpr Point(const Point& other) : x(other.x), y(other.y) {}
    constexpr explicit inline Point(const IPoint& other);
    constexpr explicit Point(float value) : x(value), y(value) {}
    constexpr Point(float X, float Y) : x(X), y(Y) {}

    float LengthSqr() const { return x * x + y * y; }
    float Length() const { return sqrtf(LengthSqr()); }

    Point GetNormalized() const { return *this * (1.0f / Length()); }
    Point& Normalize() { return *this *= (1.0f / Length()); }

    Point   GetRounded() const { return Point(roundf(x), roundf(y)); }
    Point&  Round()            { x = roundf(x); y = roundf(y); return *this; }

    Point        operator-(void) const { return(Point(-x, -y)); }
    Point        operator+(const Point& cPoint) const { return(Point(x + cPoint.x, y + cPoint.y)); }
    Point        operator-(const Point& cPoint) const { return(Point(x - cPoint.x, y - cPoint.y)); }
    Point&       operator+=(const Point& cPoint) { x += cPoint.x; y += cPoint.y; return *this; }
    Point&       operator-=(const Point& cPoint) { x -= cPoint.x; y -= cPoint.y; return *this; }

    Point        operator*(const Point& rhs) const  { return Point(x * rhs.x, y * rhs.y);   }
    Point        operator*(float rhs) const         { return Point(x * rhs, y * rhs);       }
    
    Point        operator/(const Point& rhs) const  { return Point(x / rhs.x, y / rhs.y);   }
    Point        operator/(float rhs) const         { return Point(x / rhs, y / rhs);       }

    Point&       operator*=(const Point& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
    Point&       operator*=(float rhs)        { x *= rhs; y *= rhs; return *this; }

    Point&       operator/=(const Point& rhs) { x /= rhs.x; y /= rhs.y; return *this; }
    Point&       operator/=(float rhs)        { x /= rhs; y /= rhs; return *this; }

    bool         operator<(const Point& cPoint) const { return(y < cPoint.y || (y == cPoint.y && x < cPoint.x)); }
    bool         operator>(const Point& cPoint) const { return(y > cPoint.y || (y == cPoint.y && x > cPoint.x)); }
    bool         operator==(const Point& cPoint) const { return(y == cPoint.y && x == cPoint.x); }
    bool         operator!=(const Point& cPoint) const { return(y != cPoint.y || x != cPoint.x); }
};

/**
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class IPoint
{
public:
    int x;
    int y;

    constexpr IPoint() : x(0), y(0) {}
    constexpr IPoint(const IPoint& other) : x(other.x), y(other.y) {}
    constexpr explicit inline IPoint(const Point& other);
    constexpr explicit IPoint(int value) : x(value), y(value) {}
    constexpr IPoint(int X, int Y) : x(X), y(Y) {}

    IPoint        operator-(void) const { return(IPoint(-x, -y)); }
    IPoint        operator+(const IPoint& cPoint) const { return(IPoint(x + cPoint.x, y + cPoint.y)); }
    IPoint        operator-(const IPoint& cPoint) const { return(IPoint(x - cPoint.x, y - cPoint.y)); }
    const IPoint& operator+=(const IPoint& cPoint) { x += cPoint.x; y += cPoint.y; return(*this); }
    const IPoint& operator-=(const IPoint& cPoint) { x -= cPoint.x; y -= cPoint.y; return(*this); }
    bool          operator<(const IPoint& cPoint) const { return(y < cPoint.y || (y == cPoint.y && x < cPoint.x)); }
    bool          operator>(const IPoint& cPoint) const { return(y > cPoint.y || (y == cPoint.y && x > cPoint.x)); }
    bool          operator==(const IPoint& cPoint) const { return(y == cPoint.y && x == cPoint.x); }
    bool          operator!=(const IPoint& cPoint) const { return(y != cPoint.y || x != cPoint.x); }
};


constexpr Point::Point(const IPoint& other) : x(float(other.x)), y(float(other.y)) {}
constexpr IPoint::IPoint(const Point& other) : x(int(other.x)), y(int(other.y)) {}

} // namespace os
