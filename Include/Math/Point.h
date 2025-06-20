// This file is part of PadOS.
//
// Copyright (C) 2018-2021 Kurt Skauen <http://kavionic.com/>
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
#include <Utils/String.h>

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

    constexpr Point() noexcept : x(0.0f), y(0.0f) {}
    constexpr Point(const Point& other) noexcept : x(other.x), y(other.y) {}
    constexpr explicit inline Point(const IPoint& other) noexcept;
    constexpr explicit Point(float value) noexcept : x(value), y(value) {}
    constexpr Point(float X, float Y) noexcept : x(X), y(Y) {}

    String ToString() const { return std::format("Point({}, {})", x, y); }
    static std::optional<Point> FromString(const char* string)
    {
        Point value;
        if (sscanf(string, "Point( %f , %f )", &value.x, &value.y) == 2) {
            return value;
        }
        return std::nullopt;
    }
    constexpr float LengthSqr() const noexcept { return x * x + y * y; }
    constexpr float Length() const noexcept { return sqrtf(LengthSqr()); }

    constexpr Point GetNormalized() const { return *this * (1.0f / Length()); }
    Point& Normalize() { return *this *= (1.0f / Length()); }

    constexpr Point GetRounded() const noexcept { return Point(roundf(x), roundf(y)); }
    Point&          Round() noexcept { x = roundf(x); y = roundf(y); return *this; }

    Point& operator=(const Point& rhs) noexcept = default;

    constexpr Point operator-(void) const noexcept { return Point(-x, -y); }
    constexpr Point operator+(const Point& rhs) const noexcept { return Point(x + rhs.x, y + rhs.y); }
    constexpr Point operator-(const Point& rhs) const noexcept { return Point(x - rhs.x, y - rhs.y); }
    Point&          operator+=(const Point& rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
    Point&          operator-=(const Point& rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }

    constexpr Point operator*(const Point& rhs) const noexcept  { return Point(x * rhs.x, y * rhs.y);   }
    constexpr Point operator*(float rhs) const noexcept         { return Point(x * rhs, y * rhs);       }
    
    constexpr Point operator/(const Point& rhs) const  { return Point(x / rhs.x, y / rhs.y);   }
    constexpr Point operator/(float rhs) const         { return Point(x / rhs, y / rhs);       }

    Point&          operator*=(const Point& rhs) noexcept   { x *= rhs.x; y *= rhs.y; return *this; }
    Point&          operator*=(float rhs) noexcept          { x *= rhs; y *= rhs; return *this; }

    Point&          operator/=(const Point& rhs) { x /= rhs.x; y /= rhs.y; return *this; }
    Point&          operator/=(float rhs)        { x /= rhs; y /= rhs; return *this; }

    constexpr bool  operator<(const Point& rhs) const noexcept  { return(y < rhs.y || (y == rhs.y && x < rhs.x)); }
    constexpr bool  operator>(const Point& rhs) const noexcept  { return(y > rhs.y || (y == rhs.y && x > rhs.x)); }
    constexpr bool  operator==(const Point& rhs) const noexcept { return(y == rhs.y && x == rhs.x); }
    constexpr bool  operator!=(const Point& rhs) const noexcept { return(y != rhs.y || x != rhs.x); }
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

    constexpr IPoint() noexcept : x(0), y(0) {}
    constexpr IPoint(const IPoint& other) noexcept : x(other.x), y(other.y) {}
    constexpr explicit inline IPoint(const Point& other) noexcept;
    constexpr explicit IPoint(int value) noexcept : x(value), y(value) {}
    constexpr IPoint(int X, int Y) noexcept : x(X), y(Y) {}

    IPoint& operator=(const IPoint&) = default;

    IPoint        operator-(void) const noexcept                { return(IPoint(-x, -y)); }
    IPoint        operator+(const IPoint& rhs) const noexcept   { return(IPoint(x + rhs.x, y + rhs.y)); }
    IPoint        operator-(const IPoint& rhs) const noexcept   { return(IPoint(x - rhs.x, y - rhs.y)); }
    const IPoint& operator+=(const IPoint& rhs) noexcept        { x += rhs.x; y += rhs.y; return(*this); }
    const IPoint& operator-=(const IPoint& rhs) noexcept        { x -= rhs.x; y -= rhs.y; return(*this); }
    bool          operator<(const IPoint& rhs) const noexcept   { return(y < rhs.y || (y == rhs.y && x < rhs.x)); }
    bool          operator>(const IPoint& rhs) const noexcept   { return(y > rhs.y || (y == rhs.y && x > rhs.x)); }
    bool          operator==(const IPoint& rhs) const noexcept  { return(y == rhs.y && x == rhs.x); }
    bool          operator!=(const IPoint& rhs) const noexcept  { return(y != rhs.y || x != rhs.x); }
};


constexpr Point::Point(const IPoint& other) noexcept : x(float(other.x)), y(float(other.y)) {}
constexpr IPoint::IPoint(const Point& other) noexcept : x(int(other.x)), y(int(other.y)) {}

} // namespace os
