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

#include "System/Platform.h"

#include "GUI/TextView.h"
#include "Utils/Utils.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextView::TextView(const String& name, const String& text, Ptr<View> parent, uint32_t flags) : View(name, parent, flags), m_Text(text)
{
    SetBgColor(GetEraseColor());
    PreferredSizeChanged();
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
    DEBUG_TRACK_FUNCTION();
//    ProfileTimer timer("TextView::SetText()");    
    m_Text = text;
    PreferredSizeChanged();
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size;
    if (includeWidth) {
        size.x = GetStringWidth(m_Text);
    }
    if (includeHeight) {
        FontHeight fontHeight = GetFontHeight();
        size.y = fontHeight.descender - fontHeight.ascender + fontHeight.line_gap;
    }
    *minSize = size;
    *maxSize = size;
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
