// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
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

    virtual bool    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event) override;
    virtual bool    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event) override;
    virtual bool    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event) override;
    virtual bool    OnLongPress(PPointerID pointerID, const PPoint& position, const PPointerEvent& event) override;

private:
    PMenu*           m_Menu;
    PPointerID   m_HitPointerID = PInvalidPointerID;
    PPoint           m_HitPos;
    bool            m_MouseMoved = false;
};
