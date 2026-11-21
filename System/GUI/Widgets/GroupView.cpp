// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.06.2020 16:30:00

#include <algorithm>

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/GroupView.h>
#include <Utils/XMLFactory.h>
#include <GUI/Style/ViewRendererGroupView.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupView::PGroupView(const PString& name, PViewFlags flags) : PView(name, flags | PViewFlag::WillDraw), PViewStyleGroupView(this)
{
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupView::PGroupView(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PView(context, xmlData), PViewStyleGroupView(this)
{
    MergeFlags(PViewFlag::WillDraw);
    m_GroupViewStyleData.Label = context.GetAttribute(xmlData, "label", PString::zero);
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupView::~PGroupView()
{
    ReleaseRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::OnFontChanged(Ptr<const PFont> newFont)
{
    NotifyStylePropertyChanged(PGroupViewProperty::Font);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::OnFrameSized(const PPoint& delta)
{
    PView::OnFrameSized(delta);
    NotifyStylePropertyChanged(PGroupViewProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::OnPaint(const PRect& updateRect)
{
    const PViewRendererGroupView* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    const PViewRendererGroupView* renderer = GetRenderer();
    if (renderer != nullptr)
    {
        PPoint frameMinSize;
        PPoint frameMaxSize;
        renderer->GetPreferredSize(*this, &frameMinSize, &frameMaxSize, GetRendererContext());
        if (includeWidth)
        {
            minSize->x = std::max(minSize->x, frameMinSize.x);
            maxSize->x = std::max(minSize->x, std::min(maxSize->x, frameMaxSize.x));
        }
        if (includeHeight)
        {
            minSize->y = std::max(minSize->y, frameMinSize.y);
            maxSize->y = std::max(minSize->y, std::min(maxSize->y, frameMaxSize.y));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::SetLabel(const PString& label)
{
    if (label != m_GroupViewStyleData.Label)
    {
        m_GroupViewStyleData.Label = label;
        NotifyStylePropertyChanged(PGroupViewProperty::Label);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& PGroupView::GetLabel() const
{
    return m_GroupViewStyleData.Label;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::NotifyStylePropertyChanged(PGroupViewProperties properties)
{
    const PViewRendererGroupView* renderer = GetRenderer();
    if (renderer != nullptr)
    {
        renderer->PropertyChanged(*this, properties, GetRendererContext());
    }
    else
    {
        if (properties.HasAny(PGroupViewProperties(PGroupViewProperty::Label, PGroupViewProperty::Font))) {
            PreferredSizeChanged();
        }
        Invalidate();
    }
}
