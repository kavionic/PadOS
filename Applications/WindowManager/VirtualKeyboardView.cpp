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
    PKeyEvent event;

    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::KeyEvent;
    event.ClassID = PInputClass::Keyboard;
    event.Timestamp = get_monotonic_time();
    event.EventID = PInputEventID::KeyDown;
    event.SourceID = -1;
    event.m_KeyCode = keyCode;
    strncpy(event.m_Text, text.c_str(), PKeyEvent::MAX_TEXT_LENGTH);

    p_post_to_remotesignal<ASVirtualKeyboardEvent>(
        p_get_appserver_port(),
        INVALID_HANDLE,
        TimeValNanos::infinit,
        event);

    event.EventID = PInputEventID::KeyUp;
    p_post_to_remotesignal<ASVirtualKeyboardEvent>(
        p_get_appserver_port(),
        INVALID_HANDLE,
        TimeValNanos::infinit,
        event);
}
