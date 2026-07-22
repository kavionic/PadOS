// This file is part of PadOS.
//
// Copyright (C) 2017-2025 Kurt Skauen <http://kavionic.com/>
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

#include <System/Platform.h>

#include <string.h>

#include <App/Application.h>
#include <GUI/View.h>
#include <GUI/Bitmap.h>
#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>


PApplication* PApplication::s_DefaultApplication = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication* PApplication::GetDefaultApplication()
{
    return s_DefaultApplication;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetDefaultApplication(PApplication* application)
{
    s_DefaultApplication = application;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication::PApplication(const PString& name) : PLooper(name, 1000), m_ReplyPort("app_reply", 1000)
{
    RegisterRemoteSignal(&m_RSPaintView, &PApplication::HandlePaint);
    RegisterRemoteSignal(&m_RSViewFrameChanged, &PApplication::HandleViewFrameChanged);
    RegisterRemoteSignal(&m_RSViewFocusChanged, &PApplication::HandleViewFocusChanged);
    RegisterRemoteSignal(&m_RSHandlePointerEvent, &PApplication::HandlePointerEvent);
    RegisterRemoteSignal(&m_RSHandlePointerCaptureLost, &PApplication::HandlePointerCaptureLost);

    p_post_to_remotesignal<ASRegisterApplication>(p_get_appserver_port(), INVALID_HANDLE, TimeValNanos::infinit, m_ReplyPort.GetHandle(), GetPortID(), GetName());

    m_LongPressTimer.Set(LONG_PRESS_DELAY, true);
    m_LongPressTimer.SignalTrigged.Connect(this, &PApplication::SlotLongPressTimer);

    for(;;)
    {
        MsgRegisterApplicationReply reply;
        int32_t                     code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == PAppserverProtocol::REGISTER_APPLICATION_REPLY)
            {
                m_ServerHandle = reply.m_ServerHandle;
                break;
            }
            else
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::Application() received invalid reply: {}", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::Application() receive failed: {}", strerror(get_last_error()));
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication::~PApplication()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::Idle()
{
    LayoutViews();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIRect PApplication::GetScreenIFrame()
{
    return ApplicationServer::GetScreenIFrame();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::AddView(Ptr<PView> view, PViewDockType dockType, size_t index)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (dockType == PViewDockType::ChildView) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::AddView() attempt to add top-level view as 'ViewDockType::ChildView'");
        return false;
    }
    view->HandlePreAttachToScreen(this);

    Ptr<PView>   parent       = view->GetParent();
    handler_id  parentHandle = view->GetParentServerHandle();

    CreateServerView(view, parentHandle, dockType, index);

    for (Ptr<PView> child : view->m_ChildrenList) {
        AddChildView(view, ptr_static_cast<PView>(child));
    }
    view->HandleAttachedToScreen(this);
    LayoutViews();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::AddChildView(Ptr<PView> parent, Ptr<PView> view, size_t index)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    handler_id parentHandle = view->GetParentServerHandle();

    if (view->HasFlags(PViewFlags::WillDraw)) {
        CreateServerView(view, parentHandle, PViewDockType::ChildView, index);
    }
    for (Ptr<PView> child : view->m_ChildrenList) {
        AddChildView(view, ptr_static_cast<PView>(child));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::RemoveView(Ptr<PView> view)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (view->m_ServerHandle != INVALID_HANDLE)
    {
        handle_id serverHandle = view->m_ServerHandle;
        DetachView(view);
        Post<ASDeleteView>(serverHandle);
    }
    view->HandleDetachedFromScreen();
    if (view->GetLooper() != nullptr) {
        RemoveHandler(view);
    }
    for (Ptr<PView> child : *view) {
        RemoveView(child);
    }
    view->ClearFlags(PViewFlags::IsAttachedToScreen);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::FindView(handler_id handle)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    return ptr_static_cast<PView>(FindHandler(handle));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetKeyboardFocus(Ptr<PView> view, bool focus, bool notifyServer)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (!focus && view != m_KeyboardFocusView) {
        return;
    }

    PView* newFocus = (focus) ? ptr_raw_pointer_cast(view) : nullptr;
    PView* oldFocus = m_KeyboardFocusView;
    if (newFocus != oldFocus)
    {
        m_KeyboardFocusView = newFocus;

        if (oldFocus != nullptr) {
            oldFocus->OnKeyboardFocusChanged(false);
        }
        if (newFocus != nullptr) {
            newFocus->OnKeyboardFocusChanged(true);
        }

        if (notifyServer) {
            Post<ASSetKeyboardFocus>(view->m_ServerHandle, focus);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::GetKeyboardFocus() const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    return ptr_tmp_cast(m_KeyboardFocusView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::CreateBitmap(int width, int height, PEColorSpace colorSpace, uint32_t flags, handle_id& outHandle, uint8_t*& inOutFramebuffer, size_t& inOutBytesPerRow)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASCreateBitmap>(m_ReplyPort.GetHandle(), width, height, colorSpace, (flags & PBitmap::CUSTOM_FRAMEBUFFER) ? inOutFramebuffer : nullptr, (flags & PBitmap::CUSTOM_FRAMEBUFFER) ? inOutBytesPerRow : 0, flags);
    Flush();

    for (;;)
    {
        MsgCreateBitmapReply reply;
        int32_t              code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == PAppserverProtocol::CREATE_BITMAP_REPLY)
            {
                if (reply.m_BitmapHandle == INVALID_HANDLE) {
                    return false;
                }
                outHandle         = reply.m_BitmapHandle;
                inOutFramebuffer  = reply.m_Framebuffer;
                inOutBytesPerRow  = reply.m_BytesPerRow;
                return true;
            }
            else
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::CreateBitmap() received invalid reply: {}", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::CreateBitmap() receive failed: {}", strerror(get_last_error()));
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::DeleteBitmap(handle_id bitmapHandle)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASDeleteBitmap>(bitmapHandle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMouseCursorToken PApplication::PushMouseCursor(PStandardMouseCursor cursor)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    const PMouseCursorToken token = m_NextMouseCursorToken++;
    if (m_NextMouseCursorToken == PInvalidMouseCursorToken) {
        m_NextMouseCursorToken = PInvalidMouseCursorToken + 1;
    }
    Post<ASPushMouseCursor>(token, ToMouseCursorID(cursor));
    Flush();
    return token;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::PopMouseCursor(PMouseCursorToken token)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (token != PInvalidMouseCursorToken)
    {
        Post<ASPopMouseCursor>(token);
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::Flush()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (m_UsedSendBufferSize > 0) {
        message_port_send(p_get_appserver_port(), m_ServerHandle, PAppserverProtocol::MESSAGE_BUNDLE, m_SendBuffer, m_UsedSendBufferSize);
        m_UsedSendBufferSize = 0;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::Sync()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASSync>(m_ReplyPort.GetHandle());
    Flush();
    int32_t code;
    for (;;)
    {
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, nullptr, 0) < 0 && get_last_error() != EINTR) break;
        if (code == PAppserverProtocol::SYNC_REPLY) {
            break;
        } else {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::Sync() received invalid reply: {}", code);
        }
        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::DetachView(Ptr<PView> view)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    for (auto i = m_PointerCaptureMap.begin(); i != m_PointerCaptureMap.end(); )
    {
        if (view == i->second.View)
        {
            const PPointerID pointerID = i->first;
            i = m_PointerCaptureMap.erase(i);
            view->OnPointerCaptureLost(pointerID, PPointerCaptureLostReason::ViewDetached);
        } else {
            ++i;
        }
    }
    for (auto i = m_LongPressViewMap.begin(); i != m_LongPressViewMap.end(); )
    {
        if (view == i->second) {
            i = m_LongPressViewMap.erase(i);
        } else {
            ++i;
        }
    }
    if (view == m_KeyboardFocusView) {
        m_KeyboardFocusView = nullptr;
        view->OnKeyboardFocusChanged(false);
    }
    view->SetServerHandle(INVALID_HANDLE);
    for (Ptr<PView> child : *view)
    {
        DetachView(child);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PApplication::AllocMessageBuffer(int32_t messageID, size_t size)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    assert(size > 0);

    size += sizeof(AppserverMessage);
    if (size > sizeof(m_SendBuffer)) return nullptr;
    
    if (m_UsedSendBufferSize + size > sizeof(m_SendBuffer)) {
        Flush();
    }
    AppserverMessage* buffer = reinterpret_cast<AppserverMessage*>(m_SendBuffer + m_UsedSendBufferSize);
    m_UsedSendBufferSize += size;
    buffer->m_Code = messageID;
    buffer->m_Length = size;
    return buffer + 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::CreateServerView(Ptr<PView> view, handler_id parentHandle, PViewDockType dockType, size_t index)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASCreateView>(m_ReplyPort.GetHandle()
        , view->GetHandle()
        , parentHandle
        , dockType
        , index
        , view->GetName()
        , view->m_Frame + view->m_PositionOffset
        , view->m_ScrollOffset
        , view->m_Flags
        , view->m_HitMode
        , view->m_HideCount
        , view->m_FocusKeyboardMode
        , view->m_DrawingMode
        , view->m_PenWidth
        , view->m_CapStyle
        , view->m_JointStyle
        , view->m_MiterLimit
        , view->m_DashPattern
        , view->m_DashOffset
        , (view->m_Font != nullptr) ? view->m_Font->Get() : PFontID::e_FontLarge
        , view->m_EraseColor
        , view->m_BgColor
        , view->m_FgColor
        );
    Flush();

    for (;;)
    {
        MsgCreateViewReply reply;
        int32_t            code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == PAppserverProtocol::CREATE_VIEW_REPLY)
            {
                view->SetServerHandle(reply.m_ViewHandle);
                break;
            }
            else
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::AddView() received invalid reply: {}", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::AddView() receive failed: {}", strerror(get_last_error()));
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::RegisterViewForLayout(Ptr<PView> view, bool recursive)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (!view->m_IsLayoutValid && !view->m_IsLayoutPending)
    {
        view->m_IsLayoutPending = true;
        m_ViewsNeedingLayout.insert(view);
        WakeupLooper();
    }
    if (recursive)
    {
        for (Ptr<PView> child : view->GetChildList())
        {
            RegisterViewForLayout(child, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetLongPressView(PPointerID pointerID, Ptr<PView> view, const PPointerEvent& pointerEvent)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (view != nullptr)
    {
        SetLastPointerEvent(pointerEvent);
        m_LongPressViewMap[pointerID] = ptr_raw_pointer_cast(view);
        m_LastClickEvent = pointerEvent;
        m_LongPressTimer.Start(true);
    }
    else
    {
        auto iterator = m_LongPressViewMap.find(pointerID);
        if (iterator != m_LongPressViewMap.end()) {
            m_LongPressViewMap.erase(iterator);
            if (pointerID == m_LastClickEvent.PointerID) {
                m_LongPressTimer.Stop();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::GetLongPressView(PPointerID pointerID) const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_LongPressViewMap.find(pointerID);
    if (iterator != m_LongPressViewMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::SetPointerCapture(PPointerID pointerID, Ptr<PView> view, PPointerCaptureMode mode)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (view == nullptr) {
        return false;
    }
    auto previousIterator = m_PointerCaptureMap.find(pointerID);
    if (previousIterator != m_PointerCaptureMap.end() && view != previousIterator->second.View && previousIterator->second.Mode == PPointerCaptureMode::Locked) {
        return false;
    }

    Ptr<PView> root = view->GetRoot();
    if (root == nullptr || root->m_ServerHandle == INVALID_HANDLE) {
        return false;
    }

    Post<ASSetPointerCapture>(m_ReplyPort.GetHandle(), root->m_ServerHandle, pointerID, mode);
    Flush();

    MsgSetPointerCaptureReply reply;
    int32_t code;
    for (;;)
    {
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == PAppserverProtocol::SET_POINTER_CAPTURE_REPLY)
            {
                if (reply.m_CaptureID == PInvalidPointerCaptureID) {
                    return false;
                }
                break;
            }
            else
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::SetPointerCapture() received invalid reply: {}", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::SetPointerCapture() receive failed: {}", strerror(get_last_error()));
            return false;
        }
    }

    Ptr<PView> previousCaptureView = GetPointerCaptureView(pointerID);

    m_PointerCaptureMap[pointerID] = PointerCaptureState
    {
        .View = ptr_raw_pointer_cast(view),
        .CaptureID = reply.m_CaptureID,
        .Mode = mode
    };

    if (previousCaptureView != nullptr && previousCaptureView != view)
    {
        PPointerEvent pointerEvent = GetLastPointerEvent(pointerID);
        pointerEvent.EventType = PPointerEventType::Cancel;
        previousCaptureView->DispatchPointerEventPhase(pointerEvent, PEventPhase::Target);
        previousCaptureView->OnPointerCaptureLost(pointerID, PPointerCaptureLostReason::Stolen);

        Ptr<PView> longPressView = GetLongPressView(pointerID);
        if (longPressView == previousCaptureView) {
            SetLongPressView(pointerID, nullptr, pointerEvent);
        }
    }

    if (previousCaptureView != view)
    {
        view->OnPointerCaptureGained(pointerID);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::ReleasePointerCapture(PPointerID pointerID, Ptr<PView> view, PPointerCaptureLostReason reason)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator == m_PointerCaptureMap.end()) {
        return;
    }
    if (view != nullptr && view != iterator->second.View) {
        return;
    }

    Ptr<PView> captureView = ptr_tmp_cast(iterator->second.View);
    if (captureView != nullptr)
    {
        Ptr<PView> root = captureView->GetRoot();
        if (root != nullptr && root->m_ServerHandle != INVALID_HANDLE) {
            Post<ASReleasePointerCapture>(root->m_ServerHandle, pointerID, iterator->second.CaptureID, reason);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandlePaint(handler_id viewHandle, const PRect& updateRect)
{
    Ptr<PView> view = FindView(viewHandle);
    if (view != nullptr) {
        view->HandlePaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandleViewFrameChanged(handler_id viewHandle, const PRect& frame)
{
    Ptr<PView> view = FindView(viewHandle);
    if (view != nullptr) {
        view->HandleFrameChanged(frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandleViewFocusChanged(handler_id viewHandle, bool hasFocus)
{
    Ptr<PView> view = FindView(viewHandle);
    if (view != nullptr) {
        SetKeyboardFocus(view, hasFocus, false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandlePointerEvent(handler_id viewHandle, const PPointerEvent& pointerEvent)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Ptr<PView> view = FindView(viewHandle);
    if (view != nullptr) {
        view->HandlePointerEvent(pointerEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandlePointerCaptureLost(PPointerID pointerID, PPointerCaptureID captureID, PPointerCaptureLostReason reason)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator == m_PointerCaptureMap.end()) {
        return;
    }
    if (captureID != iterator->second.CaptureID) {
        return;
    }

    Ptr<PView> captureView = ptr_tmp_cast(iterator->second.View);
    m_PointerCaptureMap.erase(iterator);

    Ptr<PView> longPressView = GetLongPressView(pointerID);
    if (longPressView == captureView) {
        SetLongPressView(pointerID, nullptr, GetLastPointerEvent(pointerID));
    }

    if (captureView != nullptr) {
        captureView->OnPointerCaptureLost(pointerID, reason);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::GetPointerCaptureView(PPointerID pointerID) const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator != m_PointerCaptureMap.end()) {
        return ptr_tmp_cast(iterator->second.View);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetLastPointerEvent(const PPointerEvent& pointerEvent)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (pointerEvent.PointerID != PInvalidPointerID) {
        m_LastPointerEventMap[pointerEvent.PointerID] = pointerEvent;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::ClearLastPointerEvent(PPointerID pointerID)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    m_LastPointerEventMap.erase(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerEvent PApplication::GetLastPointerEvent(PPointerID pointerID) const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_LastPointerEventMap.find(pointerID);
    if (iterator != m_LastPointerEventMap.end()) {
        return iterator->second;
    }
    PPointerEvent pointerEvent;
    pointerEvent.PointerID = pointerID;
    return pointerEvent;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::LayoutViews()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    std::set<Ptr<PView>> updatedViews;
    for (int i = 0; i < 100 && !m_ViewsNeedingLayout.empty(); ++i)
    {
        std::set<Ptr<PView>> list = std::move(m_ViewsNeedingLayout);
        for (Ptr<PView> view : list)
        {
            view->m_IsLayoutPending = false;
            view->RefreshLayout();
        }
        updatedViews.merge(list);
    }
    for (Ptr<PView> view : updatedViews) {
        view->OnLayoutUpdated();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SlotLongPressTimer()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Ptr<PView> lastPressedView = GetLongPressView(m_LastClickEvent.PointerID);
    if (lastPressedView != nullptr) {
        lastPressedView->DispatchLongPress(m_LastClickEvent.PointerID, m_LastClickEvent);
    }
}
