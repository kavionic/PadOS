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
#include <vector>

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
class ServerApplication;

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

    void ViewDetached(PServerView* view);
    void ServerApplicationDestructed(ServerApplication* application);
    void InvalidatePointerRoutes();

    void PushMouseCursor(ServerApplication* owner, PMouseCursorToken token, PMouseCursorID cursorID);
    void PopMouseCursor(ServerApplication* owner, PMouseCursorToken token);
    void RemoveMouseCursorStackEntries(ServerApplication* owner);

    void SetPointerCapture(PPointerID pointerID, ServerApplication* application, Ptr<PServerView> rootView, PPointerCaptureID captureID, PPointerCaptureRequestID requestID, PPointerCaptureMode mode);
    void ReleasePointerCapture(PPointerID pointerID, ServerApplication* application, PPointerCaptureID captureID);

    void            SetKeyboardFocus(Ptr<PServerView> view, bool focus);
    Ptr<PServerView> GetKeyboardFocus() const;
    void            UpdateViewFocusMode(PServerView* view);

    void            PowerLost(bool hasPower);
private:
    static constexpr size_t INPUT_EVENT_BUFFER_SIZE = 8192;
    static constexpr PMouseButton TOUCH_POINTER_BUTTON = PMouseButton::Left;

    struct InputDevice
    {
        PString Name;
        int     FileDescriptor = -1;
    };

    struct MouseCursorStackEntry
    {
        ServerApplication* Owner = nullptr;
        PMouseCursorToken  Token = PInvalidMouseCursorToken;
        PMouseCursorID     CursorID = ToMouseCursorID(PStandardMouseCursor::Pointer);
    };

    struct PointerCaptureState
    {
        ServerApplication*   Application = nullptr;
        PServerView*          RootView = nullptr;
        PPointerCaptureID    CaptureID = PInvalidPointerCaptureID;
        PPointerCaptureMode  Mode = PPointerCaptureMode::Preemptible;
    };

    struct PointerGestureState
    {
        PEventTimer  LongPressTimer;
        PPoint       StartPosition;
        TimeValNanos StartTime;
        PMouseButton Button = PMouseButton::None;
        bool         TapEligible = false;
        bool         LongPressEligible = false;
    };

    struct PointerRouteState
    {
        PPointerEvent       LastEvent;                       // Last event routed for this pointer.
        PServerView*        DeliveredRootView = nullptr;     // Root whose client currently considers the pointer inside.
        PointerCaptureState Capture;                         // Optional capture override for normal and boundary events.
        PointerGestureState Gesture;
    };

    void OpenInputDevices();
    void ReadInputEvents();
    void ReadInputEvents(int inputDevice, const PString& deviceName);
    void QueuePointerEvent(const PPointerEvent& event);
    void QueueMouseEvent(const PMouseEvent& event);
    void QueueTouchEvent(const PTouchEvent& event);
    PPoint UpdateMousePosition(const PMouseEvent& event);
    PPoint ClampMousePosition(const PPoint& position) const;
    void BeginPointerGesture(PointerRouteState& pointerState, const PPointerEvent& event);
    void UpdatePointerGesture(PointerRouteState& pointerState, const PPointerEvent& event);
    bool EndPointerGesture(PointerRouteState& pointerState, const PPointerEvent& event);
    void CancelPointerGesture(PointerRouteState& pointerState);
    void SlotLongPressTimer(PEventTimer* timer);

    static PMouseButton GetTouchPointerButton(const PTouchEvent& touchEvent);
    static PPointerButtonMask GetTouchPointerButtons(const PTouchEvent& touchEvent);
    static PPointerEventType GetPointerEventType(PInputEventID eventID);
    static PPointerEvent CreatePointerEvent(const PMouseEvent& mouseEvent, const PPoint& position);
    static PPointerEvent CreatePointerEvent(const PTouchEvent& touchEvent);

    bool ApplyMouseCursor(PMouseCursorID cursorID);
    void UpdateMouseCursor();

    void HandlePointerEvent(const PPointerEvent& event);
    void RoutePointerEvent(PointerRouteState& pointerState, const PPointerEvent& event);
    void HandleHoverPointerEvent(PointerRouteState& pointerState, const PPointerEvent& event);
    void HandleNonHoverPointerEvent(PointerRouteState& pointerState, const PPointerEvent& event);
    PointerRouteState& GetPointerRouteState(PPointerID pointerID);
    void NotifyPointerExitedRootView(PointerRouteState& pointerState, const PPointerEvent& event);
    void DeliverPointerEvent(PointerRouteState& pointerState, Ptr<PServerView> rootView, const PPointerEvent& event, PPointerCaptureID captureID);
    void ReevaluatePointerRoute(PointerRouteState& pointerState);
    void RefreshPointerRoutes();
    void BeginImplicitPointerCapture(PointerRouteState& pointerState, Ptr<PServerView> rootView);
    void GrantPointerCapture(PPointerID pointerID, PointerRouteState& pointerState, ServerApplication* application, Ptr<PServerView> rootView, PPointerCaptureRequestID requestID, PPointerCaptureMode mode);
    void RejectPointerCaptureRequest(PPointerID pointerID, PointerRouteState& pointerState, ServerApplication* application, Ptr<PServerView> rootView, PPointerCaptureRequestID requestID);
    PPointerCaptureID AllocatePointerCaptureID();

    void SlotRegisterApplication(port_id replyPort, port_id clientPort, const PString& name);

    static Ptr<PDisplayDriver>   s_DisplayDriver;
    static Ptr<PSrvBitmap>       s_ScreenBitmap;

    PMessagePort m_ReplyPort;
    PEventTimer m_PollTouchDriverTimer;

    std::queue<PPointerEvent> m_PointerEventQueue;

    alignas(PInputEvent) std::array<uint8_t, INPUT_EVENT_BUFFER_SIZE> m_InputEventBuffer;

    ASRegisterApplication::Receiver RSRegisterApplication;
    
    Ptr<PServerView> m_TopView;

    std::map<PPointerID, PointerRouteState> m_PointerRouteMap;
    PPointerCaptureID m_NextPointerCaptureID = PFirstPointerCaptureID;
    bool m_PointerRoutesInvalid = false;
    PServerView*                 m_KeyboardFocusView = nullptr;

    std::vector<MouseCursorStackEntry> m_MouseCursorStack;
    PMouseCursorID                     m_CurrentMouseCursorID = PInvalidMouseCursorID;

    PPoint                   m_MousePosition;
    std::vector<InputDevice> m_InputDevices;

    ApplicationServer(const ApplicationServer&) = delete;
    ApplicationServer& operator=(const ApplicationServer&) = delete;
};
