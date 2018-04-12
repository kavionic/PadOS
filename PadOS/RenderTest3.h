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

#pragma once

#include "System/GUI/View.h"
#include "System/Utils/EventTimer.h"

using namespace os;

class RenderTest3 : public View
{
public:
    RenderTest3();
    ~RenderTest3();
    virtual void AllAttachedToScreen() override;

    virtual void Paint(const Rect& updateRect) override
    {
        SetFgColor(255, 255, 255);
        FillRect(GetBounds());
    }

    virtual bool OnMouseDown(MouseButton_e button, const Point& position) override { return true; }
    virtual bool OnMouseUp(MouseButton_e button, const Point& position) override;

    Signal<void, Ptr<View>> SignalDone;
        
private:
    void SlotFrameProcess();
    RenderTest3( const RenderTest3 &c );
    RenderTest3& operator=( const RenderTest3 &c );

    EventTimer m_UpdateTimer;

    int m_CircleCount = 0;
};
