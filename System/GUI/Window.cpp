// This file is part of PadOS.
//
// Copyright (C) 2021-2025 Kurt Skauen <http://kavionic.com/>
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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PWindow::PWindow(const PString& title) : PView("Window", nullptr, PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize), m_Title(title)
{
    PFontHeight fontHeight = GetFontHeight();
    m_ClientBorders = PRect(8.0f, fontHeight.descender - fontHeight.ascender + 8.0f, 8.0f, 8.0f);
    Show(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PWindow::OnPaint(const PRect& updateRect)
{
    PRect outerFrame = GetBounds();
    PRect innerFrame = outerFrame;

    innerFrame.Resize(m_ClientBorders.left - 2.0f, m_ClientBorders.top - 2.0f, -m_ClientBorders.right + 2.0f, -m_ClientBorders.bottom + 2.0f);

    DrawFrame(outerFrame, FRAME_RAISED | FRAME_TRANSPARENT);

    DrawFrame(innerFrame, FRAME_RECESSED | FRAME_TRANSPARENT);

    SetFgColor(PStandardColorID::WindowBorderActive);
    FillRect(PRect(outerFrame.left + 2.0f, innerFrame.top, innerFrame.left, innerFrame.bottom));                     // Left edge.
    FillRect(PRect(innerFrame.right, innerFrame.top, outerFrame.right - 2.0f, innerFrame.bottom));                   // Right edge.
    FillRect(PRect(outerFrame.left + 2.0f, innerFrame.bottom, outerFrame.right - 2.0f, outerFrame.bottom - 2.0f));   // Bottom edge.
    FillRect(PRect(outerFrame.left + 2.0f, outerFrame.top + 2.0f, outerFrame.right - 2.0f, innerFrame.top));         // Top edge.

    SetFgColor(PNamedColors::black);
    SetBgColor(PStandardColorID::WindowBorderActive);

    MovePenTo(10.0f, 4.0f);
    DrawString(m_Title);

    SetFgColor(PStandardColorID::DefaultBackground);

    innerFrame.Resize(2.0f, 2.0f, -2.0f, -2.0f);
    FillRect(innerFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PWindow::OnFrameSized(const PPoint& delta)
{
    if (m_ClientView != nullptr)
    {
        PRect frame = GetBounds();
        frame.Resize(m_ClientBorders.left, m_ClientBorders.top, -m_ClientBorders.right, -m_ClientBorders.bottom);
        m_ClientView->SetFrame(frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PWindow::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    if (m_ClientView != nullptr)
    {
        PPoint borderSize(m_ClientBorders.left + m_ClientBorders.right, m_ClientBorders.top + m_ClientBorders.bottom);
        *minSize = m_ClientView->GetPreferredSize(PPrefSizeType::Smallest) + borderSize;
        *maxSize = m_ClientView->GetPreferredSize(PPrefSizeType::Greatest) + borderSize;
    }
    else
    {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
        *minSize = PPoint(m_ClientBorders.left + m_ClientBorders.right, m_ClientBorders.top + m_ClientBorders.bottom);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PWindow::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (m_DragHitButton != PMouseButton::None) {
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

bool PWindow::OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (button != m_DragHitButton) {
        return false;
    }
    MakeFocus(button, false);
    m_DragHitButton = PMouseButton::None;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PWindow::OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (button != m_DragHitButton) {
        return false;
    }

    PPoint delta = event.Position - m_DragHitPos;
    m_DragHitPos = event.Position;
    MoveBy(delta);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PWindow::SetClient(Ptr<PView> client)
{
    if (m_ClientView != nullptr) {
        m_ClientView->SignalPreferredSizeChanged.Disconnect(this, &PWindow::SlotClientPreferredSizeChanged);
        RemoveChild(m_ClientView);
    }
    m_ClientView = client;
    if (m_ClientView != nullptr)
    {
        m_ClientView->SetFrame(GetBounds());
        m_ClientView->SignalPreferredSizeChanged.Connect(this, &PWindow::SlotClientPreferredSizeChanged);
        AddChild(m_ClientView);
    }
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PWindow::GetClient()
{
    return m_ClientView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PWindow::Open(PApplication* application)
{
    if (application == nullptr) {
        application = PApplication::GetDefaultApplication();
    }
    PRect screenFrame = PApplication::GetScreenFrame();
    PRect frame(PPoint(0.0f, 0.0f), GetPreferredSize(PPrefSizeType::Smallest));

    frame += ((screenFrame.Size() - frame.Size()) * 0.5f).GetRounded();

    SetFrame(frame);

    application->AddView(ptr_tmp_cast(this), PViewDockType::PopupWindow);

    Show(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PWindow::Close()
{
    PApplication* application = GetApplication();
    if (application != nullptr)
    {
        application->RemoveView(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PWindow::SlotClientPreferredSizeChanged()
{
    PreferredSizeChanged();

    PRect screenFrame = PApplication::GetScreenFrame();
    PRect frame(PPoint(0.0f, 0.0f), GetPreferredSize(PPrefSizeType::Smallest));

    frame += ((screenFrame.Size() - frame.Size()) * 0.5f).GetRounded();

    SetFrame(frame);
}
