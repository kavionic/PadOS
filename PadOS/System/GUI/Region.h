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

#include <deque>

#include "System/Math/Rect.h"
#include "System/Math/Point.h"
#include "System/Ptr/PtrTarget.h"

//#include <util/locker.h>

class ILineSegment;

#define ENUMCLIPLIST( list, node ) for ( ClipRect* node = (list)->m_pcFirst ; node != nullptr ; node = node->m_Next )

//namespace os
//{
//#if 0
//}
//#endif // Fool Emacs auto-indent

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

struct ClipRect
{
    ClipRect() {}
    ClipRect( const IRect& cFrame, const IPoint& cOffset ) : m_cBounds(cFrame), m_cMove(cOffset) {}

    ClipRect* m_Next;
    ClipRect* m_Prev;
    IRect     m_cBounds;
    IPoint    m_cMove; // Used to sort rectangles, so no blits destroy needed areas
};

/** 
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ClipRectList
{
public:
    ClipRectList();
    ~ClipRectList();
    
    void        Clear();
    void        AddRect( ClipRect* pcRect );
    void        RemoveRect( ClipRect* pcRect );
    ClipRect*   RemoveHead();
    void        StealRects( ClipRectList* pcList );
    int         GetCount() const { return( m_nCount ); }
   
    ClipRect*   m_pcFirst;
    ClipRect*   m_pcLast;
    int         m_nCount;
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Region : public PtrTarget
{
public:
    Region();
    Region( const IRect& cRect );
    Region( const Region& cReg );
    Region( const Region& cReg, const IRect& cRect, bool bNormalize );

    ~Region();

    void        Set( const IRect& cRect );
    void        Set( const Region& cRect );
    void        Clear( void );
    bool        IsEmpty() const { return( m_cRects.GetCount() == 0 ); }
    int         GetClipCount() const { return( m_cRects.GetCount() ); } //! /since 0.3.7
    void        Include( const IRect& cRect );
    void        Intersect( const Region& cReg );
    void        Intersect( const Region& cReg, const IPoint& cOffset );

    void        Exclude( const IRect& cRect );
    void        Exclude( const Region& cReg );
    void        Exclude( const Region& cReg, const IPoint& cOffset );

    ClipRect*   AddRect( const IRect& cRect );
    
//    Ptr<Region>       Clone( const IRect& cRectangle, bool bNormalize );
    void        Optimize();

    static void         FreeClipRect( ClipRect* pcRect );
    static ClipRect*    AllocClipRect();
    IRect               GetBounds() const;
        
    static bool ClipLine(const IRect& rect, IPoint* point1, IPoint* point2);

    ClipRectList    m_cRects;
private:
    static void** s_pFirstClip;
//    static Locker s_cClipListMutex;
  
};

//} // End of namespace
