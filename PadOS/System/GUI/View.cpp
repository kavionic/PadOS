// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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

#include "sam.h"


#include <algorithm>
#include <stddef.h>
#include <stdio.h>

#include "View.h"
#include "Application.h"

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

View::View(const String& name, Ptr<View> parent, uint32_t flags) : ViewBase(name, Rect(), Point(), flags, 0, Color(0xffffffff), Color(0xffffffff), Color(0))
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

View::View(Ptr<View> parent, handler_id serverHandle, const String& name, const Rect& frame) : ViewBase(name, frame, Point(), ViewFlags::EAVESDROPPER, 0, Color(0xffffffff), Color(0xffffffff), Color(0))
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
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::~View()
{
    SetLayoutNode(nullptr);
    
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
    if (parent != nullptr)
    {
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
    if (parent != nullptr)
    {
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
    if (parent != nullptr)
    {
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
    if (parent != nullptr)
    {
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

void View::InvalidateLayout()
{
    if ( m_LayoutNode != nullptr ) {
        m_LayoutNode->Layout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::AdjustPrefSize(Point* minSize, Point* maxSize)
{
    if ( m_LayoutNode != nullptr ) {
        m_LayoutNode->AdjustPrefSize(minSize, maxSize);
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
    if (m_LayoutNode != nullptr)
    {
        m_LayoutNode->Layout();
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

Point View::GetPreferredSize(bool largest) const
{
    if (m_LayoutNode != nullptr) {
        Point minSize = m_LayoutNode->GetPreferredSize(false);
        Point maxSize = m_LayoutNode->GetPreferredSize(true);
        m_LayoutNode->AdjustPrefSize(&minSize, &maxSize);
        return largest ? maxSize : minSize;
    } else {
        return largest ? Point(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()) : Point(0.0f, 0.0f);
    }        
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
    Ptr<View> parent = GetParent();
    if (parent != nullptr)
    {
        parent->PreferredSizeChanged();
        parent->InvalidateLayout();        
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

void View::SetFrame(const Rect& frame)
{
    Point deltaSize = frame.Size() - m_Frame.Size();
    Point deltaPos  = frame.TopLeft() - m_Frame.TopLeft();
    m_Frame = frame;
    m_IFrame = frame;
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

void View::DrawFrame( const Rect& rect, uint32_t syleFlags)
{
    Rect frame(rect);
    frame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
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
                EraseRect(Rect(frame.left + 2.0f, frame.top + 2.0f, frame.right - 3.0f, frame.bottom - 3.0f));
            }
        }
        else
        {
            if ((syleFlags & FRAME_TRANSPARENT) == 0) {
                EraseRect(Rect(frame.left + 1.0f, frame.top + 1.0f, frame.right - 2.0f, frame.bottom - 2.0f));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::DrawBevelBox(Rect frame, bool raised)
{
    Color colorDark(0xa0,0xa0,0xa0);
    Color colorLight(0xe0,0xe0,0xe0);
    if ( !raised ) std::swap(colorDark, colorLight);

    SetFgColor(colorLight);
    DrawLine(frame.left, frame.top, frame.right, frame.top);
    
    SetFgColor(colorDark);
    DrawLine(frame.right, frame.top + 1, frame.right, frame.bottom);
    DrawLine(frame.right - 1, frame.bottom, frame.left - 1, frame.bottom);
    
    SetFgColor(colorLight);
    DrawLine(frame.left, frame.bottom, frame.left, frame.top);

    colorDark.SetRGBA(0xc0,0xc0,0xc0);
    colorLight.SetRGBA(0xff,0xff,0xff);
    if ( !raised ) std::swap(colorDark, colorLight);
    
    frame.Resize(1.0f, 1.0f, -1.0f, -1.0f);
    
    SetFgColor(colorLight);
    DrawLine(frame.left, frame.top, frame.right, frame.top);

    SetFgColor(colorDark);
    DrawLine(frame.right, frame.top + 1, frame.right, frame.bottom);
    DrawLine(frame.right - 1, frame.bottom, frame.left - 1, frame.bottom);

    SetFgColor(colorLight);
    DrawLine(frame.left, frame.bottom, frame.left, frame.top);

    frame.Resize(1.0f, 1.0f, -1.0f, -1.0f);
    FillRect(frame, (raised) ? Color(0xd0,0xd0,0xd0) : Color(0xf0,0xf0,0xf0) );
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
    if (parent->m_ServerHandle != -1 && !HasFlag(ViewFlags::EAVESDROPPER))
    {
        parent->GetApplication()->AddView(ptr_tmp_cast(this), ViewDockType::ChildView);
        
        if (m_LayoutNode != nullptr) {
            m_LayoutNode->Layout();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Called from ViewBase::UnlinkChild() to tell the view that it has been
/// detached from the parent.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void View::HandleRemovedFromParent(Ptr<View> parent)
{
    if (m_ServerHandle != -1 && !HasFlag(ViewFlags::EAVESDROPPER)) {
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
    } else {
        Flush();
    }
}

