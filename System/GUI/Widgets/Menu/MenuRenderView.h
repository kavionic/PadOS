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

namespace os
{

class MenuRenderView : public View
{
public:
    MenuRenderView(Menu* menu) : View("menu_content", ptr_tmp_cast(menu), ViewFlags::WillDraw), m_Menu(menu) {}

    virtual Point   CalculateContentSize() const override;

    virtual void    OnPaint(const Rect& updateRect) override;

    virtual bool    OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnLongPress(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;

    virtual bool    OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;

private:
    Menu*           m_Menu;
    MouseButton_e   m_HitButton = MouseButton_e::None;
    Point           m_HitPos;
    bool            m_MouseMoved = false;
};

} // namespace os;
