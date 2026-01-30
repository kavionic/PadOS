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

    virtual bool    OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnLongPress(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;

    virtual bool    OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;

private:
    PMenu*           m_Menu;
    PMouseButton   m_HitButton = PMouseButton::None;
    PPoint           m_HitPos;
    bool            m_MouseMoved = false;
};
