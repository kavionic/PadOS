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
#include <Utils/JSON.h>

class PIPoint;

/**
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PPoint
{
public:
    float x;
    float y;

    constexpr PPoint() noexcept : x(0.0f), y(0.0f) {}
    constexpr PPoint(const PPoint& other) noexcept : x(other.x), y(other.y) {}
    constexpr explicit inline PPoint(const PIPoint& other) noexcept;
    constexpr explicit PPoint(float value) noexcept : x(value), y(value) {}
    constexpr PPoint(float X, float Y) noexcept : x(X), y(Y) {}

    PString ToString() const { return PString::format_string("Point({}, {})", x, y); }
    static std::optional<PPoint> FromString(const char* string)
    {
        PPoint value;
        if (sscanf(string, "Point( %f , %f )", &value.x, &value.y) == 2) {
            return value;
        }
        return std::nullopt;
    }
    constexpr float LengthSqr() const noexcept { return x * x + y * y; }
    constexpr float Length() const noexcept { return sqrtf(LengthSqr()); }

    constexpr PPoint GetNormalized() const { return *this * (1.0f / Length()); }
    PPoint& Normalize() { return *this *= (1.0f / Length()); }

    constexpr PPoint GetRounded() const noexcept { return PPoint(roundf(x), roundf(y)); }
    PPoint&          Round() noexcept { x = roundf(x); y = roundf(y); return *this; }

    PPoint& operator=(const PPoint& rhs) noexcept = default;

    constexpr PPoint operator-(void) const noexcept { return PPoint(-x, -y); }
    constexpr PPoint operator+(const PPoint& rhs) const noexcept { return PPoint(x + rhs.x, y + rhs.y); }
    constexpr PPoint operator-(const PPoint& rhs) const noexcept { return PPoint(x - rhs.x, y - rhs.y); }
    PPoint&          operator+=(const PPoint& rhs) noexcept { x += rhs.x; y += rhs.y; return *this; }
    PPoint&          operator-=(const PPoint& rhs) noexcept { x -= rhs.x; y -= rhs.y; return *this; }

    constexpr PPoint operator*(const PPoint& rhs) const noexcept  { return PPoint(x * rhs.x, y * rhs.y);   }
    constexpr PPoint operator*(float rhs) const noexcept         { return PPoint(x * rhs, y * rhs);       }
    
    constexpr PPoint operator/(const PPoint& rhs) const  { return PPoint(x / rhs.x, y / rhs.y);   }
    constexpr PPoint operator/(float rhs) const         { return PPoint(x / rhs, y / rhs);       }

    PPoint&          operator*=(const PPoint& rhs) noexcept   { x *= rhs.x; y *= rhs.y; return *this; }
    PPoint&          operator*=(float rhs) noexcept          { x *= rhs; y *= rhs; return *this; }

    PPoint&          operator/=(const PPoint& rhs) { x /= rhs.x; y /= rhs.y; return *this; }
    PPoint&          operator/=(float rhs)        { x /= rhs; y /= rhs; return *this; }

    constexpr bool  operator<(const PPoint& rhs) const noexcept  { return(y < rhs.y || (y == rhs.y && x < rhs.x)); }
    constexpr bool  operator>(const PPoint& rhs) const noexcept  { return(y > rhs.y || (y == rhs.y && x > rhs.x)); }
    constexpr bool  operator==(const PPoint& rhs) const noexcept { return(y == rhs.y && x == rhs.x); }
    constexpr bool  operator!=(const PPoint& rhs) const noexcept { return(y != rhs.y || x != rhs.x); }

    friend void to_json(Pjson& data, const PPoint& value)
    {
        data = Pjson{ {"x", value.x}, {"y", value.y} };
    }
    friend void from_json(const Pjson& data, PPoint& p)
    {
        data.at("x").get_to(p.x);
        data.at("y").get_to(p.y);
    }
};

/**
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PIPoint
{
public:
    int x;
    int y;

    constexpr PIPoint() noexcept : x(0), y(0) {}
    constexpr PIPoint(const PIPoint& other) noexcept : x(other.x), y(other.y) {}
    constexpr explicit inline PIPoint(const PPoint& other) noexcept;
    constexpr explicit PIPoint(int value) noexcept : x(value), y(value) {}
    constexpr PIPoint(int X, int Y) noexcept : x(X), y(Y) {}

    PIPoint& operator=(const PIPoint&) = default;

    PIPoint        operator-(void) const noexcept                { return(PIPoint(-x, -y)); }
    PIPoint        operator+(const PIPoint& rhs) const noexcept   { return(PIPoint(x + rhs.x, y + rhs.y)); }
    PIPoint        operator-(const PIPoint& rhs) const noexcept   { return(PIPoint(x - rhs.x, y - rhs.y)); }
    const PIPoint& operator+=(const PIPoint& rhs) noexcept        { x += rhs.x; y += rhs.y; return(*this); }
    const PIPoint& operator-=(const PIPoint& rhs) noexcept        { x -= rhs.x; y -= rhs.y; return(*this); }
    bool          operator<(const PIPoint& rhs) const noexcept   { return(y < rhs.y || (y == rhs.y && x < rhs.x)); }
    bool          operator>(const PIPoint& rhs) const noexcept   { return(y > rhs.y || (y == rhs.y && x > rhs.x)); }
    bool          operator==(const PIPoint& rhs) const noexcept  { return(y == rhs.y && x == rhs.x); }
    bool          operator!=(const PIPoint& rhs) const noexcept  { return(y != rhs.y || x != rhs.x); }


    friend void to_json(Pjson& data, const PIPoint& value)
    {
        data = Pjson{ {"x", value.x}, {"y", value.y} };
    }
    friend void from_json(const Pjson& data, PIPoint& p)
    {
        data.at("x").get_to(p.x);
        data.at("y").get_to(p.y);
    }
};


constexpr PPoint::PPoint(const PIPoint& other) noexcept : x(float(other.x)), y(float(other.y)) {}
constexpr PIPoint::PIPoint(const PPoint& other) noexcept : x(int(other.x)), y(int(other.y)) {}
