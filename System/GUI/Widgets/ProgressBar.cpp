// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////


#include <GUI/Style/GUIStyle.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ProgressBar.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBar::PProgressBar(const PString& name, POrientation orientation, PViewFlags flags)
    : PStyledView(name, flags | PViewFlag::WillDraw | PViewFlag::ClearBackground)
{
    m_ProgressBarStyleData.Orientation = orientation;
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBar::PProgressBar(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PStyledView(context, xmlData)
{
    MergeFlags(PViewFlag::WillDraw | PViewFlag::ClearBackground);
    m_ProgressBarStyleData.Orientation = context.GetAttribute(xmlData, "orientation", POrientation::Horizontal);
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::OnFontChanged(Ptr<const PFont> newFont)
{
    NotifyStylePropertyChanged(PProgressBarProperty::Font);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::OnStyleChanged()
{
    m_ProgressBarStyleData.StyleData.Reset();
    PStyledView::OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->ProgressBarPaint(*this, GetNormalizedBounds(), m_ProgressBarStyleData);
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::OnFrameSized(const PPoint& delta)
{
    PStyledView::OnFrameSized(delta);
    NotifyStylePropertyChanged(PProgressBarProperty::Frame);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->ProgressBarGetPreferredSize(*this, m_ProgressBarStyleData, minSize, maxSize);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::SetProgress(float value)
{
    m_ProgressBarStyleData.Value = value;
    NotifyStylePropertyChanged(PProgressBarProperty::Value);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PProgressBar::GetProgress() const
{
    return m_ProgressBarStyleData.Value;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::NotifyStylePropertyChanged(PProgressBarProperties properties)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        style->ProgressBarPropertyChanged(*this, m_ProgressBarStyleData, properties);
    }
    else
    {
        if (properties.Has(PProgressBarProperty::Font)) {
            PreferredSizeChanged();
        }
        Invalidate();
    }
}
