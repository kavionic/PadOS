// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

void PMenuRenderView::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (GetBounds().DoIntersect(position))
        {
            Ptr<PMenuItem> item = m_Menu->GetItemAt(position);

            if (item != nullptr)
            {
                if (!m_Menu->HasFlags(PMenuFlags::NoKeyboardFocus)) {
                    m_Menu->SetKeyboardFocus(true);
                }
                m_Menu->SelectItem(item);
            }
        }
        else if (!m_Menu->m_HasOpenChildren)
        {
            m_Menu->Close(false, true, nullptr);
        }
        return;
    }

    if (m_HitPointerID != PInvalidPointerID) {
        PView::OnPointerDown(pointerID, position, event, phase);
        return;
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
        return;
    }

    m_HitPos    = position;
    m_HitPointerID = pointerID;

    Ptr<PMenuItem> item = m_Menu->GetItemAt(position);

    if (item != nullptr)
    {
        if (!m_Menu->HasFlags(PMenuFlags::NoKeyboardFocus)) {
            m_Menu->SetKeyboardFocus(true);
        }
        m_Menu->SelectItem(item);
    }

    SetPointerCapture(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuRenderView::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (GetBounds().DoIntersect(position))
        {
            Ptr<PMenuItem> item = m_Menu->FindMarked();

            if (item != nullptr && item == m_Menu->GetItemAt(position) && item->m_SubMenu == nullptr)
            {
                m_Menu->Close(false, true, item);
            }
        }
        else if (!m_Menu->m_HasOpenChildren)
        {
            m_Menu->Close(false, true, nullptr);
        }
        return;
    }

    if (pointerID != m_HitPointerID) {
        PView::OnPointerUp(pointerID, position, event, phase);
        return;
    }

    m_HitPointerID = PInvalidPointerID;

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
    ReleasePointerCapture(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMenuRenderView::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (GetBounds().DoIntersect(position))
        {
            Ptr<PMenuItem> item = m_Menu->GetItemAt(position);

            if (item != nullptr)
            {
                m_Menu->SelectItem(item);
//                if (m_Menu->m_eLayout == MenuLayout::Horizontal) {
//                    m_Menu->OpenSelection();
//                } else {
//                    m_Menu->StartOpenTimer(0.2);
//                }
            }
            return;
        }
        return;
    }

    if (pointerID != m_HitPointerID) {
        PView::OnPointerMove(pointerID, position, event, phase);
        return;
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
}

