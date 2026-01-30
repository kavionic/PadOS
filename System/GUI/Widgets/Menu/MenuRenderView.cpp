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

#include "MenuRenderView.h"
#include <GUI/Widgets/MenuItem.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PMenuRenderView::CalculateContentSize() const
{
    return m_Menu->m_ContentSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuRenderView::OnPaint(const PRect& updateRect)
{
    SetFgColor(PStandardColorID::MenuBackground);
    FillRect(GetBounds());

    Ptr<PView> self = ptr_tmp_cast(this);
    for (Ptr<PMenuItem> item : m_Menu->m_Items) {
        item->Draw(self);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PMenuRenderView::OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (m_HitButton != PMouseButton::None) {
        return PView::OnTouchDown(pointID, position, event);
    }

    if (m_Menu->m_HasOpenChildren)
    {
        for (Ptr<PMenuItem> item : m_Menu->m_Items)
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

    Ptr<PMenuItem> item = m_Menu->GetItemAt(position);

    if (item != nullptr)
    {
        if (!m_Menu->HasFlags(PMenuFlags::NoKeyboardFocus)) {
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

bool PMenuRenderView::OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (pointID != m_HitButton) {
        return PView::OnTouchUp(pointID, position, event);
    }

    m_HitButton = PMouseButton::None;

    if (m_MouseMoved)
    {
        m_Menu->EndSwipe();
        m_MouseMoved = false;
    }
    else
    {
        Ptr<PMenuItem> item = m_Menu->FindMarked();

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

bool PMenuRenderView::OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (pointID != m_HitButton) {
        return PView::OnTouchMove(pointID, position, event);
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

bool PMenuRenderView::OnLongPress(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PMenuRenderView::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (GetBounds().DoIntersect(position))
    {
        Ptr<PMenuItem> pcItem = m_Menu->GetItemAt(position);

        if (pcItem != nullptr)
        {
            if (!m_Menu->HasFlags(PMenuFlags::NoKeyboardFocus)) {
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

bool PMenuRenderView::OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (GetBounds().DoIntersect(position))
    {
        Ptr<PMenuItem> item = m_Menu->FindMarked();

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

bool PMenuRenderView::OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (GetBounds().DoIntersect(position))
    {
        Ptr<PMenuItem> item = m_Menu->GetItemAt(position);

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
