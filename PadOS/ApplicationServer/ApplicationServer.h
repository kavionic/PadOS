// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 17.03.2018 20:45:17

#pragma once

#include "System/Threads/Looper.h"
#include "System/Signals/SignalTarget.h"
#include "Protocol.h"
#include "System/Math/Rect.h"
#include "System/GUI/GUIEvent.h"
#include "System/Utils/EventTimer.h"
#include "ServerView.h"

namespace os
{


class ApplicationServer : public Looper, public SignalTarget
{
public:
    ApplicationServer();
    ~ApplicationServer();

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;

    static Rect  GetScreenFrame();
    static IRect GetScreenIFrame();

    Ptr<ServerView> GetTopView() { return m_TopView; }
    
    bool            RegisterView(Ptr<ServerView> view);
    Ptr<ServerView> FindView(handler_id handle) const;

    void            SetFocusView(Ptr<ServerView> view);
    Ptr<ServerView> GetFocusView() const;
    void            SetMouseDownView(Ptr<ServerView> view);
    
private:
    
    
    void HandleMouseDown(MouseButton_e button, const Point& position);
    void HandleMouseUp(MouseButton_e button, const Point& position);
    void HandleMouseMove(MouseButton_e button, const Point& position);

    void SlotRegisterApplication(port_id replyPort, port_id clientPort, const String& name);

    EventTimer m_PollTouchDriverTimer;

    ASRegisterApplication::Receiver RSRegisterApplication;
    
    Ptr<ServerView> m_TopView;
    WeakPtr<ServerView> m_MouseDownView;
    WeakPtr<ServerView> m_FocusView;

    int        m_TouchInputDevice = -1;

    ApplicationServer(const ApplicationServer&) = delete;
    ApplicationServer& operator=(const ApplicationServer&) = delete;
};


}
