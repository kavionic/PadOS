// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Widgets/RadioButton.h>
#include <Utils/XMLFactory.h>


static constexpr float RB_SIZE = 32.0f;
static constexpr float RB_LABEL_H_SPACING = 8.0f;
static constexpr float RB_LABEL_V_SPACING = 4.0f;
static constexpr float RB_CHECK_BORDER = 7.0f;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButton::PRadioButton(const PString& name, Ptr<PView> parent, uint32_t flags) : PButtonBase(name, parent, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButton::PRadioButton(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PButtonBase(context, parent, xmlData, PAlignment::Right)
{
    MergeFlags(PViewFlags::WillDraw);
	SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButton::~PRadioButton()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRadioButton::OnEnableStatusChanged(bool isEnabled)
{
    Invalidate();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRadioButton::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PPoint size(RB_SIZE, RB_SIZE);

    const PString& label = GetLabel();
    if (!label.empty())
    {
		PFontHeight fontHeight = GetFontHeight();

        PAlignment labelAlignment = GetLabelAlignment();
        if (labelAlignment == PAlignment::Top || labelAlignment == PAlignment::Bottom)
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

void PRadioButton::OnPaint(const PRect& updateRect)
{
    const PStandardColorID buttonBackgroundColor = (IsEnabled() && IsPointerOver()) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    PRect bounds = GetBounds();
    PRect buttonFrame = bounds;
    SetFgColor(PStandardColorID::DefaultBackground);
    FillRect(updateRect);

    const PString& label = GetLabel();
    if (!label.empty())
    {
        PFontHeight fontHeight = GetFontHeight();
		float labelWidth = GetStringWidth(label);
		PAlignment labelAlignment = GetLabelAlignment();
        if (labelAlignment == PAlignment::Top || labelAlignment == PAlignment::Bottom)
        {
            if (labelAlignment == PAlignment::Top) {
                MovePenTo(std::floor((bounds.Width() - labelWidth) * 0.5f), fontHeight.ascender);
                buttonFrame.top += fontHeight.ascender + fontHeight.descender + RB_LABEL_V_SPACING;
            } else {
				MovePenTo(std::floor((bounds.Width() - labelWidth) * 0.5f), RB_SIZE + RB_LABEL_V_SPACING + fontHeight.ascender);
				buttonFrame.bottom -= fontHeight.ascender + fontHeight.descender + RB_LABEL_V_SPACING;
            }
        }
        else
        {
            if (labelAlignment == PAlignment::Left) {
				MovePenTo(0.0f, std::floor(bounds.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender));
                buttonFrame.left += labelWidth + RB_LABEL_H_SPACING;
            } else {
                MovePenTo(RB_SIZE + RB_LABEL_H_SPACING, std::floor(bounds.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender));
				buttonFrame.right -= labelWidth + RB_LABEL_H_SPACING;
            }
        }
		SetFgColor(0, 0, 0);
		SetBgColor(PStandardColorID::DefaultBackground);
		DrawString(label);
    }

    SetEraseColor(PStandardColorID::DefaultBackground);

    SetFgColor(PStandardColorID::Shadow);


    float radius = std::floor(RB_SIZE * 0.5f);
    PPoint center(std::floor(buttonFrame.left + buttonFrame.Width() * 0.5f), std::floor(buttonFrame.top + buttonFrame.Height() * 0.5f));

    radius -= 1.0f;
    FillCircle(center, radius);

    SetFgColor(buttonBackgroundColor);
	radius -= 3.0f;
	FillCircle(center, radius);

    if (IsChecked())
    {
        SetFgColor(PStandardColorID::Shadow);
		radius -= 3.0f;
		FillCircle(center, radius);
    }
}
