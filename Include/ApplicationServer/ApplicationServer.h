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

#include <array>
#include <map>
#include <queue>
#include <stdint.h>

#include "Threads/Looper.h"
#include "Signals/SignalTarget.h"
#include "ApplicationServer/Protocol.h"
#include "Math/Rect.h"
#include "GUI/GUIEvent.h"
#include "Threads/EventTimer.h"
#include "ServerBitmap.h"


PDEFINE_LOG_CATEGORY(LogCategoryAppServer, "APPSERV", PLogSeverity::INFO_HIGH_VOL);

class PServerView;
class PDisplayDriver;

class ApplicationServer : public PLooper, public SignalTarget
{
public:
    ApplicationServer(Ptr<PDisplayDriver> displayDriver);
    ~ApplicationServer();

    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) override;
    virtual void Idle() override;

    static PRect             GetScreenFrame();
    static PIRect            GetScreenIFrame();
    static PSrvBitmap*       GetScreenBitmap() { return ptr_raw_pointer_cast(s_ScreenBitmap); }
    static PDisplayDriver*   GetDisplayDriver();

    Ptr<PServerView> GetTopView();
    
    bool            RegisterView(Ptr<PServerView> view);
    Ptr<PServerView> FindView(handler_id handle) const;

    void ViewDestructed(PServerView* view);

    void            SetFocusView(PPointerID pointerID, Ptr<PServerView> view, bool focus);
    Ptr<PServerView> GetFocusView(PPointerID pointerID) const;
    void            SetPointerDownView(PPointerID pointerID, Ptr<PServerView> view);
    Ptr<PServerView> GetPointerDownView(PPointerID pointerID) const;

    void            SetKeyboardFocus(Ptr<PServerView> view, bool focus);
    Ptr<PServerView> GetKeyboardFocus() const;
    void            UpdateViewFocusMode(PServerView* view);

    void            PowerLost(bool hasPower);
private:
    static constexpr size_t INPUT_EVENT_BUFFER_SIZE = 8192;
    static constexpr PMouseButton TOUCH_POINTER_BUTTON = PMouseButton::Left;

    struct QueuedPointerEvent
    {
        PInputEventID EventID;
        PPointerEvent PointerEvent;
    };

    void ReadInputEvents();
    void ReadInputEvents(int inputDevice, PInputClass inputClass, const char* deviceName);
    void QueuePointerEvent(PInputEventID eventID, const PPointerEvent& event);
    void QueueMouseEvent(const PMouseEvent& event);
    void QueueTouchEvent(const PTouchEvent& event);
    PPoint UpdateMousePosition(const PMouseEvent& event);
    PPoint ClampMousePosition(const PPoint& position) const;

    static PMouseButton GetTouchPointerButton(const PTouchEvent& touchEvent);
    static PPointerButtonMask GetTouchPointerButtons(const PTouchEvent& touchEvent);
    static PPointerEvent CreatePointerEvent(const PMouseEvent& mouseEvent, const PPoint& position);
    static PPointerEvent CreatePointerEvent(const PTouchEvent& touchEvent);

    void HandlePointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event);
    void HandlePointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event);
    void HandlePointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event);

    void SlotRegisterApplication(port_id replyPort, port_id clientPort, const PString& name);

    static Ptr<PDisplayDriver>   s_DisplayDriver;
    static Ptr<PSrvBitmap>       s_ScreenBitmap;

    PMessagePort m_ReplyPort;
    PEventTimer m_PollTouchDriverTimer;

    std::queue<QueuedPointerEvent> m_PointerEventQueue;

    alignas(PInputEvent) std::array<uint8_t, INPUT_EVENT_BUFFER_SIZE> m_InputEventBuffer;

    ASRegisterApplication::Receiver RSRegisterApplication;
    
    Ptr<PServerView> m_TopView;

    std::map<PPointerID, PServerView*> m_PointerViewMap;    // Maps pointer ID to view last hit.
    std::map<PPointerID, PServerView*> m_PointerFocusMap;   // Map of focused view per pointer ID.
    PServerView*                 m_KeyboardFocusView = nullptr;

    PPoint     m_MousePosition;
    int        m_MouseInputDevice = -1;
    int        m_TouchInputDevice = -1;

    ApplicationServer(const ApplicationServer&) = delete;
    ApplicationServer& operator=(const ApplicationServer&) = delete;
};
