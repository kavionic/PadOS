// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererDropdownMenu.h>

class PViewRendererDropdownMenuDefault : public PViewRendererDropdownMenu
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PDropdownMenuProperties properties, PtrTarget* context) const override;
    virtual PRect GetEditorFrame(const PView& target, PtrTarget* context) const override;
    virtual bool HitTest(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;
    static constexpr int DROPDOWN_MENU_ARROW_WIDTH = 19;
    static constexpr int DROPDOWN_MENU_ARROW_HEIGHT = 10;
    static constexpr float DROPDOWN_MENU_ARROW_ASPECT_RATIO = 0.9f;
    static constexpr float DROPDOWN_MENU_LABEL_PADDING = 10.0f;
    static constexpr uint32_t DROPDOWN_MENU_ARROW_BITMAP_RASTER[] = {
        0b11111111111111111110000000000000,
        0b01111111111111111100000000000000,
        0b00111111111111111000000000000000,
        0b00011111111111110000000000000000,
        0b00001111111111100000000000000000,
        0b00000111111111000000000000000000,
        0b00000011111110000000000000000000,
        0b00000001111100000000000000000000,
        0b00000000111000000000000000000000,
        0b00000000010000000000000000000000
    };
    static void UpdateDropdownMenuMetrics(const PView& target, const PDropdownMenuStyleData& data, Context& state);
    static PRect GetDropdownMenuArrowFrame(const PRect& frame);
};
