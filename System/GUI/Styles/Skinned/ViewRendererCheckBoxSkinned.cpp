// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Styles/Skinned/ViewRendererCheckBoxSkinned.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererCheckBoxSkinned::PViewRendererCheckBoxSkinned(const PCheckBoxSkin& skin) : m_CheckBoxSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererCheckBoxSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& renderContext = *static_cast<Context*>(context);
    const PButtonStyleData& data = renderContext.GetTarget().GetButtonStyleData();
    const bool isEnabled = renderContext.GetTarget().IsButtonEnabled();
    const PFontHeight fontHeight = target.GetFontHeight();
    const float labelTop = std::round(frame.top + (frame.Height() - fontHeight.ascender - fontHeight.descender) * 0.5f);
    const float labelOffset = m_CheckBoxSkin.Size + m_CheckBoxSkin.LabelSpacing;
    const float contentLeft = frame.left - (data.Label.empty() ? 0.0f : std::min(0.0f, labelOffset));
    const PPoint labelPosition(contentLeft + labelOffset, labelTop + fontHeight.ascender + m_CheckBoxSkin.LabelVerticalOffset);
    const PPoint indicatorPosition(contentLeft, std::round(frame.top + (frame.Height() - m_CheckBoxSkin.Size) * 0.5f));
    const bool overlaps = !data.Label.empty() && m_CheckBoxSkin.LabelSpacing < 0.0f;
    PaintIndicatorButton(target, frame, isEnabled, data, m_CheckBoxSkin, indicatorPosition, labelPosition, overlaps);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererCheckBoxSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& renderContext = *static_cast<Context*>(context);
    const PButtonStyleData& data = renderContext.GetTarget().GetButtonStyleData();
    PPoint size(m_CheckBoxSkin.Size);
    if (!data.Label.empty())
    {
        UpdateMetrics(target, data, renderContext);
        const PFontHeight fontHeight = target.GetFontHeight();
        size.x = GetSpacedExtent(m_CheckBoxSkin.Size, renderContext.LabelSize.x, m_CheckBoxSkin.LabelSpacing);
        size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender + 2.0f * std::abs(m_CheckBoxSkin.LabelVerticalOffset));
    }
    *minSize = size;
    *maxSize = size;
}
