// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 05.09.2020 22:15


#include <PadOS/Time.h>
#include <ApplicationServer/Protocol.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/KeyboardView.h>
#include <Utils/UTF8Utils.h>

#include "VirtualKeyboardView.h"


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PVirtualKeyboardView::PVirtualKeyboardView(bool numerical) : PView("VirtualKeyboard", nullptr, PViewFlags::WillDraw)
{
    SetLayoutNode(ptr_new<PLayoutNode>());

    m_KeyboardView = ptr_new<PKeyboardView>("Keyboard", ptr_tmp_cast(this), (numerical) ? PKeyboardViewFlags::Numerical : 0);
    m_KeyboardView->PreferredSizeChanged();

    m_KeyboardView->SignalKeyPressed.Connect(this, &PVirtualKeyboardView::SlotKeyPressed);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PVirtualKeyboardView::SetIsNumerical(bool numerical)
{
    if (numerical) {
        m_KeyboardView->MergeFlags(PKeyboardViewFlags::Numerical);
    } else {
        m_KeyboardView->ClearFlags(PKeyboardViewFlags::Numerical);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PVirtualKeyboardView::SlotKeyPressed(PKeyCodes keyCode, const PString& text)
{
    PKeyEvent event{};

    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::KeyEvent;
    event.ClassID = PInputClass::Keyboard;
    event.Timestamp = get_monotonic_time();
    event.EventID = PInputEventID::KeyDown;
    event.SourceID = -1;
    event.KeyCode = keyCode;
    event.KeySymbol = GetKeySymbol(keyCode, text);

    p_post_to_remotesignal<ASVirtualKeyboardEvent>(
        p_get_appserver_port(),
        INVALID_HANDLE,
        TimeValNanos::infinit,
        event, text);

    event.EventID = PInputEventID::KeyUp;
    p_post_to_remotesignal<ASVirtualKeyboardEvent>(
        p_get_appserver_port(),
        INVALID_HANDLE,
        TimeValNanos::infinit,
        event, PString::zero);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PKeySymbol PVirtualKeyboardView::GetKeySymbol(PKeyCodes keyCode, const PString& text)
{
    switch (keyCode)
    {
        case PKeyCodes::BACKSPACE: return PKeySymbol::BACKSPACE;
        case PKeyCodes::TAB: return PKeySymbol::TAB;
        case PKeyCodes::ENTER: return PKeySymbol::ENTER;
        case PKeyCodes::ESCAPE: return PKeySymbol::ESCAPE;
        case PKeyCodes::DELETE: return PKeySymbol::DELETE;
        case PKeyCodes::INSERT: return PKeySymbol::INSERT;
        case PKeyCodes::HOME: return PKeySymbol::HOME;
        case PKeyCodes::END: return PKeySymbol::END;
        case PKeyCodes::PAGE_UP: return PKeySymbol::PAGE_UP;
        case PKeyCodes::PAGE_DOWN: return PKeySymbol::PAGE_DOWN;
        case PKeyCodes::LEFT_ARROW: return PKeySymbol::LEFT_ARROW;
        case PKeyCodes::RIGHT_ARROW: return PKeySymbol::RIGHT_ARROW;
        case PKeyCodes::UP_ARROW: return PKeySymbol::UP_ARROW;
        case PKeyCodes::DOWN_ARROW: return PKeySymbol::DOWN_ARROW;
        default: return text.empty() ? PKeySymbol::NONE : PKeySymbol(utf8_to_unicode(text.c_str()));
    }
}
