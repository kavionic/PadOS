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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <vector>
#include <deque>
#include <algorithm>

#include "Region.h"
#include "System/Math/LineSegment.h"


void** Region::s_pFirstClip = nullptr;
//Locker Region::s_cClipListMutex( "region_cliplist_lock" );


ClipRectList::ClipRectList()
{
    m_pcFirst = nullptr;
    m_pcLast  = nullptr;
    m_nCount  = 0;
}

ClipRectList::~ClipRectList()
{
    while( m_pcFirst != nullptr ) {
        ClipRect* rect = m_pcFirst;
        m_pcFirst = rect->m_Next;
        Region::FreeClipRect(rect);
    }
}

void ClipRectList::Clear()
{
    while( m_pcFirst != nullptr ) {
        ClipRect* rect = m_pcFirst;
        m_pcFirst = rect->m_Next;
        Region::FreeClipRect(rect);
    }
    m_pcLast = nullptr;
    m_nCount = 0;
}

void ClipRectList::AddRect(ClipRect* rect)
{
    rect->m_Prev = nullptr;
    rect->m_Next = m_pcFirst;
    if ( m_pcFirst != nullptr ) {
        m_pcFirst->m_Prev = rect;
    }
    m_pcFirst = rect;
    if ( m_pcLast == nullptr ) {
        m_pcLast = rect;
    }
    m_nCount++;
}

void ClipRectList::RemoveRect(ClipRect* rect)
{
    if ( rect->m_Prev != nullptr ) {
        rect->m_Prev->m_Next = rect->m_Next;
    } else {
        m_pcFirst = rect->m_Next;
    }
    if ( rect->m_Next != nullptr ) {
        rect->m_Next->m_Prev = rect->m_Prev;
    } else {
        m_pcLast = rect->m_Prev;
    }
    m_nCount--;
}

ClipRect* ClipRectList::RemoveHead()
{
    if ( m_nCount == 0 ) {
        return nullptr;
    }
    ClipRect* clip = m_pcFirst;
    m_pcFirst = clip->m_Next;
    if ( m_pcFirst == nullptr ) {
        m_pcLast = nullptr;
    }
    m_nCount--;
    return clip;
}

