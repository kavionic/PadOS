// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 02.04.2018 13:08:46

#include <System/Platform.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/Button.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::PButton(const PString& name, const PString& label, uint32_t flags)
    : PButtonBase(name, flags | PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize)
{
    SetLabel(label);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::PButton(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PButtonBase(context, xmlData, PAlignment::Center)
{
    MergeFlags(PViewFlags::WillDraw);
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
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->ButtonGetPreferredSize(*this, m_ButtonStyleData, minSize, maxSize);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButton::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->ButtonPaint(*this, GetBounds(), IsEnabled(), m_ButtonStyleData);
    } else {
        PView::OnPaint(updateRect);
    }
}
