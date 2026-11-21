// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererDropdownMenuPopupFrame.h>
#include <GUI/Styles/SkinnableGUIStyle.h>

class PViewRendererDropdownMenuPopupFrameSkinned : public PViewRendererDropdownMenuPopupFrame
{
public:
    explicit PViewRendererDropdownMenuPopupFrameSkinned(const PDropdownMenuSkin& skin, Ptr<const PBrush> backgroundBrush);
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual PRect GetContentFrame(const PView& target, PtrTarget* context) const override;

private:
    struct Context;

    const PDropdownMenuSkin m_DropdownMenuSkin;
    const Ptr<const PBrush> m_BackgroundBrush;
};
