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
#include <GUI/Style/ViewRendererButton.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::PButton(const PString& name, const PString& label, PViewFlags flags)
    : PViewStyle(name, flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize)
{
    SetLabel(label);
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::PButton(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PViewStyle(context, xmlData, PAlignment::Center)
{
    MergeFlags(PViewFlag::WillDraw);
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButton::~PButton()
{
    ReleaseRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButton::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PViewRendererButton* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButton::OnPaint(const PRect& updateRect)
{
    const PViewRendererButton* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
    } else {
        PView::OnPaint(updateRect);
    }
}
