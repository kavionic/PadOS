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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View::View(const String& name) : ViewBase(name)
{
    RegisterRemoteSignal(&RSPaintView, &View::HandlePaint);
    RegisterRemoteSignal(&RSHandleMouseDown, &View::HandleMouseDown);
    RegisterRemoteSignal(&RSHandleMouseUp, &View::HandleMouseUp);
    RegisterRemoteSignal(&RSHandleMouseMove, &View::HandleMouseMove);    
    
//    SetFont(ptr_new<Font>(GfxDriver::e_Font7Seg));
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

Point View::GetPreferredSize(bool largest) const
{
    return largest ? Point(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()) : Point(0.0f, 0.0f);
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

void View::SetFrame(const Rect& frame, bool notifyServer)
{
    m_Frame = frame;
    UpdateScreenPos();
    if (m_ServerHandle != -1) {
        GetApplication()->SetViewFrame(m_ServerHandle, frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::Invalidate(const Rect& rect, bool recurse)
{
    Application* app = GetApplication();
    if (app != nullptr)
    {
        app->InvalidateView(m_ServerHandle, IRect(rect));
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
            Point childPos = position - child->m_Frame.LeftTop() - m_ScrollOffset;
            if (child->HandleMouseDown(button, childPos))
            {
                return true;
            }
        }
    }
    bool handled = OnMouseDown(button, position - m_ScrollOffset);
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

void View::FillRect(const Rect& rect, Color color)
{
    SetFgColor(color);
    FillRect(rect);
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
    if (m_ServerHandle != -1)
    {
        Application* app = GetApplication();
        if (app != nullptr)
        {
            Post<ASViewSync>();
            Flush();
            Ptr<View> lifeInsurance = ptr_tmp_cast(this);
            app->WaitForReply(-1, AppserverProtocol::VIEW_SYNC_REPLY);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// Called from ViewBase::LinkChild() to tell the view that it has been
/// attached to a parent.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::HandleAddedToParent(Ptr<View> parent)
{
    if (parent->m_ServerHandle != -1)
    {
        GetApplication()->AddView(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Called from ViewBase::UnlinkChild() to tell the view that it has been
/// detached from the parent.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void View::HandleRemovedFromParent(Ptr<View> parent)
{
    if (m_ServerHandle != -1) {
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
    ConstrictRectangle(&frame, Point(0.0f, 0.0f));
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void View::ConstrictRectangle(Rect* rect, const Point& offset)
{
    Point off = offset - Point( m_Frame.left, m_Frame.top ) - m_ScrollOffset;
    *rect &= m_Frame + off;
    Ptr<View> parent = m_Parent.Lock();
    if (parent != nullptr) {
        parent->ConstrictRectangle(rect, off);
    }
}
