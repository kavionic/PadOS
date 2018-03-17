// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 30.01.2014 22:22:22

#include "GUI.h"
#include "View.h"

#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"
#include "Kernel/HAL/SAME70System.h"
#include "System/System.h"

//#define QUAD_DECODER_TIMER TCC1

using namespace kernel;

GUI gui;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

GUI::GUI()
{
    m_PressedButtons = 0;
    m_WheelPos = 0;
    
    m_EventQueueInPos = -1;
    m_EventQueueOutPos = 0;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::Initialize()
{
    TouchDriver::Instance.SignalTouchEvent.Connect(this, &GUI::SlotTouchEvent);    

    m_TopView = ptr_new<View>();
    m_TopView->SetFrame(GetScreenFrame());
    m_TopView->m_IsAttachedToScreen = true;
    m_TopView->SetFrame(GetScreenFrame());

    m_PrevTouchPollTime = get_system_time();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::Tick()
{
    bigtime_t curTime = get_system_time();
        
    if ((curTime - m_PrevTouchPollTime) > bigtime_from_ms(1))
    {
        SAME70System::ResetWatchdog();
        TouchDriver::Instance.Tick();
        m_PrevTouchPollTime = curTime;
    }
        
    GUIEvent event;
    if (GetEvent(&event))
    {
        //DEBUG_DISABLE_IRQ();
        switch(event.m_EventID)
        {
            case e_EventMouseDown:
                HandleMouseDown(event.Data.Mouse.ButtonID, Point(event.Data.Mouse.x, event.Data.Mouse.y));
                break;
            case e_EventMouseUp:
                HandleMouseUp(event.Data.Mouse.ButtonID, Point(event.Data.Mouse.x, event.Data.Mouse.y));
                break;
            case e_EventMouseMove:
                HandleMouseMove(event.Data.Mouse.ButtonID, Point(event.Data.Mouse.x, event.Data.Mouse.y));
                break;
        }
    }
    SignalFrameProcess();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

Rect GUI::GetScreenFrame() const
{
    return Rect(Point(0.0f), Point(GfxDriver::Instance.GetResolution() - IPoint(1, 1)));
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

IRect GUI::GetScreenIFrame() const
{
    return IRect(IPoint(0), GfxDriver::Instance.GetResolution() - IPoint(1, 1));
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::AddView(Ptr<View> view)
{
    m_TopView->AddChild(view);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::RemoveView(Ptr<View> view)
{
    m_TopView->RemoveChild(view);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::FrameProcess()
{
/*    TouchDriver::Instance.UpdateCursorPos();
    
    bool wasPressed = (m_PressedButtons & IS_MOUSE_PRESSED) != 0;
    bool isPressed = TouchDriver::Instance.IsPressed();

    int16_t posX;
    int16_t posY;
    TouchDriver::Instance.GetCursorPos(&posX, &posY);
    if (isPressed)
    {
        m_MousePosition.x = posX;
        m_MousePosition.y = posY;
    }
    if ( isPressed != wasPressed )
    {
        GUIEvent event;
        event.m_EventID = (isPressed) ? e_EventMouseDown : e_EventMouseUp;
        //TouchDriver::Instance.GetCursorPos(&event.Data.Mouse.x, &event.Data.Mouse.y);
        event.Data.Mouse.x = m_MousePosition.x;
        event.Data.Mouse.y = m_MousePosition.y;
        AddEvent(event);
        if ( isPressed ) {
            m_PressedButtons |= IS_MOUSE_PRESSED;
        } else {
            m_PressedButtons &= ~IS_MOUSE_PRESSED;
        }
    }*/
/*    uint16_t wheelPos = QUAD_DECODER_TIMER.CNT;
    if ( wheelPos != m_WheelPos )
    {
        int16_t delta = wheelPos - m_WheelPos;
        int16_t filteredDelta = delta / 4;
        
        if ( filteredDelta )
        {
            GUIEvent event;
            event.m_EventID = e_EventMouseWheel;
            event.Data.Wheel.delta = filteredDelta;
            m_WheelPos += filteredDelta * 4;
            AddEvent(event);
        }            
    }
//    uint16 backlight = QUAD_DECODER_TIMER.CNT * (LCD_LED_PWM_MAX / 255);
//    LED_PWM_TIMER.CCA = backlight;
*/
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::AddEvent( const GUIEvent& event )
{
    //        printf_P(PSTR("Send event %d (%d, %d)\n"), event.m_EventID, event.m_Data[0], event.m_Data[1]);
    if ( m_EventQueueInPos != -1 )
    {
        if ( m_EventQueueInPos != m_EventQueueOutPos )
        {
            m_EventQueue[m_EventQueueInPos] = event;
            m_EventQueueInPos = (m_EventQueueInPos + 1) & EVENT_QUEUE_SIZE_MASK;
        }
    }
    else
    {
        m_EventQueue[m_EventQueueOutPos] = event;
        m_EventQueueInPos = (m_EventQueueOutPos + 1) & EVENT_QUEUE_SIZE_MASK;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool GUI::GetEvent( GUIEvent* event )
{
    if ( m_EventQueueInPos != -1 )
    {
        *event = m_EventQueue[m_EventQueueOutPos];
        m_EventQueueOutPos = (m_EventQueueOutPos + 1) & EVENT_QUEUE_SIZE_MASK;
        if ( m_EventQueueInPos == m_EventQueueOutPos )
        {
            m_EventQueueInPos = -1; // Queue empty
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::HandleMouseDown(MouseButton_e button, const Point& position)
{
    m_TopView->HandleMouseDown(button, position);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::HandleMouseUp(MouseButton_e button, const Point& position)
{
    if (m_MouseDownView.Lock() != nullptr)
    {
        m_MouseDownView.Lock()->OnMouseUp(button, position - m_MouseDownView.Lock()->m_ScreenPos - m_MouseDownView.Lock()->m_cScrollOffset);
    }
    if (m_FocusView.Lock() != nullptr && m_FocusView != m_MouseDownView)
    {
        m_FocusView.Lock()->OnMouseUp(button, position - m_FocusView.Lock()->m_ScreenPos - m_FocusView.Lock()->m_cScrollOffset);
    }
    m_MouseDownView = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::HandleMouseMove(MouseButton_e button, const Point& position)
{
    if (m_MouseDownView.Lock() != nullptr)
    {
        m_MouseDownView.Lock()->OnMouseMove(button, position - m_MouseDownView.Lock()->m_ScreenPos - m_MouseDownView.Lock()->m_cScrollOffset);
    }
    if (m_FocusView.Lock() != nullptr && m_FocusView != m_MouseDownView)
    {
        m_FocusView.Lock()->OnMouseMove(button, position - m_FocusView.Lock()->m_ScreenPos - m_FocusView.Lock()->m_cScrollOffset);
    }    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void GUI::SlotTouchEvent(TouchDriver::EventID_e eventID, int touchID, const IPoint& position)
{
    GUIEvent event;
    event.m_EventID = e_EventNone;
    
    switch(eventID)
    {
        case TouchDriver::EventID_e::Down: event.m_EventID = e_EventMouseDown; break;
        case TouchDriver::EventID_e::Up:   event.m_EventID = e_EventMouseUp;   break;
        case TouchDriver::EventID_e::Move: event.m_EventID = e_EventMouseMove; break;
        default: break;
    }
    if (event.m_EventID != e_EventNone)
    {
        event.Data.Mouse.ButtonID = MouseButton_e(int(MouseButton_e::FirstTouchID) + touchID);
        event.Data.Mouse.x = float(position.x);
        event.Data.Mouse.y = float(position.y);
        AddEvent(event);
    }        
}
