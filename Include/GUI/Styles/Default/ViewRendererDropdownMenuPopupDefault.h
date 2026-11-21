// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererDropdownMenuPopup.h>

class PViewRendererDropdownMenuPopupDefault : public PViewRendererDropdownMenuPopup
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PDropdownMenuPopupProperties properties, PtrTarget* context) const override;
    virtual PRect GetItemFrame(const PView& target, size_t index, PtrTarget* context) const override;
    virtual size_t GetItemFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;
    static constexpr float DROPDOWN_MENU_POPUP_LABEL_PADDING = 2.0f;
    static void UpdateDropdownMenuPopupMetrics(const PView& target, const PDropdownMenuPopupStyleData& data, Context& state);
    static PRect CalculateItemFrame(const PView& target, const PDropdownMenuPopupStyleData& data, size_t index, Context& state);
};
