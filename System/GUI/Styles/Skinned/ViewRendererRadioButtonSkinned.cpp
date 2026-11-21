// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Skinned/ViewRendererRadioButtonSkinned.h>

#include <algorithm>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererRadioButtonSkinned::PViewRendererRadioButtonSkinned(const PRadioButtonSkin& skin) : m_RadioButtonSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererRadioButtonSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& renderContext = *static_cast<Context*>(context);
    const PButtonStyleData& data = renderContext.GetTarget().GetButtonStyleData();
    const bool isEnabled = renderContext.GetTarget().IsButtonEnabled();
    const PRadioButtonSkin& skin = m_RadioButtonSkin;
    const PFontHeight fontHeight = target.GetFontHeight();
    const float labelHeight = fontHeight.ascender + fontHeight.descender;
    const float labelTop = std::round(frame.top + (frame.Height() - labelHeight) * 0.5f);
    PPoint labelPosition(frame.left + skin.Size + skin.LabelSpacing, labelTop + fontHeight.ascender + skin.LabelVerticalOffset);
    PPoint indicatorPosition(frame.left, std::round(frame.top + (frame.Height() - skin.Size) * 0.5f));
    bool overlaps = false;
    if (data.Label.empty())
    {
        indicatorPosition.x = std::round(frame.left + (frame.Width() - skin.Size) * 0.5f);
    }
    else
    {
        UpdateMetrics(target, data, renderContext);
        if (data.LabelAlignment == PAlignment::Top || data.LabelAlignment == PAlignment::Bottom)
        {
            const float labelPadding = std::abs(skin.LabelVerticalOffset);
            const float labelExtent = labelHeight + 2.0f * labelPadding;
            const bool labelFirst = data.LabelAlignment == PAlignment::Top;
            const float secondOffset = (labelFirst ? labelExtent : skin.Size) + skin.LabelVerticalSpacing;
            const float contentHeight = GetSpacedExtent(skin.Size, labelExtent, skin.LabelVerticalSpacing);
            const float contentTop = std::round(frame.top + (frame.Height() - contentHeight) * 0.5f) - std::min(0.0f, secondOffset);
            overlaps = skin.LabelVerticalSpacing < 0.0f;
            indicatorPosition.x = std::round(frame.left + (frame.Width() - skin.Size) * 0.5f);
            labelPosition.x = std::round(frame.left + (frame.Width() - renderContext.LabelSize.x) * 0.5f);
            if (labelFirst)
            {
                labelPosition.y = contentTop + labelPadding + fontHeight.ascender + skin.LabelVerticalOffset;
                indicatorPosition.y = contentTop + secondOffset;
            }
            else
            {
                indicatorPosition.y = contentTop;
                labelPosition.y = contentTop + secondOffset + labelPadding + fontHeight.ascender + skin.LabelVerticalOffset;
            }
        }
        else
        {
            const bool labelFirst = data.LabelAlignment == PAlignment::Left;
            const float secondOffset = (labelFirst ? renderContext.LabelSize.x : skin.Size) + skin.LabelSpacing;
            const float contentLeft = frame.left - std::min(0.0f, secondOffset);
            indicatorPosition.x = contentLeft + (labelFirst ? secondOffset : 0.0f);
            labelPosition.x = contentLeft + (labelFirst ? 0.0f : secondOffset);
            overlaps = skin.LabelSpacing < 0.0f;
        }
    }
    PaintIndicatorButton(target, frame, isEnabled, data, skin, indicatorPosition, labelPosition, overlaps);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererRadioButtonSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& renderContext = *static_cast<Context*>(context);
    const PButtonStyleData& data = renderContext.GetTarget().GetButtonStyleData();
    const PRadioButtonSkin& skin = m_RadioButtonSkin;
    PPoint size(skin.Size);
    if (!data.Label.empty())
    {
        UpdateMetrics(target, data, renderContext);
        const PFontHeight fontHeight = target.GetFontHeight();
        const float labelHeight = fontHeight.ascender + fontHeight.descender + 2.0f * std::abs(skin.LabelVerticalOffset);
        if (data.LabelAlignment == PAlignment::Top || data.LabelAlignment == PAlignment::Bottom)
        {
            size.x = std::max(size.x, renderContext.LabelSize.x);
            size.y = GetSpacedExtent(skin.Size, labelHeight, skin.LabelVerticalSpacing);
        }
        else
        {
            size.x = GetSpacedExtent(skin.Size, renderContext.LabelSize.x, skin.LabelSpacing);
            size.y = std::max(size.y, labelHeight);
        }
    }
    *minSize = size;
    *maxSize = size;
}
