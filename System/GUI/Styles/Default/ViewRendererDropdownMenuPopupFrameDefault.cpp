// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/Default/ViewRendererDropdownMenuPopupFrameDefault.h>

#include <algorithm>
#include <cmath>

#include <GUI/View.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleDropdownMenuPopupFrame.h>

struct PViewRendererDropdownMenuPopupFrameDefault::Context : PViewRendererContext<PViewStyleDropdownMenuPopupFrame>
{
    using PViewRendererContext<PViewStyleDropdownMenuPopupFrame>::PViewRendererContext;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererDropdownMenuPopupFrameDefault::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupFrameDefault::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    target.SetBackgroundBrush(PStandardColorID::DropdownMenuBackground);
    target.DrawFrame(frame, FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupFrameDefault::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
    const PPoint contentSize = state.GetTarget().GetPopupContentSize();
    *minSize = contentSize + PPoint(DROPDOWN_MENU_POPUP_BORDER_X * 2.0f, DROPDOWN_MENU_POPUP_BORDER_Y * 2.0f);
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuPopupFrameDefault::GetContentFrame(const PView& target, PtrTarget* context) const
{
    const PRect bounds = target.GetBounds();
    const float horizontalBorder = std::min(DROPDOWN_MENU_POPUP_BORDER_X, std::max(0.0f, bounds.Width() * 0.5f));
    const float verticalBorder = std::min(DROPDOWN_MENU_POPUP_BORDER_Y, std::max(0.0f, bounds.Height() * 0.5f));
    return bounds.GetResized(horizontalBorder, verticalBorder, -horizontalBorder, -verticalBorder);
}
