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
#include "System/GUI/Button.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PaintView::PaintView() : View("PaintView")
{
    Rect bounds = GetBounds();
    
    Ptr<Button> clearButton = ptr_new<Button>("ClearButton", "Clear");
    clearButton->SignalActivated.Connect(this, &PaintView::SlotClearButton);
    Point size = clearButton->GetPreferredSize(false);
    clearButton->SetFrame(Rect(0.0f, 0.0f, size.x, size.y));
    AddChild(clearButton);
    Ptr<Button> nextButton = ptr_new<Button>("NextButton", "Next");
    size = nextButton->GetPreferredSize(false);
    nextButton->SignalActivated.Connect(this, &PaintView::SlotNextButton);
    nextButton->SetFrame(Rect(799.0f - size.x, 0.0f, 799.0f, size.y));
    AddChild(nextButton);
    

    Rect frame(0.0f, 0.0f, 80.0f, 40.0f);
    frame += Point(50.0f, 60.0f);
    float x = 50.0f;
    for (int i = 0; i < 7; ++i)
    {
        Ptr<Button> button = ptr_new<Button>("TstButton", String::FormatString("Test%d", i + 1));
        size = button->GetPreferredSize(false);
        //button->SignalActivated.Connect(this, &PaintView::SlotClearButton);
        frame.left = x;
        frame.right = x + size.x;
        frame.bottom = frame.top + size.y;
        x += size.x + 10.0f;
        button->SetFrame(frame);
        AddChild(button);
        frame += Point(100.0f, 0.0f);        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PaintView::~PaintView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PaintView::AllAttachedToScreen()
{
    SetFgColor(255, 255, 255);
    FillRect(GetBounds());
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PaintView::OnMouseDown( MouseButton_e button, const Point& position )
{
//    printf("PaintView::OnMouseDown(%d, %.2f, %.2f)\n", int(button), position.x, position.y);
    m_WasHit = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PaintView::OnMouseUp( MouseButton_e button, const Point& position )
{
//    printf("PaintView::OnMouseUp(%d, %.2f, %.2f)\n", int(button), position.x, position.y);
    if (m_WasHit)
    {
        m_WasHit = false;
        return true;
    }
    return false;        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PaintView::OnMouseMove(MouseButton_e button, const Point& position)
{
//    printf("PaintView::OnMouseMove(%d, %.2f, %.2f)\n", int(button), position.x, position.y);
    if (m_WasHit)
    {
        SetFgColor(0,255,0);
        FillCircle(position, 4.0f);
        SetFgColor(0,255,255);
        FillCircle(position, 2.0f);
        Flush();
        return true;
    }
    return false;        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PaintView::SlotClearButton()
{
    FillRect(GetBounds(), Color(0xffffffff));
    Flush();
//    Invalidate(true);
//    UpdateIfNeeded(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PaintView::SlotNextButton()
{
    SignalDone(ptr_tmp_cast(this));
}
