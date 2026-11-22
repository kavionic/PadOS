// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 17.03.2018 20:45:17

#pragma once

#include <array>
#include <map>
#include <queue>
#include <utility>
#include <stdint.h>
#include <vector>

#include <sys/types.h>

#include "Threads/Looper.h"
#include "Signals/SignalTarget.h"
#include "ApplicationServer/Protocol.h"
#include "ApplicationServer/KeyboardManager.h"
#include "Math/Rect.h"
#include "GUI/GUIEvent.h"
#include "GUI/Style/GUIStyleInfo.h"
#include "Threads/EventTimer.h"
#include "Utils/IntrusiveList.h"
#include "ServerBitmap.h"


PDEFINE_LOG_CATEGORY(LogCategoryAppServer, "APPSERV", PLogSeverity::INFO_HIGH_VOL);

class CachedSrvBitmap;
class PServerView;
class PDisplayDriver;
class ServerApplication;
class PFontManager;
class PServerFontInstance;

class ApplicationServer : public PLooper, public SignalTarget
{
public:
    ApplicationServer(Ptr<PDisplayDriver> displayDriver);
    ~ApplicationServer();

    virtual void Idle() override;
    // Reopen closed or I/O-faulted display sessions. Only a failed reopen propagates.
    void HandleDisplayDriverError(PErrorCode error);

    static PRect             GetScreenFrame();
    static PIRect            GetScreenIFrame();
    static PSrvBitmap*       GetScreenBitmap() { return ptr_raw_pointer_cast(s_ScreenBitmap); }
    static PDisplayDriver*   GetDisplayDriver();

    Ptr<PServerView> GetTopView();
    
    bool            RegisterView(Ptr<PServerView> view);
    Ptr<PServerView> FindView(handler_id handle) const;

    // The font catalog is loaded on first use.
    Ptr<PServerFontInstance> GetFont(const PFontDescription& description);

    // Cache identity is the opened file's device/inode.
    Ptr<PSrvBitmap> LoadBitmap(const PString& path);
    // Called by a cached bitmap's final-reference hook; returns true if deleted.
    bool BitmapReleased(CachedSrvBitmap* bitmap) noexcept;

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
    bool TrimBitmapCache(const CachedSrvBitmap* releasingBitmap = nullptr) noexcept;
    void RemoveCachedBitmap(CachedSrvBitmap* bitmap) noexcept;

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

    void WaitForEvents(TimeValNanos deadline) override;
    void OpenInputDevices();
    void ReadRegisteredInputDevices(int inputDevice, const PString& deviceName);
    void ReadInputEvents();
    void ReadInputEvents(int inputDevice, const PString& deviceName);
    void HandleInputDeviceEvent(const PInputEvent& event, const PString& deviceName);
    bool HasInputDevice(PInputClass classID) const;
    void UpdateVirtualKeyboard();
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
    PPointerEvent CreatePointerEvent(const PMouseEvent& mouseEvent, const PPoint& position) const;
    PPointerEvent CreatePointerEvent(const PTouchEvent& touchEvent) const;

    bool ApplyMouseCursor(PMouseCursorID cursorID);
    void UpdateMouseCursor();
    void UpdateMouseCursorVisibility();

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
    void SlotUnregisterApplication(handler_id applicationHandle);
    void SlotVirtualKeyboardEvent(const PKeyEvent& keyEvent, const PString& text);
    void SlotGetKeyboardLayouts(port_id replyPort, uint32_t requestID, uint32_t firstIndex);
    void SlotGetKeyboardConfiguration(port_id replyPort, uint32_t requestID);

    void SlotSetKeyboardConfiguration(port_id replyPort,
                                      uint32_t requestID,
                                      const PString& layout,
                                      const PString& variant,
                                      const PString& options);

    void SendKeyboardConfiguration(port_id replyPort, uint32_t requestID, PErrorCode error);

    void SlotGetGUIStyles(port_id replyPort, uint32_t requestID, size_t firstIndex);
    void SlotGetGUIStyleConfiguration(port_id replyPort, uint32_t requestID);
    void SlotSetGUIStyleConfiguration(port_id replyPort, uint32_t requestID, const PString& style, const PString& variant);
    void SendGUIStyleConfiguration(port_id replyPort, uint32_t requestID, PErrorCode error);
    void LoadGUIStyleConfiguration();
    void SaveGUIStyleConfiguration(const PString& style, const PString& variant);
    void BroadcastGUIStyleConfiguration();

    static Ptr<PDisplayDriver>   s_DisplayDriver;
    static Ptr<PSrvBitmap>       s_ScreenBitmap;

    std::map<std::pair<dev_t, ino_t>, CachedSrvBitmap*> m_BitmapCache;
    PIntrusiveList<CachedSrvBitmap> m_UnusedBitmaps;
    size_t m_BitmapCacheSizeLimit = 4 * 1024 * 1024;
    size_t m_BitmapCacheSize = 0;

    std::queue<PPointerEvent> m_PointerEventQueue;

    alignas(PInputEvent) std::array<uint8_t, INPUT_EVENT_BUFFER_SIZE> m_InputEventBuffer;

    ASRegisterApplication       m_RSRegisterApplication;
    ASUnregisterApplication     m_RSUnregisterApplication;
    ASVirtualKeyboardEvent      m_RSVirtualKeyboardEvent;
    ASGetKeyboardLayouts        m_RSGetKeyboardLayouts;
    ASGetKeyboardConfiguration  m_RSGetKeyboardConfiguration;
    ASSetKeyboardConfiguration  m_RSSetKeyboardConfiguration;
    ASGetGUIStyles m_RSGetGUIStyles;
    ASGetGUIStyleConfiguration m_RSGetGUIStyleConfiguration;
    ASSetGUIStyleConfiguration m_RSSetGUIStyleConfiguration;

    PString m_GUIStyleName = DEFAULT_GUI_STYLE_NAME;
    PString m_GUIStyleVariant = DEFAULT_GUI_STYLE_VARIANT;
    
    PKeyboardManager    m_KeyboardManager;
    Ptr<PFontManager>   m_FontManager;
   
    Ptr<PServerView>    m_TopView;

    std::map<PPointerID, PointerRouteState> m_PointerRouteMap;
    PPointerCaptureID                       m_NextPointerCaptureID = PFirstPointerCaptureID;
    bool                                    m_PointerRoutesInvalid = false;
    bool                                    m_IsMouseCursorRequestedVisible = false;
    bool                                    m_HasPower = true;
    bool                                    m_TrimmingBitmapCache = false;
    PServerView*                            m_KeyboardFocusView = nullptr;

    std::vector<MouseCursorStackEntry> m_MouseCursorStack;
    PMouseCursorID                     m_CurrentMouseCursorID = PInvalidMouseCursorID;

    PPoint                         m_MousePosition;
    std::vector<InputDevice>       m_InputDevices;
    std::vector<uint8_t>           m_InputReadyFlags;
    std::map<int32_t, PInputClass> m_RegisteredInputDevices;

    ApplicationServer(const ApplicationServer&) = delete;
    ApplicationServer& operator=(const ApplicationServer&) = delete;
};
