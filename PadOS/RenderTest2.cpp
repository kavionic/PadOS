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
// Created: 03.12.2017 02:55:41

#include "sam.h"

#include "RenderTest2.h"
#include "System/Math/Rect.h"
#include "System/GUI/Application.h"

static const int historySize = 200;

RenderTest2::RenderTest2() : View("Test2")
{
    history.resize(historySize*2);
}

RenderTest2::~RenderTest2()
{
}

void RenderTest2::AllAttachedToScreen()
{
    SetFgColor(Color(0xffffffff));
    FillRect(GetBounds());
    
    Rect bounds = GetBounds();
    
    pos1 = Point(rand() % int(bounds.Width()), rand() % int(bounds.Height()));
    pos2 = Point(rand() % int(bounds.Width()), rand() % int(bounds.Height()));
    
    dir1 = Point(1,-1);
    dir2 = Point(1, 1);
    
    m_UpdateTimer.Set(100);
    m_UpdateTimer.SignalTrigged.Connect(this, &RenderTest2::SlotFrameProcess);
    
    Application* app = GetApplication();
    app->AddTimer(&m_UpdateTimer);
}

bool RenderTest2::OnMouseUp( MouseButton_e button, const Point& position )
{
    SignalDone(ptr_tmp_cast(this));
    return true;
}

void RenderTest2::SlotFrameProcess()
{
    Rect bounds = GetBounds();

    static const int speed = 2;

    for (int j = 0 ; j < 50 ; ++j )
    {
        pos1 += dir1;
        pos2 += dir2;
        if (pos1.x < 0) {
            pos1.x = 0;
            dir1.x = 1 + rand() % speed;
        } else if (pos1.x >= bounds.right) {
            pos1.x = bounds.right - 1.0f;
            dir1.x = -(1 + rand() % speed);
        }
        if (pos1.y < 0) {
            pos1.y = 0;
            dir1.y = 1 + rand() % speed;
        } else if (pos1.y >= bounds.bottom) {
            pos1.y = bounds.bottom - 1;
            dir1.y = -(1 + rand() % speed);
        }

        if (pos2.x < 0) {
            pos2.x = 0;
            dir2.x = 1 + rand() % speed;
        } else if (pos2.x >= bounds.right) {
            pos2.x = bounds.right - 1;
            dir2.x = -(1 + rand() % speed);
        }
        if (pos2.y < 0) {
            pos2.y = 0;
            dir2.y = 1 + rand() % speed;
        } else if (pos2.y >= bounds.bottom) {
            pos2.y = bounds.bottom - 1;
            dir2.y = -(1 + rand() % speed);
        }
        SetFgColor(Color(0xffffffff));
        DrawLine(history[historyPos*2].x, history[historyPos*2].y, history[historyPos*2+1].x, history[historyPos*2+1].y);
        history[historyPos*2] = pos1;
        history[historyPos*2+1] = pos2;
        historyPos = (historyPos + 1) % historySize;
        SetFgColor(Color(rand()));
        DrawLine(pos1.x, pos1.y, pos2.x, pos2.y);
    }        
    Sync();
}

