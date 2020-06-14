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
#include <stdio.h>

#include "GUI/View.h"
#include "App/Application.h"
#include "Utils/Utils.h"

using namespace kernel;
using namespace os;

WeakPtr<View> View::s_MouseDownView;

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
    return Color(r, g, b, color.GetAlpha());
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::View(Ptr<View> parent, handler_id serverHandle, const String& name, const Rect& frame) : ViewBase(name, frame, Point(), ViewFlags::Eavesdropper, 0, Color(0xffffffff), Color(0xffffffff), Color(0))
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
    RegisterRemoteSignal(&RSHandleMouseDown, &View::HandleMouseDown);
    RegisterRemoteSignal(&RSHandleMouseUp, &View::HandleMouseUp);
    RegisterRemoteSignal(&RSHandleMouseMove, &View::HandleMouseMove);
    
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::~View()
{
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

Application* View::GetApplication()
{
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

bool View::OnMouseDown(MouseButton_e button, const Point& position)
{
    if (!VFMouseDown.Empty()) {
        return VFMouseDown(button, position);
    } else {
        return false;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseUp(MouseButton_e button, const Point& position)
{
    if (!VFMouseUp.Empty()) {
        return VFMouseUp(button, position);
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseMove(MouseButton_e button, const Point& position)
{
    if (!VFMouseMoved.Empty()) {
        return VFMouseMoved(position);
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
            case SizeOverride::Always:    sizes[i].x = m_WidthOverride[i]; break;
            case SizeOverride::IfSmaller: sizes[i].x = std::max(sizes[i].x, m_WidthOverride[i]); break;
            case SizeOverride::IfGreater: sizes[i].x = std::min(sizes[i].x, m_WidthOverride[i]); break;
        }
        switch(m_HeightOverrideType[i])
        {
            case SizeOverride::None: break;
            case SizeOverride::Always:    sizes[i].y = m_HeightOverride[i]; break;
            case SizeOverride::IfSmaller: sizes[i].y = std::max(sizes[i].y, m_HeightOverride[i]); break;
            case SizeOverride::IfGreater: sizes[i].y = std::min(sizes[i].y, m_HeightOverride[i]); break;
        }
        if (sizes[i].x > LAYOUT_MAX_SIZE) sizes[i].x = LAYOUT_MAX_SIZE;
        if (sizes[i].y > LAYOUT_MAX_SIZE) sizes[i].y = LAYOUT_MAX_SIZE;
    }
    
    if (sizes[int(PrefSizeType::Smallest)] != m_LocalPrefSize[int(PrefSizeType::Smallest)] || sizes[int(PrefSizeType::Greatest)] != m_LocalPrefSize[int(PrefSizeType::Greatest)])
    {
        m_LocalPrefSize[int(PrefSizeType::Smallest)] = sizes[int(PrefSizeType::Smallest)];
        m_LocalPrefSize[int(PrefSizeType::Greatest)] = sizes[int(PrefSizeType::Greatest)];
        
        UpdateRingSize();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ContentSizeChanged()
{
    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::AddChild(Ptr<View> child)
{
    LinkChild(child, true);
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

void View::SetFrame(const Rect& frame)
{
    Point deltaSize = frame.Size() - m_Frame.Size();
    Point deltaPos  = frame.TopLeft() - m_Frame.TopLeft();
    m_Frame = frame;

    UpdatePosition(true);
    UpdateScreenPos();
    if (deltaSize != Point(0.0f, 0.0f)) {
        FrameSized(deltaSize);
    }
    if (deltaPos != Point(0.0f, 0.0f)) {
        FrameMoved(deltaPos);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Invalidate(const Rect& rect, bool recurse)
{
    Post<ASViewInvalidate>(IRect(rect));
/*    Application* app = GetApplication();
    if (app != nullptr)
    {
        app->InvalidateView(m_ServerHandle, IRect(rect));
    }*/
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

bool View::HandleMouseDown(MouseButton_e button, const Point& position)
{
    for (Ptr<View> child : m_ChildrenList)
    {
        if (child->m_Frame.DoIntersect(position))
        {
            Point childPos = position - child->m_Frame.TopLeft() - child->m_ScrollOffset;
            if (child->HandleMouseDown(button, childPos))
            {
                return true;
            }
        }
    }
    bool handled = OnMouseDown(button, position/* - m_ScrollOffset*/);
    if (handled)
    {
        s_MouseDownView = ptr_tmp_cast(this);
        //Application* app = GetApplication();
/*        if (app != nullptr) {
            app->SetMouseDownView(ptr_tmp_cast(this));
        }            */
    }
    return handled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleMouseUp(MouseButton_e button, const Point& position)
{
    Ptr<View> mouseView = s_MouseDownView.Lock();
    if (mouseView != nullptr) {
//        printf("View::HandleMouseUp() %p '%s'\n", ptr_raw_pointer_cast(mouseView), mouseView->GetName().c_str());

        mouseView->OnMouseUp(button, mouseView->ConvertFromRoot(ConvertToRoot(position)));
    } else {
        OnMouseUp(button, position);
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleMouseMove(MouseButton_e button, const Point& position)
{
    Ptr<View> mouseView = s_MouseDownView.Lock();
    if (mouseView != nullptr) {
        mouseView->OnMouseMove(button, mouseView->ConvertFromRoot(ConvertToRoot(position)));
    } else {
        OnMouseMove(button, position);
    }        
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
    if (parent->m_ServerHandle != -1 && !HasFlags(ViewFlags::Eavesdropper))
    {
        parent->GetApplication()->AddView(ptr_tmp_cast(this), ViewDockType::ChildView);
    }
    parent->PreferredSizeChanged();
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
	    SignalPreferredSizeChanged();
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
