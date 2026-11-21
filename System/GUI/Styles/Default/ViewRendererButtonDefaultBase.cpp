// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Default/ViewRendererButtonDefaultBase.h>
#include <GUI/View.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererButtonDefaultBase::Setup(PView& target)
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonDefaultBase::PropertyChanged(PView& target, PButtonProperties properties, PtrTarget* context)
{
    const bool sizeChanged = properties.HasAny(PButtonProperties(PButtonProperty::Label, PButtonProperty::Font, PButtonProperty::LabelAlignment));
    if (sizeChanged) {
        static_cast<Context*>(context)->IsLabelSizeValid = false;
    }
    target.Invalidate();
    if (sizeChanged) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererButtonDefaultBase::UpdateMetrics(const PView& target, const PButtonStyleData& data, Context& state)
{
    if (!state.IsLabelSizeValid)
    {
        state.LabelSize.x = target.GetStringWidth(data.Label);
        const PFontHeight fontHeight = target.GetFontHeight();
        state.LabelSize.y = fontHeight.descender + fontHeight.ascender + fontHeight.line_gap;
        state.IsLabelSizeValid = true;
    }
}
