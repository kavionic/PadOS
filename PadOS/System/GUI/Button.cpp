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

#include "System/Platform.h"

#include "GUI/Button.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Button::Button(const String& name, const String& label, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::FULL_UPDATE_ON_RESIZE), m_Label(label)
{
    m_LabelSize.x = GetStringWidth(m_Label);
    FontHeight fontHeight = GetFontHeight();
    m_LabelSize.y = fontHeight.descender - fontHeight.ascender + fontHeight.line_gap;
    
    PreferredSizeChanged();
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

void Button::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size(m_LabelSize.x + 16.0f, m_LabelSize.y + 8.0f);
    *minSize = size;
    *maxSize = size;
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
    Rect bounds = GetBounds();
    SetEraseColor(get_standard_color(StandardColorID::NORMAL));
    DrawFrame(bounds, m_IsPressed ? FRAME_RECESSED : FRAME_RAISED);
    Point labelPos(round((bounds.Width() - m_LabelSize.x) * 0.5f), round((bounds.Height() - m_LabelSize.y) * 0.5f));
    if (m_IsPressed) labelPos += Point(1.0f, 1.0f);
    MovePenTo(labelPos);
    SetFgColor(get_standard_color(StandardColorID::MENU_TEXT));
    SetBgColor(get_standard_color(StandardColorID::NORMAL));
    DrawString(m_Label);
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
    }
}
