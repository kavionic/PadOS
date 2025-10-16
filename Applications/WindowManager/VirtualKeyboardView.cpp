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

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

VirtualKeyboardView::VirtualKeyboardView(bool numerical) : View("VirtualKeyboard", nullptr, ViewFlags::WillDraw)
{
    SetLayoutNode(ptr_new<LayoutNode>());

    m_KeyboardView = ptr_new<KeyboardView>("Keyboard", ptr_tmp_cast(this), (numerical) ? KeyboardViewFlags::Numerical : 0);
    m_KeyboardView->PreferredSizeChanged();

    m_KeyboardView->SignalKeyPressed.Connect(this, &VirtualKeyboardView::SlotKeyPressed);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void VirtualKeyboardView::SetIsNumerical(bool numerical)
{
    if (numerical) {
        m_KeyboardView->MergeFlags(KeyboardViewFlags::Numerical);
    } else {
        m_KeyboardView->ClearFlags(KeyboardViewFlags::Numerical);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void VirtualKeyboardView::SlotKeyPressed(KeyCodes keyCode, const String& text)
{
    KeyEvent event;

    event.Timestamp = get_monotonic_time();
    event.EventID = MessageID::KEY_UP;
    event.m_KeyCode = keyCode;
    strncpy(event.m_Text, text.c_str(), KeyEvent::MAX_TEXT_LENGTH);

    send_message(get_input_event_port(), INVALID_HANDLE, int32_t(event.EventID), &event, sizeof(event));
}


} // namespace os
