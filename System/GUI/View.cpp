// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 30.01.2014 23:46:08

#include "System/Platform.h"


#include <algorithm>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <pugixml/src/pugixml.hpp>

#include <GUI/View.h>
#include <GUI/ScrollBar.h>
#include <GUI/Bitmap.h>
#include <App/Application.h>
#include <Utils/Utils.h>
#include <Utils/XMLFactory.h>
#include <Utils/XMLObjectParser.h>

using namespace kernel;
using namespace os;

const std::map<String, uint32_t> ViewFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, FullUpdateOnResizeH),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, FullUpdateOnResizeV),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, FullUpdateOnResize),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, WillDraw),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, Transparent),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, ClearBackground),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, DrawOnChildren),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, Eavesdropper),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, IgnoreMouse),
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, ForceHandleMouse)
};

static Color g_DefaultColors[] =
{
    Color(0xaa, 0xaa, 0xaa, 0xff),  // NORMAL
    Color(0xff, 0xff, 0xff, 0xff),  // SHINE
    Color(0x00, 0x00, 0x00, 0xff),  // SHADOW
    Color(0x66, 0x88, 0xbb, 0xff),  // SEL_WND_BORDER
    Color(0x78, 0x78, 0x78, 0xff),  // NORMAL_WND_BORDER
    Color(0x00, 0x00, 0x00, 0xff),  // MENU_TEXT
    Color(0x00, 0x00, 0x00, 0xff),  // SEL_MENU_TEXT
    Color(0xcc, 0xcc, 0xcc, 0xff),  // MENU_BACKGROUND
    Color(0x66, 0x88, 0xbb, 0xff),  // SEL_MENU_BACKGROUND
    Color(0x78, 0x78, 0x78, 0xff),  // SCROLLBAR_BG
    Color(0xaa, 0xaa, 0xaa, 0xff),  // SCROLLBAR_KNOB
    Color(0x78, 0x78, 0x78, 0xff),  // LISTVIEW_TAB
    Color(0xff, 0xff, 0xff, 0xff)   // LISTVIEW_TAB_TEXT
};

