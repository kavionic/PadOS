// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include <System/Platform.h>
#include <GUI/Widgets/Button.h>

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Button::Button(const PString& name, const PString& label, Ptr<View> parent, uint32_t flags) : ButtonBase(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    SetLabel(label);
	UpdateLabelSize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Button::Button(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : ButtonBase(context, parent, xmlData, Alignment::Center)
{
    MergeFlags(ViewFlags::WillDraw);
	UpdateLabelSize();
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

void os::Button::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    Point size(m_LabelSize.x + 16.0f, m_LabelSize.y + 8.0f);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Button::OnPaint(const Rect& updateRect)
{
    const bool isPressed = GetPressedState();
    const bool isEnabled = IsEnabled();
    Rect bounds = GetBounds();
    SetEraseColor(StandardColorID::ButtonBackground);

    if (isEnabled) {
        DrawFrame(bounds, isPressed ? FRAME_RECESSED : FRAME_RAISED);
    } else {
        DrawFrame(bounds, FRAME_DISABLED);
    }
    Point labelPos(std::round((bounds.Width() - m_LabelSize.x) * 0.5f), std::round((bounds.Height() - m_LabelSize.y) * 0.5f));
    
    if (isPressed) {
        labelPos += Point(1.0f, 1.0f);
    }

    MovePenTo(labelPos);
    SetFgColor(IsEnabled() ? StandardColorID::ButtonLabelNormal : StandardColorID::ButtonLabelDisabled);
    SetBgColor(StandardColorID::ButtonBackground);
    DrawString(GetLabel());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Button::OnLabelChanged(const PString& label)
{
    UpdateLabelSize();
    Invalidate();
    Flush();
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Button::UpdateLabelSize()
{
	m_LabelSize.x = GetStringWidth(GetLabel());
	FontHeight fontHeight = GetFontHeight();
	m_LabelSize.y = fontHeight.descender - fontHeight.ascender + fontHeight.line_gap;
}
