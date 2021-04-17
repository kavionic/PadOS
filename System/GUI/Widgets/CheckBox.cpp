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

#include <GUI/Widgets/Checkbox.h>
#include <Utils/XMLFactory.h>

using namespace os;

static constexpr float CB_SIZE = 32.0f;
static constexpr float CB_LABEL_SPACING = 8.0f;
static constexpr float CB_CHECK_BORDER = 7.0f;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(const String& name, Ptr<View> parent, uint32_t flags) : ButtonBase(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

CheckBox::CheckBox(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData) : ButtonBase(context, parent, xmlData, Alignment::Right)
{
    MergeFlags(ViewFlags::WillDraw);
	SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

CheckBox::~CheckBox()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CheckBox::OnEnableStatusChanged(bool isEnabled)
{
    Invalidate();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void os::CheckBox::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight)
{
    Point size(CB_SIZE, CB_SIZE);

    const String& label = GetLabel();
    if (!label.empty())
    {
        FontHeight fontHeight = GetFontHeight();
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

void CheckBox::Paint(const Rect& cUpdateRect)
{
    Rect bounds = GetBounds();

    SetFgColor(StandardColorID::DefaultBackground);
    FillRect(cUpdateRect);

    const String& label = GetLabel();
    if (!label.empty())
    {
        FontHeight fontHeight = GetFontHeight();
        MovePenTo(CB_SIZE + CB_LABEL_SPACING, bounds.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender);

        SetFgColor(0, 0, 0);
        SetBgColor(StandardColorID::DefaultBackground);
        DrawString(label);
    }

    SetEraseColor(StandardColorID::DefaultBackground);

    Rect buttonFrame(0, 0, CB_SIZE, CB_SIZE);

    float delta = (bounds.Height() - buttonFrame.Height()) / 2;
    buttonFrame.top += delta;
    buttonFrame.bottom += delta;

    DrawFrame(buttonFrame, FRAME_RECESSED | FRAME_TRANSPARENT);

    if (IsChecked())
    {
        Point	point1;
        Point	point2;

        SetFgColor(0, 0, 0);

        point1 = Point(buttonFrame.left + CB_CHECK_BORDER, buttonFrame.top + CB_CHECK_BORDER);
        point2 = Point(buttonFrame.right - CB_CHECK_BORDER, buttonFrame.bottom - CB_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            DrawLine(point1 + Point(float(i - 3), 0.0f), point2 + Point(float(i - 3), 0.0f));
        }
        point1 = Point(buttonFrame.left + CB_CHECK_BORDER, buttonFrame.bottom - CB_CHECK_BORDER);
        point2 = Point(buttonFrame.right - CB_CHECK_BORDER, buttonFrame.top + CB_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            DrawLine(point1 + Point(float(i - 3), 0.0f), point2 + Point(float(i - 3), 0.0f));
        }
    }
}
