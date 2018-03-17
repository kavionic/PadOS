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
// along with PadOS. If not, see < http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#pragma once

class IPoint;

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Point
{
public:
    float x;
    float y;

    Point()			 { x = y = 0.0f;	}
    Point( const Point& cPnt ) { x = cPnt.x; y = cPnt.y; }
    explicit inline Point( const IPoint& cPnt );
    explicit Point( float value ) { x = y = value;	}
    Point( float nX, float nY ) { x = nX; y = nY;	}

    Point	 operator-( void ) const 		{ return( Point( -x, -y ) ); }
    Point	 operator+( const Point& cPoint ) const  { return( Point( x + cPoint.x, y + cPoint.y ) ); }
    Point	 operator-( const Point& cPoint ) const  { return( Point( x - cPoint.x, y - cPoint.y ) ); }
    const Point& operator+=( const Point& cPoint )	 {  x += cPoint.x; y += cPoint.y; return( *this ); }
    const Point& operator-=( const Point& cPoint )	 {  x -= cPoint.x; y -= cPoint.y; return( *this ); }
    bool	 operator<( const Point& cPoint ) const  { return( y < cPoint.y || ( y == cPoint.y && x < cPoint.x ) ); }
    bool	 operator>( const Point& cPoint ) const  { return( y > cPoint.y || ( y == cPoint.y && x > cPoint.x ) ); }
    bool	 operator==( const Point& cPoint ) const { return( y == cPoint.y && x == cPoint.x ); }
    bool	 operator!=( const Point& cPoint ) const { return( y != cPoint.y || x != cPoint.x ); }
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class IPoint
{
public:
    int x;
    int y;

    IPoint()			 { x = y = 0;	}
    IPoint( const IPoint& cPnt ) { x = cPnt.x; y = cPnt.y; }
    explicit inline IPoint( const Point& cPnt );
    explicit IPoint( int value ) { x = y = value;	}
    IPoint( int nX, int nY )	 { x = nX; y = nY;	}

    IPoint	  operator-( void ) const 		   { return( IPoint( -x, -y ) ); }
    IPoint	  operator+( const IPoint& cPoint ) const  { return( IPoint( x + cPoint.x, y + cPoint.y ) ); }
    IPoint	  operator-( const IPoint& cPoint ) const  { return( IPoint( x - cPoint.x, y - cPoint.y ) ); }
    const IPoint& operator+=( const IPoint& cPoint )	   {  x += cPoint.x; y += cPoint.y; return( *this ); }
    const IPoint& operator-=( const IPoint& cPoint )	   {  x -= cPoint.x; y -= cPoint.y; return( *this ); }
    bool	  operator<( const IPoint& cPoint ) const  { return( y < cPoint.y || ( y == cPoint.y && x < cPoint.x ) ); }
    bool	  operator>( const IPoint& cPoint ) const  { return( y > cPoint.y || ( y == cPoint.y && x > cPoint.x ) ); }
    bool	  operator==( const IPoint& cPoint ) const { return( y == cPoint.y && x == cPoint.x ); }
    bool	  operator!=( const IPoint& cPoint ) const { return( y != cPoint.y || x != cPoint.x ); }
};


Point::Point( const IPoint& cPnt )
{
    x = float(cPnt.x);
    y = float(cPnt.y);
}

IPoint::IPoint( const Point& cPnt )
{
    x = int(cPnt.x);
    y = int(cPnt.y);
}

