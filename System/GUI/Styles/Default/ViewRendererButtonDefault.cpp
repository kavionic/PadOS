// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Default/ViewRendererButtonDefault.h>

#include <algorithm>
#include <cmath>

#include <GUI/View.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PButtonStyleData& data = state.GetTarget().GetButtonStyleData();
    const PRect& bounds = frame;
    const bool isEnabled = state.GetTarget().IsButtonEnabled();
    UpdateMetrics(target, data, state);

    const PStandardColorID backgroundColor = (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::ButtonBackground;
    target.SetBackgroundBrush(backgroundColor);

    if (isEnabled) {
        target.DrawFrame(bounds, (data.IsPressed ? FRAME_RECESSED : FRAME_RAISED) | FRAME_TRANSPARENT);
    } else {
        target.DrawFrame(bounds, FRAME_DISABLED | FRAME_TRANSPARENT);
    }
    PPoint labelPosition(std::round((bounds.Width() - state.LabelSize.x) * 0.5f), std::round((bounds.Height() - state.LabelSize.y) * 0.5f));

    if (data.IsPressed) {
        labelPosition += PPoint(1.0f, 1.0f);
    }

    labelPosition.y += target.GetFontHeight().ascender;
    target.SetFgColor(isEnabled ? PStandardColorID::ButtonLabelNormal : PStandardColorID::ButtonLabelDisabled);
    target.SetBrush(backgroundColor);
    target.DrawString(data.Label, labelPosition, bounds.GetFloored().GetResized(2.0f, 2.0f, -2.0f, -2.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PButtonStyleData& data = state.GetTarget().GetButtonStyleData();
    UpdateMetrics(target, data, state);
    const PPoint size(state.LabelSize.x + 16.0f, state.LabelSize.y + 8.0f);
    *minSize = size;
    *maxSize = size;
}
