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

#include "GUI/Region.h"

using namespace os;

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

Region::Region(const Region& reg) : PtrTarget()
{
    Set(reg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Region::Region(const Region& region, const IRect& rectangle, bool normalize)
{
    const IPoint topLeft = rectangle.TopLeft();
    for (const Rect& oldClip : region.m_Rects)
    {
        IRect rect = oldClip & rectangle;
            
        if (rect.IsValid())
        {
            if (normalize) {
                rect -= topLeft;
            }
            m_Rects.push_back(rect);
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

void Region::Set(const IRect& rect)
{
    try
    {
        m_Rects.clear();
        m_Rects.push_back(rect);
    }
    catch (const std::bad_alloc& error) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Set(const Region& region)
{
    try
    {
        m_Rects = region.m_Rects;
    }
    catch (const std::bad_alloc& error) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Clear()
{
    m_Rects.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::AddRect(const IRect& rect)
{
    try
    {
        m_Rects.push_back(rect);
    }
    catch (const std::bad_alloc& error) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Include(const IRect& rect)
{
    try
    {
        Region tmpReg(rect);

        // Remove all areas already present in clip-list.
        for (const IRect& clip : m_Rects) {
            tmpReg.Exclude(clip);
        }
        m_Rects.insert(m_Rects.begin(), tmpReg.m_Rects.begin(), tmpReg.m_Rects.end());
    }
    catch (const std::bad_alloc& error) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude(const IRect& rect)
{
    try
    {
        const size_t prevClipCount = m_Rects.size();
    
        size_t nextFree = 0;
    
        for (size_t i = 0; i < prevClipCount; ++i)
        {
            const IRect oldFrame = m_Rects[i];
            const IRect hide = rect & oldFrame;

            if (!hide.IsValid()) {
                continue;
            }
            m_Rects[i].left = m_Rects[i].right; // Mark it as unused.
            // Boundaries of the four possible rectangles surrounding the one to remove.

            if (oldFrame.left < hide.left) { // Left (center)
                nextFree = AddOrReuseClip(nextFree, i, IRect(oldFrame.left, hide.top, hide.left, hide.bottom));
            }
            if (hide.right < oldFrame.right) { // Right (center)
                nextFree = AddOrReuseClip(nextFree, i, IRect(hide.right, hide.top, oldFrame.right, hide.bottom));
            }
            if (oldFrame.top < hide.top) { // Above (full width)
                nextFree = AddOrReuseClip(nextFree, i, IRect(oldFrame.left, oldFrame.top, oldFrame.right, hide.top));
            }
            if (hide.bottom < oldFrame.bottom) { // Below (full width)
                nextFree = AddOrReuseClip(nextFree, i, IRect(oldFrame.left, hide.bottom, oldFrame.right, oldFrame.bottom));
            }        
        }
        RemoveUnusedClips(nextFree, prevClipCount);
    
        Validate();
    }
    catch (const std::bad_alloc& error) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude(const Region& region)
{
    for (const IRect& clip : region.m_Rects) {
        Exclude(clip);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Exclude(const Region& region, const IPoint& offset)
{
    for (const IRect& clip : region.m_Rects) {
        Exclude(clip + offset);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Intersect(const Region& region)
{
    try
    {
        std::vector<IRect> newList;
        for (const IRect& dstRect : m_Rects)
        {
            for (const IRect& srcRect : region.m_Rects)
            {
                const IRect rect = dstRect & srcRect;
                if (rect.IsValid()) {
                    newList.push_back(rect);
                }
            }
        }
        m_Rects = std::move(newList);
    }
    catch (const std::bad_alloc& error) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Intersect(const Region& region, const IPoint& offset)
{
    try
    {
        std::vector<IRect> newList;
        for (const IRect& dstRect : m_Rects)
        {
            for (const IRect& srcRect : region.m_Rects)
            {
                const IRect rect = dstRect & (srcRect + offset);
                if (rect.IsValid()) {
                    newList.push_back(rect);
                }
            }
        }
        m_Rects = std::move(newList);
    }
    catch (const std::bad_alloc& error) {}        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect Region::GetBounds() const
{
    IRect bounds(999999, 999999, -999999, -999999);
  
    for (const IRect& clip : m_Rects) {
        bounds |= clip;
    }
    return bounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Optimize()
{
    try
    {
        if ( m_Rects.size() <= 1 ) {
            return;
        }
        std::vector<IRect*> list;

        list.reserve(m_Rects.size());

        for (IRect& clip : m_Rects) {
            list.push_back(&clip);
        }
    
        bool someRemoved = true;
        while(list.size() > 1 && someRemoved)
        {
            someRemoved = false;
            std::sort(list.begin(), list.end(), [](const IRect* lhs, const IRect* rhs) { return lhs->left < rhs->left; });
            for (size_t i = 0 ; i < list.size() - 1 ;)
            {
                IRect& curr = *list[i];
                IRect& next = *list[i+1];
                if ( curr.right == next.left && curr.top == next.top && curr.bottom == next.bottom )
                {
                    curr.right = next.right; // Expand
                    next.left = next.right;  // Mark as unused
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
            std::sort(list.begin(), list.end(), [](const IRect* lhs, const IRect* rhs) { return lhs->top < rhs->top; });
            for (size_t i = 0 ; i < list.size() - 1 ;)
            {
                IRect& curr = *list[i];
                IRect& next = *list[i+1];
                if (curr.bottom == next.top && curr.left == next.left && curr.right == next.right)
                {
                    curr.bottom = next.bottom; // Expand
                    next.left = next.right;    // Mark as unused
                    list.erase(list.begin() + i + 1);
                    someRemoved = true;
                }
                else
                {
                    ++i;
                }
            }
        }
        RemoveUnusedClips(1, m_Rects.size());
    }
    catch (const std::bad_alloc& error) {}        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Region::ClipLine(const IRect& rect, IPoint* point1, IPoint* point2)
{
    // Check if each end point is visible or invisible
    const bool point1Inside = rect.DoIntersect(*point1);
    const bool point2Inside = rect.DoIntersect(*point2);

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

//    float dx,dy;           // used to holds slope deltas
    int dx, dy;           // used to holds slope deltas


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
                const int hDelta = rect.right - 1 - point2->x;
                const int rounding = dx / 2;
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
                const int hDelta = rect.left - point2->x;
                const int rounding = dx / 2;
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
                const int vDelta = rect.bottom - 1 - point2->y;
                const int rounding = dy / 2;
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
                const int vDelta = rect.top - point2->y;
                const int rounding = dy / 2;
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::Validate()
{
    for (const IRect& clip1 : m_Rects)
    {
        for (const IRect& clip2 : m_Rects)
        {
            if (&clip1 != &clip2 && clip1.DoIntersect(clip2)) {
                printf("ERROR: Region::Validate() CLIPS OEVRLAP!!!\n");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t Region::FindUnusedClip(size_t prevUnused, size_t lastToCheck)
{
    if (prevUnused != INVALID_INDEX)
    {
        for (int i = prevUnused; i <= lastToCheck; ++i)
        {
            if (m_Rects[i].left == m_Rects[i].right) {
                return i;
            }
        }
    }    
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t Region::AddOrReuseClip(size_t prevUnused, size_t lastToCheck, const IRect& frame)
{
    try
    {
        const size_t freeSlot = FindUnusedClip(prevUnused, lastToCheck);
        if (freeSlot == INVALID_INDEX)
        {
            m_Rects.push_back(frame);
            return lastToCheck + 1;
        }
        else
        {
            m_Rects[freeSlot] = frame;
            return freeSlot + 1;
        }
    }
    catch (const std::bad_alloc& error)
    {
        return prevUnused;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Region::RemoveUnusedClips(size_t firstToCheck, size_t lastToCheck)
{
    try
    {
        size_t newClipCount = m_Rects.size();
        for (size_t i = firstToCheck; i < lastToCheck; )
        {
            if (m_Rects[i].left == m_Rects[i].right)
            {
                newClipCount--;

                if (i == m_Rects.size() - 1)
                {
                    lastToCheck--;
                }
                else
                {
                    m_Rects[i] = m_Rects[newClipCount]; // Erase by swapping in the last entry.
                    if (newClipCount < lastToCheck) {
                        --lastToCheck;
                    } else {                    
                        ++i; // If the clip swapped in came from outside the range to check, skip to next.
                    }                    
                }
            }
            else
            {
                ++i;
            }
        }
        m_Rects.resize(newClipCount);
    }
    catch (const std::bad_alloc& error) {}
}
