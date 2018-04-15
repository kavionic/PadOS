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
// Created: 03.12.2017 03:16:06

#include "sam.h"
#if 0
#include "RenderTest4.h"
#include "System/System.h"
#include "System/GUI/Application.h"

extern bigtime_t g_FrameTime;

using namespace kernel;

RenderTest4::RenderTest4() : View("RenderTest4"), region2(IRect(50, 0, 400, 480))
{
}

RenderTest4::~RenderTest4()
{
}

void RenderTest4::AllAttachedToScreen()
{
    SetFgColor(Color(0xffffffff));
    FillRect(GetBounds());

    
    IRect frameRect(50.0f, 0.0f, 400.0f, 480.0f);
/*    
    region.AddRect(frameRect);
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(60, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(100, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(140, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(180, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(220, 20) );
    region.RemoveRect(IRect(0.0f, 0.0f, 30.0f, 30.0f) + IPoint(100, 60) );
*/

    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(60, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(100, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(140, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(180, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(220, 20) );
    region2.Exclude(IRect(0, 0, 30, 30.0f) + IPoint(100, 60) );
    region2.Optimize();

    m_UpdateTimer.Set(100);
    m_UpdateTimer.SignalTrigged.Connect(this, &RenderTest4::SlotFrameProcess);
    
    Application* app = GetApplication();
    app->AddTimer(&m_UpdateTimer);
}

void RenderTest4::DetachedFromScreen()
{
    m_UpdateTimer.Stop();
}

bool RenderTest4::OnMouseUp(MouseButton_e button, const Point& position)
{
    SignalDone(ptr_tmp_cast(this));
    return true;
}

void RenderTest4::SlotFrameProcess()
{
    Rect bounds = GetBounds();
        
    bigtime_t startTime = get_system_time();
    for (int i = 0; i < 10; ++i)
    {
        SetFgColor(Color(random()));
        for (int y = 10; y < 310; ++y)
        {
#if 1
            
            ENUMCLIPLIST(&region2.m_cRects, node)
            {
                int x1 = 20;
                int y1 = y;
                int x2 = 400;
                int y2 = y;
                    
//                    GfxDriver::Instance.WaitBlitter();
//                    GfxDriver::Instance.SetWindow(node->m_cBounds.left, node->m_cBounds.top, node->m_cBounds.right, node->m_cBounds.bottom);
//                    GfxDriver::Instance.DrawLine(x1, y1, x2, y2);

                if (Region::ClipLine(node->m_cBounds, &x1, &y1, &x2, &y2)) {
                    //GfxDriver::Instance.DrawLine(20, y, 400, y);
                    DrawLine(x1, y1, x2, y2);
                }
                    
/*                    if (Region::ClipLine(node->m_cBounds, &x1, &y1, &x2, &y2)) {
                    GfxDriver::Instance.DrawHLine(x1, y1, x2 - x1);
//                    GfxDriver::Instance.DrawLine(x1, y1, x2, y2);
                }*/
            }
#else
            std::deque<ILineSegment> clippedLine;
    
            region2.Clip(ILineSegment(IPoint(20.0f, y),IPoint(400.0f, y)), clippedLine);

            for (auto l : clippedLine)
            {
        //        auto l = clippable_line;
                float x1 = l.p1.x;
                float y1 = l.p1.y;
                float x2 = l.p2.x;
                float y2 = l.p2.y;
        
                GfxDriver::Instance.DrawHLine(color, x1, y1, x2 - x1);
            }
#endif
        }
        SetFgColor(Color(rand()));
        FillCircle(Point(460 + rand() % 340, 50 + rand() % 380), 5+rand() % 45);
    }        
    bigtime_t curTime = get_system_time();
    g_FrameTime = curTime - startTime;
    startTime = curTime;
    
}
#endif
