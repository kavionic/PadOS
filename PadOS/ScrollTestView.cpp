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
// Created: 05.12.2017 23:54:20

#include "sam.h"

#include <math.h>

#include "ScrollTestView.h"
#include "System/GUI/Application.h"


ScrollTestView::ScrollTestView() : View("Scroll")
{
}

ScrollTestView::~ScrollTestView()
{
}

void ScrollTestView::AllAttachedToScreen()
{
//    SetFgColor(0xffff);
//    FillRect(GetBounds());
    m_UpdateTimer.Set(100);
    m_UpdateTimer.SignalTrigged.Connect(this, &ScrollTestView::SlotFrameProcess);
    
    Application* app = GetApplication();
    app->AddTimer(&m_UpdateTimer);
//    Invalidate(true);
}

bool ScrollTestView::OnMouseDown( MouseButton_e button, const Point& position )
{
    if (m_HitButton == MouseButton_e::None)
    {
        m_HitButton = button;
        m_HitPos = position;
    }
    return true;
}

bool ScrollTestView::OnMouseUp( MouseButton_e button, const Point& position )
{
    if (button == m_HitButton)
    {
        m_HitButton = MouseButton_e::None;
    }        
    else if (m_HitButton != MouseButton_e::None)
    {
        SignalDone(ptr_tmp_cast(this));
        //ScrollTo(0.0f, 0.0f);
    }        
    return true;
}

bool ScrollTestView::OnMouseMove(MouseButton_e button, const Point& position)
{
    if (m_HitButton != MouseButton_e::None && button == m_HitButton)
    {
        ScrollBy(position - m_HitPos);
        Sync();
    }
    return true;
}

void ScrollTestView::Paint(const Rect& updateRect)
{
    Rect bounds = GetBounds();

    Color color;
    color.Set16(rand());

    SetFgColor(Color(255, 255, 255));
    FillRect(bounds);

    SetFgColor(0,255,0);

    float height = bounds.Height();
    float centerY = floor(height * 0.5f);
    Point prevPos(0.0f, centerY);
    for (int i = 2 ; i < bounds.Width(); i += 2)
    {
        float x = float(i);
        Point pos(float(i), centerY + sin(x / 10.0f) * height * 0.25f);
        DrawLine(prevPos, pos);
        prevPos = pos;
//        FillCircle(Point(i,  ), 2.0f);
    }
    
    SetFgColor(color);
    MovePenTo(0.0f, floor(bounds.Height() / 2.0f));
//    DrawString("abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    DrawString("abcdefghijklmnopqrstuvwxyz0123456789ABCDEF");
    
//    DebugDraw(Color(255, 0, 0), ViewDebugDrawFlags::DrawRegion);
}

void ScrollTestView::SlotFrameProcess()
{
    Rect bounds = GetBounds();
}

