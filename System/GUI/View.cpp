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
#include <pugixml.hpp>

#include <GUI/View.h>
#include <GUI/Widgets/ScrollBar.h>
#include <GUI/Bitmap.h>
#include <GUI/NamedColors.h>
#include <App/Application.h>
#include <Utils/Utils.h>
#include <Utils/XMLFactory.h>
#include <Utils/XMLObjectParser.h>


const std::map<PString, uint32_t> PViewFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, FullUpdateOnResizeH),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, FullUpdateOnResizeV),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, FullUpdateOnResize),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, IgnoreWhenHidden),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, WillDraw),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, Transparent),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, ClearBackground),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, DrawOnChildren),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, Eavesdropper),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, IgnoreMouse),
    DEFINE_FLAG_MAP_ENTRY(PViewFlags, ForceHandleMouse)
};

static PColor g_DefaultColors[] =
{
    [int(PStandardColorID::None)]                    = PColor(0, 0, 0),
    [int(PStandardColorID::DefaultBackground)]       = PColor(PNamedColors::darkgray),
    [int(PStandardColorID::Shine)]                   = PColor(PNamedColors::white),
    [int(PStandardColorID::Shadow)]                  = PColor(PNamedColors::black),
    [int(PStandardColorID::WindowBorderActive)]      = PColor(PNamedColors::steelblue),
    [int(PStandardColorID::WindowBorderInactive)]    = PColor(PNamedColors::slategray),
    [int(PStandardColorID::ButtonBackground)]        = PColor(PNamedColors::darkgray),
    [int(PStandardColorID::ButtonLabelNormal)]       = PColor(PNamedColors::black),
    [int(PStandardColorID::ButtonLabelDisabled)]     = PColor(PNamedColors::gray),
    [int(PStandardColorID::MenuText)]                = PColor(PNamedColors::black),
    [int(PStandardColorID::MenuTextSelected)]        = PColor(PNamedColors::black),
    [int(PStandardColorID::MenuBackground)]          = PColor(PNamedColors::silver),
    [int(PStandardColorID::MenuBackgroundSelected)]  = PColor(PNamedColors::steelblue),
    [int(PStandardColorID::ScrollBarBackground)]     = PColor(PNamedColors::slategray),
    [int(PStandardColorID::ScrollBarKnob)]           = PColor(PNamedColors::darkgray),
    [int(PStandardColorID::SliderKnobNormal)]        = PColor(PNamedColors::lightslategray),
    [int(PStandardColorID::SliderKnobPressed)]       = PColor(PNamedColors::lightsteelblue),
    [int(PStandardColorID::SliderKnobShadow)]        = PColor(PNamedColors::darkgray),
    [int(PStandardColorID::SliderKnobDisabled)]      = PColor(PNamedColors::darkgray),
    [int(PStandardColorID::SliderTrackNormal)]       = PColor(PNamedColors::darkgray),
    [int(PStandardColorID::SliderTrackDisabled)]     = PColor(PNamedColors::darkgray),
    [int(PStandardColorID::ListViewTab)]             = PColor(PNamedColors::slategray),
    [int(PStandardColorID::ListViewTabText)]         = PColor(PNamedColors::white)
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

PColor pget_standard_color(PStandardColorID colorID)
{
    uint32_t index = uint32_t(colorID);
    if (index < ARRAY_COUNT(g_DefaultColors)) {
        return g_DefaultColors[index];
    } else {
        return g_DefaultColors[int32_t(PStandardColorID::DefaultBackground)];
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void pset_standard_color(PStandardColorID colorID, PColor color)
{
    uint32_t index = uint32_t(colorID);
    if (index < ARRAY_COUNT(g_DefaultColors)) {
        g_DefaultColors[index] = color;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PColor Tint(const PColor& color, float tint)
{
    int r = int( (float(color.GetRed()) * tint + 127.0f * (1.0f - tint)) );
    int g = int( (float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)) );
    int b = int( (float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)) );
    if ( r < 0 ) r = 0; else if (r > 255) r = 255;
    if ( g < 0 ) g = 0; else if (g > 255) g = 255;
    if ( b < 0 ) b = 0; else if (b > 255) b = 255;
    return PColor(uint8_t(r), uint8_t(g), uint8_t(b), color.GetAlpha());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PView::PView(const PString& name, Ptr<PView> parent, uint32_t flags) : PViewBase(name, PRect(), PPoint(), flags, 0, 1.0f, pget_standard_color(PStandardColorID::DefaultBackground), pget_standard_color(PStandardColorID::DefaultBackground), PColor(0))
{
    Initialize();
    if (parent != nullptr) {
        parent->AddChild(ptr_tmp_cast(this));
    }
//    SetFont(ptr_new<Font>(GfxDriver::e_Font7Seg));
}

template< typename T>
static PSizeOverride GetSizeOverride(pugi::xml_attribute absoluteAttr, pugi::xml_attribute limitAttr, pugi::xml_attribute extendAttr, T& outValue)
{
    if (!absoluteAttr.empty() && p_xml_object_parser::parse(absoluteAttr.value(), outValue)) {
        return PSizeOverride::Always;
    } else if (!limitAttr.empty() && p_xml_object_parser::parse(limitAttr.value(), outValue)) {
        return PSizeOverride::Limit;
    } else if (!extendAttr.empty() && p_xml_object_parser::parse(extendAttr.value(), outValue)) {
        return PSizeOverride::Extend;
    }
    return PSizeOverride::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PView::PView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData)
    : PViewBase(xmlData.attribute("name").value(),
               PRect(),
               PPoint(),
               context.GetFlagsAttribute<uint32_t>(xmlData, PViewFlags::FlagMap, "flags", 0),
               0,
               1.0f,
               pget_standard_color(PStandardColorID::DefaultBackground),
               pget_standard_color(PStandardColorID::DefaultBackground),
               PColor(0))
{
    Initialize();

    SetHAlignment(context.GetAttribute(xmlData, "h_alignment", PAlignment::Center));
    SetVAlignment(context.GetAttribute(xmlData, "v_alignment", PAlignment::Center));

	PPoint sizeOverride;
	PPoint sizeSmallestOverride;
    PPoint sizeGreatestOverride;
	PSizeOverride overrideType;
    PSizeOverride overrideTypeSmallestH;
	PSizeOverride overrideTypeGreatestH;
	PSizeOverride overrideTypeSmallestV;
	PSizeOverride overrideTypeGreatestV;

    overrideType = GetSizeOverride(context.GetAttribute(xmlData, "size"), context.GetAttribute(xmlData, "size_limit"), context.GetAttribute(xmlData, "size_extend"), sizeOverride);
    overrideTypeSmallestH = overrideTypeGreatestH = overrideTypeSmallestV = overrideTypeGreatestV = overrideType;
    sizeSmallestOverride = sizeGreatestOverride = sizeOverride;

    float value;
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "width"), context.GetAttribute(xmlData, "width_limit"), context.GetAttribute(xmlData, "width_extend"), value);
    if (overrideType != PSizeOverride::None) {
        sizeSmallestOverride.x = sizeGreatestOverride.x = value;
        overrideTypeSmallestH = overrideTypeGreatestH = overrideType;
    }
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "height"), context.GetAttribute(xmlData, "height_limit"), context.GetAttribute(xmlData, "height_extend"), value);
	if (overrideType != PSizeOverride::None) {
		sizeSmallestOverride.y = sizeGreatestOverride.y = value;
		overrideTypeSmallestV = overrideTypeGreatestV = overrideType;
	}

	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "min_size"), context.GetAttribute(xmlData, "min_size_limit"), context.GetAttribute(xmlData, "min_size_extend"), sizeOverride);
    if (overrideType != PSizeOverride::None) {
        overrideTypeSmallestH = overrideTypeSmallestV = overrideType;
        sizeSmallestOverride = sizeOverride;
    }
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "min_width"), context.GetAttribute(xmlData, "min_width_limit"), context.GetAttribute(xmlData, "min_width_extend"), value);
	if (overrideType != PSizeOverride::None) {
		sizeSmallestOverride.x = value;
		overrideTypeSmallestH = overrideType;
	}
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "min_height"), context.GetAttribute(xmlData, "min_height_limit"), context.GetAttribute(xmlData, "min_height_extend"), value);
	if (overrideType != PSizeOverride::None) {
		sizeSmallestOverride.y = value;
		overrideTypeSmallestV = overrideType;
	}


	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "max_size"), context.GetAttribute(xmlData, "max_size_limit"), context.GetAttribute(xmlData, "max_size_extend"), sizeOverride);
	if (overrideType != PSizeOverride::None) {
		overrideTypeGreatestH = overrideTypeGreatestV = overrideType;
		sizeGreatestOverride = sizeOverride;
	}
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "max_width"), context.GetAttribute(xmlData, "max_width_limit"), context.GetAttribute(xmlData, "max_width_extend"), value);
	if (overrideType != PSizeOverride::None) {
		sizeGreatestOverride.x = value;
		overrideTypeGreatestH = overrideType;
	}
	overrideType = GetSizeOverride(context.GetAttribute(xmlData, "max_height"), context.GetAttribute(xmlData, "max_height_limit"), context.GetAttribute(xmlData, "max_height_extend"), value);
	if (overrideType != PSizeOverride::None) {
		sizeGreatestOverride.y = value;
		overrideTypeGreatestV = overrideType;
	}

    if (overrideTypeSmallestH != PSizeOverride::None) {
		SetWidthOverride(PPrefSizeType::Smallest, overrideTypeSmallestH, sizeSmallestOverride.x);
    }
	if (overrideTypeGreatestH != PSizeOverride::None) {
		SetWidthOverride(PPrefSizeType::Greatest, overrideTypeGreatestH, sizeGreatestOverride.x);
	}
	if (overrideTypeSmallestV != PSizeOverride::None) {
		SetHeightOverride(PPrefSizeType::Smallest, overrideTypeSmallestV, sizeSmallestOverride.y);
	}
	if (overrideTypeGreatestV != PSizeOverride::None) {
		SetHeightOverride(PPrefSizeType::Greatest, overrideTypeGreatestV, sizeGreatestOverride.y);
	}
    m_Wheight = context.GetAttribute(xmlData, "layout_weight", 1.0f);
    SetLayoutNode(context.GetAttribute(xmlData, "layout", Ptr<PLayoutNode>()));
    m_Borders = context.GetAttribute(xmlData, "layout_borders", PRect(0.0f));

    PString widthGroupName = context.GetAttribute(xmlData, "width_group", PString::zero);
    PString heightGroupName = context.GetAttribute(xmlData, "height_group", PString::zero);

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

