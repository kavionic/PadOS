// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include "GUI/RadioButton.h"
#include "Utils/XMLFactory.h"

using namespace os;

static constexpr float RB_SIZE = 32.0f;
static constexpr float RB_LABEL_H_SPACING = 8.0f;
static constexpr float RB_LABEL_V_SPACING = 4.0f;
static constexpr float RB_CHECK_BORDER = 7.0f;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

RadioButton::RadioButton(const String& name, Ptr<View> parent, uint32_t flags) : ButtonBase(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

RadioButton::RadioButton(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData) : ButtonBase(context, parent, xmlData, Alignment::Right)
{
    MergeFlags(ViewFlags::WillDraw);
	SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

RadioButton::~RadioButton()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RadioButton::OnEnableStatusChanged(bool isEnabled)
{
    Invalidate();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RadioButton::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    Point size(RB_SIZE, RB_SIZE);

    const String& label = GetLabel();
    if (!label.empty())
    {
		FontHeight fontHeight = GetFontHeight();

        Alignment labelAlignment = GetLabelAlignment();
        if (labelAlignment == Alignment::Top || labelAlignment == Alignment::Bottom)
        {
            float labelWidth = GetStringWidth(label);
            if (labelWidth > size.x) {
                size.x = labelWidth;
            }
            size.y += RB_LABEL_V_SPACING + fontHeight.ascender + fontHeight.descender;
        }
        else
        {
            if (fontHeight.ascender + fontHeight.descender > size.y) {
                size.y = fontHeight.ascender + fontHeight.descender;
            }
            size.x += RB_LABEL_H_SPACING + GetStringWidth(label);
        }
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RadioButton::Paint(const Rect& updateRect)
{
    Rect bounds = GetBounds();
    Rect buttonFrame = bounds;
    SetFgColor(StandardColorID::DefaultBackground);
    FillRect(updateRect);

    const String& label = GetLabel();
    if (!label.empty())
    {
        FontHeight fontHeight = GetFontHeight();
		float labelWidth = GetStringWidth(label);
		Alignment labelAlignment = GetLabelAlignment();
        if (labelAlignment == Alignment::Top || labelAlignment == Alignment::Bottom)
        {
            if (labelAlignment == Alignment::Top) {
                MovePenTo(floor((bounds.Width() - labelWidth) * 0.5f), fontHeight.ascender);
                buttonFrame.top += fontHeight.ascender + fontHeight.descender + RB_LABEL_V_SPACING;
            } else {
				MovePenTo(floor((bounds.Width() - labelWidth) * 0.5f), RB_SIZE + RB_LABEL_V_SPACING + fontHeight.ascender);
				buttonFrame.bottom -= fontHeight.ascender + fontHeight.descender + RB_LABEL_V_SPACING;
            }
        }
        else
        {
            if (labelAlignment == Alignment::Left) {
				MovePenTo(0.0f, floor(bounds.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender));
                buttonFrame.left += labelWidth + RB_LABEL_H_SPACING;
            } else {
                MovePenTo(RB_SIZE + RB_LABEL_H_SPACING, floor(bounds.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender));
				buttonFrame.right -= labelWidth + RB_LABEL_H_SPACING;
            }
        }
		SetFgColor(0, 0, 0);
		SetBgColor(StandardColorID::DefaultBackground);
		DrawString(label);
    }

    SetEraseColor(StandardColorID::DefaultBackground);

    SetFgColor(StandardColorID::Shadow);


    float radius = floor(RB_SIZE * 0.5f);
    Point center(floor(buttonFrame.left + buttonFrame.Width() * 0.5f), floor(buttonFrame.top + buttonFrame.Height() * 0.5f));

    radius -= 1.0f;
    FillCircle(center, radius);

    SetFgColor(StandardColorID::DefaultBackground);
	radius -= 3.0f;
	FillCircle(center, radius);

    if (IsChecked())
    {
        SetFgColor(StandardColorID::Shadow);
		radius -= 3.0f;
		FillCircle(center, radius);
    }
}
