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

#include <algorithm>
#include <math.h>

#include "Math/point.h"

namespace os
{

class IRect;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class Rect
{
public:
    float left;
    float top;
    float right;
    float bottom;

    constexpr Rect() : left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {}
    explicit Rect(bool initialize) { if (initialize) { left = top = right = bottom = 0.0f; } }
    constexpr explicit Rect(float value) : left(value), top(value), right(value), bottom(value) {}

    constexpr Rect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
    constexpr Rect(const Point& topLeft, const Point& bottomRight) : left(topLeft.x), top(topLeft.y), right(bottomRight.x), bottom(bottomRight.y) {}
    constexpr inline Rect(const IRect& rect);

    constexpr bool  IsValid() const { return left < right && top < bottom; }
    void  Invalidate() { left = top = 999999.0f; right = bottom = -999999.0f; }

    constexpr bool  DoIntersect(const Point& point) const { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    constexpr bool  DoIntersect(const Rect& rect) const { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }

    constexpr float Width() const { return right - left; }
    constexpr float Height() const { return bottom - top; }
    constexpr Point Size() const { return Point(right - left, bottom - top); }
    constexpr Point TopLeft() const { return Point(left, top); }
    constexpr Point TopRight() const { return Point(right, top); }
    constexpr Point BottomLeft() const { return Point(left, bottom); }
    constexpr Point BottomRight() const { return Point(right, bottom); }
    constexpr Point Center() const { return Point(right * 0.5f, bottom * 0.5f); }
    constexpr Rect  Bounds() const { return Rect(0.0f, 0.0f, right - left, bottom - top); }

    Rect& Round() { left = round(left); right = round(right); top = round(top); bottom = round(bottom); return *this; }
    Rect& Floor() { left = floor(left); right = floor(right); top = floor(top); bottom = floor(bottom); return *this; }
    Rect& Ceil() { left = ceil(left); right = ceil(right); top = ceil(top); bottom = ceil(bottom); return *this; }

    constexpr Rect GetRounded() const { return Rect(round(left), round(top), round(right), round(bottom)); }
    constexpr Rect GetFloored() const { return Rect(floor(left), floor(top), floor(right), floor(bottom)); }
    constexpr Rect GetCeiled() const  { return Rect(ceil(left), ceil(top), ceil(right), ceil(bottom)); }

    Rect& Resize(float inLeft, float inTop, float inRight, float inBottom) { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }

    constexpr Rect  operator+(const Point& point) const { return Rect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    constexpr Rect  operator-(const Point& point) const { return Rect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    constexpr Point operator+(const Rect& rect) const { return Point(left + rect.left, top + rect.top); }
    constexpr Point operator-(const Rect& rect) const { return Point(left - rect.left, top - rect.top); }

    constexpr Rect  operator&(const Rect& rect) const { return Rect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void            operator&=(const Rect& rect) { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    constexpr Rect  operator|(const Rect& rect) const { return Rect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void            operator|=(const Rect& rect) { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    constexpr Rect  operator|(const Point& point) const { return Rect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void            operator|=(const Point& point) { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }

    void            operator+=(const Point& point) { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void            operator-=(const Point& point) { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }

    constexpr bool  operator==(const Rect& rect) const { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }

    constexpr bool  operator!=(const Rect& rect) const { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }


    static constexpr Rect FromSize(const Point& size) { return Rect(Point(0.0f, 0.0f), size); }
    static constexpr Rect Centered(const Point& center, const Point& size) { return FromSize(size) - size * 0.5f + center; }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class IRect
{
public:
    int left;
    int top;
    int right;
    int bottom;

    constexpr IRect() : left(999999), top(999999), right(-999999), bottom(-999999) {}
    constexpr explicit IRect(int value) : left(value), top(value), right(value), bottom(value) {}
    constexpr IRect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
    constexpr IRect(const IPoint& topLeft, const IPoint& bottomRight) : left(topLeft.x), top(topLeft.y), right(bottomRight.x), bottom(bottomRight.y) {}
    constexpr inline IRect(const Rect& rect);

    bool   IsValid() const { return left < right && top < bottom; }
    void   Invalidate() { left = top = 999999; right = bottom = -999999; }

    bool   DoIntersect(const IPoint& point) const { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    bool   DoIntersect(const IRect& rect) const { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }

    constexpr int    Width() const { return right - left; }
    constexpr int    Height() const { return bottom - top; }
    constexpr IPoint Size() const { return IPoint(right - left, bottom - top); }
    constexpr IPoint TopLeft() const { return IPoint(left, top); }
    constexpr IPoint RightBottom() const { return IPoint(right, bottom); }
    constexpr IRect  Bounds(void) const { return IRect(0, 0, right - left, bottom - top); }

    IRect& Resize(int inLeft, int inTop, int inRight, int inBottom) { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }

    IRect operator+(const IPoint& point) const { return IRect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    IRect operator-(const IPoint& point) const { return IRect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    IPoint operator+(const IRect& rect) const { return IPoint(left + rect.left, top + rect.top); }
    IPoint operator-(const IRect& rect) const { return IPoint(left - rect.left, top - rect.top); }

    IRect operator&(const IRect& rect) const { return IRect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void  operator&=(const IRect& rect) { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    IRect operator|(const IRect& rect) const { return IRect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void  operator|=(const IRect& rect) { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    IRect operator|(const IPoint& point) const { return IRect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void  operator|=(const IPoint& point) { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }


    void operator+=(const IPoint& point) { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void operator-=(const IPoint& point) { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }

    bool operator==(const IRect& rect) const { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }

    bool operator!=(const IRect& rect) const { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

constexpr Rect::Rect(const IRect& rect) : left(float(rect.left)), top(float(rect.top)), right(float(rect.right)), bottom(float(rect.bottom)) {}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

constexpr IRect::IRect(const Rect& rect) : left(int(rect.left)), top(int(rect.top)), right(int(rect.right)), bottom(int(rect.bottom)) {}


} // namespace os
