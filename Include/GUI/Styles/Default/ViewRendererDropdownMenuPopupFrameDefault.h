// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererDropdownMenuPopupFrame.h>

class PViewRendererDropdownMenuPopupFrameDefault : public PViewRendererDropdownMenuPopupFrame
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual PRect GetContentFrame(const PView& target, PtrTarget* context) const override;

private:
    struct Context;
    static constexpr float DROPDOWN_MENU_POPUP_BORDER_X = 2.0f;
    static constexpr float DROPDOWN_MENU_POPUP_BORDER_Y = 4.0f;
};
