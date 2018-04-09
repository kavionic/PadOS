// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 02.04.2018 13:08:47

#pragma once

#include "View.h"


namespace os
{

class Button : public View
{
public:
    Button(const String& name, const String& label, Ptr<View> parent = nullptr, uint32_t flags = 0);
    ~Button();
    virtual void AllAttachedToScreen() override { Invalidate(); }

    virtual Point GetPreferredSize(bool largest) const override;

    virtual bool OnMouseDown(MouseButton_e button, const Point& position) override;
    virtual bool OnMouseUp(MouseButton_e button, const Point& position) override;
    virtual bool OnMouseMove(MouseButton_e button, const Point& position) override;

    virtual void Paint(const Rect& updateRect) override;


    void SetPressedState(bool isPressed);
    bool GetPressedState() const { return m_IsPressed; }

    Signal<void, MouseButton_e, Button*> SignalActivated;
        
private:
    bool m_WasHit    = false;
    bool m_IsPressed = false;
        
    String m_Label;
    
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;
};
    
    
} // namespace