///////////////////////////////////////////////////////////////////////////////
/// Get the value of one of the standard system colors.
/// \par Description:
///     Call this function to obtain one of the user-configurable
///     system colors. This should be used whenever possible instead
///     of hard-coding colors to make it possible for the user to
///     customize the look.
/// \param
///     colorID - One of the entries from the StandardColorID enum.
/// \return
///     The current color for the given system pen.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Color os::get_standard_color(StandardColorID colorID)
{
    uint32_t index = uint32_t(colorID);
    if (index < ARRAY_COUNT(g_DefaultColors)) {
        return g_DefaultColors[index];
    } else {
        return g_DefaultColors[int32_t(StandardColorID::NORMAL)];
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void os::set_standard_color(StandardColorID colorID, Color color)
{
    uint32_t index = uint32_t(colorID);
    if (index < ARRAY_COUNT(g_DefaultColors)) {
        g_DefaultColors[index] = color;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static Color Tint(const Color& color, float tint)
{
    int r = int( (float(color.GetRed()) * tint + 127.0f * (1.0f - tint)) );
    int g = int( (float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)) );
    int b = int( (float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)) );
    if ( r < 0 ) r = 0; else if (r > 255) r = 255;
    if ( g < 0 ) g = 0; else if (g > 255) g = 255;
    if ( b < 0 ) b = 0; else if (b > 255) b = 255;
    return Color(uint8_t(r), uint8_t(g), uint8_t(b), color.GetAlpha());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::View(const String& name, Ptr<View> parent, uint32_t flags) : ViewBase(name, Rect(), Point(), flags, 0, get_standard_color(StandardColorID::NORMAL), get_standard_color(StandardColorID::NORMAL), Color(0))
{
    Initialize();
    if (parent != nullptr) {
        parent->AddChild(ptr_tmp_cast(this));
    }
//    SetFont(ptr_new<Font>(GfxDriver::e_Font7Seg));
}

template< typename T>
static SizeOverride GetSizeOverride(pugi::xml_attribute absoluteAttr, pugi::xml_attribute limitAttr, pugi::xml_attribute extendAttr, T& outValue)
{
    if (!absoluteAttr.empty() && xml_object_parser::parse(absoluteAttr.value(), outValue)) {
        return SizeOverride::Always;
    } else if (!limitAttr.empty() && xml_object_parser::parse(limitAttr.value(), outValue)) {
        return SizeOverride::Limit;
    } else if (!extendAttr.empty() && xml_object_parser::parse(extendAttr.value(), outValue)) {
        return SizeOverride::Extend;
    }
    return SizeOverride::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::View(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData)
    : ViewBase(xmlData.attribute("name").value(),
               Rect(),
               Point(),
               context->GetFlagsAttribute<uint32_t>(xmlData, ViewFlags::FlagMap, "flags", 0),
               0,
               get_standard_color(StandardColorID::NORMAL),
               get_standard_color(StandardColorID::NORMAL),
               Color(0))
{
    Initialize();

    SetHAlignment(context->GetAttribute(xmlData, "h_alignment", Alignment::Center));
    SetVAlignment(context->GetAttribute(xmlData, "v_alignment", Alignment::Center));

	Point sizeOverride;
	Point sizeSmallestOverride;
    Point sizeGreatestOverride;
	SizeOverride overrideType;
    SizeOverride overrideTypeSmallestH;
	SizeOverride overrideTypeGreatestH;
	SizeOverride overrideTypeSmallestV;
	SizeOverride overrideTypeGreatestV;

    overrideType = GetSizeOverride(context->GetAttribute(xmlData, "size"), context->GetAttribute(xmlData, "size_limit"), context->GetAttribute(xmlData, "size_extend"), sizeOverride);
    overrideTypeSmallestH = overrideTypeGreatestH = overrideTypeSmallestV = overrideTypeGreatestV = overrideType;
    sizeSmallestOverride = sizeGreatestOverride = sizeOverride;

    float value;
	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "width"), context->GetAttribute(xmlData, "width_limit"), context->GetAttribute(xmlData, "width_extend"), value);
    if (overrideType != SizeOverride::None) {
        sizeSmallestOverride.x = sizeGreatestOverride.x = value;
        overrideTypeSmallestH = overrideTypeGreatestH = overrideType;
    }
	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "height"), context->GetAttribute(xmlData, "height_limit"), context->GetAttribute(xmlData, "height_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeSmallestOverride.y = sizeGreatestOverride.y = value;
		overrideTypeSmallestV = overrideTypeGreatestV = overrideType;
	}

	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "min_size"), context->GetAttribute(xmlData, "min_size_limit"), context->GetAttribute(xmlData, "min_size_extend"), sizeOverride);
    if (overrideType != SizeOverride::None) {
        overrideTypeSmallestH = overrideTypeSmallestV = overrideType;
        sizeSmallestOverride = sizeOverride;
    }
	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "min_width"), context->GetAttribute(xmlData, "min_width_limit"), context->GetAttribute(xmlData, "min_width_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeSmallestOverride.y = value;
		overrideTypeSmallestH = overrideType;
	}
	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "min_height"), context->GetAttribute(xmlData, "min_height_limit"), context->GetAttribute(xmlData, "min_height_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeSmallestOverride.y = value;
		overrideTypeSmallestV = overrideType;
	}


	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "max_size"), context->GetAttribute(xmlData, "max_size_limit"), context->GetAttribute(xmlData, "max_size_extend"), sizeOverride);
	if (overrideType != SizeOverride::None) {
		overrideTypeGreatestH = overrideTypeGreatestV = overrideType;
		sizeGreatestOverride = sizeOverride;
	}
	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "max_width"), context->GetAttribute(xmlData, "max_width_limit"), context->GetAttribute(xmlData, "max_width_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeGreatestOverride.y = value;
		overrideTypeGreatestH = overrideType;
	}
	overrideType = GetSizeOverride(context->GetAttribute(xmlData, "max_height"), context->GetAttribute(xmlData, "max_height_limit"), context->GetAttribute(xmlData, "max_height_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeGreatestOverride.y = value;
		overrideTypeGreatestV = overrideType;
	}

    if (overrideTypeSmallestH != SizeOverride::None) {
		SetWidthOverride(PrefSizeType::Smallest, overrideTypeSmallestH, sizeSmallestOverride.x);
    }
	if (overrideTypeGreatestH != SizeOverride::None) {
		SetWidthOverride(PrefSizeType::Greatest, overrideTypeGreatestH, sizeGreatestOverride.x);
	}
	if (overrideTypeSmallestV != SizeOverride::None) {
		SetHeightOverride(PrefSizeType::Smallest, overrideTypeSmallestV, sizeSmallestOverride.y);
	}
	if (overrideTypeGreatestV != SizeOverride::None) {
		SetHeightOverride(PrefSizeType::Greatest, overrideTypeGreatestV, sizeGreatestOverride.y);
	}

    SetLayoutNode(context->GetAttribute(xmlData, "layout", Ptr<LayoutNode>()));
    m_Borders = context->GetAttribute(xmlData, "layout_borders", Rect(0.0f));

    String widthGroupName = context->GetAttribute(xmlData, "width_group", String::zero);
	String heightGroupName = context->GetAttribute(xmlData, "height_group", String::zero);

    if (!widthGroupName.empty())
    {
		auto i = context->m_WidthRings.find(widthGroupName);
		if (i != context->m_WidthRings.end()) {
			AddToWidthRing(i->second);
		} else {
            context->m_WidthRings[widthGroupName] = ptr_tmp_cast(this);
		}
    }

	if (!heightGroupName.empty())
	{
		auto i = context->m_HeightRings.find(heightGroupName);
		if (i != context->m_HeightRings.end()) {
			AddToHeightRing(i->second);
		} else {
			context->m_HeightRings[heightGroupName] = ptr_tmp_cast(this);
		}
	}

    if (parent != nullptr) {
        parent->AddChild(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::View(Ptr<View> parent, handler_id serverHandle, const String& name, const Rect& frame) : ViewBase(name, frame, Point(), ViewFlags::Eavesdropper | ViewFlags::WillDraw, 0, Color(0xffffffff), Color(0xffffffff), Color(0))
{
    Initialize();
    m_ServerHandle = serverHandle;
    if (parent != nullptr) {
        parent->AddChild(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Initialize()
{
    RegisterRemoteSignal(&RSPaintView, &View::HandlePaint);
    RegisterRemoteSignal(&RSViewFrameChanged, &View::SetFrame);
    RegisterRemoteSignal(&RSHandleMouseDown, &View::SlotHandleMouseDown);
    RegisterRemoteSignal(&RSHandleMouseUp, &View::HandleMouseUp);
    RegisterRemoteSignal(&RSHandleMouseMove, &View::HandleMouseMove);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::~View()
{
    if (m_pcVScrollBar != nullptr) {
        m_pcVScrollBar->SetScrollTarget(nullptr);
    }
    if (m_pcHScrollBar != nullptr) {
        m_pcHScrollBar->SetScrollTarget(nullptr);
    }
    SetLayoutNode(nullptr);
    RemoveFromWidthRing();
    RemoveFromHeightRing();
       
    while(!m_ChildrenList.empty())
    {
        RemoveChild(m_ChildrenList[m_ChildrenList.size()-1]);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application* View::GetApplication() const
{
    Looper* looper = GetLooper();
    if (looper != nullptr) {
        return static_cast<Application*>(looper);
    } else {
        Ptr<const View> parent = GetParent();
        if (parent != nullptr) {
            return parent->GetApplication();
        } else {
            return nullptr;
        }
    }

    return static_cast<Application*>(GetLooper());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<LayoutNode> View::GetLayoutNode() const
{
    return m_LayoutNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetLayoutNode(Ptr<LayoutNode> node)
{
    if (m_LayoutNode != nullptr) 
    {
        Ptr<LayoutNode> layoutNode = m_LayoutNode;
        m_LayoutNode = nullptr;
        layoutNode->AttachedToView(nullptr);
    }
    m_LayoutNode = node;
    if (m_LayoutNode != nullptr) {
        m_LayoutNode->AttachedToView(this);
    }
}    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetBorders(const Rect& border)
{
    m_Borders = border;
    Ptr<View> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect View::GetBorders() const
{
    return m_Borders;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float View::GetWheight() const
{
    return m_Wheight;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetWheight(float vWheight)
{
    m_Wheight = vWheight;
    Ptr<View> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetHAlignment(Alignment alignment)
{
    m_HAlign = alignment;
    Ptr<View> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetVAlignment(Alignment alignment)
{
    m_VAlign = alignment;
    Ptr<View> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Alignment View::GetHAlignment() const
{
    return m_HAlign;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Alignment View::GetVAlignment() const
{
    return m_VAlign;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetWidthOverride(PrefSizeType sizeType, SizeOverride when, float size)
{
    if (sizeType == PrefSizeType::Smallest || sizeType == PrefSizeType::Greatest) {
        m_WidthOverride[int(sizeType)]     = size;
        m_WidthOverrideType[int(sizeType)] = when;
    } else if (sizeType == PrefSizeType::All) {
        m_WidthOverride[int(PrefSizeType::Smallest)]     = size;
        m_WidthOverrideType[int(PrefSizeType::Smallest)] = when;
        
        m_WidthOverride[int(PrefSizeType::Greatest)]     = size;
        m_WidthOverrideType[int(PrefSizeType::Greatest)] = when;        
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetHeightOverride(PrefSizeType sizeType, SizeOverride when, float size)
{
    if (sizeType == PrefSizeType::Smallest || sizeType == PrefSizeType::Greatest) {
        m_HeightOverride[int(sizeType)]     = size;
        m_HeightOverrideType[int(sizeType)] = when;
    } else if (sizeType == PrefSizeType::All) {
        m_HeightOverride[int(PrefSizeType::Smallest)]     = size;
        m_HeightOverrideType[int(PrefSizeType::Smallest)] = when;
        
        m_HeightOverride[int(PrefSizeType::Greatest)]     = size;
        m_HeightOverrideType[int(PrefSizeType::Greatest)] = when;        
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::AddToWidthRing(Ptr<View> ring)
{
    if (m_WidthRing != nullptr) {
        RemoveFromWidthRing();
    }
    m_WidthRing = (ring->m_WidthRing != nullptr) ? ring->m_WidthRing : ptr_raw_pointer_cast(ring);
    ring->m_WidthRing = this;
    UpdateRingSize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::RemoveFromWidthRing()
{
    if (m_WidthRing != nullptr)
    {
        for (View* i = m_WidthRing;; i = i->m_WidthRing)
        {
            if (i->m_WidthRing == this)
            {
                i->m_WidthRing = (m_WidthRing != i) ? m_WidthRing : nullptr;
                m_WidthRing = nullptr;
                UpdateRingSize();
                i->UpdateRingSize();
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::AddToHeightRing(Ptr<View> ring)
{
    RemoveFromHeightRing();
    m_HeightRing = (ring->m_HeightRing != nullptr) ? ring->m_HeightRing : ptr_raw_pointer_cast(ring);
    ring->m_HeightRing = this;    
    UpdateRingSize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::RemoveFromHeightRing()
{
    if (m_HeightRing != nullptr)
    {
        for (View* i = m_HeightRing;; i = i->m_HeightRing)
        {
            if (i->m_HeightRing == this)
            {
                i->m_HeightRing = (m_HeightRing != i) ? m_HeightRing : nullptr;
                m_HeightRing = nullptr;
                UpdateRingSize();
                i->UpdateRingSize();
                break;
            }
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::InvalidateLayout()
{
    if (m_IsLayoutValid)
    {
        Application* app = GetApplication();
    
        if (app != nullptr)
        {
            Ptr<View> i = ptr_tmp_cast(this);
            for (;;)
            {
                if (!i->m_IsLayoutValid) {
                    break;
                }
                Ptr<View> parent = i->GetParent();
                if (parent == nullptr) {
                    app->RegisterViewForLayout(i);
                    break;
                }
                i = parent;
            }
        }
        m_IsLayoutValid = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::UpdateLayout()
{
    if (!m_IsLayoutValid)
    {
        if (m_LayoutNode != nullptr)
        {
            m_LayoutNode->Layout();
        }
        m_IsLayoutValid = true;
    }    
    for (Ptr<View> child : *this) {
        child->UpdateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (!VFMouseDown.Empty()) {
        return VFMouseDown(button, position, event);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (!VFMouseUp.Empty()) {
        return VFMouseUp(button, position, event);
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (!VFMouseMoved.Empty()) {
        return VFMouseMoved(position, event);
    } else {
        return false;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::FrameMoved(const Point& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::FrameSized(const Point& delta)
{
    if (m_LayoutNode != nullptr) {
        InvalidateLayout();
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void  View::ScreenFrameMoved(const Point& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ViewScrolled(const Point& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::FontChanged(Ptr<Font> newFont)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    if (m_LayoutNode != nullptr) {
        m_LayoutNode->CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    } else {
        minSize->x = 0.0f;
        minSize->y = 0.0f;
        maxSize->x = LAYOUT_MAX_SIZE;
        maxSize->y = LAYOUT_MAX_SIZE;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point View::GetPreferredSize(PrefSizeType sizeType) const
{
    return m_PreferredSizes[int(sizeType)];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point View::GetContentSize() const
{
    return Point(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::PreferredSizeChanged()
{
    Point sizes[int(PrefSizeType::Count)];
    
    bool includeWidth  = m_WidthOverrideType[int(PrefSizeType::Smallest)] != SizeOverride::Always || m_WidthOverrideType[int(PrefSizeType::Greatest)] != SizeOverride::Always;
    bool includeHeight = m_HeightOverrideType[int(PrefSizeType::Smallest)] != SizeOverride::Always || m_HeightOverrideType[int(PrefSizeType::Greatest)] != SizeOverride::Always;
    
    CalculatePreferredSize(&sizes[int(PrefSizeType::Smallest)], &sizes[int(PrefSizeType::Greatest)], includeWidth, includeHeight);
    
    for (int i = 0; i < int(PrefSizeType::Count); ++i)
    {
        switch(m_WidthOverrideType[i])
        {
            case SizeOverride::None: break;
            case SizeOverride::Always:  sizes[i].x = m_WidthOverride[i]; break;
            case SizeOverride::Extend:  sizes[i].x = std::max(sizes[i].x, m_WidthOverride[i]); break;
            case SizeOverride::Limit:   sizes[i].x = std::min(sizes[i].x, m_WidthOverride[i]); break;
        }
        switch(m_HeightOverrideType[i])
        {
            case SizeOverride::None: break;
            case SizeOverride::Always:  sizes[i].y = m_HeightOverride[i]; break;
            case SizeOverride::Extend:  sizes[i].y = std::max(sizes[i].y, m_HeightOverride[i]); break;
            case SizeOverride::Limit:   sizes[i].y = std::min(sizes[i].y, m_HeightOverride[i]); break;
        }
        if (sizes[i].x > LAYOUT_MAX_SIZE) sizes[i].x = LAYOUT_MAX_SIZE;
        if (sizes[i].y > LAYOUT_MAX_SIZE) sizes[i].y = LAYOUT_MAX_SIZE;
    }
    
    if (sizes[int(PrefSizeType::Smallest)] != m_LocalPrefSize[int(PrefSizeType::Smallest)] || sizes[int(PrefSizeType::Greatest)] != m_LocalPrefSize[int(PrefSizeType::Greatest)])
    {
        m_LocalPrefSize[int(PrefSizeType::Smallest)] = sizes[int(PrefSizeType::Smallest)];
        m_LocalPrefSize[int(PrefSizeType::Greatest)] = sizes[int(PrefSizeType::Greatest)];
        
        UpdateRingSize();
        SignalPreferredSizeChanged(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ContentSizeChanged()
{
    SignalContentSizeChanged(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::AddChild(Ptr<View> child)
{
    LinkChild(child, true);
    if (HasFlags(ViewFlags::IsAttachedToScreen)) {
        child->PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::RemoveChild(Ptr<View> child)
{
    UnlinkChild(child);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> View::RemoveChild(ChildList_t::iterator iterator)
{
    return UnlinkChild(iterator);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::RemoveThis()
{
    Ptr<View> parent = m_Parent.Lock();
    if (parent != nullptr)
    {
        parent->RemoveChild(ptr_tmp_cast(this));
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> View::GetChildAt(const Point& pos)
{
    for (Ptr<View> child : reverse_ranged(*this))
    {
        if (child->GetFrame().DoIntersect(pos)) {
            return child;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> View::GetChildAt(size_t index)
{
    if (index < m_ChildrenList.size()) {
        return m_ChildrenList[index];
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ScrollBar> View::GetVScrollBar() const
{
    return ptr_tmp_cast(m_pcVScrollBar);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ScrollBar> View::GetHScrollBar() const
{
    return ptr_tmp_cast(m_pcHScrollBar);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::MakeFocus(MouseButton_e button, bool focus)
{
    Application* app = GetApplication();
    if (app != nullptr) {
        app->SetFocusView(button, ptr_tmp_cast(this), focus);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::HasFocus(MouseButton_e button) const
{
    const Application* app = GetApplication();
    return app != nullptr && ptr_raw_pointer_cast(app->GetFocusView(button)) == this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetFrame(const Rect& frame)
{
    Point deltaSize = frame.Size() - m_Frame.Size();
    Point deltaPos  = frame.TopLeft() - m_Frame.TopLeft();
    m_Frame = frame;

    UpdatePosition(true);
    if (deltaSize != Point(0.0f, 0.0f))
    {
        FrameSized(deltaSize);
        SignalFrameSized(deltaSize, ptr_tmp_cast(this));
    }
    if (deltaPos != Point(0.0f, 0.0f))
    {
        FrameMoved(deltaPos);
        SignalFrameMoved(deltaSize, ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ResizeBy(const Point& delta)
{
    SetFrame(Rect(m_Frame.left, m_Frame.top, m_Frame.right + delta.x, m_Frame.bottom + delta.y));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ResizeBy(float deltaW, float deltaH)
{
    SetFrame(Rect(m_Frame.left, m_Frame.top, m_Frame.right + deltaW, m_Frame.bottom + deltaH));
}

///////////////////////////////////////////////////////////////////////////////
/// Set a new absolute size for the view.
/// \par Description:
///     See ResizeTo(float,float)
/// \param size
///     New size (os::Point::x is width, os::Point::y is height).
/// \sa ResizeTo(float,float)
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ResizeTo(const Point& size)
{
    SetFrame(Rect(m_Frame.left, m_Frame.top, m_Frame.left + size.x, m_Frame.top + size.y));
}

///////////////////////////////////////////////////////////////////////////////
///* Set a new absolute size for the view.
/// \par Description:
///	    Move the bottom/right corner of the view to give it
///	    the new size. The top/left corner will not be affected.
///	    This will cause FrameSized() to be called and any child
///	    views to be resized according to their resize flags if
///	    the new size differ from the previous.
/// \note
///	    Coordinates start in the middle of a pixel and all rectangles
///	    are inclusive so the real width and height in pixels will be
///	    1 larger than the value given in \p w and \p h.
/// \param w
///	    New width
/// \param h
///	    New height.
/// \return
/// \par Error codes:
/// \sa ResizeTo(const Point&), ResizeBy(), SetFrame()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ResizeTo(float w, float h)
{
    SetFrame(Rect(m_Frame.left, m_Frame.top, m_Frame.left + w, m_Frame.top + h));
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Invalidate(const Rect& rect, bool recurse)
{
    if (m_ServerHandle != INVALID_HANDLE) {
        Post<ASViewInvalidate>(IRect(rect));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Invalidate(bool recurse)
{
    Invalidate(GetBounds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetDrawingMode(DrawingMode mode)
{
    if (mode != m_DrawingMode)
    {
        m_DrawingMode = mode;
        Post<ASViewSetDrawingMode>(m_DrawingMode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DrawingMode View::GetDrawingMode() const
{
    return m_DrawingMode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetFont(Ptr<Font> font)
{
    m_Font = font;
    Post<ASViewSetFont>(int(font->Get()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
Ptr<Font> View::GetFont() const
{
    return m_Font;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::SlotHandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    for (Ptr<View> child : reverse_ranged(m_ChildrenList))
    {
        if (!child->HasFlags(ViewFlags::IgnoreMouse) && child->m_Frame.DoIntersect(position))
        {
            Point childPos = position - child->m_Frame.TopLeft() - child->m_ScrollOffset;
            return child->SlotHandleMouseDown(button, childPos, event);
        }
    }
    return HandleMouseDown(button, position, event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    bool handled;
    if (button < MouseButton_e::FirstTouchID) {
        handled = OnMouseDown(button, position, event);
    } else {
        handled = OnTouchDown(button, position, event);
    }
    if (handled)
    {
        Application* app = GetApplication();
        if (app != nullptr) {
            app->SetMouseDownView(button, ptr_tmp_cast(this));
        }
    }
    else
    {
        Ptr<View> parent = GetParent();

        if (parent != nullptr)
        {
            // Event not handled by us. Check if the mouse hit any overlapping siblings below us.
            Point parentPos = ConvertToParent(position);
            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->rend())
            {
                ++i;
                if (i != parent->rend())
                {
                    for (; i != parent->rend(); ++i)
                    {
                        Ptr<View> sibling = *i;
                        if (!sibling->HasFlags(ViewFlags::IgnoreMouse) && sibling->GetFrame().DoIntersect(parentPos))
                        {
                            return sibling->HandleMouseDown(button, sibling->ConvertFromParent(parentPos), event);
                        }
                    }
                }
            }
            // No lower sibling hit, send to parent.
            return parent->HandleMouseDown(button, parentPos, event);
        }
    }
    return handled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    Application* app = GetApplication();
    Ptr<View> mouseView = (app != nullptr) ? app->GetMouseDownView(button) : nullptr;
    if (mouseView != nullptr)
    {
//        printf("View::HandleMouseUp() %p '%s'\n", ptr_raw_pointer_cast(mouseView), mouseView->GetName().c_str());

        if (button < MouseButton_e::FirstTouchID) {
            mouseView->OnMouseUp(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), event);
        } else {
            mouseView->OnTouchUp(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), event);
        }
    }
    else
    {
        OnMouseUp(button, position, event);
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    Application* app = GetApplication();
    Ptr<View> mouseView = (app != nullptr) ? app->GetFocusView(button) : nullptr;
    if (mouseView != nullptr) {
        if (button < MouseButton_e::FirstTouchID) {
            mouseView->OnMouseMove(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), event);
        } else {
            mouseView->OnTouchMove(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), event);
        }
    } else {
        if (button < MouseButton_e::FirstTouchID) {
            OnMouseMove(button, position, event);
        } else {
            OnTouchMove(button, position, event);
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ScrollBy(const Point& offset)
{
    if (offset.x == 0.0f && offset.y == 0.0f) {
        return;
    }

    m_ScrollOffset += offset;
    UpdatePosition(false);
    Post<ASViewScrollBy>(offset);

    if (offset.x != 0 && m_pcHScrollBar != nullptr) {
        m_pcHScrollBar->SetValue(-m_ScrollOffset.x);
    }
    if (offset.y != 0 && m_pcVScrollBar != nullptr) {
        m_pcVScrollBar->SetValue(-m_ScrollOffset.y);
    }
    ViewScrolled(offset);
    SignalViewScrolled(offset, ptr_tmp_cast(this));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::CopyRect(const Rect& srcRect, const Point& dstPos)
{
    if (m_BeginPainCount != 0) {
        m_DidScrollRect = true;
    }
    Post<ASViewCopyRect>(srcRect, dstPos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::DrawBitmap(Ptr<const Bitmap> bitmap, const Rect& srcRect, const Point& dstPos)
{
    Post<ASViewDrawBitmap>(bitmap->m_Handle, srcRect, dstPos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::DrawFrame( const Rect& rect, uint32_t syleFlags)
{
    Rect frame(rect);
    frame.Floor();
    bool sunken = false;

    if (((syleFlags & FRAME_RAISED) == 0) && (syleFlags & (FRAME_RECESSED))) {
        sunken = true;
    }

    Color fgColor = get_standard_color(StandardColorID::SHINE);
    Color bgColor = get_standard_color(StandardColorID::SHADOW);

    if (syleFlags & FRAME_DISABLED) {
        fgColor = Tint(fgColor, 0.6f);
        bgColor = Tint(bgColor, 0.4f);
    }
    Color fgShadowColor = Tint(fgColor, 0.6f);
    Color bgShadowColor = Tint(bgColor, 0.5f);
  
    if (syleFlags & FRAME_FLAT)
    {
        SetFgColor(sunken ? bgColor : fgColor);
        MovePenTo(frame.left, frame.bottom - 1.0f);
        DrawLine(Point( frame.left, frame.top));
        DrawLine(Point(frame.right - 1.0f, frame.top));
        DrawLine(Point(frame.right - 1.0f, frame.bottom - 1.0f));
        DrawLine(Point(frame.left, frame.bottom - 1.0f));
    }
    else
    {
        if (syleFlags & FRAME_THIN) {
            SetFgColor(sunken ? bgColor : fgColor);
        } else {
            SetFgColor(sunken ? bgColor : fgShadowColor);
        }

        MovePenTo(frame.left, frame.bottom - 1.0f);
        DrawLine(Point(frame.left, frame.top));
        DrawLine(Point(frame.right - 1.0f, frame.top));

        if (syleFlags & FRAME_THIN) {
            SetFgColor(sunken ? fgColor : bgColor);
        } else {
            SetFgColor(sunken ? fgColor : bgShadowColor);
        }
        DrawLine(Point(frame.right - 1.0f, frame.bottom - 1.0f));
        DrawLine(Point(frame.left, frame.bottom - 1.0f));


        if ((syleFlags & FRAME_THIN) == 0)
        {
            if (syleFlags & FRAME_ETCHED)
            {
                SetFgColor(sunken ? bgColor : fgColor);

                MovePenTo(frame.left + 1.0f, frame.bottom - 2.0f);

                DrawLine(Point(frame.left + 1.0f, frame.top + 1.0f));
                DrawLine(Point(frame.right - 2.0f, frame.top + 1.0f));

                SetFgColor(sunken ? fgColor : bgColor);

                DrawLine(Point(frame.right - 2.0f, frame.bottom - 2.0f));
                DrawLine(Point(frame.left + 1.0f, frame.bottom - 2.0f));
            }
            else
            {
                SetFgColor(sunken ? bgShadowColor : fgColor);

                MovePenTo(frame.left + 1.0f, frame.bottom - 2.0f);

                DrawLine(Point(frame.left + 1.0f, frame.top + 1.0f));
                DrawLine(Point(frame.right - 2.0f, frame.top + 1.0f));

                SetFgColor(sunken ? fgShadowColor : bgColor);

                DrawLine(Point(frame.right - 2.0f, frame.bottom - 2.0f));
                DrawLine(Point(frame.left + 1.0f, frame.bottom - 2.0f));
            }
            if ((syleFlags & FRAME_TRANSPARENT) == 0) {
                EraseRect(Rect(frame.left + 2.0f, frame.top + 2.0f, frame.right - 2.0f, frame.bottom - 2.0f));
            }
        }
        else
        {
            if ((syleFlags & FRAME_TRANSPARENT) == 0) {
                EraseRect(Rect(frame.left + 1.0f, frame.top + 1.0f, frame.right - 1.0f, frame.bottom - 1.0f));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FontHeight View::GetFontHeight() const
{
    if (m_Font != nullptr)
    {
        return m_Font->GetHeight();
    }
    else
    {
        FontHeight height;
        height.ascender  = 0.0f;
        height.descender = 0.0f;
        height.line_gap  = 0.0f;
        return height;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float View::GetStringWidth(const char* string, size_t length) const
{
    if (m_Font != nullptr) {
        return m_Font->GetStringWidth(string, length);
    } else {
        return 0.0f;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Flush()
{
    Application* app = GetApplication();
    if (app != nullptr) {
        app->Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Sync()
{
    Application* app = GetApplication();
    if (app != nullptr) {
        app->Sync();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Called from ViewBase::LinkChild() to tell the view that it has been
/// attached to a parent.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleAddedToParent(Ptr<View> parent)
{
    UpdatePosition(false);
    if (parent->HasFlags(ViewFlags::IsAttachedToScreen) && !HasFlags(ViewFlags::Eavesdropper))
    {
        parent->GetApplication()->AddView(ptr_tmp_cast(this), ViewDockType::ChildView);
    }
    /*parent->*/PreferredSizeChanged();
    parent->InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// Called from ViewBase::UnlinkChild() to tell the view that it has been
/// detached from the parent.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void View::HandleRemovedFromParent(Ptr<View> parent)
{
    if (m_ServerHandle != -1 && !HasFlags(ViewFlags::Eavesdropper)) {
        GetApplication()->RemoveView(ptr_tmp_cast(this));
        UpdatePosition(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Called from Application::RemoveView() to tell the view that it has been
/// detached from the screen.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleDetachedFromScreen()
{
    m_ServerHandle = -1;
    DetachedFromScreen();
    for (Ptr<View> child : m_ChildrenList) {
        child->HandleDetachedFromScreen();
    }
    AllDetachedFromScreen();
}    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandlePaint(const Rect& updateRect)
{
    Rect frame = updateRect;
    if (m_BeginPainCount++ == 0) {
        Post<ASViewBeginUpdate>();
    }        
    Paint(frame);
    if (--m_BeginPainCount == 0) {
        Post<ASViewEndUpdate>();
    }        
    if (m_DidScrollRect) {
        m_DidScrollRect = false;
        Sync();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::UpdateRingSize()
{
    if (m_WidthRing == nullptr && m_HeightRing == nullptr)
    {
        if (m_LocalPrefSize[int(PrefSizeType::Smallest)] != m_PreferredSizes[int(PrefSizeType::Smallest)] || m_LocalPrefSize[int(PrefSizeType::Greatest)] != m_PreferredSizes[int(PrefSizeType::Greatest)])
        {
            m_PreferredSizes[int(PrefSizeType::Smallest)] = m_LocalPrefSize[int(PrefSizeType::Smallest)];
            m_PreferredSizes[int(PrefSizeType::Greatest)] = m_LocalPrefSize[int(PrefSizeType::Greatest)];
            Ptr<View> parent = GetParent();
            if (parent != nullptr) {
                parent->PreferredSizeChanged();
                parent->InvalidateLayout();
            }          
        }
    }
    else
    {
        Point ringSizes[int(PrefSizeType::Count)];
        //// Calculate ring with. ////
        if (m_WidthRing == nullptr)
        {
            for (int i = 0; i < int(PrefSizeType::Count); ++i) {
                ringSizes[i].x = m_LocalPrefSize[i].x;
            }
        }
        else
        {
            View* member = this;
            do
            {
                for (int i = 0; i < int(PrefSizeType::Count); ++i)
                {
                    if (member->m_LocalPrefSize[i].x > ringSizes[i].x) ringSizes[i].x = member->m_LocalPrefSize[i].x;
                }
                member = member->m_WidthRing;
            } while (member != this);
        }
        //// Calculate ring height. ////
        if (m_HeightRing == nullptr)
        {
            for (int i = 0; i < int(PrefSizeType::Count); ++i) {
                ringSizes[i].y = m_LocalPrefSize[i].y;
            }
        }
        else
        {
            View* member = this;
            do
            {
                for (int i = 0; i < int(PrefSizeType::Count); ++i)
                {
                    if (member->m_LocalPrefSize[i].y > ringSizes[i].y) ringSizes[i].y = member->m_LocalPrefSize[i].y;
                }
                member = member->m_HeightRing;
            } while (member != this);
        }

        if (ringSizes[int(PrefSizeType::Smallest)] != m_PreferredSizes[int(PrefSizeType::Smallest)] || ringSizes[int(PrefSizeType::Greatest)] != m_PreferredSizes[int(PrefSizeType::Greatest)])
        {
            //// Update members of width ring. ////
            if (m_WidthRing != nullptr)
            {
                View* member = this;
                do
                {
                    member->m_PreferredSizes[int(PrefSizeType::Smallest)] = ringSizes[int(PrefSizeType::Smallest)];
                    member->m_PreferredSizes[int(PrefSizeType::Greatest)] = ringSizes[int(PrefSizeType::Greatest)];
                    member = member->m_WidthRing;
                } while (member != this);
            }
            //// Update members of height ring. ////
            if (m_HeightRing != nullptr)
            {
                View* member = this;
                do
                {
                    member->m_PreferredSizes[int(PrefSizeType::Smallest)] = ringSizes[int(PrefSizeType::Smallest)];
                    member->m_PreferredSizes[int(PrefSizeType::Greatest)] = ringSizes[int(PrefSizeType::Greatest)];
                    member = member->m_HeightRing;
                } while (member != this);
            }
            std::set<View*> notifiedParents;

            //// Notify parents of with ring members. ////
            if (m_WidthRing != nullptr)
            {
                View* member = this;
                do
                {
                    Ptr<View> parent = member->GetParent();
                    if (parent != nullptr && notifiedParents.count(ptr_raw_pointer_cast(parent)) == 0)
                    {
                        notifiedParents.insert(ptr_raw_pointer_cast(parent));
                        parent->PreferredSizeChanged();
                        parent->InvalidateLayout();
                    }
                    member = member->m_WidthRing;
                } while (member != this);
            }
            //// Notify parents of height ring members. ////
            if (m_HeightRing != nullptr)
            {
                View* member = this;
                do
                {
                    Ptr<View> parent = member->GetParent();
                    if (parent != nullptr && notifiedParents.count(ptr_raw_pointer_cast(parent)) == 0)
                    {
                        notifiedParents.insert(ptr_raw_pointer_cast(parent));
                        parent->PreferredSizeChanged();
                        parent->InvalidateLayout();
                    }
                    member = member->m_HeightRing;
                } while (member != this);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetVScrollBar(ScrollBar* scrollBar)
{
    m_pcVScrollBar = scrollBar;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetHScrollBar(ScrollBar* scrollBar)
{
    m_pcHScrollBar = scrollBar;
}

