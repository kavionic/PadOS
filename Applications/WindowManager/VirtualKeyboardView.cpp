// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 05.09.2020 22:15


#include <PadOS/Time.h>
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

    event.Timestamp = get_monotonic_time();
    event.EventID = PMessageID::KEY_UP;
    event.m_KeyCode = keyCode;
    strncpy(event.m_Text, text.c_str(), PKeyEvent::MAX_TEXT_LENGTH);

    message_port_send(get_input_event_port(), INVALID_HANDLE, int32_t(event.EventID), &event, sizeof(event));
}
