// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/ButtonStyleData.h>
#include <GUI/View.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonStyleData::UpdateMetrics(const PView& target)
{
    if (!IsLabelSizeValid)
    {
        LabelSize.x = target.GetStringWidth(Label);
        const PFontHeight fontHeight = target.GetFontHeight();
        LabelSize.y = fontHeight.descender + fontHeight.ascender + fontHeight.line_gap;
        IsLabelSizeValid = true;
    }
}