PView::PView(Ptr<PView> parent, handler_id serverHandle, const PString& name, const PRect& frame) : PViewBase(name, frame, PPoint(), PViewFlags::Eavesdropper | PViewFlags::WillDraw, 0, 1.0f, PColor(0xffffffff), PColor(0xffffffff), PColor(0))
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

void PView::Initialize()
{
    RegisterRemoteSignal(&RSPaintView, &PView::HandlePaint);
    RegisterRemoteSignal(&RSViewFrameChanged, &PView::SlotFrameChanged);
    RegisterRemoteSignal(&RSViewFocusChanged, &PView::SlotKeyboardFocusChanged);
    RegisterRemoteSignal(&RSHandleMouseDown, &PView::SlotHandleMouseDown);
    RegisterRemoteSignal(&RSHandleMouseUp, &PView::HandleMouseUp);
    RegisterRemoteSignal(&RSHandleMouseMove, &PView::HandleMouseMove);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PView::~PView()
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

bool PView::HandleMessage(int32_t code, const void* data, size_t length)
{
    switch (PMessageID(code))
    {
        case PMessageID::KEY_DOWN:
        {
            const PKeyEvent* keyEvent = static_cast<const PKeyEvent*>(data);
            DispatchKeyDown(keyEvent->m_KeyCode, keyEvent->m_Text, *keyEvent);
            return true;
        }
        case PMessageID::KEY_UP:
        {
            const PKeyEvent* keyEvent = static_cast<const PKeyEvent*>(data);
            DispatchKeyUp(keyEvent->m_KeyCode, keyEvent->m_Text, *keyEvent);
            return true;
        }
        default:
            return PViewBase<PView>::HandleMessage(code, data, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication* PView::GetApplication() const
{
    PLooper* const looper = GetLooper();
    if (looper != nullptr)
    {
        return static_cast<PApplication*>(looper);
    }
    else
    {
        const Ptr<const PView> parent = GetParent();
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

handler_id PView::GetParentServerHandle() const
{
    for (Ptr<const PView> i = GetParent(); i != nullptr; i = i->GetParent())
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

Ptr<PLayoutNode> PView::GetLayoutNode() const
{
    return m_LayoutNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetLayoutNode(Ptr<PLayoutNode> node)
{
    if (m_LayoutNode != nullptr) 
    {
        Ptr<PLayoutNode> layoutNode = m_LayoutNode;
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

void PView::SetBorders(const PRect& border)
{
    m_Borders = border;
    Ptr<PView> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PView::GetBorders() const
{
    return m_Borders;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PView::GetWheight() const
{
    return m_Wheight;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetWheight(float vWheight)
{
    m_Wheight = vWheight;
    Ptr<PView> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetHAlignment(PAlignment alignment)
{
    m_HAlign = alignment;
    Ptr<PView> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetVAlignment(PAlignment alignment)
{
    m_VAlign = alignment;
    Ptr<PView> parent = GetParent();
    if (parent != nullptr) {
        parent->InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PAlignment PView::GetHAlignment() const
{
    return m_HAlign;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PAlignment PView::GetVAlignment() const
{
    return m_VAlign;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetWidthOverride(PPrefSizeType sizeType, PSizeOverride when, float size)
{
    if (sizeType == PPrefSizeType::Smallest || sizeType == PPrefSizeType::Greatest)
    {
        m_WidthOverride[int(sizeType)]     = size;
        m_WidthOverrideType[int(sizeType)] = when;
    }
    else if (sizeType == PPrefSizeType::All)
    {
        m_WidthOverride[int(PPrefSizeType::Smallest)]     = size;
        m_WidthOverrideType[int(PPrefSizeType::Smallest)] = when;
        
        m_WidthOverride[int(PPrefSizeType::Greatest)]     = size;
        m_WidthOverrideType[int(PPrefSizeType::Greatest)] = when;        
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetHeightOverride(PPrefSizeType sizeType, PSizeOverride when, float size)
{
    if (sizeType == PPrefSizeType::Smallest || sizeType == PPrefSizeType::Greatest)
    {
        m_HeightOverride[int(sizeType)]     = size;
        m_HeightOverrideType[int(sizeType)] = when;
    }
    else if (sizeType == PPrefSizeType::All)
    {
        m_HeightOverride[int(PPrefSizeType::Smallest)]     = size;
        m_HeightOverrideType[int(PPrefSizeType::Smallest)] = when;
        
        m_HeightOverride[int(PPrefSizeType::Greatest)]     = size;
        m_HeightOverrideType[int(PPrefSizeType::Greatest)] = when;        
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::AddToWidthRing(Ptr<PView> ring)
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

void PView::RemoveFromWidthRing()
{
    if (m_WidthRing != nullptr)
    {
        for (PView* i = m_WidthRing;; i = i->m_WidthRing)
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

void PView::AddToHeightRing(Ptr<PView> ring)
{
    RemoveFromHeightRing();
    m_HeightRing = (ring->m_HeightRing != nullptr) ? ring->m_HeightRing : ptr_raw_pointer_cast(ring);
    ring->m_HeightRing = this;    
    UpdateRingSize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::RemoveFromHeightRing()
{
    if (m_HeightRing != nullptr)
    {
        for (PView* i = m_HeightRing;; i = i->m_HeightRing)
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

void PView::InvalidateLayout()
{
    if (m_IsLayoutValid)
    {
        m_IsLayoutValid = false;
        PApplication* app = GetApplication();
        if (app != nullptr) {
            app->RegisterViewForLayout(ptr_tmp_cast(this));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::OnKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::OnLayoutChanged()
{
    if (m_LayoutNode != nullptr)
    {
        m_LayoutNode->Layout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::OnFrameMoved(const PPoint& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::OnFrameSized(const PPoint& delta)
{
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void  PView::OnScreenFrameMoved(const PPoint& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::OnViewScrolled(const PPoint& delta)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::OnFontChanged(Ptr<PFont> newFont)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::RefreshLayout(int32_t maxIterations, bool recursive)
{
    while (!m_IsLayoutValid && maxIterations-- > 0)
    {
        m_IsLayoutValid = true;
        OnLayoutChanged();

        if (recursive)
        {
            for (Ptr<PView> child : *this) {
                child->RefreshLayout(maxIterations, true);
            }
        }
    }
    return m_IsLayoutValid;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
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

PPoint PView::CalculateContentSize() const
{
    return PPoint(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PView::GetPreferredSize(PPrefSizeType sizeType) const
{
    return m_PreferredSizes[int(sizeType)];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PView::GetContentSize() const
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

void PView::PreferredSizeChanged()
{
    PPoint sizes[int(PPrefSizeType::Count)];
    
    bool includeWidth  = m_WidthOverrideType[int(PPrefSizeType::Smallest)] != PSizeOverride::Always || m_WidthOverrideType[int(PPrefSizeType::Greatest)] != PSizeOverride::Always;
    bool includeHeight = m_HeightOverrideType[int(PPrefSizeType::Smallest)] != PSizeOverride::Always || m_HeightOverrideType[int(PPrefSizeType::Greatest)] != PSizeOverride::Always;
    
    CalculatePreferredSize(&sizes[int(PPrefSizeType::Smallest)], &sizes[int(PPrefSizeType::Greatest)], includeWidth, includeHeight);
    
    for (int i = 0; i < int(PPrefSizeType::Count); ++i)
    {
        switch(m_WidthOverrideType[i])
        {
            case PSizeOverride::None: break;
            case PSizeOverride::Always:  sizes[i].x = m_WidthOverride[i]; break;
            case PSizeOverride::Extend:  sizes[i].x = std::max(sizes[i].x, m_WidthOverride[i]); break;
            case PSizeOverride::Limit:   sizes[i].x = std::min(sizes[i].x, m_WidthOverride[i]); break;
        }
        switch(m_HeightOverrideType[i])
        {
            case PSizeOverride::None: break;
            case PSizeOverride::Always:  sizes[i].y = m_HeightOverride[i]; break;
            case PSizeOverride::Extend:  sizes[i].y = std::max(sizes[i].y, m_HeightOverride[i]); break;
            case PSizeOverride::Limit:   sizes[i].y = std::min(sizes[i].y, m_HeightOverride[i]); break;
        }
        if (sizes[i].x > LAYOUT_MAX_SIZE) sizes[i].x = LAYOUT_MAX_SIZE;
        if (sizes[i].y > LAYOUT_MAX_SIZE) sizes[i].y = LAYOUT_MAX_SIZE;
    }
    
    if (sizes[int(PPrefSizeType::Greatest)].x < sizes[int(PPrefSizeType::Smallest)].x) sizes[int(PPrefSizeType::Greatest)].x = sizes[int(PPrefSizeType::Smallest)].x;
    if (sizes[int(PPrefSizeType::Greatest)].y < sizes[int(PPrefSizeType::Smallest)].y) sizes[int(PPrefSizeType::Greatest)].y = sizes[int(PPrefSizeType::Smallest)].y;

    if (sizes[int(PPrefSizeType::Smallest)] != m_LocalPrefSize[int(PPrefSizeType::Smallest)] || sizes[int(PPrefSizeType::Greatest)] != m_LocalPrefSize[int(PPrefSizeType::Greatest)])
    {
        m_LocalPrefSize[int(PPrefSizeType::Smallest)] = sizes[int(PPrefSizeType::Smallest)];
        m_LocalPrefSize[int(PPrefSizeType::Greatest)] = sizes[int(PPrefSizeType::Greatest)];
        
        UpdateRingSize();
        SignalPreferredSizeChanged(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::ContentSizeChanged()
{
    SignalContentSizeChanged(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::AddChild(Ptr<PView> child)
{
    LinkChild(child, INVALID_INDEX);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::InsertChild(Ptr<PView> child, size_t index)
{
    LinkChild(child, index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::RemoveChild(Ptr<PView> child)
{
    UnlinkChild(child);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PView::RemoveChild(ChildList_t::iterator iterator)
{
    return UnlinkChild(iterator);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::RemoveThis()
{
    Ptr<PView> parent = m_Parent.Lock();
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

Ptr<PView> PView::GetChildAt(const PPoint& pos)
{
    for (Ptr<PView> child : p_reverse_ranged(*this))
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

Ptr<PView> PView::GetChildAt(size_t index)
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

Ptr<PScrollBar> PView::GetVScrollBar() const
{
    return ptr_tmp_cast(m_VScrollBar);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PScrollBar> PView::GetHScrollBar() const
{
    return ptr_tmp_cast(m_HScrollBar);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::Show(bool visible)
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
        for (Ptr<PView> child : *this) {
            child->Show(isVisible);
        }
        if (HasFlags(PViewFlags::IgnoreWhenHidden))
        {
            Ptr<PView> parent = GetParent();
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

bool PView::IsVisible() const
{
    return m_HideCount == 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::MakeFocus(PMouseButton button, bool focus)
{
    PApplication* app = GetApplication();
    if (app != nullptr) {
        app->SetFocusView(button, ptr_tmp_cast(this), focus);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::HasFocus(PMouseButton button) const
{
    const PApplication* app = GetApplication();
    return app != nullptr && ptr_raw_pointer_cast(app->GetFocusView(button)) == this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetKeyboardFocus(bool focus)
{
    PApplication* app = GetApplication();
    if (app != nullptr) {
        app->SetKeyboardFocus(ptr_tmp_cast(this), focus, true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::HasKeyboardFocus() const
{
    const PApplication* app = GetApplication();
    return app != nullptr && ptr_raw_pointer_cast(app->GetKeyboardFocus()) == this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetFrame(const PRect& frame)
{
    SetFrameInternal(frame, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::ResizeBy(const PPoint& delta)
{
    SetFrame(PRect(m_Frame.left, m_Frame.top, m_Frame.right + delta.x, m_Frame.bottom + delta.y));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::ResizeBy(float deltaW, float deltaH)
{
    SetFrame(PRect(m_Frame.left, m_Frame.top, m_Frame.right + deltaW, m_Frame.bottom + deltaH));
}

///////////////////////////////////////////////////////////////////////////////
/// Set a new absolute size for the view.
/// \par Description:
///     See ResizeTo(float,float)
/// \param size
///     New size (PPoint::x is width, PPoint::y is height).
/// \sa ResizeTo(float,float)
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::ResizeTo(const PPoint& size)
{
    SetFrame(PRect(m_Frame.left, m_Frame.top, m_Frame.left + size.x, m_Frame.top + size.y));
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
/// \sa ResizeTo(const PPoint&), ResizeBy(), SetFrame()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::ResizeTo(float w, float h)
{
    SetFrame(PRect(m_Frame.left, m_Frame.top, m_Frame.left + w, m_Frame.top + h));
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::Invalidate(const PRect& rect)
{
    if (m_ServerHandle != INVALID_HANDLE) {
        Post<ASViewInvalidate>(PIRect(rect));
    } else if (!HasFlags(PViewFlags::WillDraw)) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "{}: Called on client-only view {}[{}].", __PRETTY_FUNCTION__, typeid(*this).name(), GetName());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::Invalidate()
{
    Invalidate(GetBounds());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetFocusKeyboardMode(PFocusKeyboardMode mode)
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

void PView::SetDrawingMode(PDrawingMode mode)
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

PDrawingMode PView::GetDrawingMode() const
{
    return m_DrawingMode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetFont(Ptr<PFont> font)
{
    m_Font = font;
    Post<ASViewSetFont>(int(font->Get()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
Ptr<PFont> PView::GetFont() const
{
    return m_Font;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::SlotHandleMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent)
{
    // Dive down the hierarchy to find the top-most children under the mouse.
    for (Ptr<PView> child : p_reverse_ranged(m_ChildrenList))
    {
        if (child->IsVisible() && child->m_Frame.DoIntersect(position))
        {
            const PPoint childPos = child->ConvertFromParent(position);
            return child->SlotHandleMouseDown(button, childPos, motionEvent);
        }
    }
    // No children eligible for handling the mouse, so attempt to handle it ourself.
    return HandleMouseDown(button, position, motionEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::HandleMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent)
{
    bool handled = false;

    if (!HasFlags(PViewFlags::IgnoreMouse))
    {
        if (button < PMouseButton::FirstTouchID) {
            handled = DispatchMouseDown(button, position, motionEvent);
        } else {
            handled = DispatchTouchDown(button, position, motionEvent);
        }
    }
    if (handled)
    {
        PApplication* app = GetApplication();
        if (app != nullptr) {
            app->SetMouseDownView(button, ptr_tmp_cast(this), motionEvent);
        }
    }
    else
    {
        Ptr<PView> parent = GetParent();

        if (parent != nullptr)
        {
            // Event not handled by us. Check if the mouse hit any overlapping siblings below us.
            const PPoint parentPos = ConvertToParent(position);
            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->rend())
            {
                for (++i; i != parent->rend(); ++i)
                {
                    Ptr<PView> sibling = *i;
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

void PView::HandleMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent)
{
    PApplication* app = GetApplication();
    Ptr<PView> mouseView = (app != nullptr) ? app->GetMouseDownView(button) : nullptr;
    if (mouseView != nullptr)
    {
        if (button < PMouseButton::FirstTouchID) {
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

void PView::HandleMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent)
{
    PApplication* app = GetApplication();
    Ptr<PView> mouseView = (app != nullptr) ? app->GetFocusView(button) : nullptr;
    if (mouseView != nullptr)
    {
        if (button < PMouseButton::FirstTouchID) {
            mouseView->DispatchMouseMove(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), motionEvent);
        } else {
            mouseView->DispatchTouchMove(button, mouseView->ConvertFromRoot(ConvertToRoot(position)), motionEvent);
        }
    }
    else if (m_HideCount == 0 && !HasFlags(PViewFlags::IgnoreMouse))
    {
        if (button < PMouseButton::FirstTouchID) {
            DispatchMouseMove(button, position, motionEvent);
        } else {
            DispatchTouchMove(button, position, motionEvent);
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::ScrollBy(const PPoint& offset)
{
    if (offset.x == 0.0f && offset.y == 0.0f) {
        return;
    }

    m_ScrollOffset += offset;
    UpdatePosition(PView::UpdatePositionNotifyServer::IfChanged);
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

void PView::CopyRect(const PRect& srcRect, const PPoint& dstPos)
{
    if (m_BeginPainCount != 0) {
        m_DidScrollRect = true;
    }
    Post<ASViewCopyRect>(srcRect, dstPos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::DrawBitmap(Ptr<const PBitmap> bitmap, const PRect& srcRect, const PPoint& dstPos)
{
    Post<ASViewDrawBitmap>(bitmap->m_Handle, srcRect, dstPos);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::DrawBitmap(Ptr<const PBitmap> bitmap, const PRect& srcRect, const PRect& dstRect)
{
    Post<ASViewDrawScaledBitmap>(bitmap->m_Handle, srcRect, dstRect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::DrawFrame( const PRect& rect, uint32_t syleFlags)
{
    PRect frame(rect);
    frame.Floor();
    bool sunken = false;

    if (((syleFlags & FRAME_RAISED) == 0) && (syleFlags & (FRAME_RECESSED))) {
        sunken = true;
    }

    PColor fgColor = pget_standard_color(PStandardColorID::Shine);
    PColor bgColor = pget_standard_color(PStandardColorID::Shadow);

    if (syleFlags & FRAME_DISABLED) {
        fgColor = Tint(fgColor, 0.6f);
        bgColor = Tint(bgColor, 0.4f);
    }
    PColor fgShadowColor = Tint(fgColor, 0.6f);
    PColor bgShadowColor = Tint(bgColor, 0.5f);
  
    if (syleFlags & FRAME_FLAT)
    {
        SetFgColor(sunken ? bgColor : fgColor);
        MovePenTo(frame.left, frame.bottom - 1.0f);
        DrawLine(PPoint( frame.left, frame.top));
        DrawLine(PPoint(frame.right - 1.0f, frame.top));
        DrawLine(PPoint(frame.right - 1.0f, frame.bottom - 1.0f));
        DrawLine(PPoint(frame.left, frame.bottom - 1.0f));
    }
    else
    {
        if (syleFlags & FRAME_THIN) {
            SetFgColor(sunken ? bgColor : fgColor);
        } else {
            SetFgColor(sunken ? bgColor : fgShadowColor);
        }

        MovePenTo(frame.left, frame.bottom - 1.0f);
        DrawLine(PPoint(frame.left, frame.top));
        DrawLine(PPoint(frame.right - 1.0f, frame.top));

        if (syleFlags & FRAME_THIN) {
            SetFgColor(sunken ? fgColor : bgColor);
        } else {
            SetFgColor(sunken ? fgColor : bgShadowColor);
        }
        DrawLine(PPoint(frame.right - 1.0f, frame.bottom - 1.0f));
        DrawLine(PPoint(frame.left, frame.bottom - 1.0f));


        if ((syleFlags & FRAME_THIN) == 0)
        {
            if (syleFlags & FRAME_ETCHED)
            {
                SetFgColor(sunken ? bgColor : fgColor);

                MovePenTo(frame.left + 1.0f, frame.bottom - 2.0f);

                DrawLine(PPoint(frame.left + 1.0f, frame.top + 1.0f));
                DrawLine(PPoint(frame.right - 2.0f, frame.top + 1.0f));

                SetFgColor(sunken ? fgColor : bgColor);

                DrawLine(PPoint(frame.right - 2.0f, frame.bottom - 2.0f));
                DrawLine(PPoint(frame.left + 1.0f, frame.bottom - 2.0f));
            }
            else
            {
                SetFgColor(sunken ? bgShadowColor : fgColor);

                MovePenTo(frame.left + 1.0f, frame.bottom - 2.0f);

                DrawLine(PPoint(frame.left + 1.0f, frame.top + 1.0f));
                DrawLine(PPoint(frame.right - 2.0f, frame.top + 1.0f));

                SetFgColor(sunken ? fgShadowColor : bgColor);

                DrawLine(PPoint(frame.right - 2.0f, frame.bottom - 2.0f));
                DrawLine(PPoint(frame.left + 1.0f, frame.bottom - 2.0f));
            }
            if ((syleFlags & FRAME_TRANSPARENT) == 0) {
                EraseRect(PRect(frame.left + 2.0f, frame.top + 2.0f, frame.right - 2.0f, frame.bottom - 2.0f));
            }
        }
        else
        {
            if ((syleFlags & FRAME_TRANSPARENT) == 0) {
                EraseRect(PRect(frame.left + 1.0f, frame.top + 1.0f, frame.right - 1.0f, frame.bottom - 1.0f));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PFontHeight PView::GetFontHeight() const
{
    if (m_Font != nullptr)
    {
        return m_Font->GetHeight();
    }
    else
    {
        PFontHeight height;
        height.ascender  = 0.0f;
        height.descender = 0.0f;
        height.line_gap  = 0.0f;
        return height;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PView::GetStringWidth(const char* string, size_t length) const
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

void PView::Flush()
{
    PApplication* app = GetApplication();
    if (app != nullptr) {
        app->Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::Sync()
{
    PApplication* app = GetApplication();
    if (app != nullptr) {
        app->Sync();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Called from ViewBase::LinkChild() to tell the view that it has been
/// attached to a parent.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::HandleAddedToParent(Ptr<PView> parent, size_t index)
{
    UpdatePosition(PView::UpdatePositionNotifyServer::IfChanged);
    if (!parent->IsVisible()) {
        Show(false);
    }
    if (parent->HasFlags(PViewFlags::IsAttachedToScreen))
    {
        parent->InvalidateLayout();

        PApplication* app = parent->GetApplication();
        if (app != nullptr)
        {
            HandlePreAttachToScreen(app);

            if (!HasFlags(PViewFlags::Eavesdropper)) {
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
    
void PView::HandleRemovedFromParent(Ptr<PView> parent)
{
    if (!parent->IsVisible()) {
        Show(true);
    }
    OnDetachedFromParent(parent);
    if (!HasFlags(PViewFlags::Eavesdropper))
    {
        PApplication* const app = GetApplication();
        if (app != nullptr) {
            app->RemoveView(ptr_tmp_cast(this));
        }
        UpdatePosition(PView::UpdatePositionNotifyServer::IfChanged);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::HandlePreAttachToScreen(PApplication* app)
{
    MergeFlags(PViewFlags::IsAttachedToScreen);
    app->AddHandler(ptr_tmp_cast(this));
    AttachedToScreen();

    PreferredSizeChanged();

    for (Ptr<PView> child : m_ChildrenList) {
        child->HandlePreAttachToScreen(app);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::HandleAttachedToScreen(PApplication* app)
{
    for (Ptr<PView> child : m_ChildrenList) {
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

void PView::HandleDetachedFromScreen()
{
    DetachedFromScreen();
    for (Ptr<PView> child : m_ChildrenList) {
        child->HandleDetachedFromScreen();
    }
    AllDetachedFromScreen();
}    

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::HandlePaint(const PRect& updateRect)
{
    PRect frame = updateRect;
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

void PView::UpdateRingSize()
{
    if (m_WidthRing == nullptr && m_HeightRing == nullptr)
    {
        if (m_LocalPrefSize[int(PPrefSizeType::Smallest)] != m_PreferredSizes[int(PPrefSizeType::Smallest)] || m_LocalPrefSize[int(PPrefSizeType::Greatest)] != m_PreferredSizes[int(PPrefSizeType::Greatest)])
        {
            m_PreferredSizes[int(PPrefSizeType::Smallest)] = m_LocalPrefSize[int(PPrefSizeType::Smallest)];
            m_PreferredSizes[int(PPrefSizeType::Greatest)] = m_LocalPrefSize[int(PPrefSizeType::Greatest)];

            SignalPreferredSizeChanged(ptr_tmp_cast(this));
            Ptr<PView> parent = GetParent();
            if (parent != nullptr)
            {
                parent->PreferredSizeChanged();
                parent->InvalidateLayout();
            }          
        }
    }
    else
    {
        PPoint ringSizes[int(PPrefSizeType::Count)];
        //// Calculate ring with. ////
        if (m_WidthRing == nullptr)
        {
            for (int i = 0; i < int(PPrefSizeType::Count); ++i) {
                ringSizes[i].x = m_LocalPrefSize[i].x;
            }
        }
        else
        {
            PView* member = this;
            do
            {
                for (int i = 0; i < int(PPrefSizeType::Count); ++i)
                {
                    if (member->m_LocalPrefSize[i].x > ringSizes[i].x) ringSizes[i].x = member->m_LocalPrefSize[i].x;
                }
                member = member->m_WidthRing;
            } while (member != this);
        }
        //// Calculate ring height. ////
        if (m_HeightRing == nullptr)
        {
            for (int i = 0; i < int(PPrefSizeType::Count); ++i) {
                ringSizes[i].y = m_LocalPrefSize[i].y;
            }
        }
        else
        {
            PView* member = this;
            do
            {
                for (int i = 0; i < int(PPrefSizeType::Count); ++i)
                {
                    if (member->m_LocalPrefSize[i].y > ringSizes[i].y) ringSizes[i].y = member->m_LocalPrefSize[i].y;
                }
                member = member->m_HeightRing;
            } while (member != this);
        }

        std::set<Ptr<PView>> modifiedViews;

        if (m_WidthRing != nullptr)
        {
            PView* member = this;
            do
            {
                bool modified = false;
                for (int i = 0; i < int(PPrefSizeType::Count); ++i)
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
            for (int i = 0; i < int(PPrefSizeType::Count); ++i)
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
            PView* member = this;
            do
            {
                bool modified = false;
                for (int i = 0; i < int(PPrefSizeType::Count); ++i)
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
            for (int i = 0; i < int(PPrefSizeType::Count); ++i)
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

        std::set<Ptr<PView>> notifiedParents;
        for (Ptr<PView> modifiedView : modifiedViews)
        {
            modifiedView->SignalPreferredSizeChanged(modifiedView);
            Ptr<PView> parent = modifiedView->GetParent();
            if (parent != nullptr) {
                notifiedParents.insert(parent);
            }
        }
        modifiedViews.clear();

        for (Ptr<PView> modifiedParent : notifiedParents)
        {
            modifiedParent->PreferredSizeChanged();
            modifiedParent->InvalidateLayout();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetVScrollBar(PScrollBar* scrollBar)
{
    m_VScrollBar = scrollBar;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetHScrollBar(PScrollBar* scrollBar)
{
    m_HScrollBar = scrollBar;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SetFrameInternal(const PRect& frame, bool notifyServer)
{
    const PPoint deltaSize = frame.Size() - m_Frame.Size();
    const PPoint deltaPos = frame.TopLeft() - m_Frame.TopLeft();

    m_Frame = frame;

    UpdatePosition(notifyServer ? PView::UpdatePositionNotifyServer::Always : PView::UpdatePositionNotifyServer::Never);
    if (deltaSize != PPoint(0.0f, 0.0f))
    {
        OnFrameSized(deltaSize);
        SignalFrameSized(deltaSize, ptr_tmp_cast(this));
    }
    if (deltaPos != PPoint(0.0f, 0.0f))
    {
        OnFrameMoved(deltaPos);
        SignalFrameMoved(deltaPos, ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::UpdatePosition(UpdatePositionNotifyServer notifyMode)
{
    PPoint newOffset;
    PPoint newScreenPos;
    {
        Ptr<PView> parent = m_Parent.Lock();
        if (parent != nullptr && !parent->HasFlags(PViewFlags::WillDraw)) {
            newOffset = parent->m_PositionOffset + parent->m_Frame.TopLeft();
        } else {
            newOffset = PPoint(0.0f, 0.0f);
        }
        if (parent == nullptr) {
            newScreenPos = m_Frame.TopLeft();
        } else {
            newScreenPos = parent->m_ScreenPos + parent->m_ScrollOffset + m_Frame.TopLeft();
        }
    }
    if (notifyMode == PView::UpdatePositionNotifyServer::Always || (notifyMode == PView::UpdatePositionNotifyServer::IfChanged && newOffset != m_PositionOffset))
    {
        m_PositionOffset = newOffset;
        if (m_ServerHandle != INVALID_HANDLE)
        {
            Post<ASViewSetFrame>(m_Frame + m_PositionOffset, GetHandle());
        }
    }
    PPoint deltaScreenPos = m_ScreenPos - newScreenPos;
    m_ScreenPos = newScreenPos;

    for (Ptr<PView> child : m_ChildrenList) {
        child->UpdatePosition((notifyMode == PView::UpdatePositionNotifyServer::Never) ? PView::UpdatePositionNotifyServer::Never : PView::UpdatePositionNotifyServer::IfChanged);
    }
    if (deltaScreenPos.x != 0.0f || deltaScreenPos.y != 0.0f) {
        OnScreenFrameMoved(deltaScreenPos);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PView::DispatchTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)
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

bool PView::DispatchTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)
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

bool PView::DispatchTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)
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

bool PView::DispatchLongPress(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)
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

bool PView::DispatchMouseDown(PMouseButton buttonID, const PPoint& position, const PMotionEvent& motionEvent)
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

bool PView::DispatchMouseUp(PMouseButton buttonID, const PPoint& position, const PMotionEvent& motionEvent)
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

bool PView::DispatchMouseMove(PMouseButton buttonID, const PPoint& position, const PMotionEvent& motionEvent)
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

void PView::DispatchKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)
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

void PView::DispatchKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)
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

void PView::SlotFrameChanged(const PRect& frame)
{
    SetFrameInternal(frame, false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PView::SlotKeyboardFocusChanged(bool hasFocus)
{
    PApplication* app = GetApplication();
    if (app != nullptr) {
        app->SetKeyboardFocus(ptr_tmp_cast(this), hasFocus, false);
    }
}
