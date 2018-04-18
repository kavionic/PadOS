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

void ClipRectList::StealRects(ClipRectList* list)
{
    if ( list->m_pcFirst == nullptr ) {
        assert( list->m_pcLast == nullptr );
        return;
    }
    
    if ( m_pcFirst == nullptr ) {
        assert( m_pcLast == nullptr );
        m_pcFirst = list->m_pcFirst;
        m_pcLast  = list->m_pcLast;
    } else {
        m_pcLast->m_Next = list->m_pcFirst;
        list->m_pcFirst->m_Prev = m_pcLast;
        m_pcLast = list->m_pcLast;
    }
    m_nCount += list->m_nCount;
    
    list->m_pcFirst = nullptr;
    list->m_pcLast  = nullptr;
    list->m_nCount  = 0;
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

Region::Region(const IRect& rect)
{
    AddRect(rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::Region(const Region& reg)
{
    Set(reg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::Region(const Region& region, const IRect& rectangle, bool bNormalize)
{
    IPoint topLeft = rectangle.TopLeft();
    ENUMCLIPLIST( &region.m_cRects, pcOldClip )
    {
        IRect rect = pcOldClip->m_cBounds & rectangle;
    
        if (rect.IsValid())
        {
            ClipRect* newClip = AllocClipRect();
            if (newClip != nullptr)
            {
                if ( bNormalize ) {
                    rect -= topLeft;
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

void Region::Set(const IRect& rect)
{
    Clear();
    AddRect(rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Set( const Region& region )
{
    Clear();

    ENUMCLIPLIST(&region.m_cRects, clip) {
        AddRect(clip->m_cBounds);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Clear()
{
    m_cRects.Clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ClipRect *Region::AddRect(const IRect& rect)
{
    ClipRect* clipRect = AllocClipRect();

    if (clipRect != nullptr)
    {
        clipRect->m_cBounds = rect;
        m_cRects.AddRect(clipRect);
    }
    return clipRect;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Include(const IRect& rect)
{
    Region tmpReg(rect);

    // Remove all areas already present in clip-list.
    ENUMCLIPLIST(&m_cRects, clip) {
        tmpReg.Exclude(clip->m_cBounds);
    }
    m_cRects.StealRects(&tmpReg.m_cRects);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude(const IRect& rect)
{
    ClipRectList newList;

    while(m_cRects.m_pcFirst != nullptr)
    {
        ClipRect* clip = m_cRects.RemoveHead();

        IRect hide = rect & clip->m_cBounds;

        if (!hide.IsValid()) {
            newList.AddRect(clip);
            continue;
        }
        // Boundaries of the four possible rectangles surrounding the one to remove.

        IRect newRects[4];

        newRects[3] = IRect(clip->m_cBounds.left, clip->m_cBounds.top, clip->m_cBounds.right, hide.top);       // Above (full width)
        newRects[2] = IRect(clip->m_cBounds.left, hide.bottom, clip->m_cBounds.right, clip->m_cBounds.bottom); // Below (full width)
        newRects[0] = IRect(clip->m_cBounds.left, hide.top, hide.left, hide.bottom);   // Left (center)
        newRects[1] = IRect(hide.right, hide.top, clip->m_cBounds.right, hide.bottom); // Right (center)

        FreeClipRect(clip);

        // Create clip rects for the remaining areas.
        for ( int i = 0 ; i < 4 ; ++i )
        { 
            if (newRects[i].IsValid())
            {
                ClipRect* newClip = AllocClipRect();
                if (newClip != nullptr) {
                    newClip->m_cBounds = newRects[i];
                    newList.AddRect(newClip);
                }
            }
        }
    }
    m_cRects.m_pcLast = nullptr;
    m_cRects.StealRects(&newList);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude(const Region& region)
{
    ENUMCLIPLIST(&region.m_cRects, clip) {
        Exclude(clip->m_cBounds);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude(const Region& region, const IPoint& offset)
{
    ENUMCLIPLIST(&region.m_cRects, clip) {
        Exclude(clip->m_cBounds + offset);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Intersect(const Region& region)
{
    ClipRectList list;
    
    ENUMCLIPLIST(&region.m_cRects, clipV)
    {
        ENUMCLIPLIST(&m_cRects, clipH)
        {
            IRect rect = clipV->m_cBounds & clipH->m_cBounds;
            if (rect.IsValid())
            {
                ClipRect* newClip = AllocClipRect();
                if (newClip != nullptr)
                {
                    newClip->m_cBounds = rect;
                    list.AddRect(newClip);
                }
            }
        }
    }

    Clear();

    m_cRects.StealRects(&list);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Intersect(const Region& region, const IPoint& offset)
{
    ClipRectList list;
    
    ENUMCLIPLIST(&region.m_cRects, clipV)
    {
        ENUMCLIPLIST(&m_cRects, clipH)
        {
            IRect rect = (clipV->m_cBounds + offset) & clipH->m_cBounds;
            if (rect.IsValid())
            {
                ClipRect* newClip = AllocClipRect();
                if (newClip != nullptr)
                {
                    newClip->m_cBounds = rect;
                    list.AddRect(newClip);
                }
            }
        }
    }
    Clear();
    m_cRects.StealRects(&list);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect Region::GetBounds() const
{
    IRect bounds( 999999, 999999, -999999, -999999 );
  
    ENUMCLIPLIST( &m_cRects , clip ) {
        bounds |= clip->m_cBounds;
    }
    return bounds;
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
    std::vector<ClipRect*> list;

    if ( m_cRects.GetCount() <= 1 ) {
        return;
    }
    list.reserve( m_cRects.GetCount() );

    ENUMCLIPLIST( &m_cRects, clip ) {
        list.push_back( clip );
    }
    bool someRemoved = true;
    while(list.size() > 1 && someRemoved)
    {
        someRemoved = false;
        std::sort(list.begin(), list.end(), HSortCmp());
        for (size_t i = 0 ; i < list.size() - 1 ;)
        {
            IRect& curr = list[i]->m_cBounds;
            IRect& next = list[i+1]->m_cBounds;
            if ( curr.right == next.left && curr.top == next.top && curr.bottom == next.bottom )
            {
                curr.right = next.right;
                m_cRects.RemoveRect(list[i+1]);
                Region::FreeClipRect(list[i+1]);
                list.erase(list.begin() + i + 1);
                someRemoved = true;
            }
            else
            {
                ++i;
            }
        }
        if (list.size() <= 1) {
            break;
        }
        std::sort(list.begin(), list.end(), VSortCmp());
        for (size_t i = 0 ; i < list.size() - 1 ;)
        {
            IRect& curr = list[i]->m_cBounds;
            IRect& next = list[i+1]->m_cBounds;
            if (curr.bottom == next.top && curr.left == next.left && curr.right == next.right)
            {
                curr.bottom = next.bottom;
                m_cRects.RemoveRect(list[i+1]);
                Region::FreeClipRect(list[i+1]);
                list.erase(list.begin() + i + 1);
                someRemoved = true;
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

bool Region::ClipLine(const IRect& rect, IPoint* point1, IPoint* point2)
{
    // Check if each end point is visible or invisible
    bool point1Inside = rect.DoIntersect(*point1);
    bool point2Inside = rect.DoIntersect(*point2);

    // test endpoints
    if (point1Inside && point2Inside) {
        return true;
    }

    bool clipAlways = false; // used for clipping override

    int xi=0;  // point of intersection
    int yi=0;

    bool rightEdge  = false; // which edges are the endpoints beyond
    bool leftEdge   = false;
    bool topEdge    = false;
    bool bottomEdge = false;

    bool success = false;  // was there a successful clipping

    float dx,dy;           // used to holds slope deltas


    // test if line is completely invisible
    if (!point1Inside && !point2Inside)
    {
        // Must test to see if each endpoint is on the same side of one of
        // the bounding planes created by each clipping region boundary

        if ( ((point1->x <  rect.left)   && (point2->x <  rect.left))  ||  // to the left
             ((point1->x >= rect.right)  && (point2->x >= rect.right)) ||  // to the right
             ((point1->y <  rect.top)    && (point2->y <  rect.top))   ||  // above
             ((point1->y >= rect.bottom) && (point2->y >= rect.bottom)))   // below
        {
            return false; // the entire line is outside the rectangle
        }

        // if we got here we have the special case where the line cuts into and
        // out of the clipping region
        clipAlways = true;
    }

    // take care of case where either endpoint is in clipping region
    if (point1Inside || clipAlways)
    {
        dx = point2->x - point1->x; // compute deltas
        dy = point2->y - point1->y;

        // compute what boundary line need to be clipped against
        if (point2->x >= rect.right)
        {
            // flag right edge
            rightEdge = true;

            // compute intersection with right edge
            if (dx!=0) {
                int hOffset = rect.right - 1 - point1->x;
                yi = (dy * hOffset + dx - 1) / dx + point1->y;
            } else {
                yi = -1;  // invalidate intersection
            }                
        }
        else if (point2->x < rect.left)
        {
            // flag left edge
            leftEdge = true;

            // compute intersection with left edge
            if (dx!=0) {
                int hOffset = rect.left - point1->x;
                yi = (dy * hOffset + dx - 1) / dx + point1->y;
            } else {
                yi = -1;  // invalidate intersection
            }
        }

        // horizontal intersections
        if (point2->y >= rect.bottom)
        {
            bottomEdge = true; // flag bottom edge

            // compute intersection with right edge
            if (dy!=0) {
                int vDelta = rect.bottom - 1 - point1->y;
                int rounding = dy / 2;
                xi = (dx * vDelta + rounding) / dy + point1->x;
            } else {
                xi = -1;  // invalidate inntersection
            }
        }
        else if (point2->y < rect.top)
        {
            // flag top edge
            topEdge = true;

            // compute intersection with top edge
            if (dy!=0) {
                int vDelta = rect.top - point1->y;
                int rounding = dy / 2;
                xi = (dx * vDelta + rounding) / dy + point1->x;
            } else {
                xi = -1;  // invalidate intersection
            }
        }

        // now we know where the line passed thru
        // compute which edge is the proper intersection

        if (rightEdge == true && (yi >= rect.top && yi < rect.bottom))
        {
            point2->x = rect.right - 1;
            point2->y = yi;
            success = true;
        }
        else if (leftEdge && (yi>=rect.top && yi < rect.bottom))
        {
            point2->x = rect.left;
            point2->y = yi;
            success = true;
        }

        if (bottomEdge == true && (xi >= rect.left && xi < rect.right))
        {
            point2->x = xi;
            point2->y = rect.bottom - 1;
            success = true;
        }
        else if (topEdge && (xi>=rect.left && xi<rect.right))
        {
            point2->x = xi;
            point2->y = rect.top;
            success = true;
        }
    } // end if point 1 is visible

    // reset edge flags
    rightEdge = leftEdge = topEdge = bottomEdge = false;

    // test second endpoint
    if (point2Inside || clipAlways)
    {
        dx = point1->x - point2->x; // compute deltas
        dy = point1->y - point2->y;

        // compute what boundary line need to be clipped against
        if ( point1->x >= rect.right )
        {
            // flag right edge
            rightEdge = true;

            // compute intersection with right edge
            if (dx!=0) {
                int hDelta = rect.right - 1 - point2->x;
                int rounding = dx / 2;
                yi = (dy * hDelta + rounding) / dx + point2->y;
            } else {
                yi = -1;  // invalidate intersection
            }
        }
        else if (point1->x < rect.left)
        {
            leftEdge = true; // flag left edge

            // compute intersection with left edge
            if (dx!=0) {
                int hDelta = rect.left - point2->x;
                int rounding = dx / 2;
                yi = (dy * hDelta + rounding) / dx + point2->y;
            } else {
                yi = -1;  // invalidate intersection
            }
        }

        // horizontal intersections
        if (point1->y >= rect.bottom)
        {
            bottomEdge = true; // flag bottom edge

            // compute intersection with right edge
            if (dy!=0) {
                int vDelta = rect.bottom - 1 - point2->y;
                int rounding = dy / 2;
                xi = (dx * vDelta + rounding) / dy + point2->x;
            } else {
                xi = -1;  // invalidate intersection
            }
        }
        else if (point1->y < rect.top)
        {
            topEdge = true; // flag top edge

            // compute intersection with top edge
            if (dy!=0) {
                int vDelta = rect.top - point2->y;
                int rounding = dy / 2;
                xi = (dx * vDelta + rounding) / dy + point2->x;
            } else {
                xi = -1;  // invalidate intersection
            }
        }

        // now we know where the line passed through
        // compute which edge is the proper intersection
        if (rightEdge && (yi >= rect.top && yi < rect.bottom))
        {
            point1->x = rect.right - 1;
            point1->y = yi;
            success = true;
        }
        else if (leftEdge && (yi >= rect.top && yi < rect.bottom))
        {
            point1->x = rect.left;
            point1->y = yi;
            success = true;
        }

        if (bottomEdge && (xi >= rect.left && xi < rect.right))
        {
            point1->x = xi;
            point1->y = rect.bottom - 1;
            success = true;
        }
        else if (topEdge == 1 && (xi >= rect.left && xi < rect.right))
        {
            point1->x = xi;
            point1->y = rect.top;
            success = true;
        }
    } // end if point 2 is visible

    return success;
}
