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

#include "sam.h"

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

static void RemBorders( Rect* pcRect, const Rect& cBorders )
{
    pcRect->Resize( cBorders.left, cBorders.top, -cBorders.right, -cBorders.bottom );
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

LayoutNode::LayoutNode(float wheight) :
    m_Borders(0.0f,0.0f,0.0f,0.0f), m_MinSize(0.0f,0.0f), m_MaxSizeExtend(0.0f,0.0f), m_MaxSizeLimit(LAYOUT_MAX_SIZE,LAYOUT_MAX_SIZE)

{
    m_HAlign = Alignment::Center;
    m_VAlign = Alignment::Center;
    
    m_Wheight = wheight;
    
    m_WidthRing.m_Next  = this;
    m_WidthRing.m_Prev  = this;
    m_HeightRing.m_Next = this;
    m_HeightRing.m_Prev = this;
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
        Rect frame(m_View->GetNormalizedBounds());

        frame.Resize(m_Borders.left, m_Borders.top, -m_Borders.right, -m_Borders.bottom);
    
        for (Ptr<View> child : *m_View) {
            child->SetFrame(frame);
        }            
    }
    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::SetBorders(const Rect& border)
{
    m_Borders = border;
    Layout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect LayoutNode::GetBorders() const
{
    return m_Borders;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float LayoutNode::GetWheight() const
{
    return m_Wheight;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::SetWheight(float vWheight)
{
    m_Wheight = vWheight;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::SetHAlignment(Alignment alignment)
{
    m_HAlign = alignment;
    Layout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::SetVAlignment(Alignment alignment)
{
    m_VAlign = alignment;
    Layout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Alignment LayoutNode::GetHAlignment() const
{
    return m_HAlign;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Alignment LayoutNode::GetVAlignment() const
{
    return m_VAlign;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::AdjustPrefSize(Point* pcMinSize, Point* pcMaxSize)
{
    if ( pcMinSize->x < m_MinSize.x ) pcMinSize->x = m_MinSize.x;
    if ( pcMinSize->y < m_MinSize.y ) pcMinSize->y = m_MinSize.y;

    if ( pcMaxSize->x < m_MaxSizeExtend.x ) pcMaxSize->x = m_MaxSizeExtend.x;
    if ( pcMaxSize->y < m_MaxSizeExtend.y ) pcMaxSize->y = m_MaxSizeExtend.y;

    if ( pcMaxSize->x > m_MaxSizeLimit.x ) pcMaxSize->x = m_MaxSizeLimit.x;
    if ( pcMaxSize->y > m_MaxSizeLimit.y ) pcMaxSize->y = m_MaxSizeLimit.y;

    if ( pcMaxSize->x < pcMinSize->x ) pcMaxSize->x = pcMinSize->x;
    if ( pcMaxSize->y < pcMinSize->y ) pcMaxSize->y = pcMinSize->y;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point LayoutNode::CalculatePreferredSize(bool largest)
{
    Point size(0.0f,0.0f);
    if (m_View != nullptr)
    {
        for (Ptr<View> child : *m_View)
        {
            Point currentSize = child->GetPreferredSize(largest);
            if (largest)
            {
                size.x = std::min(size.x, currentSize.x);
                size.y = std::min(size.y, currentSize.y);
            }
            else
            {
                size.x = std::max(size.x, currentSize.x);
                size.y = std::max(size.y, currentSize.y);
            }            
        }
        if (largest)
        {
            size.x = floor(size.x);
            size.y = floor(size.y);
        }
        else
        {
            size.x = ceil(size.x);
            size.y = ceil(size.y);            
        }            
    }
    return size + Point(m_Borders.left + m_Borders.right, m_Borders.top + m_Borders.bottom);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point LayoutNode::GetPreferredSize(bool largest)
{
    if (m_WidthRing.m_Next == this && m_HeightRing.m_Next == this) {
        return CalculatePreferredSize(largest);
    }
    
    Point cMinSize = CalculatePreferredSize(false);
    Point cMaxSize;
    if (m_WidthRing.m_Next == this || m_HeightRing.m_Next == this) {
        cMaxSize = CalculatePreferredSize(true);
    } else {
        cMaxSize = cMinSize;
    }
    Point size = cMinSize;
    if (largest)
    {
        if (m_WidthRing.m_Next == this) {
            size.x = cMaxSize.x;
        }
        if (m_HeightRing.m_Next == this) {
            size.y = cMaxSize.y;
        }
    }
    for (LayoutNode* pcNode = m_WidthRing.m_Next ; pcNode != this ; pcNode = pcNode->m_WidthRing.m_Next)
    {
        Point cSSize = pcNode->CalculatePreferredSize(false);
        if ( cSSize.x > size.x ) {
            size.x = cSSize.x;
        }
    }
    for (LayoutNode* pcNode = m_HeightRing.m_Next ; pcNode != this ; pcNode = pcNode->m_HeightRing.m_Next)
    {
        Point cSSize = pcNode->CalculatePreferredSize(false);
        if ( cSSize.y > size.y ) {
            size.y = cSSize.y;
        }
    }    
    return size;
}


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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::ExtendMinSize( const Point& cMinSize )
{
    m_MinSize = cMinSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::LimitMaxSize( const Point& cMaxSize )
{
    m_MaxSizeLimit = cMaxSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::ExtendMaxSize( const Point& cMaxSize )
{
    m_MaxSizeExtend = cMaxSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::AddToWidthRing( LayoutNode* ring )
{
    m_WidthRing.m_Next = ring;
    m_WidthRing.m_Prev = ring->m_WidthRing.m_Prev;
    ring->m_WidthRing.m_Prev->m_WidthRing.m_Next = this;
    ring->m_WidthRing.m_Prev = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutNode::AddToHeightRing(LayoutNode* ring)
{
    m_HeightRing.m_Next = ring;
    m_HeightRing.m_Prev = ring->m_HeightRing.m_Prev;
    ring->m_HeightRing.m_Prev->m_HeightRing.m_Next = this;
    ring->m_HeightRing.m_Prev = this;
}
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

static float SpaceOut(uint nCount, float vTotalSize, float vTotalMinSize, float vTotalWheight,
                      float* minSizes, float* maxSizes, float* wheights, float* finalSizes)
{
    float vExtraSpace = vTotalSize - vTotalMinSize;
    
    bool*  abDone = (bool*)alloca(sizeof(bool) * nCount);

    memset(abDone, 0, sizeof(bool) * nCount);

    for (;;)
    {
        uint i;
        for ( i = 0 ; i < nCount ; ++i )
        {
            if ( abDone[i] ) {
                continue;
            }
            float vWeight = wheights[i] / vTotalWheight;

            finalSizes[i] = minSizes[i] + vExtraSpace * vWeight;
            if (finalSizes[i] >= maxSizes[i])
            {
                vExtraSpace    -= maxSizes[i] - minSizes[i];
                vTotalWheight  -= wheights[i];
                finalSizes[i] = maxSizes[i];
                abDone[i] = true;
                break;
            }
        }
        if ( i == nCount ) {
            break;
        }
    }
    for ( uint i = 0 ; i < nCount ; ++i ) {
        vTotalSize -= finalSizes[i];
    }
    return vTotalSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HLayoutNode::HLayoutNode(float vWheight) : LayoutNode(vWheight)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point HLayoutNode::CalculatePreferredSize(bool largest)
{
    Rect  borders( GetBorders() );
    Point size( 0.0f, 0.0f );

    if (m_View != nullptr)
    {
        for (Ptr<View> child : *m_View)
        {
            Point childSize = child->GetPreferredSize(largest);
            size.x += childSize.x;
            if (childSize.y > size.y) {
                size.y = childSize.y;
            }
        }
    }
    size += Point(borders.left + borders.right, borders.top + borders.bottom);
    return size;
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

        RemBorders(&bounds, GetBorders());
    
        float* minWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* maxWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* maxHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* wheights     = (float*)alloca(sizeof(float)*childList.size());
        float* finalHeights = (float*)alloca(sizeof(float)*childList.size());
            
        float  vMinWidth = 0.0f;
        float  vTotalWheight = 0.0f;
    
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Ptr<View> child = childList[i];
            
            Point cMinSize = child->GetPreferredSize(false);
            Point cMaxSize = child->GetPreferredSize(true);

            child->AdjustPrefSize(&cMinSize, &cMaxSize);
            wheights[i]   = child->GetWheight();

            maxHeights[i] = cMaxSize.y;
            maxWidths[i]  = cMaxSize.x;
            minWidths[i]  = cMinSize.x;

            vMinWidth += cMinSize.x;
            vTotalWheight += wheights[i];
        }
    //    if ( vMinWidth > bounds.Width() + 1.0f ) {
    //      printf( "Error: HLayoutNode::Layout() Width=%.2f, Required width=%.2f\n", bounds.Width() + 1.0f, vMinWidth  );
    //    }
        float unusedWidth = SpaceOut(childList.size(), bounds.Width() + 1.0f, vMinWidth, vTotalWheight, minWidths, maxWidths, wheights, finalHeights);

//        printf("HLayout: Unused space: %f (%f)\n", unusedWidth, bounds.Width());
        unusedWidth /= float(childList.size());
        float x = bounds.left + unusedWidth * 0.5f;
    
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Rect frame(0.0f, 0.0f, finalHeights[i] - 1.0f, bounds.bottom);
            if (frame.Height() + 1.0f > maxHeights[i]) {
                frame.bottom = maxHeights[i] - 1.0f;
            }
            frame += Point( x, bounds.top + (bounds.Height() - frame.Height()) * 0.5f);
            x += frame.Width() + 1.0f + unusedWidth;
            frame.Floor();
//            printf("    %d: %.2f->%.2f\n", i, frame.left, frame.right);
            childList[i]->SetFrame(frame);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

VLayoutNode::VLayoutNode(float vWheight) : LayoutNode(vWheight)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point VLayoutNode::CalculatePreferredSize(bool largest)
{
    Rect  borders(GetBorders());
    Point size(0.0f, 0.0f);
    
    if (m_View != nullptr)
    {
        for (Ptr<View> child : *m_View)
        {
            Point childSize = child->GetPreferredSize(largest);
            size.y += childSize.y;
            if (childSize.x > size.x) {
                size.x = childSize.x;
            }
        }
    }        
    size += Point(borders.left + borders.right, borders.top + borders.bottom);
    return size;
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

        RemBorders(&bounds, GetBorders());
    
        float* maxWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* minHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* maxHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* wheights     = (float*)alloca(sizeof(float)*childList.size());
        float* finalHeights = (float*)alloca(sizeof(float)*childList.size());

        float  vTotalWheight = 0.0f;
        float  vMinHeight = 0.0f;
        float  vMaxHeight = 0.0f;

        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Ptr<View> child = childList[i];
            Point cMinSize = child->GetPreferredSize(false);
            Point cMaxSize = child->GetPreferredSize(true);

            child->AdjustPrefSize( &cMinSize, &cMaxSize );

            wheights[i]   = child->GetWheight();
            maxWidths[i]  = cMaxSize.x;
            minHeights[i] = cMinSize.y;
            maxHeights[i] = cMaxSize.y;

            vMinHeight += cMinSize.y;
            vMaxHeight += cMaxSize.y;
            vTotalWheight += wheights[i];
        }
    //    if ( vMinHeight > bounds.Height() + 1.0f ) {
    //      printf( "Error: HLayoutNode::Layout() Width=%.2f, Required width=%.2f\n", bounds.Height() + 1.0f, vMinHeight  );
    //    }
        float vUnusedHeight = SpaceOut(childList.size(), bounds.Height() + 1.0f,  vMinHeight, vTotalWheight, minHeights, maxHeights, wheights, finalHeights);

        vUnusedHeight /= float(childList.size());
        float y = bounds.top + vUnusedHeight * 0.5f;
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Rect frame( 0.0f, 0.0f, bounds.right, finalHeights[i] - 1.0f );
            if (frame.Width() + 1.0f > maxWidths[i]) {
                frame.right = maxWidths[i] - 1.0f;
            }
            float x;
            switch(GetHAlignment())
            {
                case Alignment::Left:   x = bounds.left; break;
                case Alignment::Right:  x = bounds.right - frame.Width() - 1.0f; break;
                default:           printf( "Error: VLayoutNode::Layout() node '%s' has invalid h-alignment %d\n", m_View->GetName().c_str(), int(GetHAlignment()) );
                case Alignment::Center: x = bounds.left + (bounds.Width() - frame.Width()) * 0.5f; break;
            }
            frame += Point(x, y);
            y += frame.Height() + 1.0f + vUnusedHeight;
            frame.Floor();
            childList[i]->SetFrame(frame);
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

LayoutSpacer::LayoutSpacer(float vWheight, const Point& cMinSize, const Point& cMaxSize) : LayoutNode(vWheight)
{
    m_MinSize = cMinSize;
    m_cMaxSize = cMaxSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutSpacer::SetMinSize(const Point& size)
{
    m_MinSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void LayoutSpacer::SetMaxSize(const Point& size)
{
    m_cMaxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point LayoutSpacer::CalculatePreferredSize( bool bLargest )
{
    return( (bLargest) ? m_cMaxSize : m_MinSize );
}