void ClipRectList::StealRects( ClipRectList* pcList )
{
    if ( pcList->m_pcFirst == nullptr ) {
        assert( pcList->m_pcLast == nullptr );
        return;
    }
    
    if ( m_pcFirst == nullptr ) {
        assert( m_pcLast == nullptr );
        m_pcFirst = pcList->m_pcFirst;
        m_pcLast  = pcList->m_pcLast;
    } else {
        m_pcLast->m_Next = pcList->m_pcFirst;
        pcList->m_pcFirst->m_Prev = m_pcLast;
        m_pcLast = pcList->m_pcLast;
    }
    m_nCount += pcList->m_nCount;
    
    pcList->m_pcFirst = nullptr;
    pcList->m_pcLast  = nullptr;
    pcList->m_nCount  = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::Region()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::Region( const IRect& cRect )
{
    AddRect( cRect );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::Region( const Region& cReg )
{
    Set( cReg );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::Region( const Region& cReg, const IRect& cRectangle, bool bNormalize )
{
    IPoint cLefTop = cRectangle.LeftTop();
    ENUMCLIPLIST( &cReg.m_cRects, pcOldClip )
    {
        IRect rect = pcOldClip->m_cBounds & cRectangle;
    
        if (rect.IsValid())
        {
            ClipRect* newClip = AllocClipRect();
            if (newClip != nullptr)
            {
                if ( bNormalize ) {
                    rect -= cLefTop;
                }
                newClip->m_cBounds = rect;
                m_cRects.AddRect(newClip);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::~Region()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ClipRect* Region::AllocClipRect()
{
/*    int nError;
  
    while( (nError = s_cClipListMutex.Lock() ) < 0 ) {
        dbprintf( "Region::AllocClipRect failed to lock list : %s (%d)\n", strerror( errno ), nError );
        if ( EINTR != errno ) {
            return nullptr;
        }
    }*/
    if ( s_pFirstClip == nullptr ) {
//      s_cClipListMutex.Unlock();
        return new ClipRect;
    }
  
    void** pHeader = s_pFirstClip;
    s_pFirstClip = (void**) pHeader[0];

//    s_cClipListMutex.Unlock();
  
    return (ClipRect*) pHeader;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::FreeClipRect(ClipRect* rect)
{
/*    int nError;
  
    while( (nError = s_cClipListMutex.Lock() ) < 0 ) {
        dbprintf( "Region::FreeClipRect failed to lock list : %s (%d)\n", strerror( errno ), nError );
        if ( EINTR != errno ) {
            return;
        }
    }*/

    ((void**)rect)[0] = s_pFirstClip;
    s_pFirstClip = (void**)rect;

//    s_cClipListMutex.Unlock();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Set( const IRect& cRect )
{
    Clear();
    AddRect( cRect );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Set( const Region& cReg )
{
    Clear();

    ENUMCLIPLIST( &cReg.m_cRects, clip ) {
        AddRect( clip->m_cBounds );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Clear( void )
{
    m_cRects.Clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ClipRect *Region::AddRect( const IRect& cRect )
{
    ClipRect* clipRect = AllocClipRect();

    if ( clipRect != nullptr  )
    {
        clipRect->m_cBounds = cRect;
        m_cRects.AddRect( clipRect );
    }
    return clipRect;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Include( const IRect& cRect )
{
    Region cTmpReg( cRect );


    ENUMCLIPLIST( &m_cRects, DClip ) { /* remove all areas already present in drawlist */
        cTmpReg.Exclude( DClip->m_cBounds );
    }
    m_cRects.StealRects( &cTmpReg.m_cRects );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude( const IRect& cRect )
{
    ClipRectList cNewList;

    while( m_cRects.m_pcFirst != nullptr )
    {
        ClipRect* clip = m_cRects.RemoveHead();

        IRect cHide = cRect & clip->m_cBounds;

        if ( cHide.IsValid() == false ) {
            cNewList.AddRect( clip );
            continue;
        }
        // Boundaries of the four possible rectangles surrounding the one to remove.

        IRect newRects[4];

        newRects[3] = IRect( clip->m_cBounds.left, clip->m_cBounds.top, clip->m_cBounds.right, cHide.top - 1 );
        newRects[2] = IRect( clip->m_cBounds.left, cHide.bottom + 1, clip->m_cBounds.right, clip->m_cBounds.bottom );
        newRects[0] = IRect( clip->m_cBounds.left, cHide.top, cHide.left - 1, cHide.bottom );
        newRects[1] = IRect( cHide.right + 1, cHide.top, clip->m_cBounds.right, cHide.bottom );

        FreeClipRect(clip);

        // Create clip rects for the remaining areas.
        for ( int i = 0 ; i < 4 ; ++i )
        { 
            if (newRects[i].IsValid())
            {
                ClipRect* newClip = AllocClipRect();

                if (newClip != nullptr) {
                    newClip->m_cBounds = newRects[i];
                    cNewList.AddRect(newClip);
                }
            }
        }
    }
    m_cRects.m_pcLast = nullptr;
    m_cRects.StealRects( &cNewList );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude( const Region& cReg )
{
    ENUMCLIPLIST( &cReg.m_cRects, clip ) {
        Exclude( clip->m_cBounds );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude( const Region& cReg, const IPoint& cOffset )
{
    ENUMCLIPLIST( &cReg.m_cRects, clip ) {
        Exclude( clip->m_cBounds + cOffset );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Intersect( const Region& cReg )
{
    ClipRectList cList;
    
    ENUMCLIPLIST( &cReg.m_cRects , pcVClip )
    {
        ENUMCLIPLIST( &m_cRects , pcHClip )
        {
            IRect cRect = pcVClip->m_cBounds & pcHClip->m_cBounds;
            if ( cRect.IsValid() )
            {
                ClipRect* newClip = AllocClipRect();

                if (newClip != nullptr) {
                    newClip->m_cBounds = cRect;
                    cList.AddRect(newClip);
                }
            }
        }
    }

    Clear();

    m_cRects.StealRects( &cList );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Intersect( const Region& cReg, const IPoint& cOffset )
{
    ClipRectList cList;
    
    ENUMCLIPLIST( &cReg.m_cRects , pcVClip )
    {
        ENUMCLIPLIST( &m_cRects , pcHClip )
        {
            IRect cRect = (pcVClip->m_cBounds + cOffset) & pcHClip->m_cBounds;
            if ( cRect.IsValid() )
            {
                ClipRect* newClip = AllocClipRect();
                if (newClip != nullptr) {
                    newClip->m_cBounds = cRect;
                    cList.AddRect(newClip);
                }
            }
        }
    }
    Clear();
    m_cRects.StealRects( &cList );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect Region::GetBounds() const
{
    IRect cBounds( 999999, 999999, -999999, -999999 );
  
    ENUMCLIPLIST( &m_cRects , clip ) {
        cBounds |= clip->m_cBounds;
    }
    return cBounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Clip(const ILineSegment& line, std::deque<ILineSegment>& result)
{
    ENUMCLIPLIST(&m_cRects, node)
    {
        int x1 = line.p1.x;
        int y1 = line.p1.y;
        int x2 = line.p2.x;
        int y2 = line.p2.y;
        if (Region::ClipLine(node->m_cBounds, &x1, &y1, &x2, &y2)) {
            result.push_back(ILineSegment(IPoint(x1, y1), IPoint(x2, y2)));
        }                
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class HSortCmp
{
public:
    bool operator()(const ClipRect* rect1, ClipRect* rect2 ) {
        return rect1->m_cBounds.left < rect2->m_cBounds.left;
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class VSortCmp
{
public:
    bool operator()(const ClipRect* rect1, ClipRect* rect2 ) {
        return rect1->m_cBounds.top < rect2->m_cBounds.top;
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Optimize()
{
    std::vector<ClipRect*> cList;

    if ( m_cRects.GetCount() <= 1 ) {
        return;
    }
    cList.reserve( m_cRects.GetCount() );

    ENUMCLIPLIST( &m_cRects, clip ) {
        cList.push_back( clip );
    }
    bool bSomeRemoved = true;
    while( cList.size() > 1 && bSomeRemoved )
    {
        bSomeRemoved = false;
        std::sort( cList.begin(), cList.end(), HSortCmp() );
        for ( int i = 0 ; i < int(cList.size()) - 1 ; )
        {
            if ( cList[i]->m_cBounds.right == cList[i+1]->m_cBounds.left - 1 &&
                 cList[i]->m_cBounds.top == cList[i+1]->m_cBounds.top && cList[i]->m_cBounds.bottom == cList[i+1]->m_cBounds.bottom )
            {
                cList[i]->m_cBounds.right = cList[i+1]->m_cBounds.right;
                m_cRects.RemoveRect( cList[i+1] );
                Region::FreeClipRect( cList[i+1] );
                cList.erase( cList.begin() + i + 1 );
                bSomeRemoved = true;
            }
            else
            {
                ++i;
            }
        }
        if ( cList.size() <= 1 ) {
            break;
        }
        std::sort( cList.begin(), cList.end(), VSortCmp() );
        for ( int i = 0 ; i < int(cList.size()) - 1 ; )
        {
            if ( cList[i]->m_cBounds.bottom == cList[i+1]->m_cBounds.top - 1 &&
                 cList[i]->m_cBounds.left == cList[i+1]->m_cBounds.left && cList[i]->m_cBounds.right == cList[i+1]->m_cBounds.right )
            {
                cList[i]->m_cBounds.bottom = cList[i+1]->m_cBounds.bottom;
                m_cRects.RemoveRect( cList[i+1] );
                Region::FreeClipRect( cList[i+1] );
                cList.erase( cList.begin() + i + 1 );
                bSomeRemoved = true;
            }
            else
            {
                ++i;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Region::ClipLine( const IRect& cRect, int* x1, int* y1, int* x2, int* y2 )
{
    bool point_1 = false;
    bool point_2 = false;     // tracks if each end point is visible or invisible

    bool clip_always = false; // used for clipping override

    int xi=0,yi=0;            // point of intersection

    bool right_edge  = false; // which edges are the endpoints beyond
    bool left_edge   = false;
    bool top_edge    = false;
    bool bottom_edge = false;


    bool success = false;               // was there a successfull clipping

    float dx,dy;                   // used to holds slope deltas

    // test if line is completely visible

    if ( (*x1>=cRect.left) && (*x1<=cRect.right) && (*y1>=cRect.top) && (*y1<=cRect.bottom) ) {
        point_1 = true;
    }

    if ( (*x2 >= cRect.left) && (*x2 <= cRect.right) && (*y2 >= cRect.top) && (*y2 <= cRect.bottom) ) {
        point_2 = true;
    }

    // test endpoints
    if (point_1 && point_2) {
        return true;
    }

    // test if line is completely invisible
    if (point_1==false && point_2==false)
    {
        // must test to see if each endpoint is on the same side of one of
        // the bounding planes created by each clipping region boundary

        if ( ( (*x1<cRect.left) && (*x2<cRect.left) )   ||  // to the left
             ( (*x1>cRect.right) && (*x2>cRect.right) ) ||  // to the right
             ( (*y1<cRect.top) && (*y2<cRect.top) )     ||  // above
             ( (*y1>cRect.bottom) && (*y2>cRect.bottom) ) ) { // below
            return 0; // the entire line is otside the rectangle
        }

        // if we got here we have the special case where the line cuts into and
        // out of the clipping region
        clip_always = true;
    }

    // take care of case where either endpoint is in clipping region
    if ( point_1 || clip_always )
    {
        dx = *x2 - *x1; // compute deltas
        dy = *y2 - *y1;

        // compute what boundary line need to be clipped against
        if (*x2 > cRect.right)
        {
            // flag right edge
            right_edge = true;

            // compute intersection with right edge
            if (dx!=0) {
                yi = (int)(.5 + (dy/dx) * (cRect.right - *x1) + *y1);
            } else {
                yi = -1;  // invalidate intersection
            }                
        }
        else if (*x2 < cRect.left)
        {
            // flag left edge
            left_edge = true;

            // compute intersection with left edge
            if (dx!=0) {
                yi = (int)(.5 + (dy/dx) * (cRect.left - *x1) + *y1);
            } else {
                yi = -1;  // invalidate intersection
            }
        }

        // horizontal intersections
        if (*y2 > cRect.bottom)
        {
            bottom_edge = true; // flag bottom edge

            // compute intersection with right edge
            if (dy!=0) {
                xi = (int)(.5 + (dx/dy) * (cRect.bottom - *y1) + *x1);
            } else {
                xi = -1;  // invalidate inntersection
            }
        }
        else if (*y2 < cRect.top)
        {
            // flag top edge
            top_edge = true;

            // compute intersection with top edge
            if (dy!=0) {
                xi = (int)(.5 + (dx/dy) * (cRect.top - *y1) + *x1);
            } else {
                xi = -1;  // invalidate intersection
            }
        }

        // now we know where the line passed thru
        // compute which edge is the proper intersection

        if ( right_edge == true && (yi >= cRect.top && yi <= cRect.bottom) )
        {
            *x2 = cRect.right;
            *y2 = yi;
            success = true;
        }
        else if (left_edge && (yi>=cRect.top && yi<=cRect.bottom) )
        {
            *x2 = cRect.left;
            *y2 = yi;
            success = true;
        }

        if ( bottom_edge == true && (xi >= cRect.left && xi <= cRect.right) )
        {
            *x2 = xi;
            *y2 = cRect.bottom;
            success = true;
        }
        else if (top_edge && (xi>=cRect.left && xi<=cRect.right) )
        {
            *x2 = xi;
            *y2 = cRect.top;
            success = true;
        }
    } // end if point_1 is visible

    // reset edge flags
    right_edge = left_edge = top_edge = bottom_edge = false;

    // test second endpoint
    if ( point_2 || clip_always )
    {
        dx = *x1 - *x2; // compute deltas
        dy = *y1 - *y2;

        // compute what boundary line need to be clipped against
        if ( *x1 > cRect.right )
        {
            // flag right edge
            right_edge = true;

            // compute intersection with right edge
            if (dx!=0) {
                yi = (int)(.5 + (dy/dx) * (cRect.right - *x2) + *y2);
            } else {
                yi = -1;  // invalidate inntersection
            }
        }
        else if (*x1 < cRect.left)
        {
            left_edge = true; // flag left edge

            // compute intersection with left edge
            if (dx!=0) {
                yi = (int)(.5 + (dy/dx) * (cRect.left - *x2) + *y2);
            } else {
                yi = -1;  // invalidate intersection
            }
        }

        // horizontal intersections
        if (*y1 > cRect.bottom)
        {
            bottom_edge = true; // flag bottom edge

            // compute intersection with right edge
            if (dy!=0) {
                xi = (int)(.5 + (dx/dy) * (cRect.bottom - *y2) + *x2);
                } else {
                xi = -1;  // invalidate inntersection
            }
        }
        else if (*y1 < cRect.top)
        {
            top_edge = true; // flag top edge

            // compute intersection with top edge
            if (dy!=0) {
                xi = (int)(.5 + (dx/dy) * (cRect.top - *y2) + *x2);
                } else {
                xi = -1;  // invalidate inntersection
            }
        }

        // now we know where the line passed thru
        // compute which edge is the proper intersection
        if ( right_edge && (yi >= cRect.top && yi <= cRect.bottom) )
        {
            *x1 = cRect.right;
            *y1 = yi;
            success = true;
        }
        else if ( left_edge && (yi >= cRect.top && yi <= cRect.bottom) )
        {
            *x1 = cRect.left;
            *y1 = yi;
            success = true;
        }

        if (bottom_edge && (xi >= cRect.left && xi <= cRect.right) )
        {
            *x1 = xi;
            *y1 = cRect.bottom;
            success = true;
        }
        else if (top_edge==1 && (xi>=cRect.left && xi<=cRect.right) )
        {
            *x1 = xi;
            *y1 = cRect.top;
            success = true;
        }
    } // end if point_2 is visible

    return success;
}











