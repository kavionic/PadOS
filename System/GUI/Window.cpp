// This file is part of PadOS.
//
// Copyright (C) 2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.04.2021 15:40

#include <GUI/Window.h>

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Window::Window(const String& title) : View("Window", nullptr, ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize), m_Title(title)
{
    FontHeight fontHeight = GetFontHeight();
    m_ClientBorders = Rect(8.0f, fontHeight.descender - fontHeight.ascender + 8.0f, 8.0f, 8.0f);
    Show(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Window::Paint(const Rect& updateRect)
{
    Rect outerFrame = GetBounds();
    Rect innerFrame = outerFrame;

    innerFrame.Resize(m_ClientBorders.left - 2.0f, m_ClientBorders.top - 2.0f, -m_ClientBorders.right + 2.0f, -m_ClientBorders.bottom + 2.0f);

    DrawFrame(outerFrame, FRAME_RAISED | FRAME_TRANSPARENT);

    DrawFrame(innerFrame, FRAME_RECESSED | FRAME_TRANSPARENT);

    SetFgColor(StandardColorID::WindowBorderActive);
    FillRect(Rect(outerFrame.left + 2.0f, innerFrame.top, innerFrame.left, innerFrame.bottom));                     // Left edge.
    FillRect(Rect(innerFrame.right, innerFrame.top, outerFrame.right - 2.0f, innerFrame.bottom));                   // Right edge.
    FillRect(Rect(outerFrame.left + 2.0f, innerFrame.bottom, outerFrame.right - 2.0f, outerFrame.bottom - 2.0f));   // Bottom edge.
    FillRect(Rect(outerFrame.left + 2.0f, outerFrame.top + 2.0f, outerFrame.right - 2.0f, innerFrame.top));         // Top edge.

    SetFgColor(NamedColors::black);
    SetBgColor(StandardColorID::WindowBorderActive);

    MovePenTo(10.0f, 4.0f);
    DrawString(m_Title);

    SetFgColor(StandardColorID::DefaultBackground);

    innerFrame.Resize(2.0f, 2.0f, -2.0f, -2.0f);
    FillRect(innerFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Window::FrameSized(const Point& delta)
{
    if (m_ClientView != nullptr)
    {
        Rect frame = GetBounds();
        frame.Resize(m_ClientBorders.left, m_ClientBorders.top, -m_ClientBorders.right, -m_ClientBorders.bottom);
        m_ClientView->SetFrame(frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Window::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    if (m_ClientView != nullptr)
    {
        Point borderSize(m_ClientBorders.left + m_ClientBorders.right, m_ClientBorders.top + m_ClientBorders.bottom);
        *minSize = m_ClientView->GetPreferredSize(PrefSizeType::Smallest) + borderSize;
        *maxSize = m_ClientView->GetPreferredSize(PrefSizeType::Greatest) + borderSize;
    }
    else
    {
        View::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
        *minSize = Point(m_ClientBorders.left + m_ClientBorders.right, m_ClientBorders.top + m_ClientBorders.bottom);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Window::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (m_DragHitButton != MouseButton_e::None) {
        return false;
    }
    if (m_ClientView != nullptr && m_ClientView->GetFrame().DoIntersect(position)) {
        return false;
    }
    m_DragHitButton = button;
    m_DragHitPos = event.Position;
    MakeFocus(button, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Window::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (button != m_DragHitButton) {
        return false;
    }
    MakeFocus(button, false);
    m_DragHitButton = MouseButton_e::None;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Window::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (button != m_DragHitButton) {
        return false;
    }

    Point delta = event.Position - m_DragHitPos;
    m_DragHitPos = event.Position;
    MoveBy(delta);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Window::SetClient(Ptr<View> client)
{
    if (m_ClientView != nullptr) {
        m_ClientView->SignalPreferredSizeChanged.Disconnect(this, &Window::SlotClientPreferredSizeChanged);
        RemoveChild(m_ClientView);
    }
    m_ClientView = client;
    if (m_ClientView != nullptr)
    {
        m_ClientView->SetFrame(GetBounds());
        m_ClientView->SignalPreferredSizeChanged.Connect(this, &Window::SlotClientPreferredSizeChanged);
        AddChild(m_ClientView);
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> Window::GetClient()
{
    return m_ClientView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Window::Open(Application* application)
{
    if (application == nullptr) {
        application = Application::GetDefaultApplication();
    }
    Rect screenFrame = Application::GetScreenFrame();
    Rect frame(Point(0.0f, 0.0f), GetPreferredSize(PrefSizeType::Smallest));

    frame += ((screenFrame.Size() - frame.Size()) * 0.5f).GetRounded();

    SetFrame(frame);

    application->AddView(ptr_tmp_cast(this), ViewDockType::PopupWindow);

    Show(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Window::Close()
{
    Application* application = GetApplication();
    if (application != nullptr)
    {
        application->RemoveView(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Window::SlotClientPreferredSizeChanged()
{
    PreferredSizeChanged();

    Rect screenFrame = Application::GetScreenFrame();
    Rect frame(Point(0.0f, 0.0f), GetPreferredSize(PrefSizeType::Smallest));

    frame += ((screenFrame.Size() - frame.Size()) * 0.5f).GetRounded();

    SetFrame(frame);
}

} //namespace os
