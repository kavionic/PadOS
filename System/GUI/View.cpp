// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
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
#include <GUI/Widgets/ScrollBar.h>
#include <GUI/Bitmap.h>
#include <GUI/NamedColors.h>
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
    DEFINE_FLAG_MAP_ENTRY(ViewFlags, IgnoreWhenHidden),
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
    [int(StandardColorID::None)]                    = Color(0, 0, 0),
    [int(StandardColorID::DefaultBackground)]       = Color(NamedColors::darkgray),
    [int(StandardColorID::Shine)]                   = Color(NamedColors::white),
    [int(StandardColorID::Shadow)]                  = Color(NamedColors::black),
    [int(StandardColorID::WindowBorderActive)]      = Color(NamedColors::steelblue),
    [int(StandardColorID::WindowBorderInactive)]    = Color(NamedColors::slategray),
    [int(StandardColorID::ButtonBackground)]        = Color(NamedColors::darkgray),
    [int(StandardColorID::ButtonLabelNormal)]       = Color(NamedColors::black),
    [int(StandardColorID::ButtonLabelDisabled)]     = Color(NamedColors::gray),
    [int(StandardColorID::MenuText)]                = Color(NamedColors::black),
    [int(StandardColorID::MenuTextSelected)]        = Color(NamedColors::black),
    [int(StandardColorID::MenuBackground)]          = Color(NamedColors::silver),
    [int(StandardColorID::MenuBackgroundSelected)]  = Color(NamedColors::steelblue),
    [int(StandardColorID::ScrollBarBackground)]     = Color(NamedColors::slategray),
    [int(StandardColorID::ScrollBarKnob)]           = Color(NamedColors::darkgray),
    [int(StandardColorID::SliderKnobNormal)]        = Color(NamedColors::lightslategray),
    [int(StandardColorID::SliderKnobPressed)]       = Color(NamedColors::lightsteelblue),
    [int(StandardColorID::SliderKnobShadow)]        = Color(NamedColors::darkgray),
    [int(StandardColorID::SliderKnobDisabled)]      = Color(NamedColors::darkgray),
    [int(StandardColorID::SliderTrackNormal)]       = Color(NamedColors::darkgray),
    [int(StandardColorID::SliderTrackDisabled)]     = Color(NamedColors::darkgray),
    [int(StandardColorID::ListViewTab)]             = Color(NamedColors::slategray),
    [int(StandardColorID::ListViewTabText)]         = Color(NamedColors::white)
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
        return g_DefaultColors[int32_t(StandardColorID::DefaultBackground)];
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

View::View(const String& name, Ptr<View> parent, uint32_t flags) : ViewBase(name, Rect(), Point(), flags, 0, 1.0f, get_standard_color(StandardColorID::DefaultBackground), get_standard_color(StandardColorID::DefaultBackground), Color(0))
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

