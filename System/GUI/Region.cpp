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

#include <Utils/Logging.h>
#include <GUI/Region.h>
#include <GUI/GUIDefines.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRegion::PRegion()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRegion::PRegion(const PIRect& rect)
{
    AddRect(rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRegion::PRegion(const PRegion& reg) : PtrTarget()
{
    Set(reg);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRegion::PRegion(const PRegion& region, const PIRect& rectangle, bool normalize)
{
    const PIPoint topLeft = rectangle.TopLeft();
    for (const PIRect& oldClip : region.m_Rects)
    {
        PIRect rect = oldClip & rectangle;
            
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

PRegion::~PRegion()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRegion::Set(const PIRect& rect)
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

void PRegion::Set(const PRegion& region)
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

void PRegion::Clear()
{
    m_Rects.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRegion::AddRect(const PIRect& rect)
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

void PRegion::Include(const PIRect& rect)
{
    try
    {
        PRegion tmpReg(rect);

        // Remove all areas already present in clip-list.
        for (const PIRect& clip : m_Rects) {
            tmpReg.Exclude(clip);
        }
        m_Rects.insert(m_Rects.begin(), tmpReg.m_Rects.begin(), tmpReg.m_Rects.end());
    }
    catch (const std::bad_alloc& error) {}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRegion::Exclude(const PIRect& rect)
{
    try
    {
        const size_t prevClipCount = m_Rects.size();
    
        size_t nextFree = 0;
    
        for (size_t i = 0; i < prevClipCount; ++i)
        {
            const PIRect oldFrame = m_Rects[i];
            const PIRect hide = rect & oldFrame;

            if (!hide.IsValid()) {
                continue;
            }
            m_Rects[i].left = m_Rects[i].right; // Mark it as unused.
            // Boundaries of the four possible rectangles surrounding the one to remove.

            if (oldFrame.left < hide.left) { // Left (center)
                nextFree = AddOrReuseClip(nextFree, i, PIRect(oldFrame.left, hide.top, hide.left, hide.bottom));
            }
            if (hide.right < oldFrame.right) { // Right (center)
                nextFree = AddOrReuseClip(nextFree, i, PIRect(hide.right, hide.top, oldFrame.right, hide.bottom));
            }
            if (oldFrame.top < hide.top) { // Above (full width)
                nextFree = AddOrReuseClip(nextFree, i, PIRect(oldFrame.left, oldFrame.top, oldFrame.right, hide.top));
            }
            if (hide.bottom < oldFrame.bottom) { // Below (full width)
                nextFree = AddOrReuseClip(nextFree, i, PIRect(oldFrame.left, hide.bottom, oldFrame.right, oldFrame.bottom));
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

void PRegion::Exclude(const PRegion& region)
{
    for (const PIRect& clip : region.m_Rects) {
        Exclude(clip);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRegion::Exclude(const PRegion& region, const PIPoint& offset)
{
    for (const PIRect& clip : region.m_Rects) {
        Exclude(clip + offset);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRegion::Intersect(const PRegion& region)
{
    try
    {
        std::vector<PIRect> newList;
        for (const PIRect& dstRect : m_Rects)
        {
            for (const PIRect& srcRect : region.m_Rects)
            {
                const PIRect rect = dstRect & srcRect;
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

void PRegion::Intersect(const PRegion& region, const PIPoint& offset)
{
    try
    {
        std::vector<PIRect> newList;
        for (const PIRect& dstRect : m_Rects)
        {
            for (const PIRect& srcRect : region.m_Rects)
            {
                const PIRect rect = dstRect & (srcRect + offset);
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

PIRect PRegion::GetBounds() const
{
    PIRect bounds(999999, 999999, -999999, -999999);
  
    for (const PIRect& clip : m_Rects) {
        bounds |= clip;
    }
    return bounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRegion::Optimize()
{
    try
    {
        if ( m_Rects.size() <= 1 ) {
            return;
        }
        std::vector<PIRect*> list;

        list.reserve(m_Rects.size());

        for (PIRect& clip : m_Rects) {
            list.push_back(&clip);
        }
    
        bool someRemoved = true;
        while(list.size() > 1 && someRemoved)
        {
            someRemoved = false;
            std::sort(list.begin(), list.end(), [](const PIRect* lhs, const PIRect* rhs) { return lhs->left < rhs->left; });
            for (size_t i = 0 ; i < list.size() - 1 ;)
            {
                PIRect& curr = *list[i];
                PIRect& next = *list[i+1];
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
            std::sort(list.begin(), list.end(), [](const PIRect* lhs, const PIRect* rhs) { return lhs->top < rhs->top; });
            for (size_t i = 0 ; i < list.size() - 1 ;)
            {
                PIRect& curr = *list[i];
                PIRect& next = *list[i+1];
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

bool PRegion::ClipLine(const PIRect& rect, PIPoint* point1, PIPoint* point2)
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

void PRegion::Validate()
{
    for (const PIRect& clip1 : m_Rects)
    {
        for (const PIRect& clip2 : m_Rects)
        {
            if (&clip1 != &clip2 && clip1.DoIntersect(clip2)) {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Region::Validate() CLIPS OVERLAP!!!");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PRegion::FindUnusedClip(size_t prevUnused, size_t lastToCheck)
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

size_t PRegion::AddOrReuseClip(size_t prevUnused, size_t lastToCheck, const PIRect& frame)
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

void PRegion::RemoveUnusedClips(size_t firstToCheck, size_t lastToCheck)
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
