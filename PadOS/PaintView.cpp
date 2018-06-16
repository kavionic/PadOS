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

#include "sam.h"

#include "PaintView.h"
#include "System/GUI/Button.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PaintView::PaintView() : View("Paint")
{
    Rect bounds = GetBounds();

    SetLayoutNode(ptr_new<VLayoutNode>());
    
    Ptr<View> topBar    = ptr_new<View>("TopBar", ptr_tmp_cast(this), ViewFlags::TRANSPARENT | ViewFlags::CLIENT_ONLY);
    Ptr<View> bottomBar = ptr_new<View>("BottomBar", ptr_tmp_cast(this), ViewFlags::TRANSPARENT | ViewFlags::CLIENT_ONLY);
    
    topBar->SetLayoutNode(ptr_new<HLayoutNode>());
    bottomBar->SetLayoutNode(ptr_new<HLayoutNode>());
 
    topBar->SetWidthOverride(PrefSizeType::Smallest, SizeOverride::Always, 0.0f);
    topBar->SetWidthOverride(PrefSizeType::Greatest, SizeOverride::Always, LAYOUT_MAX_SIZE);
    topBar->SetHeightOverride(PrefSizeType::Smallest, SizeOverride::Always, 0.0f);
    topBar->SetHeightOverride(PrefSizeType::Greatest, SizeOverride::Always, LAYOUT_MAX_SIZE);

    bottomBar->SetWidthOverride(PrefSizeType::Smallest, SizeOverride::Always, 0.0f);
    bottomBar->SetWidthOverride(PrefSizeType::Greatest, SizeOverride::Always, LAYOUT_MAX_SIZE);
    bottomBar->SetHeightOverride(PrefSizeType::Smallest, SizeOverride::Always, 0.0f);
    bottomBar->SetHeightOverride(PrefSizeType::Greatest, SizeOverride::Always, LAYOUT_MAX_SIZE);
        
    Ptr<Button> clearButton = ptr_new<Button>("ClearButton", "Clear", topBar);
    clearButton->SignalActivated.Connect(this, &PaintView::SlotClearButton);
    Ptr<Button> nextButton = ptr_new<Button>("NextButton", "Next", topBar);
    nextButton->SignalActivated.Connect(this, &PaintView::SlotNextButton);

    for (int i = 0; i < 6; ++i)
    {
        Ptr<Button> button = ptr_new<Button>("TstButton", String::format_string("Test%d", i + 1), bottomBar);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PaintView::~PaintView()
{
    printf("PaintView::~PaintView()\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PaintView::AllAttachedToScreen()
{
    SetFgColor(255, 255, 255);
    FillRect(GetBounds());
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
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PaintView::SlotNextButton()
{
    SignalDone(ptr_tmp_cast(this));
}
