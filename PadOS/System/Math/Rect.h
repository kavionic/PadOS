// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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

#include "point.h"


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

    Rect() { left = top = right = bottom = 0.0f; }
    explicit Rect(bool initialize) { if (initialize) { left = top = right = bottom = 0.0f; } }
    explicit Rect(float value)     { left = top = right = bottom = value; }
        
    Rect(float l, float t, float r, float b)             { left = l; top = t; right = r; bottom = b; }
    Rect(const Point& topLeft, const Point& bottomRight) { left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }
    inline Rect(const IRect& rect);

    ~Rect() {}

    bool  IsValid() const     { return left < right && top < bottom; }
    void  Invalidate() { left = top = 999999.0f; right = bottom = -999999.0f; }
        
    bool  DoIntersect(const Point& point) const { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    bool  DoIntersect(const Rect& rect) const { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }

    float Width() const       { return right - left; }
    float Height() const      { return bottom - top; }
    Point Size() const        { return Point(right - left, bottom - top); }
    Point TopLeft() const     { return Point(left, top); }
    Point RightBottom() const { return Point(right, bottom); }
    Rect  Bounds() const      { return Rect(0.0f, 0.0f, right - left, bottom - top); }
    
    Rect& Floor()             { left = floor( left ); right = floor( right ); top = floor( top ); bottom = floor( bottom ); return *this; }
    Rect& Ceil()              { left = ceil( left ); right = ceil( right ); top = ceil( top ); bottom = ceil( bottom ); return *this; }
    
    Rect& Resize(float inLeft, float inTop, float inRight, float inBottom) { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }
        
    Rect operator+(const Point& point) const { return Rect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    Rect operator-(const Point& point) const { return Rect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    Point operator+(const Rect& rect) const  { return Point(left + rect.left, top + rect.top); }
    Point operator-(const Rect& rect) const  { return Point(left - rect.left, top - rect.top); }

    Rect operator&(const Rect& rect) const   { return Rect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void operator&=(const Rect& rect)        { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    Rect operator|(const Rect& rect) const   { return Rect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void operator|=(const Rect& rect)        { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    Rect operator|(const Point& point) const { return Rect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void operator|=(const Point& point)      { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }


    void operator+=(const Point& point) { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void operator-=(const Point& point) { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }
  
    bool operator==(const Rect& rect) const { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }
  
    bool operator!=(const Rect& rect) const { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }
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

    IRect()   { left = top = 999999; right = bottom = -999999; }
    explicit IRect(int value)             { left = top = right = bottom = value; }
    IRect(int l, int t, int r, int b ) { left = l; top = t; right = r; bottom = b; }
    IRect(const IPoint& topLeft, const IPoint& bottomRight) { left = topLeft.x; top = topLeft.y; right = bottomRight.x; bottom = bottomRight.y; }
    inline IRect(const Rect& rect);

    bool   IsValid() const { return left < right && top < bottom; }
    void   Invalidate()    { left = top = 999999; right = bottom = -999999; }
    
    bool   DoIntersect(const IPoint& point) const { return !(point.x < left || point.x >= right || point.y < top || point.y >= bottom); }
    bool   DoIntersect(const IRect& rect) const { return !(rect.right <= left || rect.left >= right || rect.bottom <= top || rect.top >= bottom); }
    
    int    Width() const        { return right - left; }
    int    Height() const       { return bottom - top; }
    IPoint Size() const         { return IPoint(right - left, bottom - top); }
    IPoint TopLeft() const      { return IPoint(left, top); }
    IPoint RightBottom() const  { return IPoint(right, bottom); }
    IRect  Bounds( void ) const { return IRect(0, 0, right - left, bottom - top); }

    IRect& Resize(int inLeft, int inTop, int inRight, int inBottom) { left += inLeft; top += inTop; right += inRight; bottom += inBottom; return *this; }
  
    IRect operator+(const IPoint& point) const { return IRect(left + point.x, top + point.y, right + point.x, bottom + point.y); }
    IRect operator-(const IPoint& point) const { return IRect(left - point.x, top - point.y, right - point.x, bottom - point.y); }

    IPoint operator+(const IRect& rect) const  { return IPoint(left + rect.left, top + rect.top); }
    IPoint operator-(const IRect& rect) const  { return IPoint(left - rect.left, top - rect.top); }

    IRect operator&(const IRect& rect) const   { return IRect(std::max(left, rect.left), std::max(top, rect.top), std::min(right, rect.right), std::min(bottom, rect.bottom)); }
    void  operator&=(const IRect& rect)        { left = std::max(left, rect.left); top = std::max(top, rect.top); right = std::min(right, rect.right); bottom = std::min(bottom, rect.bottom); }
    IRect operator|(const IRect& rect) const   { return IRect(std::min(left, rect.left), std::min(top, rect.top), std::max(right, rect.right), std::max(bottom, rect.bottom)); }
    void  operator|=(const IRect& rect)        { left = std::min(left, rect.left); top = std::min(top, rect.top); right = std::max(right, rect.right); bottom = std::max(bottom, rect.bottom); }
    IRect operator|(const IPoint& point) const { return IRect(std::min(left, point.x), std::min(top, point.y), std::max(right, point.x), std::max(bottom, point.y)); }
    void  operator|=(const IPoint& point)      { left = std::min(left, point.x); top = std::min(top, point.y); right = std::max(right, point.x); bottom = std::max(bottom, point.y); }


    void operator+=(const IPoint& point) { left += point.x; top += point.y; right += point.x; bottom += point.y; }
    void operator-=(const IPoint& point) { left -= point.x; top -= point.y; right -= point.x; bottom -= point.y; }
    
    bool operator==(const IRect& rect) const { return left == rect.left && top == rect.top && right == rect.right && bottom == rect.bottom; }
    
    bool operator!=(const IRect& rect) const { return left != rect.left || top != rect.top || right != rect.right || bottom != rect.bottom; }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect::Rect(const IRect& rect)
{
    left   = float(rect.left);
    top    = float(rect.top);
    right  = float(rect.right);
    bottom = float(rect.bottom);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect::IRect(const Rect& rect)
{
    left   = int(rect.left);
    top    = int(rect.top);
    right  = int(rect.right);
    bottom = int(rect.bottom);
}
