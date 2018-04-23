// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 15.04.2018 11:46:11

#include "sam.h"

#include "HistoryGraphView.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HistoryGraphView::HistoryGraphView(const String& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

HistoryGraphView::~HistoryGraphView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HistoryGraphView::AddValue(float value)
{
    m_Values.push_back(value);
    if (m_Values.size() > 1000) m_Values.erase(m_Values.begin());

    Rect bounds = GetBounds();
    int pointsToRender = std::min(int(bounds.Width()), int(m_Values.size()));

    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();
    
    for (int i = m_Values.size() - pointsToRender; i < int(m_Values.size()); ++i)
    {
        float curVal = m_Values[i];
        if (curVal < minValue) minValue = curVal;
        if (curVal > maxValue) maxValue = curVal;
    }
    
    bool scaleChanged = false;
    if (minValue != m_MinValue) {
        m_MinValue = minValue;
        scaleChanged = true;
    }        
    if (maxValue != m_MaxValue){
        m_MaxValue = maxValue;
        scaleChanged = true;
    }
    if (!scaleChanged) {
        Rect scrollRect = GetBounds();
        scrollRect.left = 1.0f;
        CopyRect(scrollRect, Point(0.0f, 0.0f));
    } else {
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HistoryGraphView::Paint(const Rect& updateRect)
{
    Rect bounds = GetBounds();
    IRect ibounds(bounds);
    
    int pointsVisible = std::min(ibounds.Width(), int(m_Values.size()));
    int pointsToRender = std::min(ibounds.right - int(updateRect.left), int(m_Values.size()));

    if (pointsVisible < ibounds.Width())
    {
        EraseRect(Rect(updateRect.left, bounds.top, bounds.right - float(pointsVisible), bounds.bottom));
    }
    float x = bounds.right - float(pointsToRender);
    
    for (int i = m_Values.size() - pointsToRender; i < int(m_Values.size()); ++i)
    {
        float value = (m_MinValue != m_MaxValue) ? ((m_Values[i] - m_MinValue) / (m_MaxValue - m_MinValue)) : 0.5f;
        float center = round(bounds.Height() * (1.0f - value));
        
        FillRect(Rect(x, center, x + 1.0f, bounds.bottom));
        EraseRect(Rect(x, bounds.top, x + 1.0f, center));
        x += 1.0f;
    }
}
