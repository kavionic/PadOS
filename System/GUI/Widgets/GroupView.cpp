// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2020 16:30:00

#include <math.h>

#include <GUI/Widgets/GroupView.h>
#include <Utils/XMLFactory.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupView::PGroupView(const PString& name, Ptr<PView> parent, uint32_t flags) : PView(name, parent, flags | PViewFlags::WillDraw)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupView::PGroupView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PView(context, parent, xmlData)
{
    MergeFlags(PViewFlags::WillDraw);
	m_Label = context.GetAttribute(xmlData, "label", PString::zero);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::OnPaint(const PRect& updateRect)
{
    EraseRect(updateRect);

	float labelHeight = 0.0f;
	float labelWidth = 0.0f;

    if (!m_Label.empty())
    {
        PFontHeight fontHeight = GetFontHeight();

        SetFgColor(PStandardColorID::Shadow);
		MovePenTo(30.0f, fontHeight.ascender);
		DrawString(m_Label);

        labelHeight = fontHeight.descender - fontHeight.ascender;
        labelWidth = GetStringWidth(m_Label);
    }

    PRect bound = GetNormalizedBounds();
    bound.Resize(0.0f, std::floor(labelHeight * 0.5f), -1.0f, -1.0f);

    for (int i = 0; i < 2; ++i)
    {
        SetFgColor(PStandardColorID::Shadow);
        MovePenTo(bound.BottomLeft());
        DrawLine(bound.TopLeft());

        if (!m_Label.empty())
        {
            DrawLine(15.0f, bound.top);

            MovePenTo(30.0f + labelWidth + 10.0f, bound.top);
        }
        DrawLine(bound.TopRight());

        SetFgColor(PStandardColorID::Shine);
        MovePenTo(bound.TopRight() + PPoint(0.0f, 1.0f));
        DrawLine(bound.BottomRight());
        DrawLine(bound.BottomLeft() + PPoint(1.0f, 0.0f));

		bound.Resize(1.0f, 1.0f, -1.0f, -1.0f);
    }
}
