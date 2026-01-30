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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::PButton(const PString& name, const PString& label, Ptr<PView> parent, uint32_t flags) : PButtonBase(name, parent, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    SetLabel(label);
	UpdateLabelSize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::PButton(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PButtonBase(context, parent, xmlData, PAlignment::Center)
{
    MergeFlags(PViewFlags::WillDraw);
	UpdateLabelSize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::~PButton()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButton::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PPoint size(m_LabelSize.x + 16.0f, m_LabelSize.y + 8.0f);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButton::OnPaint(const PRect& updateRect)
{
    const bool isPressed = GetPressedState();
    const bool isEnabled = IsEnabled();
    PRect bounds = GetBounds();
    SetEraseColor(PStandardColorID::ButtonBackground);

    if (isEnabled) {
        DrawFrame(bounds, isPressed ? FRAME_RECESSED : FRAME_RAISED);
    } else {
        DrawFrame(bounds, FRAME_DISABLED);
    }
    PPoint labelPos(std::round((bounds.Width() - m_LabelSize.x) * 0.5f), std::round((bounds.Height() - m_LabelSize.y) * 0.5f));
    
    if (isPressed) {
        labelPos += PPoint(1.0f, 1.0f);
    }

    MovePenTo(labelPos);
    SetFgColor(IsEnabled() ? PStandardColorID::ButtonLabelNormal : PStandardColorID::ButtonLabelDisabled);
    SetBgColor(PStandardColorID::ButtonBackground);
    DrawString(GetLabel());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButton::OnLabelChanged(const PString& label)
{
    UpdateLabelSize();
    Invalidate();
    Flush();
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButton::UpdateLabelSize()
{
	m_LabelSize.x = GetStringWidth(GetLabel());
	PFontHeight fontHeight = GetFontHeight();
	m_LabelSize.y = fontHeight.descender - fontHeight.ascender + fontHeight.line_gap;
}
