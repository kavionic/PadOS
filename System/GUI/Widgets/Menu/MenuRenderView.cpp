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

#include "MenuRenderView.h"
#include <GUI/Widgets/MenuItem.h>


namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point MenuRenderView::GetContentSize() const
{
    return m_Menu->m_ContentSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MenuRenderView::Paint(const Rect& updateRect)
{
    SetFgColor(StandardColorID::MenuBackground);
    FillRect(GetBounds());

    Ptr<View> self = ptr_tmp_cast(this);
    for (Ptr<MenuItem> item : m_Menu->m_Items) {
        item->Draw(self);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MenuRenderView::OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (m_HitButton != MouseButton_e::None) {
        return View::OnTouchDown(pointID, position, event);
    }

    if (m_Menu->m_HasOpenChildren)
    {
        for (Ptr<MenuItem> item : m_Menu->m_Items)
        {
            if (item->m_SubMenu != nullptr && item->m_SubMenu->m_IsOpen) {
                item->m_SubMenu->Close(true, false, nullptr);
                break;
            }
        }
        return true;
    }

    m_HitPos    = position;
    m_HitButton = pointID;

    Ptr<MenuItem> item = m_Menu->GetItemAt(position);

    if (item != nullptr)
    {
        if (!m_Menu->HasFlags(MenuFlags::NoKeyboardFocus)) {
            m_Menu->SetKeyboardFocus(true);
        }
        m_Menu->SelectItem(item);
    }

    MakeFocus(pointID, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MenuRenderView::OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchUp(pointID, position, event);
    }

    m_HitButton = MouseButton_e::None;

    if (m_MouseMoved)
    {
        m_Menu->EndSwipe();
        m_MouseMoved = false;
    }
    else
    {
        Ptr<MenuItem> item = m_Menu->FindMarked();

        if (item != nullptr && item == m_Menu->GetItemAt(position) && item->m_SubMenu == nullptr)
        {
            m_Menu->Close(false, true, item);
        }
    }
    MakeFocus(pointID, false);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MenuRenderView::OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchMove(pointID, position, event);
    }
    if (m_MouseMoved)
    {
        m_Menu->SwipeMove(position);
        m_Menu->SelectItem(nullptr);
    }
    else
    {
        if ((position - m_HitPos).LengthSqr() > 20.0f * 20.0f)
        {
            m_Menu->BeginSwipe(position);
            m_MouseMoved = true;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MenuRenderView::OnLongPress(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MenuRenderView::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (GetBounds().DoIntersect(position))
    {
        Ptr<MenuItem> pcItem = m_Menu->GetItemAt(position);

        if (pcItem != nullptr)
        {
            if (!m_Menu->HasFlags(MenuFlags::NoKeyboardFocus)) {
                m_Menu->SetKeyboardFocus(true);
            }
            m_Menu->SelectItem(pcItem);
        }
    }
    else
    {
        if (!m_Menu->m_HasOpenChildren)
        {
            m_Menu->Close(false, true, nullptr);
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MenuRenderView::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (GetBounds().DoIntersect(position))
    {
        Ptr<MenuItem> item = m_Menu->FindMarked();

        if (item != nullptr && item == m_Menu->GetItemAt(position) && item->m_SubMenu == nullptr)
        {
            m_Menu->Close(false, true, item);
        }
    }
    else
    {
        if (!m_Menu->m_HasOpenChildren) {
            m_Menu->Close(false, true, nullptr);
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MenuRenderView::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (GetBounds().DoIntersect(position))
    {
        Ptr<MenuItem> item = m_Menu->GetItemAt(position);

        if (item != nullptr)
        {
            m_Menu->SelectItem(item);
//            if (m_Menu->m_eLayout == MenuLayout::Horizontal) {
//                m_Menu->OpenSelection();
//            } else {
//                m_Menu->StartOpenTimer(0.2);
//            }
        }
        return true;
    }
    return false;
}

} // namespace os;
