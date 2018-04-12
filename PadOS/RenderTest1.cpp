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
// Created: 26.11.2017 16:53:43

#include "sam.h"

#include "RenderTest1.h"
#include "System/GUI/Application.h"

RenderTest1::RenderTest1() : View("Test1")
{

    m_Pos.x = 50 + rand() % 700;
    m_Pos.y = 50 + rand() % 380;
    m_Direction.x = 1.0f;
    m_Direction.y = 1.0f;

}

RenderTest1::~RenderTest1()
{
}

void RenderTest1::AllAttachedToScreen()
{
    SetFgColor(255, 255, 255);
    FillRect(GetBounds());
    
   
    m_UpdateTimer.Set(100);
    m_UpdateTimer.SignalTrigged.Connect(this, &RenderTest1::SlotFrameProcess);
    
    Application* app = GetApplication();
    app->AddTimer(&m_UpdateTimer);
}

bool RenderTest1::OnMouseUp( MouseButton_e button, const Point& position )
{
    SignalDone(ptr_tmp_cast(this));
    return true;
}

void RenderTest1::SlotFrameProcess()
{
    Rect bounds = GetBounds();
    static const float radius = 40.0f;
    static const int speed = 5;
//    for (uint16_t j = 0 ; j < 10 ; ++j )
    {
//        if (IsScreenTouched()) return;
        for (uint16_t j = 0 ; j < 5 ; ++j )
        {
            m_Pos.x += m_Direction.x;
            m_Pos.y += m_Direction.y;
            m_Direction.y += 0.05f;
            if (m_Pos.x < bounds.left + radius) {
                m_Pos.x = bounds.left + radius;
                m_Direction.x = 2 + rand() % speed;
            } else if (m_Pos.x > bounds.right - radius) {
                m_Pos.x = bounds.right - radius;
                m_Direction.x = -(2 + rand() % speed);
            }
            if (m_Pos.y < bounds.top + radius) {
                m_Pos.y = bounds.top + radius;
                m_Direction.y = 2 + rand() % speed;
            } else if (m_Pos.y > bounds.bottom - radius) {
                m_Pos.y = bounds.bottom - radius;
                m_Direction.y = -(2 + rand() % speed);
            }
            Color color;
            color.Set16(rand());
            SetFgColor(color);
            FillCircle(m_Pos, radius);
            //            Display::Instance.FillRect(rand() % 480, rand() % 320, rand() % 480, rand() % 320, rand());
            //            gui.FrameProcess();
            //            if ( TouchDriver::Instance.IsPressed() ) break;
        }
    }
//    SetFgColor(rand());
//    FillCircle(Point(50 + rand() % 700, 50 + rand() % 380), 5+rand() % 45);

//    SetFgColor(0, 0, 0);
//    MovePenTo(50.0f, 50.0f);
//    DrawString("Hello world", 512.0f, 0);
    Sync();
}

