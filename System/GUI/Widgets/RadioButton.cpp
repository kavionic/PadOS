// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/RadioButton.h>
#include <Utils/XMLFactory.h>
#include <GUI/Style/ViewRendererRadioButton.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButton::PRadioButton(const PString& name, PViewFlags flags) : PViewStyle(name, flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize)
{
    SetCheckable(true);
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButton::PRadioButton(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PViewStyle(context, xmlData, PAlignment::Right)
{
    MergeFlags(PViewFlag::WillDraw);
    SetCheckable(true);
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButton::~PRadioButton()
{
    ReleaseRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRadioButton::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PViewRendererRadioButton* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PRadioButton::OnPaint(const PRect& updateRect)
{
    const PViewRendererRadioButton* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
    } else {
        PView::OnPaint(updateRect);
    }
}
