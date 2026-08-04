// This file is part of PadOS.
//
// Copyright (C) 2017-2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.11.2017 23:22:03

#pragma once

#include <set>
#include <vector>

#include <Math/Rect.h>
#include <Threads/Looper.h>
#include <Threads/EventTimer.h>
#include <ApplicationServer/Protocol.h>

class PView;


class PApplication : public PLooper, public SignalTarget
{
public:
    PApplication(const PString& name);
    ~PApplication();

    static PApplication* GetDefaultApplication();
    static void SetDefaultApplication(PApplication* application);

    static PApplication* GetCurrentApplication() { return dynamic_cast<PApplication*>(GetCurrentThread()); }

    virtual void Idle() override;
    static PIRect    GetScreenIFrame();
    static PRect     GetScreenFrame() { return PRect(GetScreenIFrame()); }
    
    bool AddView(Ptr<PView> view, PViewDockType dockType, size_t index = INVALID_INDEX);
    bool AddChildView(Ptr<PView> parent, Ptr<PView> view, size_t index = INVALID_INDEX);
    bool RemoveView(Ptr<PView> view);
    
    Ptr<PView> FindView(handler_id handle);


    void SetKeyboardFocus(Ptr<PView> view, bool focus, bool notifyServer);
    Ptr<PView> GetKeyboardFocus() const;

    bool CreateBitmap(int width, int height, PEColorSpace colorSpace, uint32_t flags, handle_id& outHandle, uint8_t*& inOutFramebuffer, size_t& inOutBytesPerRow);
    void DeleteBitmap(handle_id bitmapHandle);

    PMouseCursorToken PushMouseCursor(PStandardMouseCursor cursor);
    void PopMouseCursor(PMouseCursorToken token);

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args) { SIGNAL::Sender::Emit(this, &PApplication::AllocMessageBuffer, SIGNAL::GetID(), args...); }
     
    void Flush();
    void Sync();

    uint32_t GetQualifiers() const { return 0; }
private:
    friend class PView;
 
    struct PointerCaptureState
    {
        PView*               View = nullptr;       // Local capture target, including a pending request.
        PView*               RootView = nullptr;   // Server-confirmed root. Null while a request is pending.
        PPointerCaptureID    CaptureID = PInvalidPointerCaptureID;
        PPointerCaptureRequestID RequestID = PInvalidPointerCaptureRequestID;
        PPointerCaptureMode  Mode = PPointerCaptureMode::Preemptible;
    };

    struct PointerState
    {
        PPointerEvent            LastEvent;                  // Last physical input for this pointer.
        Ptr<PView>               DeliveryRootView;           // Client root view selected by the application server.
        std::vector<Ptr<PView>>  EffectivePath;              // Capture override or hit-test target through its ancestors.
        PointerCaptureState      Capture;                    // Optional target override.
    };
    
    void DetachView(Ptr<PView> view, bool detachChildren);
    void RemoveViewFromPointerState(Ptr<PView> view);
    
    void* AllocMessageBuffer(int32_t messageID, size_t size);

    bool CreateServerView(Ptr<PView> view, handler_id parentHandle, PViewDockType dockType, size_t index);
    void RegisterViewForLayout(Ptr<PView> view, bool recursive = false);
    void InvalidatePointerPaths();

    PointerState& GetPointerState(PPointerID pointerID);
    void      EraseInactivePointerState(PPointerID pointerID);
    void      SetLocalPointerCapture(PPointerID pointerID, PointerState& pointerState, Ptr<PView> view, Ptr<PView> rootView, PPointerCaptureID captureID, PPointerCaptureRequestID requestID, PPointerCaptureMode mode);
    void      ClearLocalPointerCapture(PPointerID pointerID, PointerState& pointerState, PPointerCaptureLostReason reason);
    void      RefreshPointerPathAfterCaptureChange(PointerState& pointerState);
    void      BeginPointerCaptureRequest(PPointerID pointerID, PointerState& pointerState, Ptr<PView> view, Ptr<PView> rootView, PPointerCaptureMode mode);
    void      UpdateConfirmedPointerCapture(PPointerID pointerID, PointerState& pointerState, Ptr<PView> view, Ptr<PView> rootView, PPointerCaptureMode mode);
    bool      SetPointerCapture(PPointerID pointerID, Ptr<PView> view, PPointerCaptureMode mode);
    void      ReleasePointerCapture(PPointerID pointerID, Ptr<PView> view, PPointerCaptureLostReason reason);
    PPointerCaptureRequestID AllocatePointerCaptureRequestID();
    void      HandlePaint(handler_id viewHandle, const PRect& updateRect);
    void      HandleViewFrameChanged(handler_id viewHandle, const PRect& frame);
    void      HandleViewScreenPositionChanged(handler_id viewHandle, const PPoint& screenPosition);
    void      HandleViewFocusChanged(handler_id viewHandle, bool hasFocus);
    void      HandlePointerEvent(handler_id viewHandle, const PPointerEvent& pointerEvent, PPointerCaptureID captureID);
    void      HandlePointerCaptureRequestReply(PPointerID pointerID, PPointerCaptureRequestID requestID, handler_id rootViewHandle, PPointerCaptureID captureID, const PPointerEvent& pointerEvent);
    void      HandlePointerCaptureLost(PPointerID pointerID, PPointerCaptureID captureID, PPointerCaptureLostReason reason);
    void      HandlePointerRootViewUpdate(handler_id rootViewHandle, const PPointerEvent& pointerEvent, PPointerRootViewUpdateType updateType);
    Ptr<PView> GetPointerCaptureView(PPointerID pointerID) const;
    bool      HasConfirmedPointerCapture(PPointerID pointerID) const;
    Ptr<PView> UpdateEffectivePointerPath(Ptr<PView> deliveryRootView, const PPointerEvent& pointerEvent);
    void      ClearEffectivePointerPath(PPointerID pointerID, const PPointerEvent& pointerEvent);
    void      RefreshPointerPaths();

    void LayoutViews();

    static PApplication* s_DefaultApplication;
    PMessagePort m_ReplyPort;
    handler_id m_ServerHandle = -1;
    PMouseCursorToken m_NextMouseCursorToken = PInvalidMouseCursorToken + 1;
    PPointerCaptureRequestID m_NextPointerCaptureRequestID = PFirstPointerCaptureRequestID;

    uint8_t m_SendBuffer[PAPPSERVER_MSG_BUFFER_SIZE]; 
    int32_t m_UsedSendBufferSize = 0;

    std::map<PPointerID, PointerState> m_PointerStateMap;
    PView*                   m_KeyboardFocusView = nullptr;
    bool                     m_PointerPathsInvalid = false;

    ASPaintView              m_RSPaintView;
    ASViewFrameChanged       m_RSViewFrameChanged;
    ASViewScreenPositionChanged m_RSViewScreenPositionChanged;
    ASViewFocusChanged       m_RSViewFocusChanged;
    ASHandlePointerEvent     m_RSHandlePointerEvent;
    ASHandlePointerCaptureRequestReply m_RSHandlePointerCaptureRequestReply;
    ASHandlePointerCaptureLost m_RSHandlePointerCaptureLost;
    ASUpdatePointerRootView  m_RSUpdatePointerRootView;

    std::set<Ptr<PView>>     m_ViewsNeedingLayout;


    PApplication(const PApplication &) = delete;
    PApplication& operator=(const PApplication &) = delete;
};
