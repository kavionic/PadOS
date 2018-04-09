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
// Created: 28.03.2018 20:48:35

#include "sam.h"

#include <string.h>

#include "ServerApplication.h"
#include "ApplicationServer.h"
#include "Protocol.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ServerApplication::ServerApplication(ApplicationServer* server, const String& name, port_id clientPort) : EventHandler(name), m_Server(server), m_ClientPort(clientPort)
{
    RegisterRemoteSignal(&RSSync,           &ServerApplication::SlotSync);
    RegisterRemoteSignal(&RSCreateView,     &ServerApplication::SlotCreateView);
    RegisterRemoteSignal(&RSDeleteView,     &ServerApplication::SlotDeleteView);
    RegisterRemoteSignal(&RSSetViewFrame,   &ServerApplication::SlotSetViewFrame);
    RegisterRemoteSignal(&RSInvalidateView, &ServerApplication::SlotInvalidateView);    
    
    RegisterRemoteSignal(&RSViewBeginUpdate,   &ServerApplication::SlotViewBeginUpdate);
    RegisterRemoteSignal(&RSViewEndUpdate,     &ServerApplication::SlotViewEndUpdate);
    
    RegisterRemoteSignal(&RSViewSetFgColor,    &ServerApplication::SlotViewSetFgColor);
    RegisterRemoteSignal(&RSViewSetBgColor,    &ServerApplication::SlotViewSetBgColor);
    RegisterRemoteSignal(&RSViewSetEraseColor, &ServerApplication::SlotViewSetEraseColor);
    RegisterRemoteSignal(&RSViewSetFont,       &ServerApplication::SlotViewSetFont);
    RegisterRemoteSignal(&RSViewMovePenTo,     &ServerApplication::SlotViewMovePenTo);
    RegisterRemoteSignal(&RSViewDrawLine1,     &ServerApplication::SlotViewDrawLine1);
    RegisterRemoteSignal(&RSViewDrawLine2,     &ServerApplication::SlotViewDrawLine2);
    RegisterRemoteSignal(&RSViewFillRect,      &ServerApplication::SlotViewFillRect);
    RegisterRemoteSignal(&RSViewFillCircle,    &ServerApplication::SlotViewFillCircle);
    RegisterRemoteSignal(&RSViewDrawString,    &ServerApplication::SlotViewDrawString);
    RegisterRemoteSignal(&RSViewScrollBy,      &ServerApplication::SlotViewScrollBy);
    RegisterRemoteSignal(&RSViewCopyRect,      &ServerApplication::SlotViewCopyRect);
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
        case AppserverProtocol::MESSAGE_BUNDLE:
        {
            for (size_t i = 0; i < length;)
            {
                const AppserverMessage* message = reinterpret_cast<const AppserverMessage*>(reinterpret_cast<const uint8_t*>(data) + i);
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
    if (m_LowestInvalidView != nullptr && (code != AppserverProtocol::SET_VIEW_FRAME  ||
                                           code != AppserverProtocol::SHOW_VIEW       ||
                                           code != AppserverProtocol::VIEW_SET_DRAW_REGION ||
                                           code != AppserverProtocol::VIEW_SET_SHAPE_REGION))
    {
        UpdateRegions();
    }    
    
    RemoteSignalRXBase* handler = GetSignalForMessage(code);
    if (handler != nullptr) {
        handler->Dispatch(data, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::UpdateRegions()
{
    if (m_LowestInvalidView != nullptr)
    {
        m_LowestInvalidView->UpdateRegions(false);
        //HandleMouseTransaction();
        m_LowestInvalidView = nullptr;
        m_LowestInvalidLevel = std::numeric_limits<int>::max();
    }
    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::UpdateLowestInvalidView(Ptr<ServerView> view)
{
    if (view->m_Level < m_LowestInvalidLevel) {
        m_LowestInvalidLevel = view->m_Level;
        m_LowestInvalidView = view;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotCreateView(port_id clientPort, port_id replyPort, handler_id replyTarget, handler_id parentHandle, const String& name, const Rect& frame, const Point& scrollOffset, uint32_t flags, int32_t hideCount, Color eraseColor, Color bgColor, Color fgColor)
{
    Ptr<ServerView> parent = (parentHandle == -1) ? m_Server->GetTopView() : m_Server->FindView(parentHandle);
    
    if (parent != nullptr)
    {
        Ptr<ServerView> view = ptr_new<ServerView>(name, frame, scrollOffset, flags, hideCount, eraseColor, bgColor, fgColor);
        m_Server->RegisterView(view);
        parent->AddChild(view);
        view->SetClientHandle(clientPort, replyTarget);
        
        MsgCreateViewReply reply;
        reply.m_ViewHandle = view->GetHandle();
        if (send_message(replyPort, -1, AppserverProtocol::CREATE_VIEW_REPLY, &reply, sizeof(reply), 0) < 0) {
            printf("ERROR: ServerApplication::SlotCreateView() failed to send message: %s\n", strerror(get_last_error()));
        }

        IRect modifiedFrame = view->GetFrame();
        Ptr<ServerView> opacParent = ServerView::GetOpacParent(parent, &modifiedFrame);
        opacParent->MarkModified(modifiedFrame);
        UpdateLowestInvalidView(opacParent);
    }
    else
    {
        MsgCreateViewReply reply;
        reply.m_ViewHandle = -1;
        if (send_message(replyPort, -1, AppserverProtocol::CREATE_VIEW_REPLY, &reply, sizeof(reply), 0) < 0) {
            printf("ERROR: ServerApplication::SlotCreateView() failed to send message: %s\n", strerror(get_last_error()));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotDeleteView(handler_id clientHandle)
{
    Ptr<ServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        Ptr<ServerView> parent = view->GetParent();
        
        
        IRect modifiedFrame = view->GetIFrame();
        Ptr<ServerView> opacParent = ServerView::GetOpacParent(parent, &modifiedFrame);

        view->RemoveThis();
        m_Server->RemoveHandler(view);
        
        opacParent->MarkModified(modifiedFrame);
        UpdateLowestInvalidView(opacParent);
    }
    else
    {
        printf("ERROR: ServerApplication::SlotDeleteView() no view with ID %d\n", clientHandle);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotSetViewFrame(handler_id clientHandle, const Rect& frame)
{
    Ptr<ServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr)
    {
        IRect modifiedFrame = view->GetIFrame();
        view->SetFrame(frame);
        modifiedFrame |= view->GetIFrame();
        Ptr<ServerView> opacParent = ServerView::GetOpacParent(view->GetParent(), &modifiedFrame);
        opacParent->MarkModified(modifiedFrame);
        UpdateLowestInvalidView(opacParent);
    }
    else
    {
        printf("ERROR: ServerApplication::SlotSetViewFrame() no view with ID %d\n", clientHandle);
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerApplication::SlotInvalidateView(handler_id clientHandle, const IRect& frame)
{
    Ptr<ServerView> view = m_Server->FindView(clientHandle);
    if (view != nullptr) {
        IRect invalidFrame = frame;
        view = ServerView::GetOpacParent(view, &invalidFrame);
        view->Invalidate(invalidFrame);
        UpdateLowestInvalidView(view);
    } else {
        printf("ERROR: ServerApplication::SlotSetViewFrame() no view with ID %d\n", clientHandle);
    }    
}

