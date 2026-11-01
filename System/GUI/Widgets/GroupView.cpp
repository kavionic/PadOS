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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupView::PGroupView(const PString& name, PViewFlags flags) : PStyledView(name, flags | PViewFlag::WillDraw)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupView::PGroupView(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PStyledView(context, xmlData)
{
    MergeFlags(PViewFlag::WillDraw);
    m_GroupViewStyleData.Label = context.GetAttribute(xmlData, "label", PString::zero);
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

void PGroupView::OnStyleChanged()
{
    m_GroupViewStyleData.StyleData.Reset();
    PStyledView::OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::OnFrameSized(const PPoint& delta)
{
    PStyledView::OnFrameSized(delta);
    NotifyStylePropertyChanged(PGroupViewProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PGroupView::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->GroupViewPaint(*this, updateRect, m_GroupViewStyleData);
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
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        PPoint frameMinSize;
        PPoint frameMaxSize;
        style->GroupViewGetPreferredSize(*this, m_GroupViewStyleData, &frameMinSize, &frameMaxSize);
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
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        style->GroupViewPropertyChanged(*this, m_GroupViewStyleData, properties);
    }
    else
    {
        if (properties.HasAny(PGroupViewProperties(PGroupViewProperty::Label, PGroupViewProperty::Font))) {
            PreferredSizeChanged();
        }
        Invalidate();
    }
}
