// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleDropdownMenuPopupFrame.h>
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererDropdownMenuPopupFrameSkinned.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

struct PViewRendererDropdownMenuPopupFrameSkinned::Context : PViewRendererContext<PViewStyleDropdownMenuPopupFrame>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererDropdownMenuPopupFrameSkinned::PViewRendererDropdownMenuPopupFrameSkinned(const PDropdownMenuSkin& skin, Ptr<const PBrush> backgroundBrush)
    : m_DropdownMenuSkin(skin)
    , m_BackgroundBrush(std::move(backgroundBrush))
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererDropdownMenuPopupFrameSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupFrameSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_BackgroundBrush);
    target.SetBrush(nullptr);
    target.SetBrushOrigin(frame.TopLeft());
    target.FillRect(frame, m_DropdownMenuSkin.PopupFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererDropdownMenuPopupFrameSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PPoint contentSize = static_cast<Context*>(context)->GetTarget().GetPopupContentSize();
    const PRect& insets = m_DropdownMenuSkin.PopupInsets;
    *minSize = PPoint(std::max(0.0f, contentSize.x + insets.left + insets.right), std::max(0.0f, contentSize.y + insets.top + insets.bottom));
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererDropdownMenuPopupFrameSkinned::GetContentFrame(const PView& target, PtrTarget* context) const
{
    return PInsetSkinnedFrame(target.GetBounds(), m_DropdownMenuSkin.PopupInsets);
}
