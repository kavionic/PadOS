// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 28.03.2018 20:48:35

#include "System/Platform.h"

#include <string.h>

#include "ServerApplication.h"
#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <ApplicationServer/Protocol.h>
#include <Utils/Utils.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ServerApplication::ServerApplication(ApplicationServer* server, const PString& name, port_id clientPort) : PEventHandler(name), m_Server(server), m_ClientPort(clientPort)
{
    RegisterRemoteSignal(&RSSync,               &ServerApplication::SlotSync);
    RegisterRemoteSignal(&RSCreateView,         &ServerApplication::SlotCreateView);
    RegisterRemoteSignal(&RSDeleteView,         &ServerApplication::SlotDeleteView);
    RegisterRemoteSignal(&RSSetPointerCapture,  &ServerApplication::SlotSetPointerCapture);
    RegisterRemoteSignal(&RSReleasePointerCapture, &ServerApplication::SlotReleasePointerCapture);
    RegisterRemoteSignal(&RSSetKeyboardFocus,   &ServerApplication::SlotSetKeyboardFocus);
    RegisterRemoteSignal(&RSCreateBitmap,       &ServerApplication::SlotCreateBitmap);
    RegisterRemoteSignal(&RSDeleteBitmap,       &ServerApplication::SlotDeleteBitmap);
    RegisterRemoteSignal(&RSPushMouseCursor,    &ServerApplication::SlotPushMouseCursor);
    RegisterRemoteSignal(&RSPopMouseCursor,     &ServerApplication::SlotPopMouseCursor);
    RegisterRemoteSignal(&RSViewSetFrame,       &ServerApplication::SlotViewSetFrame);
    RegisterRemoteSignal(&RSViewInvalidate,     &ServerApplication::SlotViewInvalidate);
    RegisterRemoteSignal(&RSViewAddChild,       &ServerApplication::SlotViewAddChild);
    RegisterRemoteSignal(&RSViewToggleDepth,    &ServerApplication::SlotViewToggleDepth);
    RegisterRemoteSignal(&RSViewBeginUpdate,    &ServerApplication::SlotViewBeginUpdate);
    RegisterRemoteSignal(&RSViewEndUpdate,      &ServerApplication::SlotViewEndUpdate);

    RegisterRemoteSignal(&RSViewShow,                   &ServerApplication::SlotViewShow);
    RegisterRemoteSignal(&RSViewSetHitMode,             &ServerApplication::SlotViewSetHitMode);
    RegisterRemoteSignal(&RSViewSetFgColor,             &ServerApplication::SlotViewSetFgColor);
    RegisterRemoteSignal(&RSViewSetBgColor,             &ServerApplication::SlotViewSetBgColor);
    RegisterRemoteSignal(&RSViewSetEraseColor,          &ServerApplication::SlotViewSetEraseColor);
    RegisterRemoteSignal(&RSViewSetFocusKeyboardMode,   &ServerApplication::SlotViewSetFocusKeyboardMode);
    RegisterRemoteSignal(&RSViewSetDrawingMode,         &ServerApplication::SlotViewSetDrawingMode);
    RegisterRemoteSignal(&RSViewSetFont,                &ServerApplication::SlotViewSetFont);
    RegisterRemoteSignal(&RSViewSetPenWidth,            &ServerApplication::SlotViewSetPenWidth);
    RegisterRemoteSignal(&RSViewSetCapStyle,            &ServerApplication::SlotViewSetCapStyle);
    RegisterRemoteSignal(&RSViewSetJointStyle,          &ServerApplication::SlotViewSetJointStyle);
    RegisterRemoteSignal(&RSViewSetMiterLimit,          &ServerApplication::SlotViewSetMiterLimit);
    RegisterRemoteSignal(&RSViewSetDashPattern,         &ServerApplication::SlotViewSetDashPattern);
    RegisterRemoteSignal(&RSViewSetDashOffset,          &ServerApplication::SlotViewSetDashOffset);
    RegisterRemoteSignal(&RSViewBeginPolyline,          &ServerApplication::SlotViewBeginPolyline);
    RegisterRemoteSignal(&RSViewAddPolylinePoint,       &ServerApplication::SlotViewAddPolylinePoint);
    RegisterRemoteSignal(&RSViewEndPolyline,            &ServerApplication::SlotViewEndPolyline);
    RegisterRemoteSignal(&RSViewMovePenTo,              &ServerApplication::SlotViewMovePenTo);
    RegisterRemoteSignal(&RSViewDrawLine1,              &ServerApplication::SlotViewDrawLine1);
    RegisterRemoteSignal(&RSViewDrawLine2,              &ServerApplication::SlotViewDrawLine2);
    RegisterRemoteSignal(&RSViewFillRect,               &ServerApplication::SlotViewFillRect);
    RegisterRemoteSignal(&RSViewFillCircle,             &ServerApplication::SlotViewFillCircle);
    RegisterRemoteSignal(&RSViewDrawEllipse,            &ServerApplication::SlotViewDrawEllipse);
    RegisterRemoteSignal(&RSViewDrawPie,                &ServerApplication::SlotViewDrawPie);
    RegisterRemoteSignal(&RSViewFillTriangle,           &ServerApplication::SlotViewFillTriangle);
    RegisterRemoteSignal(&RSViewBeginTriangles,         &ServerApplication::SlotViewBeginTriangles);
    RegisterRemoteSignal(&RSViewAddTriangle,            &ServerApplication::SlotViewAddTriangle);
    RegisterRemoteSignal(&RSViewEndTriangles,           &ServerApplication::SlotViewEndTriangles);
    RegisterRemoteSignal(&RSViewDrawString,             &ServerApplication::SlotViewDrawString);
    RegisterRemoteSignal(&RSViewScrollBy,               &ServerApplication::SlotViewScrollBy);
    RegisterRemoteSignal(&RSViewCopyRect,               &ServerApplication::SlotViewCopyRect);
    RegisterRemoteSignal(&RSViewDrawBitmap,             &ServerApplication::SlotViewDrawBitmap);
    RegisterRemoteSignal(&RSViewDrawScaledBitmap,       &ServerApplication::SlotViewDrawScaledBitmap);
    RegisterRemoteSignal(&RSViewDebugDraw,              &ServerApplication::SlotViewDebugDraw);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ServerApplication::~ServerApplication()
{
    if (m_Server != nullptr) {
        m_Server->ServerApplicationDestructed(this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerApplication::HandleMessage(int32_t code, const void* data, size_t length)
{
    bool wasHandled = false;
    switch(code)
    {
        case PAppserverProtocol::MESSAGE_BUNDLE:
        {
            for (size_t i = 0; i < length;)
            {
                const AppserverMessage* const message = reinterpret_cast<const AppserverMessage*>(reinterpret_cast<const uint8_t*>(data) + i);

                if (message->m_Length < sizeof(AppserverMessage) || (i + message->m_Length) > length)
                {
                    p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: Message {} has invalid length {} ({})", __PRETTY_FUNCTION__, message->m_Code, message->m_Length, length);
                    break;
                }

                ProcessMessage(message->m_Code, message + 1, message->m_Length - sizeof(AppserverMessage));

                i += message->m_Length;
            }
            wasHandled = true;
            break;
        }
    }
    if (m_HaveInvalidRegions) {
        UpdateRegions();
    }
    return wasHandled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::ProcessMessage(int32_t code, const void* data, size_t length)
{
    if (m_HaveInvalidRegions && !CanDeferRegionUpdate(code)) {
        UpdateRegions();
    }

    PRemoteSignalRXBase* const handler = GetSignalForMessage(code);
    if (handler != nullptr) {
        handler->Dispatch(data, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerApplication::CanDeferRegionUpdate(int32_t messageCode) const
{
    switch (messageCode)
    {
        case PAppserverProtocol::CREATE_VIEW:
        case PAppserverProtocol::VIEW_SET_FRAME:
        case PAppserverProtocol::VIEW_INVALIDATE:
        case PAppserverProtocol::VIEW_ADD_CHILD:
        case PAppserverProtocol::VIEW_SHOW:
        case PAppserverProtocol::VIEW_SET_DRAW_REGION:
        case PAppserverProtocol::VIEW_SET_SHAPE_REGION:
            return true;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PSrvBitmap> ServerApplication::GetBitmap(handle_id bitmapHandle) const
{
    auto i = m_BitmapMap.find(bitmapHandle);
    if (i != m_BitmapMap.end()) {
        return i->second;
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerCaptureID ServerApplication::SetPointerCapture(PPointerID pointerID, Ptr<PServerView> rootView, PPointerCaptureMode mode)
{
    if (m_Server == nullptr || rootView == nullptr) {
        return PInvalidPointerCaptureID;
    }

    const PPointerCaptureID captureID = m_Server->SetPointerCapture(pointerID, this, mode);
    if (captureID == PInvalidPointerCaptureID) {
        return PInvalidPointerCaptureID;
    }

    m_PointerCaptureMap[pointerID] = PointerCaptureState
    {
        .RootView = ptr_raw_pointer_cast(rootView),
        .CaptureID = captureID
    };
    return captureID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::ReleasePointerCapture(PPointerID pointerID, Ptr<PServerView> rootView, PPointerCaptureID captureID, PPointerCaptureLostReason reason)
{
    if (m_Server == nullptr) {
        return;
    }
    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator == m_PointerCaptureMap.end()) {
        return;
    }
    if (rootView != nullptr && rootView != iterator->second.RootView) {
        return;
    }
    if (captureID != iterator->second.CaptureID) {
        return;
    }
    m_Server->ReleasePointerCapture(pointerID, this, captureID, reason);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::HandlePointerUp(PPointerID pointerID, const PPointerEvent& event)
{
    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator == m_PointerCaptureMap.end()) {
        return;
    }
    Ptr<PServerView> rootView = ptr_tmp_cast(iterator->second.RootView);
    if (rootView != nullptr) {
        rootView->HandlePointerUp(pointerID, rootView->ConvertFromRoot(event.ScreenPosition), event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::HandlePointerMove(PPointerID pointerID, const PPointerEvent& event)
{
    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator == m_PointerCaptureMap.end()) {
        return;
    }
    Ptr<PServerView> rootView = ptr_tmp_cast(iterator->second.RootView);
    if (rootView != nullptr) {
        rootView->HandlePointerMove(pointerID, rootView->ConvertFromRoot(event.ScreenPosition), event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::HandlePointerCancel(PPointerID pointerID, const PPointerEvent& event)
{
    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator == m_PointerCaptureMap.end()) {
        return;
    }
    Ptr<PServerView> rootView = ptr_tmp_cast(iterator->second.RootView);
    if (rootView != nullptr) {
        rootView->HandlePointerCancel(pointerID, rootView->ConvertFromRoot(event.ScreenPosition), event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::HandlePointerCaptureLost(PPointerID pointerID, PPointerCaptureID captureID, PPointerCaptureLostReason reason)
{
    auto iterator = m_PointerCaptureMap.find(pointerID);
    if (iterator == m_PointerCaptureMap.end()) {
        return;
    }
    if (captureID != iterator->second.CaptureID) {
        return;
    }

    m_PointerCaptureMap.erase(iterator);

    if (!p_post_to_remotesignal<ASHandlePointerCaptureLost>(m_ClientPort, INVALID_HANDLE, TimeValNanos::zero, pointerID, captureID, reason)) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerApplication::HandlePointerCaptureLost() failed to send message: {}", strerror(get_last_error()));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::ViewDestructed(PServerView* view)
{
    if (m_Server == nullptr) {
        return;
    }
    for (auto iterator = m_PointerCaptureMap.begin(); iterator != m_PointerCaptureMap.end(); )
    {
        if (iterator->second.RootView == view)
        {
            const PPointerID pointerID = iterator->first;
            const PPointerCaptureID captureID = iterator->second.CaptureID;
            iterator = m_PointerCaptureMap.erase(iterator);
            m_Server->ReleasePointerCapture(pointerID, this, captureID, PPointerCaptureLostReason::ViewDetached);
        }
        else
        {
            ++iterator;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::UpdateRegions()
{
    Ptr<PServerView> updateView = (m_LowestInvalidView != nullptr) ? m_LowestInvalidView : m_Server->GetTopView();
    if (updateView != nullptr) {
        updateView->UpdateRegions();
    }
    m_LowestInvalidView = nullptr;
    m_HaveInvalidRegions = false;
}

///////////////////////////////////////////////////////////////////////////////
/// Find the deepest common ancestor for two views with pending region changes.
///
/// m_LowestInvalidView is the root used for a deferred UpdateRegions() call.
/// When another view changes, the update root must be high enough to contain
/// both the previously dirty subtree and the new one, because visible/full
/// regions are computed from parent and sibling relationships. A nullptr input
/// means there is no current candidate yet. If the views are unexpectedly
/// disconnected, fall back to the top view so the next update rebuilds from a
/// known common root.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PServerView> ServerApplication::GetCommonInvalidView(Ptr<PServerView> currentInvalidRoot, Ptr<PServerView> newInvalidView) const
{
    if (currentInvalidRoot == nullptr) {
        return newInvalidView;
    }
    if (newInvalidView == nullptr) {
        return currentInvalidRoot;
    }
    while (currentInvalidRoot != nullptr && currentInvalidRoot->m_Level > newInvalidView->m_Level) {
        currentInvalidRoot = currentInvalidRoot->GetParent();
    }
    while (currentInvalidRoot != nullptr && newInvalidView != nullptr && newInvalidView->m_Level > currentInvalidRoot->m_Level) {
        newInvalidView = newInvalidView->GetParent();
    }
    while (currentInvalidRoot != nullptr && newInvalidView != nullptr && currentInvalidRoot != newInvalidView)
    {
        currentInvalidRoot = currentInvalidRoot->GetParent();
        newInvalidView = newInvalidView->GetParent();
    }
    if (currentInvalidRoot != nullptr) {
        return currentInvalidRoot;
    }
    return m_Server->GetTopView();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::UpdateLowestInvalidView(Ptr<PServerView> view)
{
    if (view != nullptr)
    {
        m_HaveInvalidRegions = true;
        m_LowestInvalidView = GetCommonInvalidView(m_LowestInvalidView, view);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotCreateView(port_id              replyPort,
                                       handler_id           replyTarget,
                                       handler_id           parentHandle,
                                       PViewDockType         dockType,
                                       size_t               index,
                                       const PString&       name,
                                       const PRect&          frame,
                                       const PPoint&         scrollOffset,
                                       uint32_t             flags,
                                       PViewHitMode         hitMode,
                                       int32_t              hideCount,
                                       PFocusKeyboardMode    focusKeyboardMode,
                                       PDrawingMode          drawingMode,
                                       float                penWidth,
                                       PCapStyle             capStyle,
                                       PJointStyle           jointStyle,
                                       float                 miterLimit,
                                       const std::vector<float>& dashPattern,
                                       float                 dashOffset,
                                       PFontID               fontID,
                                       PColor                eraseColor,
                                       PColor                bgColor,
                                       PColor                fgColor)
{
    Ptr<PServerView> parent;
    
    if (dockType == PViewDockType::RootLevelView)
    {
        parent = m_Server->GetTopView();
    }
    else if (dockType == PViewDockType::ChildView)
    {
        parent = m_Server->FindView(parentHandle);
        if (parent == nullptr)
        {
            MsgCreateViewReply reply;
            reply.m_ViewHandle = -1;
            const PErrorCode result = message_port_send_timeout_ns(replyPort, -1, PAppserverProtocol::CREATE_VIEW_REPLY, &reply, sizeof(reply), 0);
            if (result != PErrorCode::Success) {
                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: failed to send message: {}", __PRETTY_FUNCTION__, strerror(std::to_underlying(result)));
            }
            return;
        }
    }
    
    Ptr<PServerView> view = ptr_new<PServerView>(ApplicationServer::GetScreenBitmap(), name, frame, scrollOffset, dockType, flags, hitMode, hideCount, focusKeyboardMode, drawingMode, penWidth, capStyle, jointStyle, miterLimit, dashPattern, dashOffset, fontID, eraseColor, bgColor, fgColor);
    m_Server->RegisterView(view);
    if (parent != nullptr) {
        parent->AddChild(view, index);
    } else {
        view->SetIsWindowManagerControlled(true);
        p_post_to_window_manager<ASWindowManagerRegisterView>(INVALID_HANDLE, view->GetHandle(), dockType, view->GetName(), frame);
    }
    view->SetClientHandle(replyTarget, this);
        
    MsgCreateViewReply reply;
    reply.m_ViewHandle = view->GetHandle();
    const PErrorCode result = message_port_send_timeout_ns(replyPort, INVALID_HANDLE, PAppserverProtocol::CREATE_VIEW_REPLY, &reply, sizeof(reply), 0);
    if (result != PErrorCode::Success) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: failed to send message: {}", __PRETTY_FUNCTION__, strerror(std::to_underlying(result)));
    }
    view->Invalidate(true);
    if (parent != nullptr)
    {
        PIRect modifiedFrame = view->GetFrame();
        const Ptr<PServerView> opacParent = PServerView::GetOpacParent(parent, &modifiedFrame);
        opacParent->MarkModified(modifiedFrame);
        UpdateLowestInvalidView(opacParent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotDeleteView(handler_id clientHandle)
{
    const Ptr<PServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        const Ptr<PServerView> parent = view->GetParent();
        
        
        PIRect modifiedFrame = view->GetIFrame();
        const Ptr<PServerView> opacParent = PServerView::GetOpacParent(parent, &modifiedFrame);

        if (view->IsWindowManagerControlled()) {
            p_post_to_window_manager<ASWindowManagerUnregisterView>(INVALID_HANDLE, view->GetHandle());
        }
        view->RemoveThis(true);
        
        opacParent->MarkModified(modifiedFrame);
        UpdateLowestInvalidView(opacParent);
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: no view with ID {}", __PRETTY_FUNCTION__, clientHandle);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotSetPointerCapture(port_id replyPort, handler_id clientHandle, PPointerID pointerID, PPointerCaptureMode mode)
{
    const Ptr<PServerView> view = m_Server->FindView(clientHandle);
    MsgSetPointerCaptureReply reply;
    if (view != nullptr)
    {
        reply.m_CaptureID = SetPointerCapture(pointerID, view, mode);
    }
    const PErrorCode result = message_port_send_timeout_ns(replyPort, INVALID_HANDLE, PAppserverProtocol::SET_POINTER_CAPTURE_REPLY, &reply, sizeof(reply), 0);
    if (result != PErrorCode::Success) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: failed to send message: {}", __PRETTY_FUNCTION__, strerror(std::to_underlying(result)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotReleasePointerCapture(handler_id clientHandle, PPointerID pointerID, PPointerCaptureID captureID, PPointerCaptureLostReason reason)
{
    const Ptr<PServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        ReleasePointerCapture(pointerID, view, captureID, reason);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotSetKeyboardFocus(handler_id clientHandle, bool focus)
{
    const Ptr<PServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        m_Server->SetKeyboardFocus(view, focus);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotCreateBitmap(port_id replyPort, int width, int height, PEColorSpace colorSpace, void* raster, size_t bytesPerRow, uint32_t flags)
{
    const Ptr<PSrvBitmap> bitmap = ptr_new<PSrvBitmap>(PIPoint(width, height), colorSpace, static_cast<uint8_t*>(raster), bytesPerRow);

    const handle_id handle = m_NextBitmapHandle++;

    m_BitmapMap[handle] = bitmap;

    MsgCreateBitmapReply reply;
    reply.m_BitmapHandle = handle;
    reply.m_Framebuffer  = bitmap->m_Raster;
    reply.m_BytesPerRow  = bitmap->m_BytesPerLine;

    const PErrorCode result = message_port_send_timeout_ns(replyPort, INVALID_HANDLE, PAppserverProtocol::CREATE_BITMAP_REPLY, &reply, sizeof(reply), 0);
    if (result != PErrorCode::Success) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: failed to send message: {}", __PRETTY_FUNCTION__, strerror(std::to_underlying(result)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotDeleteBitmap(handle_id bitmapHandle)
{
    auto i = m_BitmapMap.find(bitmapHandle);
    if (i != m_BitmapMap.end()) {
        m_BitmapMap.erase(i);
    } else {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: invalid handle: {}", __PRETTY_FUNCTION__, bitmapHandle);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotPushMouseCursor(PMouseCursorToken token, PMouseCursorID cursorID)
{
    m_Server->PushMouseCursor(this, token, cursorID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotPopMouseCursor(PMouseCursorToken token)
{
    m_Server->PopMouseCursor(this, token);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotViewSetFrame(handler_id clientHandle, const PRect& frame, handler_id requestingClient)
{
    const Ptr<PServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        PIRect modifiedFrame = view->GetIFrame();
        view->SetFrame(frame, requestingClient);
        modifiedFrame |= view->GetIFrame();
        const Ptr<PServerView> opacParent = PServerView::GetOpacParent(view->GetParent(), &modifiedFrame);
        if (opacParent != nullptr)
        {
            opacParent->MarkModified(modifiedFrame);
            UpdateLowestInvalidView(opacParent);
        }
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: no view with ID {}", __PRETTY_FUNCTION__, clientHandle);
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotViewInvalidate(handler_id clientHandle, const PIRect& frame)
{
    Ptr<PServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        PIRect invalidFrame = frame + PIPoint(view->GetScrollOffset());
        view = PServerView::GetOpacParent(view, &invalidFrame);
        assert(view != nullptr);
        view->Invalidate(invalidFrame);
        UpdateLowestInvalidView(view);
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: no view with ID {}", __PRETTY_FUNCTION__, clientHandle);
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotViewAddChild(size_t index, handler_id viewHandle, handler_id childHandle, handler_id managerHandle)
{
    const Ptr<PServerView> view = m_Server->FindView(viewHandle);
    if (view != nullptr)
    {
        Ptr<PServerView> child = m_Server->FindView(childHandle);
        if (child != nullptr)
        {
            child->SetManagerHandle(managerHandle);
            view->AddChild(child, index);

            PIRect modifiedFrame = view->GetIFrame();
            Ptr<PServerView> opacParent = PServerView::GetOpacParent(view->GetParent(), &modifiedFrame);
            if (opacParent != nullptr)
            {
                opacParent->MarkModified(modifiedFrame);
                UpdateLowestInvalidView(opacParent);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotViewShow(handler_id viewHandle, bool show)
{
    const Ptr<PServerView> view = m_Server->FindView(viewHandle);
    if (view != nullptr)
    {
        const bool wasVisible = view->IsVisible();
        view->Show(show);
        if (view->IsVisible() != wasVisible)
        {
            PIRect modifiedFrame = view->GetIFrame();
            const Ptr<PServerView> opacParent = PServerView::GetOpacParent(view->GetParent(), &modifiedFrame);
            if (opacParent != nullptr)
            {
                opacParent->MarkModified(modifiedFrame);
                UpdateLowestInvalidView(opacParent);
            }
        }
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "{}: no view with ID {}", __PRETTY_FUNCTION__, viewHandle);
    }
}
