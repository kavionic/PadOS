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
// Created: 30.01.2014 22:25:16

#pragma once

#include <stdint.h>

#include "Kernel/Drivers/RA8875Driver/TouchDriver.h"

#include "System/Math/Point.h"

#include "System/GUI/GUIEvent.h"
#include "System/GUI/View.h"

class View;

class GUI : public SignalTarget
{
public:
    GUI();
    
    void Initialize();
    
    void Tick();
    
    Rect  GetScreenFrame() const;
    IRect GetScreenIFrame() const;
    
    void AddView(Ptr<View> view);
    void RemoveView(Ptr<View> view);

    void      SetFocusView(Ptr<View> view) { m_FocusView = view; }
    Ptr<View> GetFocusView() const     { return m_FocusView.Lock(); }
    
    void FrameProcess();
    
    void AddEvent(const GUIEvent& event);
    bool GetEvent(GUIEvent* event);
    
    
    Signal<void> SignalFrameProcess;
private:
    friend class View;
    
   
    void SetMouseDownView(Ptr<View> view)
    {
        m_MouseDownView = view;
    }

    
    void HandleMouseDown(MouseButton_e button, const Point& position);
    void HandleMouseUp(MouseButton_e button, const Point& position);
    void HandleMouseMove(MouseButton_e button, const Point& position);

    void SlotTouchEvent(kernel::TouchDriver::EventID_e eventID, int touchID, const IPoint& position);
    
    static const uint8_t EVENT_QUEUE_SIZE = 128;
    static const uint8_t EVENT_QUEUE_SIZE_MASK = EVENT_QUEUE_SIZE - 1;

    Ptr<View>     m_TopView; //       = nullptr;
    WeakPtr<View> m_MouseDownView; // = nullptr;
    WeakPtr<View> m_FocusView; //     = nullptr;

    bigtime_t m_PrevTouchPollTime = 0;

    GUIEvent m_EventQueue[EVENT_QUEUE_SIZE];
    int8_t m_EventQueueInPos;
    int8_t m_EventQueueOutPos;
    
    static const uint8_t IS_MOUSE_PRESSED = 0x01;
    static const uint8_t IS_WHEEL_PRESSED = 0x02;
    
    IPoint   m_MousePosition = IPoint(0, 0);
    uint8_t  m_PressedButtons;
    uint16_t m_WheelPos;
};

extern GUI gui;
