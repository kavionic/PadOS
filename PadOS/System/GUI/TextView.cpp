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
// Created: 15.04.2018 16:38:59

#include "sam.h"

#include "TextView.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextView::TextView(const String& name, const String& text, Ptr<View> parent, uint32_t flags) : View(name, parent, flags), m_Text(text)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextView::~TextView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextView::SetText(const String& text)
{
    m_Text = text;
    PreferredSizeChanged();
    Invalidate();
    Sync();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point TextView::GetPreferredSize(bool largest) const
{
    FontHeight fontHeight = GetFontHeight();
    float      stringWidth = GetStringWidth(m_Text);
    return Point(stringWidth, fontHeight.descender - fontHeight.ascender + fontHeight.line_gap);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextView::Paint(const Rect& updateRect)
{
    Rect bounds = GetBounds();
    EraseRect(bounds);

    MovePenTo(0.0f, 0.0f);
    DrawString(m_Text);

//    DrawRect(bounds);    
//    DebugDraw(Color(255, 0, 0), ViewDebugDrawFlags::ViewFrame);
}
