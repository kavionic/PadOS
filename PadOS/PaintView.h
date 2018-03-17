// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 07.11.2017 22:20:43

#pragma once

#include "System/GUI//View.h"


class Button : public View
{
public:
    virtual bool OnMouseDown(MouseButton_e button, const Point& position) override;
    virtual bool OnMouseUp(MouseButton_e button, const Point& position) override;
    virtual bool OnMouseMove(MouseButton_e button, const Point& position) override;

    virtual void Render() override;


    void SetPressedState(bool isPressed);
    bool GetPressedState() const { return m_IsPressed; }

    Signal<void, MouseButton_e, Button*> SignalActivated;
    
private:
    bool m_WasHit    = false;
    bool m_IsPressed = false;
        
};

class PaintView : public View
{
public:
    PaintView();
    ~PaintView();

    virtual void PostAttachedToViewport() override;

    virtual bool OnMouseDown(MouseButton_e button, const Point& position) override;
    virtual bool OnMouseUp(MouseButton_e button, const Point& position) override;
    virtual bool OnMouseMove(MouseButton_e button, const Point& position) override;


    //virtual void Render() override;

    Signal<void, Ptr<View>> SignalDone;
private:
    void SlotClearButton();
    void SlotNextButton();

    bool m_WasHit = false;



    PaintView( const PaintView &c );
    PaintView& operator=( const PaintView &c );

};
