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
// Created: 03.12.2017 03:15:54

#include "sam.h"

#include "RenderTest3.h"
#include "System/GUI/Application.h"

RenderTest3::RenderTest3() : View("Test3")
{
}

RenderTest3::~RenderTest3()
{
}

void RenderTest3::AllAttachedToScreen()
{
    SetFgColor(Color(0xffffffff));
    FillRect(GetBounds());

    m_UpdateTimer.Set(100);
    m_UpdateTimer.SignalTrigged.Connect(this, &RenderTest3::SlotFrameProcess);
    
    Application* app = GetApplication();
    app->AddTimer(&m_UpdateTimer);
}

bool RenderTest3::OnMouseUp(MouseButton_e button, const Point& position)
{
    SignalDone(ptr_tmp_cast(this));
    return true;
}

void RenderTest3::SlotFrameProcess()
{
    Rect bounds = GetBounds();
    for (int j = 0 ; j < 50 ; ++j )
    {    
        SetFgColor(Color(rand()));
        FillCircle(Point(50 + rand() % int(bounds.Width() - 100), 50 + rand() % int(bounds.Height() - 100)), 5+rand() % 45);
    
        if (m_CircleCount++ > 1000)
        {
            m_CircleCount = 0;
            SetFgColor(Color(rand()));
            FillRect(bounds);
        }
    }    
    Sync();    
}
