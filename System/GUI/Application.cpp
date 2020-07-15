// This file is part of PadOS.
//
// Copyright (C) 2017-2020 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <string.h>

#include "App/Application.h"
#include "GUI/View.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application::Application(const String& name) : Looper(name, 1000), m_ReplyPort("app_reply", 1000)
{
    ASRegisterApplication::Sender::Emit(get_appserver_port(), -1, INFINIT_TIMEOUT, m_ReplyPort.GetHandle(), GetPortID(), GetName());
    
    for(;;)
    {
        MsgRegisterApplicationReply reply;
        int32_t                     code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == AppserverProtocol::REGISTER_APPLICATION_REPLY) {
                m_ServerHandle = reply.m_ServerHandle;
                break;
            } else {
                printf("ERROR: Application::Application() received invalid reply: %" PRId32 "\n", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            printf("ERROR: Application::Application() receive failed: %s\n", strerror(get_last_error()));            
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application::~Application()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::Idle()
{
    while(!m_ViewsNeedingLayout.empty())
    {
        std::set<Ptr<View>> list = std::move(m_ViewsNeedingLayout);
        for (Ptr<View> view : list)
        {
            view->UpdateLayout();
        }
    }
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect Application::GetScreenIFrame()
{
    return IRect(IPoint(0), kernel::GfxDriver::Instance.GetResolution() - IPoint(1, 1));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::AddView(Ptr<View> view, ViewDockType dockType)
{
    Ptr<View> parent = view->GetParent();
    handler_id parentHandle = INVALID_HANDLE;
    for (Ptr<View> i = parent; i != nullptr; i = i->GetParent())
    {
        handler_id curParentHandle = i->GetServerHandle();
        if (curParentHandle != INVALID_HANDLE /*i->HasFlags(ViewFlags::WillDraw)*/)
        {
            parentHandle = curParentHandle; // i->GetServerHandle();
            break;
        }
    }
    if (dockType != ViewDockType::ChildView || view->HasFlags(ViewFlags::WillDraw))
    {
        AddHandler(view);
        Post<ASCreateView>(GetPortID()
                            , m_ReplyPort.GetHandle()
                            , view->GetHandle()
                            , parentHandle
                            , dockType
                            , view->GetName()
                            , view->m_Frame + view->m_PositionOffset
                            , view->m_ScrollOffset
                            , view->m_Flags
                            , view->m_HideCount
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
                if (code == AppserverProtocol::CREATE_VIEW_REPLY) {
                    view->SetServerHandle(reply.m_ViewHandle);
                    break;
                } else {
                    printf("ERROR: Application::AddView() received invalid reply: %" PRId32 "\n", code);
                }
            }
            else if (get_last_error() != EINTR)
            {
                printf("ERROR: Application::AddView() receive failed: %s\n", strerror(get_last_error()));
                break;
            }
        }
    }
    else // Client only.
    {
        if (parent == nullptr) {
            printf("ERROR: Application::AddView() attempt to add client-only view '%s' to viewport.\n", view->GetName().c_str());
            return false;
        }
    }
    view->MergeFlags(ViewFlags::IsAttachedToScreen);
    view->AttachedToScreen();
    for (Ptr<View> child : view->m_ChildrenList) {
        AddView(ptr_static_cast<View>(child), ViewDockType::ChildView);
    }
    view->AllAttachedToScreen();
	view->PreferredSizeChanged();
    view->m_IsLayoutValid = false;
    if (parent == nullptr) {
        RegisterViewForLayout(view);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::RemoveView(Ptr<View> view)
{
    if (view->m_ServerHandle != INVALID_HANDLE)
    {
        Post<ASDeleteView>(view->m_ServerHandle);
        DetachView(view);
    }
    if (view->GetLooper() != nullptr) {
        RemoveHandler(view);
    }
    for (Ptr<View> child : *view) {
        RemoveView(child);
    }
	view->ClearFlags(ViewFlags::IsAttachedToScreen);
    view->HandleDetachedFromScreen();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::Flush()
{
    if (m_UsedSendBufferSize > 0) {
        send_message(get_appserver_port(), m_ServerHandle, AppserverProtocol::MESSAGE_BUNDLE, m_SendBuffer, m_UsedSendBufferSize);
        m_UsedSendBufferSize = 0;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::Sync()
{
    Post<ASSync>(m_ReplyPort.GetHandle());
    Flush();
    int32_t code;
    for (;;)
    {
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, nullptr, 0) < 0 && get_last_error() != EINTR) break;
        if (code == AppserverProtocol::SYNC_REPLY) {
            break;
        } else {
            printf("ERROR: Application::Sync() received invalid reply: %" PRIi32 "\n", code);
        }
        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::DetachView(Ptr<View> view)
{
    view->SetServerHandle(INVALID_HANDLE);
    for (Ptr<View> child : *view)
    {
        DetachView(child);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* Application::AllocMessageBuffer(int32_t messageID, size_t size)
{
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
