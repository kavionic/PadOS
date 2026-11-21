// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRendererCheckBox.h>
#include <GUI/Widgets/CheckBox.h>
#include <Utils/XMLFactory.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::PCheckBox(const PString& name, PViewFlags flags) : PViewStyle(name, flags | PViewFlag::WillDraw | PViewFlag::FullUpdateOnResize)
{
    SetCheckable(true);
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::PCheckBox(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PViewStyle(context, xmlData, PAlignment::Right)
{
    MergeFlags(PViewFlag::WillDraw);
	SetCheckable(true);
    SetChecked(context.GetAttribute<bool>(xmlData, "value", false));
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBox::~PCheckBox()
{
    ReleaseRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PCheckBox::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PViewRendererCheckBox* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PCheckBox::OnPaint(const PRect& updateRect)
{
    const PViewRendererCheckBox* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
    } else {
        PView::OnPaint(updateRect);
    }
}
