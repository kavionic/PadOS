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

#include "point.h"
#include <math.h>


class IRect;

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Rect
{
public:
    float left;
    float top;
    float right;
    float bottom;

    Rect()                                       {}
    Rect( float l, float t, float r, float b )   { left = l; top = t; right = r; bottom = b; }
    Rect( const Point& cMin, const Point& cMax ) { left = cMin.x; top = cMin.y; right = cMax.x; bottom = cMax.y; }
    inline Rect( const IRect& cRect );

    ~Rect() {}

    bool        IsValid() const     { return( left <= right && top <= bottom ); }
    void        Invalidate() { left = top = 999999.0f; right = bottom = -999999.0f; }
    bool        DoIntersect( const Point& cPoint ) const
    { return( !( cPoint.x < left || cPoint.x > right || cPoint.y < top || cPoint.y > bottom ) ); }
  
    bool DoIntersect(const Rect& cRect) const
    { return( !( cRect.right < left || cRect.left > right || cRect.bottom < top || cRect.top > bottom ) ); }

    float  Width() const       { return right - left + 1.0f; }
    float  Height() const      { return bottom - top + 1.0f; }
    Point  Size() const        { return Point(right - left + 1.0f, bottom - top + 1.0f); }
    Point  LeftTop() const     { return Point(left, top); }
    Point  RightBottom() const { return Point(right, bottom); }
    Rect   Bounds() const      { return Rect(0, 0, right - left, bottom - top); }
    Rect&  Floor()         { left = floor( left ); right = floor( right ); top = floor( top ); bottom = floor( bottom ); return( *this ); }
    Rect&  Ceil()          { left = ceil( left ); right = ceil( right ); top = ceil( top ); bottom = ceil( bottom ); return( *this ); }
    
    Rect&   Resize( float inLeft, float inTop, float inRight, float inBottom ) {
        left += inLeft; top += inTop; right += inRight; bottom += inBottom;
        return( *this );
    }
    Rect operator+( const Point& cPoint ) const
    { return( Rect( left + cPoint.x, top + cPoint.y, right + cPoint.x, bottom + cPoint.y ) ); }
    Rect operator-( const Point& cPoint ) const
    { return( Rect( left - cPoint.x, top - cPoint.y, right - cPoint.x, bottom - cPoint.y ) ); }

    Point operator+( const Rect& cRect ) const  { return( Point( left + cRect.left, top + cRect.top ) ); }
    Point operator-( const Rect& cRect ) const  { return( Point( left - cRect.left, top - cRect.top ) ); }

    Rect operator&( const Rect& cRect ) const
    { return( Rect( _max( left, cRect.left ), _max( top, cRect.top ), _min( right, cRect.right ), _min( bottom, cRect.bottom ) ) ); }
    void    operator&=( const Rect& cRect )
    { left = _max( left, cRect.left ); top = _max( top, cRect.top );
    right = _min( right, cRect.right ); bottom = _min( bottom, cRect.bottom ); }
    Rect operator|( const Rect& cRect ) const
    { return( Rect( _min( left, cRect.left ), _min( top, cRect.top ), _max( right, cRect.right ), _max( bottom, cRect.bottom ) ) ); }
    void    operator|=( const Rect& cRect )
    { left = _min( left, cRect.left ); top = _min( top, cRect.top );
    right = _max( right, cRect.right ); bottom = _max( bottom, cRect.bottom ); }
    Rect operator|( const Point& cPoint ) const
    { return( Rect( _min( left, cPoint.x ), _min( top, cPoint.y ), _max( right, cPoint.x ), _max( bottom, cPoint.y ) ) ); }
    void operator|=( const Point& cPoint )
    { left = _min( left, cPoint.x ); top =  _min( top, cPoint.y ); right = _max( right, cPoint.x ); bottom = _max( bottom, cPoint.y ); }


    void operator+=( const Point& cPoint ) { left += cPoint.x; top += cPoint.y; right += cPoint.x; bottom += cPoint.y; }
    void operator-=( const Point& cPoint ) { left -= cPoint.x; top -= cPoint.y; right -= cPoint.x; bottom -= cPoint.y; }
  
    bool operator==( const Rect& cRect ) const
    { return( left == cRect.left && top == cRect.top && right == cRect.right && bottom == cRect.bottom ); }
  
    bool operator!=( const Rect& cRect ) const
    { return( left != cRect.left || top != cRect.top || right != cRect.right || bottom != cRect.bottom ); }
private:
    float _min( float a, float b ) const { return( (a<b) ? a : b ); }
    float _max( float a, float b ) const { return( (a>b) ? a : b ); }    
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class IRect
{
public:
    int left;
    int top;
    int right;
    int bottom;

    IRect( void )   { left = top = 999999; right = bottom = -999999; }
    IRect( int l, int t, int r, int b ) { left = l; top = t; right = r; bottom = b; }
    IRect( const IPoint& cMin, const IPoint& cMax ) { left = cMin.x; top = cMin.y; right = cMax.x; bottom = cMax.y; }
    inline IRect( const Rect& cRect );
    ~IRect() {}

    bool        IsValid() const { return( left <= right && top <= bottom ); }
    bool        IsValid2() const     { return( left < right && top < bottom ); }
    void        Invalidate( void ) { left = top = 999999; right = bottom = -999999; }
    bool        DoIntersect( const IPoint& cPoint ) const
    { return( !( cPoint.x < left || cPoint.x > right || cPoint.y < top || cPoint.y > bottom ) ); }
  
    bool DoIntersect( const IRect& cRect ) const
    { return( !( cRect.right < left || cRect.left > right || cRect.bottom < top || cRect.top > bottom ) ); }

    int    Width() const        { return right - left + 1; }
    int    Height() const       { return bottom - top + 1; }
    IPoint Size() const         { return IPoint(right - left + 1, bottom - top + 1); }
    IPoint LeftTop() const      { return IPoint(left, top); }
    IPoint RightBottom() const  { return IPoint(right, bottom); }
    IRect  Bounds( void ) const { return IRect(0, 0, right - left, bottom - top); }
    IRect&  Resize( int nLeft, int nTop, int nRight, int nBottom ) {
        left += nLeft; top += nTop; right += nRight; bottom += nBottom;
        return( *this );
    }
    IRect operator+( const IPoint& cPoint ) const
    { return( IRect( left + cPoint.x, top + cPoint.y, right + cPoint.x, bottom + cPoint.y ) ); }
    IRect operator-( const IPoint& cPoint ) const
    { return( IRect( left - cPoint.x, top - cPoint.y, right - cPoint.x, bottom - cPoint.y ) ); }

    IPoint operator+( const IRect& cRect ) const        { return( IPoint( left + cRect.left, top + cRect.top ) ); }
    IPoint operator-( const IRect& cRect ) const        { return( IPoint( left - cRect.left, top - cRect.top ) ); }

    IRect operator&( const IRect& cRect ) const
    { return( IRect( _max( left, cRect.left ), _max( top, cRect.top ), _min( right, cRect.right ), _min( bottom, cRect.bottom ) ) ); }
    void    operator&=( const IRect& cRect )
    { left = _max( left, cRect.left ); top = _max( top, cRect.top );
    right = _min( right, cRect.right ); bottom = _min( bottom, cRect.bottom ); }
    IRect operator|( const IRect& cRect ) const
    { return( IRect( _min( left, cRect.left ), _min( top, cRect.top ), _max( right, cRect.right ), _max( bottom, cRect.bottom ) ) ); }
    void    operator|=( const IRect& cRect )
    { left = _min( left, cRect.left ); top = _min( top, cRect.top );
    right = _max( right, cRect.right ); bottom = _max( bottom, cRect.bottom ); }
    IRect operator|( const IPoint& cPoint ) const
    { return( IRect( _min( left, cPoint.x ), _min( top, cPoint.y ), _max( right, cPoint.x ), _max( bottom, cPoint.y ) ) ); }
    void operator|=( const IPoint& cPoint )
    { left = _min( left, cPoint.x ); top =  _min( top, cPoint.y ); right = _max( right, cPoint.x ); bottom = _max( bottom, cPoint.y ); }


    void operator+=( const IPoint& cPoint ) { left += cPoint.x; top += cPoint.y; right += cPoint.x; bottom += cPoint.y; }
    void operator-=( const IPoint& cPoint ) { left -= cPoint.x; top -= cPoint.y; right -= cPoint.x; bottom -= cPoint.y; }
  
    bool operator==( const IRect& cRect ) const
    { return( left == cRect.left && top == cRect.top && right == cRect.right && bottom == cRect.bottom ); }
  
    bool operator!=( const IRect& cRect ) const
    { return( left != cRect.left || top != cRect.top || right != cRect.right || bottom != cRect.bottom ); }
private:
    int _min( int a, int b ) const { return( (a<b) ? a : b ); }
    int _max( int a, int b ) const { return( (a>b) ? a : b ); }    

};

Rect::Rect( const IRect& cRect )
{
    left   = float(cRect.left);
    top    = float(cRect.top);
    right  = float(cRect.right);
    bottom = float(cRect.bottom);
}

IRect::IRect( const Rect& cRect )
{
    left   = int(cRect.left);
    top    = int(cRect.top);
    right  = int(cRect.right);
    bottom = int(cRect.bottom);
}
