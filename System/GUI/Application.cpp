// This file is part of PadOS.
//
// Copyright (c) 2017-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 06.11.2017 23:22:03

#include <System/Platform.h>

#include <algorithm>
#include <optional>
#include <string.h>

#include <App/Application.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/View.h>
#include <GUI/Bitmap.h>
#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>
#include <System/ExceptionHandling.h>
#include <Utils/UTF8Utils.h>


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

PApplication::PApplication(const PString& name) : PLooper(name, 1000, PAPPSERVER_MSG_BUFFER_SIZE), m_ReplyPort("app_reply", 1000)
{
    m_GUIStyle = ptr_new<PDefaultGUIStyle>();

    RegisterRemoteSignal(&m_RSPaintView, &PApplication::HandlePaint);
    RegisterRemoteSignal(&m_RSViewFrameChanged, &PApplication::HandleViewFrameChanged);
    RegisterRemoteSignal(&m_RSViewScreenPositionChanged, &PApplication::HandleViewScreenPositionChanged);
    RegisterRemoteSignal(&m_RSViewFocusChanged, &PApplication::HandleViewFocusChanged);
    RegisterRemoteSignal(&m_RSHandleKeyboardEvent, &PApplication::HandleKeyboardEvent);
    RegisterRemoteSignal(&m_RSKeyboardLayoutsReply, &PApplication::HandleKeyboardLayouts);
    RegisterRemoteSignal(&m_RSKeyboardConfigurationReply, &PApplication::HandleKeyboardConfiguration);
    RegisterRemoteSignal(&m_RSHandlePointerEvent, &PApplication::HandlePointerEvent);
    RegisterRemoteSignal(&m_RSHandlePointerCaptureRequestReply, &PApplication::HandlePointerCaptureRequestReply);
    RegisterRemoteSignal(&m_RSHandlePointerCaptureLost, &PApplication::HandlePointerCaptureLost);
    RegisterRemoteSignal(&m_RSUpdatePointerRootView, &PApplication::HandlePointerRootViewUpdate);

    p_post_to_remotesignal<ASRegisterApplication>(p_get_appserver_port(), INVALID_HANDLE, TimeValNanos::infinit, m_ReplyPort.GetHandle(), GetPortID(), GetName());

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
    std::optional<MutexObjGuard> mutexGuard;
    if (IsRunning() && !GetMutex().IsLocked()) {
        mutexGuard.emplace(GetMutex());
    }
    // Release queued views while they can still use the application.
    while (!m_ViewsNeedingLayout.empty())
    {
        auto pendingView = m_ViewsNeedingLayout.extract(m_ViewsNeedingLayout.begin());
        pendingView.value()->m_IsLayoutPending = false;
    }
    Flush();
    if (m_ServerHandle != INVALID_HANDLE)
    {
        if (!p_post_to_remotesignal<ASUnregisterApplication>(p_get_appserver_port(), INVALID_HANDLE, TimeValNanos::infinit, m_ServerHandle))
        {
            p_system_log<PLogSeverity::ERROR>(
                LogCategoryGUITK,
                "Application::~Application() failed to unregister application: {}",
                strerror(get_last_error()));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::Idle()
{
    DispatchPendingPointerEvents();
    LayoutViews();
    RefreshPointerPaths();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    if (targetHandler != INVALID_HANDLE
        || (code != ASPaintView::GetID() && code != ASHandlePointerEvent::GetID()))
    {
        DispatchPendingPointerEvents();
    }
    return false;
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

    const PGUIStyle* previousStyle = view->GetStyle();
    InvalidatePointerPaths();

    const handle_id serverHandle = view->m_ServerHandle;
    DetachView(view, serverHandle != INVALID_HANDLE);
    if (serverHandle != INVALID_HANDLE && !view->HasFlags(PViewFlags::Eavesdropper)) {
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
    if (view->GetStyle() != previousStyle) {
        view->OnStyleChanged();
    }
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

bool PApplication::RequestKeyboardLayouts(KeyboardLayoutsCallback callback, uint32_t firstIndex)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    if (!callback)
    {
        set_last_error(EINVAL);
        return false;
    }
    const uint32_t requestID = AllocateKeyboardRequestID();
    m_KeyboardLayoutsCallbacks.emplace(requestID, std::move(callback));
    PScopeExit rollback([this, requestID]() { m_KeyboardLayoutsCallbacks.erase(requestID); });
    if (!p_post_to_remotesignal<ASGetKeyboardLayouts>(
        p_get_appserver_port(),
        INVALID_HANDLE,
        TimeValNanos::zero,
        GetPortID(),
        requestID,
        firstIndex)) {
        return false;
    }
    rollback.release();
    return true;
}

bool PApplication::RequestKeyboardConfiguration(KeyboardConfigurationCallback callback)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    if (!callback)
    {
        set_last_error(EINVAL);
        return false;
    }
    const uint32_t requestID = AllocateKeyboardRequestID();
    m_KeyboardConfigurationCallbacks.emplace(requestID, std::move(callback));
    PScopeExit rollback([this, requestID]() { m_KeyboardConfigurationCallbacks.erase(requestID); });
    if (!p_post_to_remotesignal<ASGetKeyboardConfiguration>(
        p_get_appserver_port(),
        INVALID_HANDLE,
        TimeValNanos::zero,
        GetPortID(),
        requestID)) {
        return false;
    }
    rollback.release();
    return true;
}

bool PApplication::SetKeyboardConfiguration(
    KeyboardConfigurationCallback callback,
    const PString& layout,
    const PString& variant,
    const PString& options)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    if (!callback)
    {
        set_last_error(EINVAL);
        return false;
    }
    const uint32_t requestID = AllocateKeyboardRequestID();
    if (ASSetKeyboardConfiguration::Sender::Serializer::GetSize(GetPortID(), requestID, layout, variant, options) > PAPPSERVER_MSG_BUFFER_SIZE)
    {
        set_last_error(EINVAL);
        return false;
    }
    m_KeyboardConfigurationCallbacks.emplace(requestID, std::move(callback));
    PScopeExit rollback([this, requestID]() { m_KeyboardConfigurationCallbacks.erase(requestID); });
    if (!p_post_to_remotesignal<ASSetKeyboardConfiguration>(
        p_get_appserver_port(),
        INVALID_HANDLE,
        TimeValNanos::zero,
        GetPortID(),
        requestID,
        layout,
        variant,
        options)) {
        return false;
    }
    rollback.release();
    return true;
}

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

handle_id PApplication::CreateFont(const PFontDescription& description, PFontHeight& outHeight)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (m_ServerHandle == INVALID_HANDLE) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    if (ASCreateFont::Sender::Serializer::GetSize(m_ReplyPort.GetHandle(), description) > PAPPSERVER_MSG_BUFFER_SIZE) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    if (!p_post_to_remotesignal<ASCreateFont>(
        p_get_appserver_port(),
        m_ServerHandle,
        TimeValNanos::infinit,
        m_ReplyPort.GetHandle(),
        description)) {
        PERROR_THROW_CODE(PErrorCode(get_last_error()));
    }
    MsgCreateFontReply reply;
    ReceiveReply(PAppserverProtocol::CREATE_FONT_REPLY, &reply, sizeof(reply));
    PERROR_ERRORCODE_THROW_ON_FAIL(reply.Error);
    outHeight = reply.Height;
    return reply.FontHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::DeleteFont(handle_id fontHandle)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASDeleteFont>(fontHandle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PFontTextMetrics PApplication::MeasureFont(handle_id fontHandle, std::string_view text, float width, bool includeLast)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (m_ServerHandle == INVALID_HANDLE) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    PFontTextMetrics result;
    uint32_t previousGlyph = 0;
    while (result.ByteCount < text.size())
    {
        const size_t remaining = text.size() - result.ByteCount;
        size_t chunkLength = std::min(remaining, FONT_MEASUREMENT_CHUNK_BYTES);
        if (chunkLength < remaining)
        {
            while (chunkLength != 0 && is_utf8_continuation_byte(uint8_t(text[result.ByteCount + chunkLength]))) {
                --chunkLength;
            }
            if (chunkLength == 0) {
                chunkLength = std::min(remaining, FONT_MEASUREMENT_CHUNK_BYTES);
            }
        }
        const PFontTextMetrics metrics = MeasureFontChunk(
            fontHandle,
            text.substr(result.ByteCount, chunkLength),
            width - result.Width,
            includeLast,
            previousGlyph);
        result.Advance += metrics.Advance;
        result.Width += metrics.Width;
        result.ByteCount += metrics.ByteCount;
        if (metrics.ByteCount < chunkLength || result.Width > width) {
            break;
        }
    }
    return result;
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

    if (m_UsedSendBufferSize > 0)
    {
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

void PApplication::DetachView(Ptr<PView> view, bool detachChildren)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    RemoveViewFromPointerState(view);
    if (view == m_KeyboardFocusView) {
        m_KeyboardFocusView = nullptr;
        view->OnKeyboardFocusChanged(false);
    }
    view->SetServerHandle(INVALID_HANDLE);
    if (detachChildren)
    {
        for (Ptr<PView> child : *view)
        {
            DetachView(child, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::RemoveViewFromPointerState(Ptr<PView> view)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    std::vector<PPointerID> pointerRoutesToClear;
    std::vector<PPointerID> capturesToRelease;
    for (auto& [pointerID, pointerState] : m_PointerStateMap)
    {
        if (pointerState.DeliveryRootView == view
            || std::find(pointerState.EffectivePath.begin(), pointerState.EffectivePath.end(), view)
                != pointerState.EffectivePath.end())
        {
            pointerRoutesToClear.push_back(pointerID);
        }
        if (pointerState.Capture.View == ptr_raw_pointer_cast(view)) {
            capturesToRelease.push_back(pointerID);
        }
    }

    for (PPointerID pointerID : pointerRoutesToClear)
    {
        auto iterator = m_PointerStateMap.find(pointerID);
        if (iterator != m_PointerStateMap.end()) {
            ClearEffectivePointerPath(pointerID, iterator->second.LastEvent);
        }
    }

    for (PPointerID pointerID : capturesToRelease)
    {
        auto iterator = m_PointerStateMap.find(pointerID);
        if (iterator != m_PointerStateMap.end()
            && iterator->second.Capture.View == ptr_raw_pointer_cast(view))
        {
            const PointerCaptureState captureState = iterator->second.Capture;
            ClearLocalPointerCapture(
                pointerID, iterator->second, PPointerCaptureLostReason::ViewDetached);
            if (captureState.CaptureID != PInvalidPointerCaptureID) {
                Post<ASReleasePointerCapture>(
                    pointerID,
                    captureState.CaptureID);
            }
            EraseInactivePointerState(pointerID);
        }
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

void PApplication::ReceiveReply(int32_t expectedCode, void* reply, size_t replySize)
{
    for (;;)
    {
        int32_t code = 0;
        const ssize_t length = m_ReplyPort.ReceiveMessage(nullptr, &code, reply, replySize);
        if (length >= 0)
        {
            if (code != expectedCode || size_t(length) != replySize) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
            return;
        }
        if (get_last_error() != EINTR) {
            PERROR_THROW_CODE(PErrorCode(get_last_error()));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PFontTextMetrics PApplication::MeasureFontChunk(handle_id fontHandle, std::string_view text, float width, bool includeLast, uint32_t& previousGlyph)
{
    const PString chunk(text.data(), text.size());
    if (!p_post_to_remotesignal<ASMeasureFont>(
        p_get_appserver_port(),
        m_ServerHandle,
        TimeValNanos::infinit,
        m_ReplyPort.GetHandle(),
        fontHandle,
        chunk,
        width,
        includeLast,
        previousGlyph)) {
        PERROR_THROW_CODE(PErrorCode(get_last_error()));
    }
    MsgMeasureFontReply reply;
    ReceiveReply(PAppserverProtocol::MEASURE_FONT_REPLY, &reply, sizeof(reply));
    PERROR_ERRORCODE_THROW_ON_FAIL(reply.Error);
    previousGlyph = reply.PreviousGlyph;
    return reply.Metrics;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::CreateServerView(Ptr<PView> view, handler_id parentHandle, PViewDockType dockType, size_t index)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (view->m_Font->GetApplication() != this) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Font belongs to another application");
    }
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
        , view->m_Font->GetHandle()
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
                view->HandleScreenPositionChanged(reply.m_ScreenPosition);
                InvalidatePointerPaths();
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

void PApplication::InvalidatePointerPaths()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    m_PointerPathsInvalid = true;
    WakeupLooper();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication::PointerState& PApplication::GetPointerState(PPointerID pointerID)
{
    return m_PointerStateMap[pointerID];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::EraseInactivePointerState(PPointerID pointerID)
{
    auto iterator = m_PointerStateMap.find(pointerID);
    if (iterator != m_PointerStateMap.end() && !iterator->second.LastEvent.SupportsHover
        && iterator->second.EffectivePath.empty() && iterator->second.Capture.View == nullptr) {
        m_PointerStateMap.erase(iterator);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetLocalPointerCapture(
    PPointerID pointerID,
    PointerState& pointerState,
    Ptr<PView> view,
    Ptr<PView> rootView,
    PPointerCaptureID captureID,
    PPointerCaptureRequestID requestID,
    PPointerCaptureMode mode)
{
    const PointerCaptureState previousCapture = pointerState.Capture;
    Ptr<PView> previousCaptureView = ptr_tmp_cast(previousCapture.View);

    pointerState.Capture = PointerCaptureState
    {
        .View = ptr_raw_pointer_cast(view),
        .RootView = ptr_raw_pointer_cast(rootView),
        .CaptureID = captureID,
        .RequestID = requestID,
        .Mode = mode
    };

    if (previousCaptureView != nullptr && previousCaptureView != view)
    {
        previousCaptureView->OnPointerCaptureLost(
            pointerID, PPointerCaptureLostReason::Stolen);
    }

    const bool previousCaptureWasConfirmed = previousCapture.CaptureID != PInvalidPointerCaptureID && previousCapture.View != nullptr;
    if (captureID != PInvalidPointerCaptureID
        && view != nullptr
        && (!previousCaptureWasConfirmed || previousCaptureView != view)) {
        view->OnPointerCaptureGained(pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::ClearLocalPointerCapture(
    PPointerID pointerID,
    PointerState& pointerState,
    PPointerCaptureLostReason reason)
{
    Ptr<PView> captureView = ptr_tmp_cast(pointerState.Capture.View);
    pointerState.Capture = PointerCaptureState();

    if (captureView != nullptr) {
        captureView->OnPointerCaptureLost(pointerID, reason);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::RefreshPointerPathAfterCaptureChange(PointerState& pointerState)
{
    if ((pointerState.LastEvent.SupportsHover && pointerState.DeliveryRootView != nullptr)
        || (!pointerState.LastEvent.SupportsHover && pointerState.Capture.View != nullptr)) {
        UpdateEffectivePointerPath(pointerState.DeliveryRootView, pointerState.LastEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::BeginPointerCaptureRequest(
    PPointerID pointerID,
    PointerState& pointerState,
    Ptr<PView> view,
    Ptr<PView> rootView,
    PPointerCaptureMode mode)
{
    const PPointerCaptureRequestID requestID = AllocatePointerCaptureRequestID();
    SetLocalPointerCapture(
        pointerID,
        pointerState,
        view,
        nullptr,
        PInvalidPointerCaptureID,
        requestID,
        mode);
    RefreshPointerPathAfterCaptureChange(pointerState);
    Post<ASSetPointerCapture>(
        rootView->m_ServerHandle,
        pointerID,
        PInvalidPointerCaptureID,
        requestID,
        mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::UpdateConfirmedPointerCapture(
    PPointerID pointerID,
    PointerState& pointerState,
    Ptr<PView> view,
    Ptr<PView> rootView,
    PPointerCaptureMode mode)
{
    const PointerCaptureState previousCapture = pointerState.Capture;
    const bool rootViewChanged =
        previousCapture.RootView != ptr_raw_pointer_cast(rootView);
    const bool modeChanged = previousCapture.Mode != mode;

    SetLocalPointerCapture(pointerID, pointerState, view, rootView, previousCapture.CaptureID, PInvalidPointerCaptureRequestID, mode);

    if (rootViewChanged) {
        pointerState.DeliveryRootView = rootView;
    }
    RefreshPointerPathAfterCaptureChange(pointerState);

    if (rootViewChanged || modeChanged) {
        Post<ASSetPointerCapture>(rootView->m_ServerHandle, pointerID, previousCapture.CaptureID, PInvalidPointerCaptureRequestID, mode);
    }
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

    Ptr<PView> rootView = view->GetRoot();
    if (rootView == nullptr || rootView->m_ServerHandle == INVALID_HANDLE) {
        return false;
    }

    PointerState& pointerState = GetPointerState(pointerID);
    const PointerCaptureState previousCapture = pointerState.Capture;
    if (view != previousCapture.View
        && previousCapture.View != nullptr
        && previousCapture.Mode == PPointerCaptureMode::Locked) {
        return false;
    }

    const bool captureIsConfirmed =
        previousCapture.CaptureID != PInvalidPointerCaptureID;
    const bool captureIsPending =
        previousCapture.RequestID != PInvalidPointerCaptureRequestID;
    if (captureIsConfirmed)
    {
        UpdateConfirmedPointerCapture(
            pointerID, pointerState, view, rootView, mode);
        return true;
    }

    const Ptr<PView> pendingRootView = (captureIsPending && previousCapture.View != nullptr) ? previousCapture.View->GetRoot() : nullptr;
    if (captureIsPending
        && pendingRootView == rootView
        && previousCapture.Mode == mode)
    {
        SetLocalPointerCapture(
            pointerID,
            pointerState,
            view,
            nullptr,
            PInvalidPointerCaptureID,
            previousCapture.RequestID,
            mode);
        RefreshPointerPathAfterCaptureChange(pointerState);
    }
    else
    {
        BeginPointerCaptureRequest(
            pointerID, pointerState, view, rootView, mode);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::ReleasePointerCapture(PPointerID pointerID, Ptr<PView> view, PPointerCaptureLostReason reason)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerStateMap.find(pointerID);
    if (iterator == m_PointerStateMap.end()) {
        return;
    }

    const PointerCaptureState& captureState = iterator->second.Capture;
    if (view != nullptr && view != captureState.View) {
        return;
    }

    const bool captureWasPending =
        captureState.RequestID != PInvalidPointerCaptureRequestID;
    const PPointerCaptureID captureID = captureState.CaptureID;
    ClearLocalPointerCapture(pointerID, iterator->second, reason);

    if (captureID != PInvalidPointerCaptureID) {
        Post<ASReleasePointerCapture>(pointerID, captureID);
    } else if (captureWasPending) {
        RefreshPointerPathAfterCaptureChange(iterator->second);
    }
    EraseInactivePointerState(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerCaptureRequestID PApplication::AllocatePointerCaptureRequestID()
{
    const PPointerCaptureRequestID requestID = m_NextPointerCaptureRequestID;
    m_NextPointerCaptureRequestID++;
    if (m_NextPointerCaptureRequestID == PInvalidPointerCaptureRequestID) {
        m_NextPointerCaptureRequestID = PFirstPointerCaptureRequestID;
    }
    return requestID;
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

void PApplication::HandleViewScreenPositionChanged(handler_id viewHandle, const PPoint& screenPosition)
{
    Ptr<PView> view = FindView(viewHandle);
    if (view != nullptr) {
        view->HandleScreenPositionChanged(screenPosition);
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

void PApplication::HandleKeyboardEvent(handler_id viewHandle, const PKeyEvent& keyEvent, const PString& text)
{
    m_KeyQualifiers = keyEvent.Qualifiers & ~QUAL_REPEAT;
    Ptr<PView> view = FindView(viewHandle);
    if (view != nullptr) {
        view->HandleKeyboardEvent(keyEvent, text);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t PApplication::AllocateKeyboardRequestID()
{
    assert(!IsRunning() || GetMutex().IsLocked());
    for (;;)
    {
        const uint32_t requestID = m_NextKeyboardRequestID++;
        if (!m_KeyboardLayoutsCallbacks.contains(requestID) && !m_KeyboardConfigurationCallbacks.contains(requestID)) {
            return requestID;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandleKeyboardLayouts(
    uint32_t requestID,
    PErrorCode error,
    uint32_t firstIndex,
    uint32_t totalCount,
    const std::vector<PKeyboardLayoutInfo>& layouts)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    auto pending = m_KeyboardLayoutsCallbacks.extract(requestID);
    if (!pending.empty()) {
        pending.mapped()(error, firstIndex, totalCount, layouts);
    }
}

void PApplication::HandleKeyboardConfiguration(
    uint32_t requestID,
    PErrorCode error,
    const PString& layout,
    const PString& variant,
    const PString& options)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    auto pending = m_KeyboardConfigurationCallbacks.extract(requestID);
    if (!pending.empty()) {
        pending.mapped()(error, layout, variant, options);
    }
}

void PApplication::HandlePointerEvent(
    handler_id viewHandle,
    const PPointerEvent& pointerEvent,
    PPointerCaptureID captureID)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    const bool isMoveEvent = pointerEvent.EventType == PPointerEventType::Move;
    const bool isWheelEvent = pointerEvent.EventType == PPointerEventType::Wheel;
    
    if ((isMoveEvent || isWheelEvent) && !m_PendingPointerEvents.empty())
    {
        PendingPointerEvent& pendingEvent = m_PendingPointerEvents.back();
    
        if (pendingEvent.ViewHandle == viewHandle
            && pendingEvent.CaptureID == captureID
            && pendingEvent.Event.PointerID     == pointerEvent.PointerID
            && pendingEvent.Event.EventType     == pointerEvent.EventType
            && pendingEvent.Event.ToolType      == pointerEvent.ToolType
            && pendingEvent.Event.SupportsHover == pointerEvent.SupportsHover
            && pendingEvent.Event.Button        == pointerEvent.Button
            && pendingEvent.Event.Buttons       == pointerEvent.Buttons
            && pendingEvent.Event.Qualifiers    == pointerEvent.Qualifiers)
        {
            if (isMoveEvent)
            {
                pendingEvent.Event = pointerEvent;
                return;
            }

            const bool horizontalDirectionMatches =
                pendingEvent.Event.WheelDelta.x == 0.0f || pointerEvent.WheelDelta.x == 0.0f
                || (pendingEvent.Event.WheelDelta.x < 0.0f) == (pointerEvent.WheelDelta.x < 0.0f);
            
            const bool verticalDirectionMatches =
                pendingEvent.Event.WheelDelta.y == 0.0f || pointerEvent.WheelDelta.y == 0.0f
                || (pendingEvent.Event.WheelDelta.y < 0.0f) == (pointerEvent.WheelDelta.y < 0.0f);

            if (horizontalDirectionMatches && verticalDirectionMatches)
            {
                const PPoint accumulatedWheelDelta = pendingEvent.Event.WheelDelta + pointerEvent.WheelDelta;
                pendingEvent.Event = pointerEvent;
                pendingEvent.Event.WheelDelta = accumulatedWheelDelta;
                return;
            }
        }
    }

    m_PendingPointerEvents.push_back(PendingPointerEvent
        {
            .ViewHandle = viewHandle,
            .Event = pointerEvent,
            .CaptureID = captureID
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::DispatchPendingPointerEvents()
{
    while (!m_PendingPointerEvents.empty())
    {
        const PendingPointerEvent pendingEvent = m_PendingPointerEvents.front();
        m_PendingPointerEvents.pop_front();
        DispatchPointerEvent(pendingEvent.ViewHandle, pendingEvent.Event, pendingEvent.CaptureID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::DispatchPointerEvent(
    handler_id viewHandle,
    const PPointerEvent& pointerEvent,
    PPointerCaptureID captureID)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    m_KeyQualifiers = pointerEvent.Qualifiers;

    Ptr<PView> view = FindView(viewHandle);
    if (view != nullptr)
    {
        PointerState& pointerState = GetPointerState(pointerEvent.PointerID);
        if (!pointerEvent.SupportsHover && pointerEvent.EventType == PPointerEventType::Down && captureID != PInvalidPointerCaptureID)
        {
            if (pointerState.Capture.View != nullptr) {
                ClearLocalPointerCapture(
                    pointerEvent.PointerID,
                    pointerState,
                    PPointerCaptureLostReason::PointerCancel);
            }
            pointerState.Capture = PointerCaptureState
            {
                .View = nullptr,
                .RootView = ptr_raw_pointer_cast(view),
                .CaptureID = captureID,
                .RequestID = PInvalidPointerCaptureRequestID,
                .Mode = PPointerCaptureMode::Preemptible
            };
        }

        view->HandlePointerEvent(pointerEvent);

        auto pointerIterator = m_PointerStateMap.find(pointerEvent.PointerID);
        if (pointerIterator != m_PointerStateMap.end()
            && (pointerEvent.EventType == PPointerEventType::Up
                || pointerEvent.EventType == PPointerEventType::Cancel))
        {
            PointerCaptureState& captureState = pointerIterator->second.Capture;
            const bool matchingActiveCapture =
                captureID != PInvalidPointerCaptureID
                && captureState.CaptureID == captureID;
            const bool pendingCaptureWithoutServerCapture =
                captureID == PInvalidPointerCaptureID
                && captureState.RequestID != PInvalidPointerCaptureRequestID;
            if (matchingActiveCapture || pendingCaptureWithoutServerCapture)
            {
                const PPointerCaptureLostReason reason =
                    (pointerEvent.EventType == PPointerEventType::Cancel)
                        ? PPointerCaptureLostReason::PointerCancel
                        : PPointerCaptureLostReason::PointerUp;
                ClearLocalPointerCapture(
                    pointerEvent.PointerID, pointerIterator->second, reason);
            }
            if (!pointerEvent.SupportsHover) {
                ClearEffectivePointerPath(pointerEvent.PointerID, pointerEvent);
            }
        }
        EraseInactivePointerState(pointerEvent.PointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandlePointerCaptureRequestReply(
    PPointerID pointerID,
    PPointerCaptureRequestID requestID,
    handler_id rootViewHandle,
    PPointerCaptureID captureID,
    const PPointerEvent& pointerEvent)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto pointerIterator = m_PointerStateMap.find(pointerID);
    if (pointerIterator == m_PointerStateMap.end()
        || pointerIterator->second.Capture.RequestID != requestID)
    {
        if (captureID != PInvalidPointerCaptureID) {
            Post<ASReleasePointerCapture>(
                pointerID, captureID);
        }
        return;
    }

    PointerState& pointerState = pointerIterator->second;
    Ptr<PView> captureView = ptr_tmp_cast(pointerState.Capture.View);
    Ptr<PView> rootView = FindView(rootViewHandle);
    const bool captureTargetIsValid =
        captureView != nullptr
        && rootView != nullptr
        && captureView->GetRoot() == rootView;

    if (captureID == PInvalidPointerCaptureID)
    {
        ClearLocalPointerCapture(
            pointerID, pointerState, PPointerCaptureLostReason::Rejected);
        RefreshPointerPathAfterCaptureChange(pointerState);
    }
    else if (!captureTargetIsValid)
    {
        ClearLocalPointerCapture(
            pointerID, pointerState, PPointerCaptureLostReason::ViewDetached);
        Post<ASReleasePointerCapture>(
            pointerID, captureID);
        RefreshPointerPathAfterCaptureChange(pointerState);
    }
    else
    {
        const PPointerCaptureMode mode = pointerState.Capture.Mode;
        pointerState.LastEvent = pointerEvent;
        pointerState.DeliveryRootView = rootView;
        SetLocalPointerCapture(
            pointerID,
            pointerState,
            captureView,
            rootView,
            captureID,
            PInvalidPointerCaptureRequestID,
            mode);
        RefreshPointerPathAfterCaptureChange(pointerState);
    }
    EraseInactivePointerState(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandlePointerCaptureLost(PPointerID pointerID, PPointerCaptureID captureID, PPointerCaptureLostReason reason)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerStateMap.find(pointerID);
    if (iterator == m_PointerStateMap.end()) {
        return;
    }
    if (captureID != iterator->second.Capture.CaptureID) {
        return;
    }

    const PPointerEvent pointerEvent = iterator->second.LastEvent;
    ClearEffectivePointerPath(pointerID, pointerEvent);

    iterator = m_PointerStateMap.find(pointerID);
    if (iterator != m_PointerStateMap.end()) {
        ClearLocalPointerCapture(pointerID, iterator->second, reason);
    }
    EraseInactivePointerState(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::HandlePointerRootViewUpdate(
    handler_id              rootViewHandle,
    const PPointerEvent&    pointerEvent,
    PPointerRootViewUpdateType  updateType
)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    switch (updateType)
    {
        case PPointerRootViewUpdateType::Reevaluate:
        {
            Ptr<PView> rootView = FindView(rootViewHandle);
            if (rootView != nullptr) {
                UpdateEffectivePointerPath(rootView, pointerEvent);
            }
            break;
        }
        case PPointerRootViewUpdateType::Exited:
            ClearEffectivePointerPath(pointerEvent.PointerID, pointerEvent);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::GetPointerCaptureView(PPointerID pointerID) const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerStateMap.find(pointerID);
    if (iterator != m_PointerStateMap.end()) {
        return ptr_tmp_cast(iterator->second.Capture.View);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::HasConfirmedPointerCapture(PPointerID pointerID) const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerStateMap.find(pointerID);
    return iterator != m_PointerStateMap.end()
        && iterator->second.Capture.CaptureID != PInvalidPointerCaptureID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::UpdateEffectivePointerPath(Ptr<PView> deliveryRootView, const PPointerEvent& pointerEvent)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    PointerState& pointerState = GetPointerState(pointerEvent.PointerID);
    pointerState.LastEvent = pointerEvent;
    pointerState.DeliveryRootView = deliveryRootView;

    Ptr<PView> targetView = ptr_tmp_cast(pointerState.Capture.View);
    if (targetView == nullptr && deliveryRootView != nullptr)
    {
        const PPoint position = deliveryRootView->ConvertFromScreen(pointerEvent.ScreenPosition);
        targetView = deliveryRootView->FindPointerTarget(position);
    }

    PView::PointerEventPath targetPath;
    if (targetView != nullptr) {
        targetView->BuildPointerEventPath(targetPath);
    }

    PView::PointerEventPath previousPath = std::move(pointerState.EffectivePath);
    pointerState.EffectivePath = targetPath;

    if (previousPath != targetPath) {
        PView::DispatchPointerBoundaryEvents(pointerEvent, previousPath, targetPath);
    }
    return targetView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::ClearEffectivePointerPath(PPointerID pointerID, const PPointerEvent& pointerEvent)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    auto iterator = m_PointerStateMap.find(pointerID);
    if (iterator != m_PointerStateMap.end())
    {
        PView::PointerEventPath previousPath = std::move(iterator->second.EffectivePath);
        iterator->second.DeliveryRootView = nullptr;
        iterator->second.EffectivePath.clear();
        PView::DispatchPointerBoundaryEvents(pointerEvent, previousPath, PView::PointerEventPath());
        EraseInactivePointerState(pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::RefreshPointerPaths()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (m_PointerPathsInvalid)
    {
        m_PointerPathsInvalid = false;

        std::vector<PPointerID> pointerIDs;
        for (const auto& [pointerID, pointerState] : m_PointerStateMap)
        {
            if (pointerState.LastEvent.SupportsHover && pointerState.DeliveryRootView != nullptr) {
                pointerIDs.push_back(pointerID);
            }
        }

        for (PPointerID pointerID : pointerIDs)
        {
            auto iterator = m_PointerStateMap.find(pointerID);
            if (iterator != m_PointerStateMap.end()) {
                UpdateEffectivePointerPath(iterator->second.DeliveryRootView, iterator->second.LastEvent);
            }
        }
    }
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

