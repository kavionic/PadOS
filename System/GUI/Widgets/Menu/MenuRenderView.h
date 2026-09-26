// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/View.h>
#include <GUI/Widgets/Menu.h>


class PMenuRenderView : public PView
{
public:
    PMenuRenderView(PMenu* menu) : PView("menu_content", ptr_tmp_cast(menu), PViewFlags::WillDraw), m_Menu(menu) {}

    virtual PPoint   CalculateContentSize() const override;

    virtual void    OnPaint(const PRect& updateRect) override;

    virtual void    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;

private:
    PMenu*           m_Menu;
    PPointerID   m_HitPointerID = PInvalidPointerID;
    PPoint           m_HitPos;
    bool            m_MouseMoved = false;
};
