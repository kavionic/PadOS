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

    constexpr Rect() noexcept : left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {}
    explicit Rect(bool initialize) noexcept { if (initialize) { left = top = right = bottom = 0.0f; } }
    constexpr explicit Rect(float value) noexcept : left(value), top(value), right(value), bottom(value) {}

    constexpr Rect(float l, float t, float r, float b) noexcept : left(l), top(t), right(r), bottom(b) {}
    constexpr Rect(const Point& topLeft, const Point& bottomRight) noexcept : left(topLeft.x), top(topLeft.y), right(bottomRight.x), bottom(bottomRight.y) {}
    constexpr inline Rect(const IRect& rect) noexcept;

    constexpr bool  IsValid() const noexcept { return left < right && top < bottom; }
    void  Invalidate() noexcept  { left = top = 999999.0f; right = bottom = -999999.0f; }

    constexpr bool  DoIntersect(const Point& point) const noexcept { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    constexpr bool  DoIntersect(const Rect& rect) const noexcept { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }

    constexpr float Width() const noexcept { return right - left; }
    constexpr float Height() const noexcept { return bottom - top; }
    constexpr Point Size() const noexcept { return Point(right - left, bottom - top); }
    constexpr Point TopLeft() const noexcept { return Point(left, top); }
    constexpr Point TopRight() const noexcept { return Point(right, top); }
    constexpr Point BottomLeft() const noexcept { return Point(left, bottom); }
    constexpr Point BottomRight() const noexcept { return Point(right, bottom); }
    constexpr Point Center() const noexcept { return Point(right * 0.5f, bottom * 0.5f); }
    constexpr Rect  Bounds() const noexcept { return Rect(0.0f, 0.0f, right - left, bottom - top); }

    Rect& Round() noexcept { left = std::round(left); right = std::round(right); top = std::round(top); bottom = std::round(bottom); return *this; }
    Rect& Floor() noexcept { left = std::floor(left); right = std::floor(right); top = std::floor(top); bottom = std::floor(bottom); return *this; }
    Rect& Ceil() noexcept { left = std::ceil(left); right = std::ceil(right); top = std::ceil(top); bottom = std::ceil(bottom); return *this; }

    constexpr Rect GetRounded() const noexcept  { return Rect(std::round(left), std::round(top), std::round(right), std::round(bottom)); }
    constexpr Rect GetFloored() const noexcept  { return Rect(std::floor(left), std::floor(top), std::floor(right), std::floor(bottom)); }
    constexpr Rect GetCeiled() const noexcept   { return Rect(std::ceil(left),  std::ceil(top),  std::ceil(right),  std::ceil(bottom)); }

    Rect& Resize(float inLeft, float inTop, float inRight, float inBottom) noexcept { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }
    Rect  GetResized(float inLeft, float inTop, float inRight, float inBottom) const noexcept { Rect result = *this; result.Resize(inLeft, inTop, inRight, inBottom); return result; }

    constexpr Rect  operator+(const Point& point) const noexcept { return Rect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    constexpr Rect  operator-(const Point& point) const noexcept { return Rect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    constexpr Point operator+(const Rect& rect) const noexcept { return Point(left + rect.left, top + rect.top); }
    constexpr Point operator-(const Rect& rect) const noexcept { return Point(left - rect.left, top - rect.top); }

    constexpr Rect  operator&(const Rect& rect) const noexcept { return Rect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void            operator&=(const Rect& rect) noexcept { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    constexpr Rect  operator|(const Rect& rect) const noexcept { return Rect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void            operator|=(const Rect& rect) noexcept { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    constexpr Rect  operator|(const Point& point) const noexcept { return Rect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void            operator|=(const Point& point) noexcept { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }

    void            operator+=(const Point& point) noexcept { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void            operator-=(const Point& point) noexcept { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }

    constexpr bool  operator==(const Rect& rect) const noexcept { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }

    constexpr bool  operator!=(const Rect& rect) const noexcept { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }


    static constexpr Rect FromSize(const Point& size) noexcept { return Rect(Point(0.0f, 0.0f), size); }
    static constexpr Rect FromSize(float x, float y) noexcept  { return Rect(0.0f, 0.0f, x, y); }
    static constexpr Rect Centered(const Point& center, const Point& size) noexcept { return FromSize(size) - size * 0.5f + center; }
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

    constexpr IRect() noexcept  : left(999999), top(999999), right(-999999), bottom(-999999) {}
    constexpr explicit IRect(int value) noexcept : left(value), top(value), right(value), bottom(value) {}
    constexpr IRect(int l, int t, int r, int b) noexcept : left(l), top(t), right(r), bottom(b) {}
    constexpr IRect(const IPoint& topLeft, const IPoint& bottomRight) noexcept : left(topLeft.x), top(topLeft.y), right(bottomRight.x), bottom(bottomRight.y) {}
    constexpr inline IRect(const Rect& rect) noexcept;

    constexpr bool  IsValid() const noexcept { return left < right && top < bottom; }
    void            Invalidate() noexcept { left = top = 999999; right = bottom = -999999; }

    constexpr bool  DoIntersect(const IPoint& point) const noexcept { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    constexpr bool  DoIntersect(const IRect& rect) const noexcept { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }

    constexpr int    Width() const noexcept { return right - left; }
    constexpr int    Height() const noexcept { return bottom - top; }
    constexpr IPoint Size() const noexcept { return IPoint(right - left, bottom - top); }
    constexpr IPoint TopLeft() const noexcept { return IPoint(left, top); }
    constexpr IPoint RightBottom() const noexcept { return IPoint(right, bottom); }
    constexpr IRect  Bounds(void) const noexcept { return IRect(0, 0, right - left, bottom - top); }

    IRect& Resize(int inLeft, int inTop, int inRight, int inBottom) noexcept { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }

    constexpr IRect operator+(const IPoint& point) const noexcept { return IRect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    constexpr IRect operator-(const IPoint& point) const noexcept { return IRect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    constexpr IPoint operator+(const IRect& rect) const noexcept { return IPoint(left + rect.left, top + rect.top); }
    constexpr IPoint operator-(const IRect& rect) const noexcept { return IPoint(left - rect.left, top - rect.top); }

    constexpr IRect operator&(const IRect& rect) const noexcept { return IRect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void            operator&=(const IRect& rect) noexcept { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    constexpr IRect operator|(const IRect& rect) const noexcept { return IRect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void            operator|=(const IRect& rect) noexcept { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    constexpr IRect operator|(const IPoint& point) const noexcept { return IRect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void            operator|=(const IPoint& point) noexcept  { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }


    void operator+=(const IPoint& point) noexcept { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void operator-=(const IPoint& point) noexcept { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }

    constexpr bool operator==(const IRect& rect) const noexcept { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }
    constexpr bool operator!=(const IRect& rect) const noexcept { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

constexpr Rect::Rect(const IRect& rect) noexcept : left(float(rect.left)), top(float(rect.top)), right(float(rect.right)), bottom(float(rect.bottom)) {}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

constexpr IRect::IRect(const Rect& rect) noexcept : left(int(rect.left)), top(int(rect.top)), right(int(rect.right)), bottom(int(rect.bottom)) {}


} // namespace os
