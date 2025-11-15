// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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

#include <queue>

#include "Threads/Looper.h"
#include "Signals/SignalTarget.h"
#include "ApplicationServer/Protocol.h"
#include "Math/Rect.h"
#include "GUI/GUIEvent.h"
#include "Threads/EventTimer.h"
#include "ServerBitmap.h"

namespace os
{

PDEFINE_LOG_CATEGORY(LogCategoryAppServer, "APPSERV", PLogSeverity::INFO_HIGH_VOL);

class ServerView;
class DisplayDriver;

class ApplicationServer : public Looper, public SignalTarget
{
public:
    ApplicationServer(Ptr<DisplayDriver> displayDriver);
    ~ApplicationServer();

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;
    virtual void Idle() override;

    static Rect             GetScreenFrame();
    static IRect            GetScreenIFrame();
    static SrvBitmap*       GetScreenBitmap() { return ptr_raw_pointer_cast(s_ScreenBitmap); }
    static DisplayDriver*   GetDisplayDriver();

    Ptr<ServerView> GetTopView();
    
    bool            RegisterView(Ptr<ServerView> view);
    Ptr<ServerView> FindView(handler_id handle) const;

    void ViewDestructed(ServerView* view);

    void            SetFocusView(MouseButton_e button, Ptr<ServerView> view, bool focus);
    Ptr<ServerView> GetFocusView(MouseButton_e button) const;
    void            SetMouseDownView(MouseButton_e button, Ptr<ServerView> view);
    Ptr<ServerView> GetMouseDownView(MouseButton_e button) const;

    void            SetKeyboardFocus(Ptr<ServerView> view, bool focus);
    Ptr<ServerView> GetKeyboardFocus() const;
    void            UpdateViewFocusMode(ServerView* view);

    void            PowerLost(bool hasPower);
private:
    void HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event);
    void HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event);
    void HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event);

    void SlotRegisterApplication(port_id replyPort, port_id clientPort, const String& name);

    static Ptr<DisplayDriver>   s_DisplayDriver;
    static Ptr<SrvBitmap>       s_ScreenBitmap;

    MessagePort m_ReplyPort;
    EventTimer m_PollTouchDriverTimer;

    std::queue<MotionEvent> m_MouseEventQueue;

    ASRegisterApplication::Receiver RSRegisterApplication;
    
    Ptr<ServerView> m_TopView;

    std::map<int, ServerView*>  m_MouseViewMap;    // Maps pointing device or touch point to view last hit.
    std::map<int, ServerView*>  m_MouseFocusMap;   // Map of focused view per pointing device or touch point.
    ServerView*                 m_KeyboardFocusView = nullptr;

    int        m_TouchInputDevice = -1;

    ApplicationServer(const ApplicationServer&) = delete;
    ApplicationServer& operator=(const ApplicationServer&) = delete;
};


}
