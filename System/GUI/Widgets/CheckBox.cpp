// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/CheckBox.h>
#include <Utils/XMLFactory.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::PCheckBox(const PString& name, uint32_t flags) : PButtonBase(name, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    SetCheckable(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::PCheckBox(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PButtonBase(context, xmlData, PAlignment::Right)
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

void PCheckBox::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->CheckBoxGetPreferredSize(*this, m_ButtonStyleData, minSize, maxSize);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PCheckBox::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->CheckBoxPaint(*this, GetBounds(), IsEnabled(), m_ButtonStyleData);
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PCheckBox::OnEnableStatusChanged(bool isEnabled)
{
    Invalidate();
    Flush();
}
