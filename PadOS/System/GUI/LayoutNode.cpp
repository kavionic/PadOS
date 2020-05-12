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

#include "Platform.h"

#include <algorithm>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "LayoutNode.h"
#include "View.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

LayoutNode::LayoutNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

LayoutNode::~LayoutNode()
{
    if (m_View != nullptr) {
        m_View->SetLayoutNode(nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::Layout()
{
    if (m_View != nullptr)
    {
        Rect frame   = m_View->GetNormalizedBounds();
    
        for (Ptr<View> child : *m_View) {
            Rect borders = child->GetBorders();
            Rect childFrame = frame;
            childFrame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
            child->SetFrame(childFrame);
        }            
    }
    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::CalculatePreferredSize(Point* minSizeOut, Point* maxSizeOut, bool includeWidth, bool includeHeight)
{
    Point minSize(0.0f,0.0f);
    Point maxSize(0.0f, 0.0f);
    
    if (m_View != nullptr)
    {
        for (Ptr<View> child : *m_View)
        {
            Rect borders = child->GetBorders();
            Point borderSize(borders.left + borders.right, borders.top + borders.bottom);
            Point currentMinSize = child->GetPreferredSize(PrefSizeType::Smallest) + borderSize;
            Point currentMaxSize = child->GetPreferredSize(PrefSizeType::Greatest) + borderSize;
            if (includeWidth) {
                minSize.x = std::max(minSize.x, currentMinSize.x);
                maxSize.x = std::min(maxSize.x, currentMaxSize.x);
            }
            if (includeHeight) {
                minSize.y = std::max(minSize.y, currentMinSize.y);
                maxSize.y = std::min(maxSize.y, currentMaxSize.y);
            }                
        }
        if (includeWidth) {
            minSizeOut->x = ceil(minSize.x);
            maxSizeOut->x = floor(maxSize.x);
        }            
        if (includeHeight) {
            minSizeOut->y = ceil(minSize.y);
            maxSizeOut->y = floor(maxSize.y);
        }            
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


/*
#define FOR_EACH_NAME( name1, func ) { \
    va_list pArgs; va_start( pArgs, name1 );    \
    for( const char* name = name1 ; name != nullptr ; name = va_arg(pArgs,const char*) ) {  \
        LayoutNode* pcNode = FindNode( name, true, true );                                  \
        if ( pcNode != nullptr ) {                                                          \
            pcNode->##func;                                                                 \
        } else {                                                                            \
            dbprintf( "Warning: LayoutNode::%s() could not find node '%s'\n", __FUNCTION__, name ); \
        }                                                                                   \
    }                                                                                       \
    va_end( pArgs );                                                                        \
}


void LayoutNode::SetBorders( const Rect& cBorders, const char* pzName1, ... )
{
    FOR_EACH_NAME( pzName1, SetBorders( cBorders ) );
}

void LayoutNode::SetWheights( float vWheight, const char* pzFirstName, ... )
{
    FOR_EACH_NAME( pzFirstName, SetWheight( vWheight ) );
}

void LayoutNode::SetHAlignments( Alignment align, const char* pzFirstName, ... )
{
    FOR_EACH_NAME( pzFirstName, SetHAlignment( align ) );
}

void LayoutNode::SetVAlignments( Alignment align, const char* pzFirstName, ... )
{
    FOR_EACH_NAME( pzFirstName, SetVAlignment( align ) );
}
*/

/*
void LayoutNode::SameWidth( const char* pzName1, ... )
{

    va_list pArgs;
    va_start( pArgs, pzName1 );
    LayoutNode* pcFirstNode = FindNode( pzName1, true, true );

    if ( pcFirstNode == nullptr ) {
        dbprintf( "LayoutNode::SameWidth() failed to find node '%s'\n", pzName1 );
        return;
    }
    
    for( const char* pzName = va_arg(pArgs,const char*) ; pzName != nullptr ; pzName = va_arg(pArgs,const char*) ) {
        LayoutNode* pcNode = FindNode( pzName, true, true );
        if ( pcNode == nullptr ) {
            dbprintf( "LayoutNode::SameWidth() failed to find node '%s'\n", pzName );
            continue;
        }
        pcNode->AddToWidthRing( pcFirstNode );
    }
    va_end( pArgs );
}

void LayoutNode::SameHeight( const char* pzName1, ... )
{

    va_list pArgs;
    va_start( pArgs, pzName1 );
    LayoutNode* pcFirstNode = FindNode( pzName1, true, true );

    if ( pcFirstNode == nullptr ) {
        dbprintf( "LayoutNode::SameHeight() failed to find node '%s'\n", pzName1 );
        return;
    }
    
    for( const char* pzName = va_arg(pArgs,const char*) ; pzName != nullptr ; pzName = va_arg(pArgs,const char*) ) {
        LayoutNode* pcNode = FindNode( pzName, true, true );
        if ( pcNode == nullptr ) {
            dbprintf( "LayoutNode::SameHeight() failed to find node '%s'\n", pzName );
            continue;
        }
        pcNode->AddToHeightRing( pcFirstNode );
    }
    va_end( pArgs );
}*/


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::AttachedToView(View* view)
{
    if (m_View != nullptr) {
        m_View->SetLayoutNode(nullptr);
    }
    m_View = view;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static float SpaceOut(uint32_t count, float totalSize, float totalMinSize, float totalWheight,
                      float* minSizes, float* maxSizes, float* wheights, float* finalSizes)
{
    float extraSpace = totalSize - totalMinSize;
    
    bool*  doneFlags = (bool*)alloca(sizeof(bool) * count);

    memset(doneFlags, 0, sizeof(bool) * count);

    for (;;)
    {
        uint32_t i;
        for (i = 0 ; i < count ; ++i)
        {
            if (doneFlags[i]) {
                continue;
            }
            float vWeight = wheights[i] / totalWheight;

            finalSizes[i] = minSizes[i] + extraSpace * vWeight;
            if (finalSizes[i] >= maxSizes[i])
            {
                extraSpace   -= maxSizes[i] - minSizes[i];
                totalWheight -= wheights[i];
                finalSizes[i] = maxSizes[i];
                doneFlags[i] = true;
                break;
            }
        }
        if (i == count) {
            break;
        }
    }
    for (uint32_t i = 0 ; i < count ; ++i) {
        totalSize -= finalSizes[i];
    }
    return totalSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HLayoutNode::HLayoutNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HLayoutNode::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    *minSize = Point(0.0f, 0.0f);
    *maxSize = Point(0.0f, 0.0f);

    if (m_View != nullptr)
    {
        for (Ptr<View> child : *m_View)
        {
            Rect borders = child->GetBorders();
            Point borderSize(borders.left + borders.right, borders.top + borders.bottom);
            Point childMinSize = child->GetPreferredSize(PrefSizeType::Smallest) + borderSize;
            Point childMaxSize = child->GetPreferredSize(PrefSizeType::Greatest) + borderSize;
            if (includeWidth) {
                minSize->x += childMinSize.x;
                maxSize->x += childMaxSize.x;
            }
            if (includeHeight) {
                if (childMinSize.y > minSize->y) minSize->y = childMinSize.y;
                if (childMaxSize.y > maxSize->y) maxSize->y = childMaxSize.y;
            }                
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HLayoutNode::Layout()
{
    if (m_View != nullptr)
    {
        const auto& childList = m_View->GetChildList();

        Rect bounds = m_View->GetNormalizedBounds();

        float* minWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* maxWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* maxHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* wheights     = (float*)alloca(sizeof(float)*childList.size());
        float* finalHeights = (float*)alloca(sizeof(float)*childList.size());
            
        float  vMinWidth = 0.0f;
        float  totalWheight = 0.0f;
    
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Ptr<View> child = childList[i];
  
            Rect borders = child->GetBorders();
            Point borderSize(borders.left + borders.right, borders.top + borders.bottom);
            
            Point minSize = child->GetPreferredSize(PrefSizeType::Smallest);
            Point maxSize = child->GetPreferredSize(PrefSizeType::Greatest);

            minSize += borderSize;
            maxSize += borderSize;
            
            wheights[i]   = child->GetWheight();

            maxHeights[i] = maxSize.y;
            maxWidths[i]  = maxSize.x;
            minWidths[i]  = minSize.x;

            vMinWidth += minSize.x;
            totalWheight += wheights[i];
        }
    //    if ( vMinWidth > bounds.Width() + 1.0f ) {
    //      printf( "Error: HLayoutNode::Layout() Width=%.2f, Required width=%.2f\n", bounds.Width() + 1.0f, vMinWidth  );
    //    }
        float unusedWidth = SpaceOut(childList.size(), bounds.Width() + 1.0f, vMinWidth, totalWheight, minWidths, maxWidths, wheights, finalHeights);

//        printf("HLayout: Unused space: %f (%f)\n", unusedWidth, bounds.Width());
        unusedWidth /= float(childList.size());
        float x = bounds.left + unusedWidth * 0.5f;
    
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Rect frame(0.0f, 0.0f, finalHeights[i], bounds.bottom);
            if (frame.Height() > maxHeights[i]) {
                frame.bottom = maxHeights[i];
            }
            float y = 0.0f;
            switch(childList[i]->GetVAlignment())
            {
                case Alignment::Top:    y = bounds.top; break;
                case Alignment::Right:  y = bounds.bottom - frame.Height() - 1.0f; break;
                default:           printf( "Error: HLayoutNode::Layout() node '%s' has invalid v-alignment %d\n", m_View->GetName().c_str(), int(childList[i]->GetVAlignment()) );
                [[fallthrough]];
                case Alignment::Center: y = bounds.top + (bounds.Height() - frame.Height()) * 0.5f; break;
            }
            
            frame += Point(x, y);
            Rect borders = childList[i]->GetBorders();
            
            float width = frame.Width();
            frame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
            
            x += width + unusedWidth;
            frame.Floor();
//            printf("    %d: %.2f->%.2f\n", i, frame.left, frame.right);
            childList[i]->SetFrame(frame);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

VLayoutNode::VLayoutNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void VLayoutNode::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    *minSize = Point(0.0f, 0.0f);
    *maxSize = Point(0.0f, 0.0f);
    
    if (m_View != nullptr)
    {
        for (Ptr<View> child : *m_View)
        {
            Rect  borders = child->GetBorders();
            Point borderSize(borders.left + borders.right, borders.top + borders.bottom);

            Point childMinSize = child->GetPreferredSize(PrefSizeType::Smallest) + borderSize;
            Point childMaxSize = child->GetPreferredSize(PrefSizeType::Greatest) + borderSize;
            
            if (includeWidth) {
                if (childMinSize.x > minSize->x) minSize->x = childMinSize.x;
                if (childMaxSize.x > maxSize->x) maxSize->x = childMaxSize.x;
            }
            if (includeHeight) {
                minSize->y += childMinSize.y;
                maxSize->y += childMaxSize.y;
            }
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void VLayoutNode::Layout()
{
    if (m_View != nullptr)
    {
        const auto& childList = m_View->GetChildList();
        Rect bounds = m_View->GetNormalizedBounds();
    
        float* maxWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* minHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* maxHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* wheights     = (float*)alloca(sizeof(float)*childList.size());
        float* finalHeights = (float*)alloca(sizeof(float)*childList.size());

        float  totalWheight = 0.0f;
        float  vMinHeight = 0.0f;
        float  vMaxHeight = 0.0f;

        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Ptr<View> child = childList[i];
            
            Rect borders = child->GetBorders();
            Point borderSize(borders.left + borders.right, borders.top + borders.bottom);
            
            Point minSize = child->GetPreferredSize(PrefSizeType::Smallest);
            Point maxSize = child->GetPreferredSize(PrefSizeType::Greatest);

            minSize += borderSize;
            maxSize += borderSize;

            wheights[i]   = child->GetWheight();
            maxWidths[i]  = maxSize.x;
            minHeights[i] = minSize.y;
            maxHeights[i] = maxSize.y;

            vMinHeight += minSize.y;
            vMaxHeight += maxSize.y;
            totalWheight += wheights[i];
        }
    //    if ( vMinHeight > bounds.Height() + 1.0f ) {
    //      printf( "Error: HLayoutNode::Layout() Width=%.2f, Required width=%.2f\n", bounds.Height() + 1.0f, vMinHeight  );
    //    }
        float vUnusedHeight = SpaceOut(childList.size(), bounds.Height() + 1.0f,  vMinHeight, totalWheight, minHeights, maxHeights, wheights, finalHeights);

        vUnusedHeight /= float(childList.size());
        float y = bounds.top + vUnusedHeight * 0.5f;
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Rect frame( 0.0f, 0.0f, bounds.right, finalHeights[i]);
            if (frame.Width() > maxWidths[i]) {
                frame.right = maxWidths[i];
            }
            float x;
            switch(childList[i]->GetHAlignment())
            {
                case Alignment::Left:   x = bounds.left; break;
                case Alignment::Right:  x = bounds.right - frame.Width() - 1.0f; break;
                default:           printf( "Error: VLayoutNode::Layout() node '%s' has invalid h-alignment %d\n", m_View->GetName().c_str(), int(childList[i]->GetHAlignment()) );
                [[fallthrough]];
                case Alignment::Center: x = bounds.left + (bounds.Width() - frame.Width()) * 0.5f; break;
            }
            frame += Point(x, y);
            
            float height = frame.Height();
            Rect borders = childList[i]->GetBorders();
            frame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
            y += height + vUnusedHeight;
            frame.Floor();
            childList[i]->SetFrame(frame);
        }
    }        
}