View::View(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData)
    : ViewBase(xmlData.attribute("name").value(),
               Rect(),
               Point(),
               context.GetFlagsAttribute<uint32_t>(xmlData, ViewFlags::FlagMap, "flags", 0),
               0,
               1.0f,
               get_standard_color(StandardColorID::DefaultBackground),
               get_standard_color(StandardColorID::DefaultBackground),
               Color(0))
{
    Initialize();

    SetHAlignment(context.GetAttribute(xmlData, "h_alignment", Alignment::Center));
    SetVAlignment(context.GetAttribute(xmlData, "v_alignment", Alignment::Center));

	Point sizeOverride;
	Point sizeSmallestOverride;
    Point sizeGreatestOverride;
	SizeOverride overrideType;
    SizeOverride overrideTypeSmallestH;
	SizeOverride overrideTypeGreatestH;
	SizeOverride overrideTypeSmallestV;
	SizeOverride overrideTypeGreatestV;

    overrideType = GetSizeOverride(context.GetAttribute(xmlData, "size"), context.GetAttribute(xmlData, "size_limit"), context.GetAttribute(xmlData, "size_extend"), sizeOverride);
    overrideTypeSmallestH = overrideTypeGreatestH = overrideTypeSmallestV = overrideTypeGreatestV = overrideType;
    sizeSmallestOverride = sizeGreatestOverride = sizeOverride;

    float value;
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "width"), context.GetAttribute(xmlData, "width_limit"), context.GetAttribute(xmlData, "width_extend"), value);
    if (overrideType != SizeOverride::None) {
        sizeSmallestOverride.x = sizeGreatestOverride.x = value;
        overrideTypeSmallestH = overrideTypeGreatestH = overrideType;
    }
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "height"), context.GetAttribute(xmlData, "height_limit"), context.GetAttribute(xmlData, "height_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeSmallestOverride.y = sizeGreatestOverride.y = value;
		overrideTypeSmallestV = overrideTypeGreatestV = overrideType;
	}

	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "min_size"), context.GetAttribute(xmlData, "min_size_limit"), context.GetAttribute(xmlData, "min_size_extend"), sizeOverride);
    if (overrideType != SizeOverride::None) {
        overrideTypeSmallestH = overrideTypeSmallestV = overrideType;
        sizeSmallestOverride = sizeOverride;
    }
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "min_width"), context.GetAttribute(xmlData, "min_width_limit"), context.GetAttribute(xmlData, "min_width_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeSmallestOverride.x = value;
		overrideTypeSmallestH = overrideType;
	}
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "min_height"), context.GetAttribute(xmlData, "min_height_limit"), context.GetAttribute(xmlData, "min_height_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeSmallestOverride.y = value;
		overrideTypeSmallestV = overrideType;
	}


	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "max_size"), context.GetAttribute(xmlData, "max_size_limit"), context.GetAttribute(xmlData, "max_size_extend"), sizeOverride);
	if (overrideType != SizeOverride::None) {
		overrideTypeGreatestH = overrideTypeGreatestV = overrideType;
		sizeGreatestOverride = sizeOverride;
	}
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "max_width"), context.GetAttribute(xmlData, "max_width_limit"), context.GetAttribute(xmlData, "max_width_extend"), value);
	if (overrideType != SizeOverride::None) {
		sizeGreatestOverride.x = value;
		overrideTypeGreatestH = overrideType;
	}
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "max_height"), context.GetAttribute(xmlData, "max_height_limit"), context.GetAttribute(xmlData, "max_height_extend"), value);
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
    m_Wheight = context.GetAttribute(xmlData, "layout_weight", 1.0f);
    SetLayoutNode(context.GetAttribute(xmlData, "layout", Ptr<LayoutNode>()));
    m_Borders = context.GetAttribute(xmlData, "layout_borders", Rect(0.0f));

    String widthGroupName = context.GetAttribute(xmlData, "width_group", String::zero);
	String heightGroupName = context.GetAttribute(xmlData, "height_group", String::zero);

    if (!widthGroupName.empty())
    {
		auto i = context.m_WidthRings.find(widthGroupName);
		if (i != context.m_WidthRings.end()) {
			AddToWidthRing(i->second);
		} else {
            context.m_WidthRings[widthGroupName] = ptr_tmp_cast(this);
		}
    }

	if (!heightGroupName.empty())
	{
		auto i = context.m_HeightRings.find(heightGroupName);
		if (i != context.m_HeightRings.end()) {
			AddToHeightRing(i->second);
		} else {
            context.m_HeightRings[heightGroupName] = ptr_tmp_cast(this);
		}
	}

    if (parent != nullptr) {
        parent->AddChild(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::View(Ptr<View> parent, handler_id serverHandle, const String& name, const Rect& frame) : ViewBase(name, frame, Point(), ViewFlags::Eavesdropper | ViewFlags::WillDraw, 0, 1.0f, Color(0xffffffff), Color(0xffffffff), Color(0))
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
    RegisterRemoteSignal(&RSViewFrameChanged, &View::SlotFrameChanged);
    RegisterRemoteSignal(&RSViewFocusChanged, &View::SlotKeyboardFocusChanged);
    RegisterRemoteSignal(&RSHandleMouseDown, &View::SlotHandleMouseDown);
    RegisterRemoteSignal(&RSHandleMouseUp, &View::HandleMouseUp);
    RegisterRemoteSignal(&RSHandleMouseMove, &View::HandleMouseMove);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::~View()
{
    if (m_VScrollBar != nullptr) {
        m_VScrollBar->SetScrollTarget(nullptr);
    }
    if (m_HScrollBar != nullptr) {
        m_HScrollBar->SetScrollTarget(nullptr);
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

bool View::HandleMessage(int32_t code, const void* data, size_t length)
{
    switch (MessageID(code))
    {
        case os::MessageID::KEY_DOWN:
        {
            const KeyEvent* keyEvent = static_cast<const KeyEvent*>(data);
            DispatchKeyDown(keyEvent->m_KeyCode, keyEvent->m_Text, *keyEvent);
            return true;
        }
        case os::MessageID::KEY_UP:
        {
            const KeyEvent* keyEvent = static_cast<const KeyEvent*>(data);
            DispatchKeyUp(keyEvent->m_KeyCode, keyEvent->m_Text, *keyEvent);
            return true;
        }
        default:
            return ViewBase<View>::HandleMessage(code, data, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application* View::GetApplication() const
{
    Looper* const looper = GetLooper();
    if (looper != nullptr)
    {
        return static_cast<Application*>(looper);
    }
    else
    {
        const Ptr<const View> parent = GetParent();
        if (parent != nullptr) {
            return parent->GetApplication();
        } else {
            return nullptr;
        }
    }

    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

handler_id View::GetParentServerHandle() const
{
    for (Ptr<const View> i = GetParent(); i != nullptr; i = i->GetParent())
    {
        handler_id curParentHandle = i->GetServerHandle();
        if (curParentHandle != INVALID_HANDLE) {
            return curParentHandle;
        }
    }
    return INVALID_HANDLE;
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
    PreferredSizeChanged();
    InvalidateLayout();
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
    if (sizeType == PrefSizeType::Smallest || sizeType == PrefSizeType::Greatest)
    {
        m_WidthOverride[int(sizeType)]     = size;
        m_WidthOverrideType[int(sizeType)] = when;
    }
    else if (sizeType == PrefSizeType::All)
    {
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
    if (sizeType == PrefSizeType::Smallest || sizeType == PrefSizeType::Greatest)
    {
        m_HeightOverride[int(sizeType)]     = size;
        m_HeightOverrideType[int(sizeType)] = when;
    }
    else if (sizeType == PrefSizeType::All)
    {
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
        m_IsLayoutValid = false;
        Application* app = GetApplication();
        if (app != nullptr) {
            app->RegisterViewForLayout(ptr_tmp_cast(this));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& motionEvent)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& motionEvent)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& motionEvent)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::OnKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& keyEvent)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::OnKeyUp(KeyCodes keyCode, const String& text, const KeyEvent& keyEvent)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::OnLayoutChanged()
{
    if (m_LayoutNode != nullptr)
    {
        m_LayoutNode->Layout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::OnFrameMoved(const Point& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::OnFrameSized(const Point& delta)
{
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void  View::OnScreenFrameMoved(const Point& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::OnViewScrolled(const Point& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::OnFontChanged(Ptr<Font> newFont)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::RefreshLayout(int32_t maxIterations, bool recursive)
{
    while (!m_IsLayoutValid && maxIterations-- > 0)
    {
        m_IsLayoutValid = true;
        OnLayoutChanged();

        if (recursive)
        {
            for (Ptr<View> child : *this) {
                child->RefreshLayout(maxIterations, true);
            }
        }
    }
    return m_IsLayoutValid;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void os::View::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    if (m_LayoutNode != nullptr)
    {
        m_LayoutNode->CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
    else
    {
        minSize->x = 0.0f;
        minSize->y = 0.0f;
        maxSize->x = LAYOUT_MAX_SIZE;
        maxSize->y = LAYOUT_MAX_SIZE;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

os::Point View::CalculateContentSize() const
{
    return Point(0.0f, 0.0f);
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
    if (VFCalculateContentSize.Empty()) {
        return CalculateContentSize();
    } else {
        return VFCalculateContentSize();
    }
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
    
    if (sizes[int(PrefSizeType::Greatest)].x < sizes[int(PrefSizeType::Smallest)].x) sizes[int(PrefSizeType::Greatest)].x = sizes[int(PrefSizeType::Smallest)].x;
    if (sizes[int(PrefSizeType::Greatest)].y < sizes[int(PrefSizeType::Smallest)].y) sizes[int(PrefSizeType::Greatest)].y = sizes[int(PrefSizeType::Smallest)].y;

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
    LinkChild(child, INVALID_INDEX);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::InsertChild(Ptr<View> child, size_t index)
{
    LinkChild(child, index);
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
    return ptr_tmp_cast(m_VScrollBar);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ScrollBar> View::GetHScrollBar() const
{
    return ptr_tmp_cast(m_HScrollBar);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Show(bool visible)
{
    const bool wasVisible = IsVisible();

    if (visible) {
        m_HideCount--;
    } else {
        m_HideCount++;
    }
    const bool isVisible = IsVisible();
    if (isVisible != wasVisible)
    {
        if (m_ServerHandle != INVALID_HANDLE) {
            Post<ASViewShow>(isVisible);
        }
        for (Ptr<View> child : *this) {
            child->Show(isVisible);
        }
        if (HasFlags(ViewFlags::IgnoreWhenHidden))
        {
            Ptr<View> parent = GetParent();
            if (parent != nullptr) {
                parent->PreferredSizeChanged();
                parent->InvalidateLayout();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::IsVisible() const
{
    return m_HideCount == 0;
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

void View::SetKeyboardFocus(bool focus)
{
    Application* app = GetApplication();
    if (app != nullptr) {
        app->SetKeyboardFocus(ptr_tmp_cast(this), focus, true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::HasKeyboardFocus() const
{
    const Application* app = GetApplication();
    return app != nullptr && ptr_raw_pointer_cast(app->GetKeyboardFocus()) == this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetFrame(const Rect& frame)
{
    SetFrameInternal(frame, true);
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

void View::Invalidate(const Rect& rect)
{
    if (m_ServerHandle != INVALID_HANDLE) {
        Post<ASViewInvalidate>(IRect(rect));
    } else if (!HasFlags(ViewFlags::WillDraw)) {
        kernel_log(LogCategoryGUITK, KLogSeverity::ERROR, "%s: Called on client-only view %s[%s].\n", __PRETTY_FUNCTION__, typeid(*this).name(), GetName().c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Invalidate()
{
    Invalidate(GetBounds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetFocusKeyboardMode(FocusKeyboardMode mode)
{
    if (mode != m_FocusKeyboardMode)
    {
        m_FocusKeyboardMode = mode;
        Post<ASViewSetFocusKeyboardMode>(m_FocusKeyboardMode);
    }
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

bool View::SlotHandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& motionEvent)
{
    // Dive down the hierarchy to find the top-most children under the mouse.
    for (Ptr<View> child : reverse_ranged(m_ChildrenList))
    {
        if (child->IsVisible() && child->m_Frame.DoIntersect(position))
        {
            const Point childPos = child->ConvertFromParent(position);
            return child->SlotHandleMouseDown(button, childPos, motionEvent);
        }
    }
    // No children eligible for handling the mouse, so attempt to handle it ourself.
    return HandleMouseDown(button, position, motionEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& motionEvent)
{
    bool handled = false;

    if (!HasFlags(ViewFlags::IgnoreMouse))
    {
        if (button < MouseButton_e::FirstTouchID) {
            handled = DispatchMouseDown(button, position, motionEvent);
        } else {
            handled = DispatchTouchDown(button, position, motionEvent);
        }
    }
    if (handled)
    {
        Application* app = GetApplication();
        if (app != nullptr) {
            app->SetMouseDownView(button, ptr_tmp_cast(this), motionEvent);
        }
    }
    else
    {
        Ptr<View> parent = GetParent();

        if (parent != nullptr)
        {
            // Event not handled by us. Check if the mouse hit any overlapping siblings below us.
            const Point parentPos = ConvertToParent(position);
            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->rend())
            {
                for (++i; i != parent->rend(); ++i)
                {
                    Ptr<View> sibling = *i;
                    if (sibling->IsVisible() && sibling->GetFrame().DoIntersect(parentPos)) {
                        return sibling->HandleMouseDown(button, sibling->ConvertFromParent(parentPos), motionEvent);
                    }
                }
            }
            // No lower sibling hit, send to parent.
            return parent->HandleMouseDown(button, parentPos, motionEvent);
        }
    }
    return handled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& motionEvent)
{
    Application* app = GetApplication();
    Ptr<View> mouseView = (app != nullptr) ? app->GetMouseDownView(button) : nullptr;
    if (mouseView != nullptr)
    {
        if (button < MouseButton_e::FirstTouchID) {
            mouseView->DispatchMouseUp(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), motionEvent);
        } else {
            mouseView->DispatchTouchUp(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), motionEvent);
        }
    }
    else
    {
        DispatchMouseUp(button, position, motionEvent);
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& motionEvent)
{
    Application* app = GetApplication();
    Ptr<View> mouseView = (app != nullptr) ? app->GetFocusView(button) : nullptr;
    if (mouseView != nullptr)
    {
        if (button < MouseButton_e::FirstTouchID) {
            mouseView->DispatchMouseMove(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), motionEvent);
        } else {
            mouseView->DispatchTouchMove(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), motionEvent);
        }
    }
    else if (m_HideCount == 0 && !HasFlags(ViewFlags::IgnoreMouse))
    {
        if (button < MouseButton_e::FirstTouchID) {
            DispatchMouseMove(button, position, motionEvent);
        } else {
            DispatchTouchMove(button, position, motionEvent);
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
    UpdatePosition(View::UpdatePositionNotifyServer::IfChanged);
    Post<ASViewScrollBy>(offset);

    if (offset.x != 0 && m_HScrollBar != nullptr) {
        m_HScrollBar->SetValue(-m_ScrollOffset.x);
    }
    if (offset.y != 0 && m_VScrollBar != nullptr) {
        m_VScrollBar->SetValue(-m_ScrollOffset.y);
    }
    OnViewScrolled(offset);
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

void View::DrawBitmap(Ptr<const Bitmap> bitmap, const Rect& srcRect, const Rect& dstRect)
{
    Post<ASViewDrawScaledBitmap>(bitmap->m_Handle, srcRect, dstRect);
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

    Color fgColor = get_standard_color(StandardColorID::Shine);
    Color bgColor = get_standard_color(StandardColorID::Shadow);

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

void View::HandleAddedToParent(Ptr<View> parent, size_t index)
{
    UpdatePosition(View::UpdatePositionNotifyServer::IfChanged);
    if (!parent->IsVisible()) {
        Show(false);
    }
    if (parent->HasFlags(ViewFlags::IsAttachedToScreen))
    {
        parent->InvalidateLayout();

        Application* app = parent->GetApplication();
        if (app != nullptr)
        {
            HandlePreAttachToScreen(app);

            if (!HasFlags(ViewFlags::Eavesdropper)) {
                app->AddChildView(parent, ptr_tmp_cast(this), index);
            }
            HandleAttachedToScreen(app);
            app->LayoutViews();
        }
    }
    OnAttachedToParent(parent);
}

///////////////////////////////////////////////////////////////////////////////
/// Called from ViewBase::UnlinkChild() to tell the view that it has been
/// detached from the parent.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void View::HandleRemovedFromParent(Ptr<View> parent)
{
    if (!parent->IsVisible()) {
        Show(true);
    }
    OnDetachedFromParent(parent);
    if (!HasFlags(ViewFlags::Eavesdropper))
    {
        Application* const app = GetApplication();
        if (app != nullptr) {
            app->RemoveView(ptr_tmp_cast(this));
        }
        UpdatePosition(View::UpdatePositionNotifyServer::IfChanged);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandlePreAttachToScreen(Application* app)
{
    MergeFlags(ViewFlags::IsAttachedToScreen);
    app->AddHandler(ptr_tmp_cast(this));
    AttachedToScreen();

    PreferredSizeChanged();

    for (Ptr<View> child : m_ChildrenList) {
        child->HandlePreAttachToScreen(app);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleAttachedToScreen(Application* app)
{
    for (Ptr<View> child : m_ChildrenList) {
        child->HandleAttachedToScreen(app);
    }
    if (!m_IsLayoutValid) {
        app->RegisterViewForLayout(ptr_tmp_cast(this));
    }
    AllAttachedToScreen();
}

///////////////////////////////////////////////////////////////////////////////
/// Called from Application::RemoveView() to tell the view that it has been
/// detached from the screen.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleDetachedFromScreen()
{
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
    OnPaint(frame);
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

            SignalPreferredSizeChanged(ptr_tmp_cast(this));
            Ptr<View> parent = GetParent();
            if (parent != nullptr)
            {
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

        std::set<Ptr<View>> modifiedViews;

        if (m_WidthRing != nullptr)
        {
            View* member = this;
            do
            {
                bool modified = false;
                for (int i = 0; i < int(PrefSizeType::Count); ++i)
                {
                    if (ringSizes[i].x != member->m_PreferredSizes[i].x)
                    {
                        modified = true;
                        member->m_PreferredSizes[i].x = ringSizes[i].x;
                    }
                }
                if (modified) {
                    modifiedViews.insert(ptr_tmp_cast(member));
                }
                member = member->m_WidthRing;
            } while (member != this);
        }
        else
        {
            bool modified = false;
            for (int i = 0; i < int(PrefSizeType::Count); ++i)
            {
                if (m_LocalPrefSize[i].x != m_PreferredSizes[i].x)
                {
                    modified = true;
                    m_PreferredSizes[i].x = m_LocalPrefSize[i].x;
                }
            }
            if (modified) {
                modifiedViews.insert(ptr_tmp_cast(this));
            }
        }

        if (m_HeightRing != nullptr)
        {
            View* member = this;
            do
            {
                bool modified = false;
                for (int i = 0; i < int(PrefSizeType::Count); ++i)
                {
                    if (ringSizes[i].y != member->m_PreferredSizes[i].y)
                    {
                        modified = true;
                        member->m_PreferredSizes[i].y = ringSizes[i].y;
                    }
                }
                if (modified) {
                    modifiedViews.insert(ptr_tmp_cast(member));
                }
                member = member->m_HeightRing;
            } while (member != this);
        }
        else
        {
            bool modified = false;
            for (int i = 0; i < int(PrefSizeType::Count); ++i)
            {
                if (m_LocalPrefSize[i].y != m_PreferredSizes[i].y)
                {
                    modified = true;
                    m_PreferredSizes[i].y = m_LocalPrefSize[i].y;
                }
            }
            if (modified) {
                modifiedViews.insert(ptr_tmp_cast(this));
            }
        }

        std::set<Ptr<View>> notifiedParents;
        for (Ptr<View> modifiedView : modifiedViews)
        {
            modifiedView->SignalPreferredSizeChanged(modifiedView);
            Ptr<View> parent = modifiedView->GetParent();
            if (parent != nullptr) {
                notifiedParents.insert(parent);
            }
        }
        modifiedViews.clear();

        for (Ptr<View> modifiedParent : notifiedParents)
        {
            modifiedParent->PreferredSizeChanged();
            modifiedParent->InvalidateLayout();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetVScrollBar(ScrollBar* scrollBar)
{
    m_VScrollBar = scrollBar;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetHScrollBar(ScrollBar* scrollBar)
{
    m_HScrollBar = scrollBar;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SetFrameInternal(const Rect& frame, bool notifyServer)
{
    const Point deltaSize = frame.Size() - m_Frame.Size();
    const Point deltaPos = frame.TopLeft() - m_Frame.TopLeft();

    m_Frame = frame;

    UpdatePosition(notifyServer ? View::UpdatePositionNotifyServer::Always : View::UpdatePositionNotifyServer::Never);
    if (deltaSize != Point(0.0f, 0.0f))
    {
        OnFrameSized(deltaSize);
        SignalFrameSized(deltaSize, ptr_tmp_cast(this));
    }
    if (deltaPos != Point(0.0f, 0.0f))
    {
        OnFrameMoved(deltaPos);
        SignalFrameMoved(deltaPos, ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::UpdatePosition(UpdatePositionNotifyServer notifyMode)
{
    Point newOffset;
    Point newScreenPos;
    {
        Ptr<View> parent = m_Parent.Lock();
        if (parent != nullptr && !parent->HasFlags(ViewFlags::WillDraw)) {
            newOffset = parent->m_PositionOffset + parent->m_Frame.TopLeft();
        } else {
            newOffset = Point(0.0f, 0.0f);
        }
        if (parent == nullptr) {
            newScreenPos = m_Frame.TopLeft();
        } else {
            newScreenPos = parent->m_ScreenPos + parent->m_ScrollOffset + m_Frame.TopLeft();
        }
    }
    if (notifyMode == View::UpdatePositionNotifyServer::Always || (notifyMode == View::UpdatePositionNotifyServer::IfChanged && newOffset != m_PositionOffset))
    {
        m_PositionOffset = newOffset;
        if (m_ServerHandle != INVALID_HANDLE)
        {
            Post<ASViewSetFrame>(m_Frame + m_PositionOffset, GetHandle());
        }
    }
    Point deltaScreenPos = m_ScreenPos - newScreenPos;
    m_ScreenPos = newScreenPos;

    for (Ptr<View> child : m_ChildrenList) {
        child->UpdatePosition((notifyMode == View::UpdatePositionNotifyServer::Never) ? View::UpdatePositionNotifyServer::Never : View::UpdatePositionNotifyServer::IfChanged);
    }
    if (deltaScreenPos.x != 0.0f || deltaScreenPos.y != 0.0f) {
        OnScreenFrameMoved(deltaScreenPos);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::DispatchTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    if (VFTouchDown.Empty()) {
        return OnTouchDown(pointID, position, motionEvent);
    } else {
        return VFTouchDown(this, pointID, position, motionEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::DispatchTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    if (VFTouchUp.Empty()) {
        return OnTouchUp(pointID, position, motionEvent);
    } else {
        return VFTouchUp(this, pointID, position, motionEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::DispatchTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    if (VFTouchMove.Empty()) {
        return OnTouchMove(pointID, position, motionEvent);
    } else {
        return VFTouchMove(this, pointID, position, motionEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::DispatchLongPress(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    if (VFLongPress.Empty()) {
        return OnLongPress(pointID, position, motionEvent);
    } else {
        return VFLongPress(this, pointID, position, motionEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::DispatchMouseDown(MouseButton_e buttonID, const Point& position, const MotionEvent& motionEvent)
{
    if (VFMouseDown.Empty()) {
        return OnMouseDown(buttonID, position, motionEvent);
    } else {
        return VFMouseDown(this, buttonID, position, motionEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::DispatchMouseUp(MouseButton_e buttonID, const Point& position, const MotionEvent& motionEvent)
{
    if (VFMouseUp.Empty()) {
        return OnMouseUp(buttonID, position, motionEvent);
    } else {
        return VFMouseUp(this, buttonID, position, motionEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::DispatchMouseMove(MouseButton_e buttonID, const Point& position, const MotionEvent& motionEvent)
{
    if (VFMouseMove.Empty()) {
        return OnMouseMove(buttonID, position, motionEvent);
    } else {
        return VFMouseMove(this, buttonID, position, motionEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::DispatchKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& keyEvent)
{
    if (VFKeyDown.Empty()) {
        OnKeyDown(keyCode, text, keyEvent);
    } else {
        VFKeyDown(this, keyCode, text, keyEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::DispatchKeyUp(KeyCodes keyCode, const String& text, const KeyEvent& keyEvent)
{
    if (VFKeyUp.Empty()) {
        OnKeyUp(keyCode, text, keyEvent);
    } else {
        VFKeyUp(this, keyCode, text, keyEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SlotFrameChanged(const Rect& frame)
{
    SetFrameInternal(frame, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::SlotKeyboardFocusChanged(bool hasFocus)
{
    Application* app = GetApplication();
    if (app != nullptr) {
        app->SetKeyboardFocus(ptr_tmp_cast(this), hasFocus, false);
    }
}
