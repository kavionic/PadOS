// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleTextBox.h>
#include <GUI/Styles/Default/ViewRendererTextBoxDefault.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererTextBoxDefault::Setup(PView& target) const
{
    return ptr_new<PViewRendererContext<PViewStyleTextBox>>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTextBoxDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PTextBoxStyleData& data = static_cast<PViewRendererContext<PViewStyleTextBox>*>(context)->GetTarget().GetTextBoxStyleData();
    target.SetBackgroundBrush(target.GetStyle()->GetBrush(GetBackgroundColorID(data)));
    target.DrawFrame(frame, data.HasRaisedFrame ? FRAME_RAISED : FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTextBoxDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PTextBoxStyleData& data = static_cast<PViewRendererContext<PViewStyleTextBox>*>(context)->GetTarget().GetTextBoxStyleData();
    *minSize = GetSizeForContent(target, data.EditorSize, context);
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTextBoxDefault::PropertyChanged(PView& target, PTextBoxProperties properties, PtrTarget* context) const
{
    if (properties.Has(PTextBoxProperty::EditorSize)) {
        target.PreferredSizeChanged();
    }
    if (properties != PTextBoxProperty::EditorSize) {
        target.Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererTextBoxDefault::GetEditorFrame(const PView& target, PtrTarget* context) const
{
    PRect frame = target.GetBounds();
    frame.left = std::min(frame.right, frame.left + EDITOR_INSET);
    frame.top = std::min(frame.bottom, frame.top + EDITOR_INSET);
    frame.right = std::max(frame.left, frame.right - EDITOR_INSET);
    frame.bottom = std::max(frame.top, frame.bottom - EDITOR_INSET);
    return frame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererTextBoxDefault::GetSizeForContent(const PView& target, const PPoint& contentSize, PtrTarget* context) const
{
    return contentSize + PPoint(EDITOR_INSET * 2.0f);
}
