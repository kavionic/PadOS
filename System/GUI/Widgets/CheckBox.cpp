// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Widgets/Checkbox.h>
#include <Utils/XMLFactory.h>


static constexpr float CB_SIZE = 32.0f;
static constexpr float CB_LABEL_SPACING = 8.0f;
static constexpr float CB_CHECK_BORDER = 7.0f;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::PCheckBox(const PString& name, Ptr<PView> parent, uint32_t flags) : PButtonBase(name, parent, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::PCheckBox(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PButtonBase(context, parent, xmlData, PAlignment::Right)
{
    MergeFlags(PViewFlags::WillDraw);
	SetCheckable(true);
    SetChecked(context.GetAttribute<bool>(xmlData, "value", false));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::~PCheckBox()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PCheckBox::OnEnableStatusChanged(bool isEnabled)
{
    Invalidate();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PCheckBox::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PPoint size(CB_SIZE, CB_SIZE);

    const PString& label = GetLabel();
    if (!label.empty())
    {
        PFontHeight fontHeight = GetFontHeight();
        if (fontHeight.ascender + fontHeight.descender > size.y) {
            size.y = fontHeight.ascender + fontHeight.descender;
        }
        size.x += CB_LABEL_SPACING + GetStringWidth(label);
    }
    *minSize = size;
    *maxSize = size;
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PCheckBox::OnPaint(const PRect& cUpdateRect)
{
    const PStandardColorID buttonBackgroundColor = (IsEnabled() && IsPointerOver()) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    PRect bounds = GetBounds();

    SetFgColor(PStandardColorID::DefaultBackground);
    FillRect(cUpdateRect);

    const PString& label = GetLabel();
    if (!label.empty())
    {
        PFontHeight fontHeight = GetFontHeight();
        MovePenTo(CB_SIZE + CB_LABEL_SPACING, bounds.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender);

        SetFgColor(0, 0, 0);
        SetBgColor(PStandardColorID::DefaultBackground);
        DrawString(label);
    }

    SetEraseColor(buttonBackgroundColor);

    PRect buttonFrame(0, 0, CB_SIZE, CB_SIZE);

    float delta = (bounds.Height() - buttonFrame.Height()) / 2;
    buttonFrame.top += delta;
    buttonFrame.bottom += delta;

    DrawFrame(buttonFrame, FRAME_RECESSED);

    if (IsChecked())
    {
        PPoint	point1;
        PPoint	point2;

        SetFgColor(0, 0, 0);

        point1 = PPoint(buttonFrame.left + CB_CHECK_BORDER, buttonFrame.top + CB_CHECK_BORDER);
        point2 = PPoint(buttonFrame.right - CB_CHECK_BORDER, buttonFrame.bottom - CB_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
        point1 = PPoint(buttonFrame.left + CB_CHECK_BORDER, buttonFrame.bottom - CB_CHECK_BORDER);
        point2 = PPoint(buttonFrame.right - CB_CHECK_BORDER, buttonFrame.top + CB_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
    }
}
