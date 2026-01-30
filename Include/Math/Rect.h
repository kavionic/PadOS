// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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

#include <Math/point.h>

class PIRect;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PRect
{
public:
    float left;
    float top;
    float right;
    float bottom;

    constexpr PRect() noexcept : left(0.0f), top(0.0f), right(0.0f), bottom(0.0f) {}
    explicit PRect(bool initialize) noexcept { if (initialize) { left = top = right = bottom = 0.0f; } }
    constexpr explicit PRect(float value) noexcept : left(value), top(value), right(value), bottom(value) {}

    constexpr PRect(float l, float t, float r, float b) noexcept : left(l), top(t), right(r), bottom(b) {}
    constexpr PRect(const PPoint& topLeft, const PPoint& bottomRight) noexcept : left(topLeft.x), top(topLeft.y), right(bottomRight.x), bottom(bottomRight.y) {}
    constexpr inline PRect(const PIRect& rect) noexcept;

    constexpr bool  IsValid() const noexcept { return left < right && top < bottom; }
    void  Invalidate() noexcept  { left = top = 999999.0f; right = bottom = -999999.0f; }

    constexpr bool  DoIntersect(const PPoint& point) const noexcept { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    constexpr bool  DoIntersect(const PRect& rect) const noexcept { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }

    constexpr float Width() const noexcept { return right - left; }
    constexpr float Height() const noexcept { return bottom - top; }
    constexpr PPoint Size() const noexcept { return PPoint(right - left, bottom - top); }
    constexpr PPoint TopLeft() const noexcept { return PPoint(left, top); }
    constexpr PPoint TopRight() const noexcept { return PPoint(right, top); }
    constexpr PPoint BottomLeft() const noexcept { return PPoint(left, bottom); }
    constexpr PPoint BottomRight() const noexcept { return PPoint(right, bottom); }
    constexpr PPoint Center() const noexcept { return PPoint(right * 0.5f, bottom * 0.5f); }
    constexpr PRect  Bounds() const noexcept { return PRect(0.0f, 0.0f, right - left, bottom - top); }

    PRect& Round() noexcept { left = std::round(left); right = std::round(right); top = std::round(top); bottom = std::round(bottom); return *this; }
    PRect& Floor() noexcept { left = std::floor(left); right = std::floor(right); top = std::floor(top); bottom = std::floor(bottom); return *this; }
    PRect& Ceil() noexcept { left = std::ceil(left); right = std::ceil(right); top = std::ceil(top); bottom = std::ceil(bottom); return *this; }

    constexpr PRect GetRounded() const noexcept  { return PRect(std::round(left), std::round(top), std::round(right), std::round(bottom)); }
    constexpr PRect GetFloored() const noexcept  { return PRect(std::floor(left), std::floor(top), std::floor(right), std::floor(bottom)); }
    constexpr PRect GetCeiled() const noexcept   { return PRect(std::ceil(left),  std::ceil(top),  std::ceil(right),  std::ceil(bottom)); }

    PRect& Resize(float inLeft, float inTop, float inRight, float inBottom) noexcept { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }
    PRect  GetResized(float inLeft, float inTop, float inRight, float inBottom) const noexcept { PRect result = *this; result.Resize(inLeft, inTop, inRight, inBottom); return result; }

    constexpr PRect  operator+(const PPoint& point) const noexcept { return PRect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    constexpr PRect  operator-(const PPoint& point) const noexcept { return PRect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    constexpr PPoint operator+(const PRect& rect) const noexcept { return PPoint(left + rect.left, top + rect.top); }
    constexpr PPoint operator-(const PRect& rect) const noexcept { return PPoint(left - rect.left, top - rect.top); }

    constexpr PRect  operator&(const PRect& rect) const noexcept { return PRect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void            operator&=(const PRect& rect) noexcept { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    constexpr PRect  operator|(const PRect& rect) const noexcept { return PRect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void            operator|=(const PRect& rect) noexcept { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    constexpr PRect  operator|(const PPoint& point) const noexcept { return PRect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void            operator|=(const PPoint& point) noexcept { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }

    void            operator+=(const PPoint& point) noexcept { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void            operator-=(const PPoint& point) noexcept { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }

    constexpr bool  operator==(const PRect& rect) const noexcept { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }

    constexpr bool  operator!=(const PRect& rect) const noexcept { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }


    static constexpr PRect FromSize(const PPoint& size) noexcept { return PRect(PPoint(0.0f, 0.0f), size); }
    static constexpr PRect FromSize(float x, float y) noexcept  { return PRect(0.0f, 0.0f, x, y); }
    static constexpr PRect Centered(const PPoint& center, const PPoint& size) noexcept { return FromSize(size) - size * 0.5f + center; }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PIRect
{
public:
    int left;
    int top;
    int right;
    int bottom;

    constexpr PIRect() noexcept  : left(999999), top(999999), right(-999999), bottom(-999999) {}
    constexpr explicit PIRect(int value) noexcept : left(value), top(value), right(value), bottom(value) {}
    constexpr PIRect(int l, int t, int r, int b) noexcept : left(l), top(t), right(r), bottom(b) {}
    constexpr PIRect(const PIPoint& topLeft, const PIPoint& bottomRight) noexcept : left(topLeft.x), top(topLeft.y), right(bottomRight.x), bottom(bottomRight.y) {}
    constexpr inline PIRect(const PRect& rect) noexcept;

    constexpr bool  IsValid() const noexcept { return left < right && top < bottom; }
    void            Invalidate() noexcept { left = top = 999999; right = bottom = -999999; }

    constexpr bool  DoIntersect(const PIPoint& point) const noexcept { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    constexpr bool  DoIntersect(const PIRect& rect) const noexcept { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }

    constexpr int    Width() const noexcept { return right - left; }
    constexpr int    Height() const noexcept { return bottom - top; }
    constexpr PIPoint Size() const noexcept { return PIPoint(right - left, bottom - top); }
    constexpr PIPoint TopLeft() const noexcept { return PIPoint(left, top); }
    constexpr PIPoint RightBottom() const noexcept { return PIPoint(right, bottom); }
    constexpr PIRect  Bounds(void) const noexcept { return PIRect(0, 0, right - left, bottom - top); }

    PIRect& Resize(int inLeft, int inTop, int inRight, int inBottom) noexcept { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }

    constexpr PIRect operator+(const PIPoint& point) const noexcept { return PIRect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    constexpr PIRect operator-(const PIPoint& point) const noexcept { return PIRect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    constexpr PIPoint operator+(const PIRect& rect) const noexcept { return PIPoint(left + rect.left, top + rect.top); }
    constexpr PIPoint operator-(const PIRect& rect) const noexcept { return PIPoint(left - rect.left, top - rect.top); }

    constexpr PIRect operator&(const PIRect& rect) const noexcept { return PIRect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void            operator&=(const PIRect& rect) noexcept { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    constexpr PIRect operator|(const PIRect& rect) const noexcept { return PIRect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void            operator|=(const PIRect& rect) noexcept { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    constexpr PIRect operator|(const PIPoint& point) const noexcept { return PIRect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void            operator|=(const PIPoint& point) noexcept  { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }


    void operator+=(const PIPoint& point) noexcept { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void operator-=(const PIPoint& point) noexcept { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }

    constexpr bool operator==(const PIRect& rect) const noexcept { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }
    constexpr bool operator!=(const PIRect& rect) const noexcept { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

constexpr PRect::PRect(const PIRect& rect) noexcept : left(float(rect.left)), top(float(rect.top)), right(float(rect.right)), bottom(float(rect.bottom)) {}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

constexpr PIRect::PIRect(const PRect& rect) noexcept : left(int(rect.left)), top(int(rect.top)), right(int(rect.right)), bottom(int(rect.bottom)) {}
