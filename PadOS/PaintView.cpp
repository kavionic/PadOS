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

#include "PaintView.h"

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool Button::OnMouseDown(MouseButton_e button, const Point& position)
{
    printf("Button: Mouse down %d, %.1f/%.1f\n", int(button), position.x, position.y);
    m_WasHit = true;
    SetPressedState(true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool Button::OnMouseUp(MouseButton_e button, const Point& position)
{
    printf("Button: Mouse up %d, %.1f/%.1f\n", int(button), position.x, position.y);
    if (m_WasHit)
    {
        m_WasHit = false;
        if (m_IsPressed)
        {
            SetPressedState(false);
            SignalActivated(button, this);
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool Button::OnMouseMove(MouseButton_e button, const Point& position)
{
    if (m_WasHit)
    {
        printf("Button: Mouse move %d, %.1f/%.1f\n", int(button), position.x, position.y);
        SetPressedState(GetBounds().DoIntersect(position));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Button::Render()
{
    DrawBevelBox(GetBounds(), !m_IsPressed);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void Button::SetPressedState(bool isPressed)
{
    if (isPressed != m_IsPressed)
    {
        m_IsPressed = isPressed;
        Invalidate();
        UpdateIfNeeded(false);
    }
}


///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

PaintView::PaintView()
{
    Rect bounds = GetBounds();
    
    Ptr<Button> clearButton = ptr_new<Button>();
    clearButton->SignalActivated.Connect(this, &PaintView::SlotClearButton);
    clearButton->SetFrame(Rect(0.0f, 0.0f, 80.0f, 40.0f));
    AddChild(clearButton);
    Ptr<Button> nextButton = ptr_new<Button>();
    nextButton->SignalActivated.Connect(this, &PaintView::SlotNextButton);
    nextButton->SetFrame(Rect(720.0f, 0.0f, 799.0f, 40.0f));
    AddChild(nextButton);
    

    Rect frame(0.0f, 0.0f, 80.0f, 40.0f);
    frame += Point(50.0f, 60.0f);
    for (int i = 0; i < 7; ++i)
    {
        Ptr<Button> button = ptr_new<Button>();
        //button->SignalActivated.Connect(this, &PaintView::SlotClearButton);
        button->SetFrame(frame);
        AddChild(button);
        frame += Point(100.0f, 0.0f);        
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

PaintView::~PaintView()
{
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PaintView::PostAttachedToViewport()
{
    SetFgColor(0xffff);
    FillRect(GetBounds());
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool PaintView::OnMouseDown( MouseButton_e button, const Point& position )
{
    m_WasHit = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool PaintView::OnMouseUp( MouseButton_e button, const Point& position )
{
    if (m_WasHit)
    {
        m_WasHit = false;
        return true;
    }
    return false;        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool PaintView::OnMouseMove(MouseButton_e button, const Point& position)
{
    if (m_WasHit)
    {
        SetFgColor(0,255,0);
        FillCircle(position, 4.0f);
        SetFgColor(0,255,255);
        FillCircle(position, 2.0f);
        return true;
    }
    return false;        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PaintView::SlotClearButton()
{
    FillRect(GetBounds(), 0xffff);
//    Invalidate(true);
//    UpdateIfNeeded(false);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PaintView::SlotNextButton()
{
    SignalDone(ptr_tmp_cast(this));
}
