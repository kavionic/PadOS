// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Styles/Default/ViewRendererCheckBoxDefault.h>
#include <GUI/View.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererCheckBoxDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PButtonStyleData& data = state.GetTarget().GetButtonStyleData();
    const bool isEnabled = state.GetTarget().IsButtonEnabled();
    const PStandardColorID backgroundColor = (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    const PFontHeight fontHeight = target.GetFontHeight();
    const float baseline = frame.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender;
    target.SetFgColor(target.GetStyle()->GetColor(GetLabelColorID(CHECK_BOX_LABEL_COLORS, isEnabled, data)));
    target.SetBrush(PStandardColorID::DefaultBackground);
    target.DrawString(target.GetBrush(), data.Label, PPoint(CHECK_BOX_SIZE + CHECK_BOX_LABEL_SPACING, baseline), frame);

    target.SetBackgroundBrush(backgroundColor);

    const float buttonTop = (frame.Height() - CHECK_BOX_SIZE) * 0.5f;
    const PRect buttonFrame(0.0f, buttonTop, CHECK_BOX_SIZE, buttonTop + CHECK_BOX_SIZE);
    target.DrawFrame(buttonFrame, FRAME_RECESSED);

    if (data.IsChecked)
    {
        target.SetFgColor(0, 0, 0);
        PPoint point1(buttonFrame.left + CHECK_BOX_CHECK_BORDER, buttonFrame.top + CHECK_BOX_CHECK_BORDER);
        PPoint point2(buttonFrame.right - CHECK_BOX_CHECK_BORDER, buttonFrame.bottom - CHECK_BOX_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            target.DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
        point1 = PPoint(buttonFrame.left + CHECK_BOX_CHECK_BORDER, buttonFrame.bottom - CHECK_BOX_CHECK_BORDER);
        point2 = PPoint(buttonFrame.right - CHECK_BOX_CHECK_BORDER, buttonFrame.top + CHECK_BOX_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            target.DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererCheckBoxDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PButtonStyleData& data = state.GetTarget().GetButtonStyleData();
    PPoint size(CHECK_BOX_SIZE, CHECK_BOX_SIZE);
    if (!data.Label.empty())
    {
        UpdateMetrics(target, data, state);
        const PFontHeight fontHeight = target.GetFontHeight();
        size.x += CHECK_BOX_LABEL_SPACING + state.LabelSize.x;
        size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender);
    }
    *minSize = size;
    *maxSize = size;
}
