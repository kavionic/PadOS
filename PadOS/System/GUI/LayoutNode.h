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


#include <vector>

#include "System/Ptr/PtrTarget.h"
#include "System/Math/Rect.h"

namespace os
{
class View;

#define LAYOUT_MAX_SIZE 100000.0f

enum class Alignment
{
    Left,
    Right,
    Top,
    Bottom,
    Center
};

enum class Orientation
{
    Horizontal,
    Vertical
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class LayoutNode : public PtrTarget
{
public:
    LayoutNode(float wheight = 1.0f);
    virtual ~LayoutNode();

    virtual void        Layout();
    
    virtual void        SetBorders(const Rect& border);
    virtual Rect        GetBorders() const;

    float               GetWheight() const;
    void                SetWheight(float wheight);

    void                ExtendMinSize(const Point& minSize);
    void                LimitMaxSize(const Point& maxSize);
    void                ExtendMaxSize(const Point& maxSize);

    void                SetHAlignment(Alignment alignment);
    void                SetVAlignment(Alignment alignment);
    Alignment           GetHAlignment() const;
    Alignment           GetVAlignment() const;
    
    void                AdjustPrefSize(Point* minSize, Point* maxSize);
    virtual Point       GetPreferredSize(bool largest);

//    void SameWidth( const char* pzName1, ... );
//    void SameHeight( const char* pzName1, ... );
    
//    void SetBorders( const Rect& cBorders, const char* pzFirstName, ... );
//    void SetWheights( float vWheight, const char* pzFirstName, ... );
//    void SetHAlignments( Alignment eAlign, const char* pzFirstName, ... );
//    void SetVAlignments( Alignment eAlign, const char* pzFirstName, ... );

    void AddToWidthRing(LayoutNode* ring);
    void AddToHeightRing(LayoutNode* ring);
    
protected:    
    virtual Point    CalculatePreferredSize(bool largest);
    View*            m_View = nullptr;

private:
    friend class View;

    void AttachedToView(View* view);

    struct ShareNode {
        LayoutNode* m_Next;
        LayoutNode* m_Prev;
    };
    Rect      m_Borders;
    float     m_Wheight;
    Point     m_MinSize;
    Point     m_MaxSizeExtend;
    Point     m_MaxSizeLimit;

    Alignment m_HAlign;
    Alignment m_VAlign;

    ShareNode m_WidthRing;
    ShareNode m_HeightRing;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class LayoutSpacer : public LayoutNode
{
public:
    LayoutSpacer(float vWheight = 1.0f, const Point& cMinSize = Point(0.0f,0.0f), const Point& cMaxSize = Point(LAYOUT_MAX_SIZE,LAYOUT_MAX_SIZE));

    void SetMinSize( const Point& cSize );
    void SetMaxSize( const Point& cSize );

private:
    virtual Point CalculatePreferredSize( bool bLargest ) override;

    Point m_MinSize;
    Point m_cMaxSize;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class VLayoutSpacer : public LayoutSpacer
{
public:
    VLayoutSpacer(float vMinHeight = 0.0f, float vMaxHeight = LAYOUT_MAX_SIZE, float vWheight = 1.0f) :
        LayoutSpacer(vWheight, Point( 0.0f, vMinHeight ), Point( 0.0f, vMaxHeight ) )
    {}
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class HLayoutSpacer : public LayoutSpacer
{
public:
    HLayoutSpacer(float vMinWidth = 0.0f, float vMaxWidth = LAYOUT_MAX_SIZE, float vWheight = 1.0f ) :
        LayoutSpacer(vWheight, Point( vMinWidth, 0.0f ), Point( vMaxWidth, 0.0f ) )
    {}
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class HLayoutNode : public LayoutNode
{
public:
    HLayoutNode(float vWheight = 1.0f);
//    virtual Point GetPreferredSize( bool bLargest );
    virtual void  Layout() override;
private:
    virtual Point CalculatePreferredSize( bool bLargest ) override;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class VLayoutNode : public LayoutNode
{
public:
    VLayoutNode(float vWheight = 1.0f);
//    virtual Point GetPreferredSize( bool bLargest );
    virtual void  Layout() override;

protected:
    virtual Point CalculatePreferredSize( bool bLargest ) override;
private:
};




} // end of namespace
