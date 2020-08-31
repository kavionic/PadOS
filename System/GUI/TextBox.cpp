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
// Created: 22.08.2020 15:30

#include <System/Platform.h>

#include <GUI/TextBox.h>
#include <Utils/Utils.h>
#include <Utils/XMLObjectParser.h>

using namespace os;

const std::map<String, uint32_t> TextBoxFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(TextBoxFlags, IncludeLineGap),
    DEFINE_FLAG_MAP_ENTRY(TextBoxFlags, RaisedFrame)
};


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextBox::TextBox(const String& name, const String& text, Ptr<View> parent, uint32_t flags) : Control(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize), m_Text(text)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextBox::TextBox(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData) : Control(context, parent, xmlData)
{
    MergeFlags(context->GetFlagsAttribute<uint32_t>(xmlData, TextBoxFlags::FlagMap, "flags", 0) | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize);
	m_Text = context->GetAttribute(xmlData, "text", String::zero);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextBox::~TextBox()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextBox::SetText(const String& text)
{
    DEBUG_TRACK_FUNCTION();
//    ProfileTimer timer("TextBox::SetText()");    
    m_Text = text;
    PreferredSizeChanged();
    Invalidate();

    SignalTextChanged(m_Text, true, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextBox::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size = GetSizeForString(m_Text, includeWidth, includeHeight);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextBox::Paint(const Rect& updateRect)
{
    SetEraseColor(GetBgColor());
    Rect bounds = GetBounds();
    EraseRect(bounds);

    MovePenTo(2.0f, 2.0f);
    SetFgColor(0, 0, 0);
    DrawString(m_Text);

    DrawFrame(bounds, (HasFlags(TextBoxFlags::RaisedFrame) ? FRAME_RAISED : FRAME_RECESSED) | FRAME_TRANSPARENT);

//    DrawRect(bounds);    
//    DebugDraw(Color(255, 0, 0), ViewDebugDrawFlags::ViewFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// Calculate the minimum size the text-box must have to fit the given text.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point TextBox::GetSizeForString(const String& text, bool includeWidth, bool includeHeight) const
{
    Point size;
    if (includeWidth) {
        size.x = GetStringWidth(text);
        size.x += 4.0f; // Make place for borders.
    }
    if (includeHeight)
    {
        FontHeight fontHeight = GetFontHeight();
        size.y = fontHeight.descender - fontHeight.ascender;
        if (HasFlags(TextBoxFlags::IncludeLineGap)) {
            size.y += fontHeight.line_gap;
        }
        size.y += 4.0f; // Make place for borders.
    }
    return size;
}
