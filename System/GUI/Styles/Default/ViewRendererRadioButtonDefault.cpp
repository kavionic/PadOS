// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Default/ViewRendererRadioButtonDefault.h>

#include <algorithm>
#include <cmath>

#include <GUI/View.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererRadioButtonDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PButtonStyleData& data = state.GetTarget().GetButtonStyleData();
    const bool isEnabled = state.GetTarget().IsButtonEnabled();
    const PStandardColorID backgroundColor = (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    const PRect buttonFrame = DrawRadioButtonLabel(target, frame, data, state);

    target.SetBackgroundBrush(PStandardColorID::DefaultBackground);
    target.SetFgColor(PStandardColorID::Shadow);

    float radius = std::floor(RADIO_BUTTON_SIZE * 0.5f);
    const PPoint center(std::floor(buttonFrame.left + buttonFrame.Width() * 0.5f), std::floor(buttonFrame.top + buttonFrame.Height() * 0.5f));

    radius -= 1.0f;
    target.FillCircle(center, radius);

    target.SetFgColor(backgroundColor);
    radius -= 3.0f;
    target.FillCircle(center, radius);

    if (data.IsChecked)
    {
        target.SetFgColor(PStandardColorID::Shadow);
        radius -= 3.0f;
        target.FillCircle(center, radius);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererRadioButtonDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PButtonStyleData& data = state.GetTarget().GetButtonStyleData();
    PPoint size(RADIO_BUTTON_SIZE, RADIO_BUTTON_SIZE);
    if (!data.Label.empty())
    {
        UpdateMetrics(target, data, state);
        const PFontHeight fontHeight = target.GetFontHeight();
        if (data.LabelAlignment == PAlignment::Top || data.LabelAlignment == PAlignment::Bottom)
        {
            size.x = std::max(size.x, state.LabelSize.x);
            size.y += RADIO_BUTTON_LABEL_VERTICAL_SPACING + fontHeight.ascender + fontHeight.descender;
        }
        else
        {
            size.x += RADIO_BUTTON_LABEL_HORIZONTAL_SPACING + state.LabelSize.x;
            size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender);
        }
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererRadioButtonDefault::DrawRadioButtonLabel(PView& target, const PRect& frame, const PButtonStyleData& data, Context& state)
{
    PRect buttonFrame = frame;
    if (!data.Label.empty())
    {
        UpdateMetrics(target, data, state);
        const PFontHeight fontHeight = target.GetFontHeight();
        const float labelWidth = state.LabelSize.x;
        const PAlignment labelAlignment = data.LabelAlignment;
        if (labelAlignment == PAlignment::Top || labelAlignment == PAlignment::Bottom)
        {
            const float labelLeft = std::floor((frame.Width() - labelWidth) * 0.5f);
            if (labelAlignment == PAlignment::Top)
            {
                target.MovePenTo(labelLeft, fontHeight.ascender);
                buttonFrame.top += fontHeight.ascender + fontHeight.descender + RADIO_BUTTON_LABEL_VERTICAL_SPACING;
            }
            else
            {
                target.MovePenTo(labelLeft, RADIO_BUTTON_SIZE + RADIO_BUTTON_LABEL_VERTICAL_SPACING + fontHeight.ascender);
                buttonFrame.bottom -= fontHeight.ascender + fontHeight.descender + RADIO_BUTTON_LABEL_VERTICAL_SPACING;
            }
        }
        else
        {
            const float baseline = std::floor(frame.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender);
            if (labelAlignment == PAlignment::Left)
            {
                target.MovePenTo(0.0f, baseline);
                buttonFrame.left += labelWidth + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING;
            }
            else
            {
                target.MovePenTo(RADIO_BUTTON_SIZE + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING, baseline);
                buttonFrame.right -= labelWidth + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING;
            }
        }
        target.SetFgColor(0, 0, 0);
        target.SetBrush(PStandardColorID::DefaultBackground);
        target.DrawString(target.GetBrush(), data.Label, target.GetPenPosition(), frame);
    }
    else
    {
        target.FillRect(frame, PStandardColorID::DefaultBackground);
    }
    return buttonFrame;
}
