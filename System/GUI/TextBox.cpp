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
    DEFINE_FLAG_MAP_ENTRY(TextBoxFlags, RaisedFrame),
    DEFINE_FLAG_MAP_ENTRY(TextBoxFlags, ReadOnly)
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextBox::TextBox(const String& name, const String& text, Ptr<View> parent, uint32_t flags) : Control(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    Initialize(text);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextBox::TextBox(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData) : Control(context, parent, xmlData)
{
    MergeFlags(context->GetFlagsAttribute<uint32_t>(xmlData, TextBoxFlags::FlagMap, "flags", 0) | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize);
    Initialize(context->GetAttribute(xmlData, "text", String::zero));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextBox::Initialize(const String& text)
{
    m_Editor = ptr_new<TextEditView>("Editor", text, ptr_tmp_cast(this), GetFlags());
    m_Editor->SetBorders(4.0f, 4.0f, 4.0f, 4.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextBox::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size =m_Editor->GetPreferredSize(PrefSizeType::Smallest);
    Rect borders = m_Editor->GetBorders();
    size.x += borders.left + borders.right;
    size.y += borders.top + borders.bottom;

    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextBox::FrameSized(const Point& delta)
{
    Rect frame = GetBounds();
    Rect borders = m_Editor->GetBorders();
    frame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
    m_Editor->SetFrame(frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextBox::Paint(const Rect& updateRect)
{
    SetEraseColor(GetBgColor());
    Rect bounds = GetBounds();

    DrawFrame(bounds, HasFlags(TextBoxFlags::RaisedFrame) ? FRAME_RAISED : FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point TextBox::GetSizeForString(const String& text, bool includeWidth, bool includeHeight) const
{
    Point size    = m_Editor->GetSizeForString(text, includeWidth, includeHeight);
    Rect  borders = m_Editor->GetBorders();

    size.x += borders.left + borders.right;
    size.y += borders.top + borders.bottom;
    return size;
}

