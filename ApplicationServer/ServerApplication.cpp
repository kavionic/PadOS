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
    RegisterRemoteSignal(&RSFocusView,          &ServerApplication::SlotFocusView);
    RegisterRemoteSignal(&RSSetKeyboardFocus,   &ServerApplication::SlotSetKeyboardFocus);
    RegisterRemoteSignal(&RSCreateBitmap,       &ServerApplication::SlotCreateBitmap);
    RegisterRemoteSignal(&RSDeleteBitmap,       &ServerApplication::SlotDeleteBitmap);
    RegisterRemoteSignal(&RSViewSetFrame,       &ServerApplication::SlotViewSetFrame);
    RegisterRemoteSignal(&RSViewInvalidate,     &ServerApplication::SlotViewInvalidate);
    RegisterRemoteSignal(&RSViewAddChild,       &ServerApplication::SlotViewAddChild);
    RegisterRemoteSignal(&RSViewToggleDepth,    &ServerApplication::SlotViewToggleDepth);
    RegisterRemoteSignal(&RSViewBeginUpdate,    &ServerApplication::SlotViewBeginUpdate);
    RegisterRemoteSignal(&RSViewEndUpdate,      &ServerApplication::SlotViewEndUpdate);
    
    RegisterRemoteSignal(&RSViewShow,                   &ServerApplication::SlotViewShow);
    RegisterRemoteSignal(&RSViewSetFgColor,             &ServerApplication::SlotViewSetFgColor);
    RegisterRemoteSignal(&RSViewSetBgColor,             &ServerApplication::SlotViewSetBgColor);
    RegisterRemoteSignal(&RSViewSetEraseColor,          &ServerApplication::SlotViewSetEraseColor);
    RegisterRemoteSignal(&RSViewSetFocusKeyboardMode,   &ServerApplication::SlotViewSetFocusKeyboardMode);
    RegisterRemoteSignal(&RSViewSetDrawingMode,         &ServerApplication::SlotViewSetDrawingMode);
    RegisterRemoteSignal(&RSViewSetFont,                &ServerApplication::SlotViewSetFont);
    RegisterRemoteSignal(&RSViewSetPenWidth,            &ServerApplication::SlotViewSetPenWidth);
    RegisterRemoteSignal(&RSViewMovePenTo,              &ServerApplication::SlotViewMovePenTo);
    RegisterRemoteSignal(&RSViewDrawLine1,              &ServerApplication::SlotViewDrawLine1);
    RegisterRemoteSignal(&RSViewDrawLine2,              &ServerApplication::SlotViewDrawLine2);
    RegisterRemoteSignal(&RSViewFillRect,               &ServerApplication::SlotViewFillRect);
    RegisterRemoteSignal(&RSViewFillCircle,             &ServerApplication::SlotViewFillCircle);
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
    UpdateRegions();
    return wasHandled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::ProcessMessage(int32_t code, const void* data, size_t length)
{
//    if (m_HaveInvalidRegions && (
//                                 //code != AppserverProtocol::SHOW_VIEW       ||
//                                 code != AppserverProtocol::VIEW_SET_DRAW_REGION ||
//                                 code != AppserverProtocol::VIEW_SET_SHAPE_REGION))
//    {
//        UpdateRegions();
//    }    
    
    PRemoteSignalRXBase* const handler = GetSignalForMessage(code);
    if (handler != nullptr) {
        handler->Dispatch(data, length);
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

void ServerApplication::UpdateRegions()
{
    m_Server->GetTopView()->UpdateRegions();
    m_HaveInvalidRegions = false;

//    if (m_LowestInvalidView != nullptr)
//    {
//        m_LowestInvalidView->UpdateRegions();
//        //HandleMouseTransaction();
//        m_LowestInvalidView = nullptr;
//        m_LowestInvalidLevel = std::numeric_limits<int>::max();
//    }
    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::UpdateLowestInvalidView(Ptr<PServerView> view)
{
    m_HaveInvalidRegions = true;
//    if (view->m_Level < m_LowestInvalidLevel) {
//        m_LowestInvalidLevel = view->m_Level;
//        m_LowestInvalidView = view;
//    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotCreateView(port_id              clientPort,
                                       port_id              replyPort,
                                       handler_id           replyTarget,
                                       handler_id           parentHandle,
                                       PViewDockType         dockType,
                                       size_t               index,
                                       const PString&       name,
                                       const PRect&          frame,
                                       const PPoint&         scrollOffset,
                                       uint32_t             flags,
                                       int32_t              hideCount,
                                       PFocusKeyboardMode    focusKeyboardMode,
                                       PDrawingMode          drawingMode,
                                       float                penWidth,
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
    
    Ptr<PServerView> view = ptr_new<PServerView>(ApplicationServer::GetScreenBitmap(), name, frame, scrollOffset, dockType, flags, hideCount, focusKeyboardMode, drawingMode, penWidth, fontID, eraseColor, bgColor, fgColor);
    m_Server->RegisterView(view);
    if (parent != nullptr) {
        parent->AddChild(view, index);
    } else {
        view->SetIsWindowManagerControlled(true);
        p_post_to_window_manager<ASWindowManagerRegisterView>(INVALID_HANDLE, view->GetHandle(), dockType, view->GetName(), frame);
    }
    view->SetClientHandle(clientPort, replyTarget);
        
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

void ServerApplication::SlotFocusView(handler_id clientHandle, PMouseButton button, bool focus)
{
    const Ptr<PServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        m_Server->SetFocusView(button, view, focus);
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

void ServerApplication::SlotViewSetFrame(handler_id clientHandle, const PRect& frame, handler_id requestingClient)
{
    const Ptr<PServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
//        if (m_HaveInvalidRegions)
//        {
//            UpdateRegions();
//        }
        PIRect modifiedFrame = view->GetIFrame();
        view->SetFrame(frame, requestingClient);
        modifiedFrame |= view->GetIFrame();
        const Ptr<PServerView> opacParent = PServerView::GetOpacParent(view->GetParent(), &modifiedFrame);
        assert(opacParent != nullptr);
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
        if (m_HaveInvalidRegions)
        {
            UpdateRegions();
        }
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
