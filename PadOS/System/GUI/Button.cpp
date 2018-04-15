// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 02.04.2018 13:08:46

#include "sam.h"

#include "Button.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Button::Button(const String& name, const String& label, Ptr<View> parent, uint32_t flags) : View(name, parent, flags), m_Label(label)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Button::~Button()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point Button::GetPreferredSize(bool largest) const
{
    FontHeight fontHeight = GetFontHeight();
    float stringWidth = GetStringWidth(m_Label);
    return Point(stringWidth + 16.0f, fontHeight.descender - fontHeight.ascender + fontHeight.line_gap + 8.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Button::OnMouseDown(MouseButton_e button, const Point& position)
{
//    printf("Button: Mouse down %d, %.1f/%.1f\n", int(button), position.x, position.y);
    m_WasHit = true;
    SetPressedState(true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Button::OnMouseUp(MouseButton_e button, const Point& position)
{
//    printf("Button: Mouse up %d, %.1f/%.1f\n", int(button), position.x, position.y);
    if (m_WasHit)
    {
        m_WasHit = false;
        if (m_IsPressed)
        {
            SetPressedState(false);
            SignalActivated(button, this);
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Button::OnMouseMove(MouseButton_e button, const Point& position)
{
    if (m_WasHit)
    {
//        printf("Button: Mouse move %d, %.1f/%.1f\n", int(button), position.x, position.y);
        SetPressedState(GetBounds().DoIntersect(position));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Button::Paint(const Rect& updateRect)
{
    SetEraseColor(get_standard_color(StandardColorID::NORMAL));
//    DrawBevelBox(GetBounds(), !m_IsPressed);
    DrawFrame(GetBounds(), m_IsPressed ? FRAME_RECESSED : FRAME_RAISED);
    Point labelPos(8.0f, 4.0f);
    if (m_IsPressed) labelPos += Point(1.0f, 1.0f);
    MovePenTo(labelPos);
    SetFgColor(get_standard_color(StandardColorID::MENU_TEXT));
//    SetBgColor(m_IsPressed ? Color(0xf0,0xf0,0xf0) : Color(0xd0,0xd0,0xd0));
    SetBgColor(get_standard_color(StandardColorID::NORMAL));
    DrawString(m_Label, 1024.0f, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Button::SetPressedState(bool isPressed)
{
    if (isPressed != m_IsPressed)
    {
        m_IsPressed = isPressed;
        Invalidate();
        Flush();
//        UpdateIfNeeded(false);
    }
}

