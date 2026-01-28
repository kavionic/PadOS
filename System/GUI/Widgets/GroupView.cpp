// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.06.2020 16:30:00

#include <math.h>

#include <GUI/Widgets/GroupView.h>
#include <Utils/XMLFactory.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GroupView::GroupView(const PString& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::WillDraw)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GroupView::GroupView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : View(context, parent, xmlData)
{
    MergeFlags(ViewFlags::WillDraw);
	m_Label = context.GetAttribute(xmlData, "label", PString::zero);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GroupView::OnPaint(const Rect& updateRect)
{
    EraseRect(updateRect);

	float labelHeight = 0.0f;
	float labelWidth = 0.0f;

    if (!m_Label.empty())
    {
        FontHeight fontHeight = GetFontHeight();

        SetFgColor(StandardColorID::Shadow);
		MovePenTo(30.0f, fontHeight.ascender);
		DrawString(m_Label);

        labelHeight = fontHeight.descender - fontHeight.ascender;
        labelWidth = GetStringWidth(m_Label);
    }

    Rect bound = GetNormalizedBounds();
    bound.Resize(0.0f, std::floor(labelHeight * 0.5f), -1.0f, -1.0f);

    for (int i = 0; i < 2; ++i)
    {
        SetFgColor(StandardColorID::Shadow);
        MovePenTo(bound.BottomLeft());
        DrawLine(bound.TopLeft());

        if (!m_Label.empty())
        {
            DrawLine(15.0f, bound.top);

            MovePenTo(30.0f + labelWidth + 10.0f, bound.top);
        }
        DrawLine(bound.TopRight());

        SetFgColor(StandardColorID::Shine);
        MovePenTo(bound.TopRight() + Point(0.0f, 1.0f));
        DrawLine(bound.BottomRight());
        DrawLine(bound.BottomLeft() + Point(1.0f, 0.0f));

		bound.Resize(1.0f, 1.0f, -1.0f, -1.0f);
    }
}
