// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////


#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ViewRendererProgressBar.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ProgressBar.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBar::PProgressBar(const PString& name, POrientation orientation, PViewFlags flags)
    : PView(name, flags | PViewFlag::WillDraw | PViewFlag::ClearBackground), PViewStyleProgressBar(this)
{
    m_ProgressBarStyleData.Orientation = orientation;
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBar::PProgressBar(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PView(context, xmlData), PViewStyleProgressBar(this)
{
    MergeFlags(PViewFlag::WillDraw | PViewFlag::ClearBackground);
    m_ProgressBarStyleData.Orientation = context.GetAttribute(xmlData, "orientation", POrientation::Horizontal);
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBar::~PProgressBar()
{
    ReleaseRenderer();
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

void PProgressBar::OnPaint(const PRect& updateRect)
{
    const PViewRendererProgressBar* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetNormalizedBounds(), updateRect, GetRendererContext());
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::OnFrameSized(const PPoint& delta)
{
    PView::OnFrameSized(delta);
    NotifyStylePropertyChanged(PProgressBarProperty::Frame);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PProgressBar::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PViewRendererProgressBar* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
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
    const PViewRendererProgressBar* renderer = GetRenderer();
    if (renderer != nullptr)
    {
        renderer->PropertyChanged(*this, properties, GetRendererContext());
    }
    else
    {
        if (properties.Has(PProgressBarProperty::Font)) {
            PreferredSizeChanged();
        }
        Invalidate();
    }
}
