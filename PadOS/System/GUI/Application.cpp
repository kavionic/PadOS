// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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

#include "Application.h"
#include "System/GUI/View.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application::Application(const String& name) : Looper(name, 10)
{
//    RSCreateView.SetTransmitter(this, &Application::SlotTransmitRemoteSignal);
//    RSDeleteView.SetTransmitter(this, &Application::SlotTransmitRemoteSignal);
//    RSSetViewFrame.SetTransmitter(this, &Application::SlotTransmitRemoteSignal);
//    RSInvalidateView.SetTransmitter(this, &Application::SlotTransmitRemoteSignal);
    
    RSRegisterApplicationReply.Connect(this, &Application::SlotRegisterApplicationReply);
    RSCreateViewReply.Connect(this, &Application::SlotCreateViewReply);
    
//    size_t size = RSRegisterApplication.GetSize(GetPortID(), GetName());
//    RSRegisterApplication(this, &Application::AllocMessageBuffer, GetPortID(), GetName());
    ASRegisterApplication::Sender::Emit(g_AppserverPort, -1, GetPortID(), GetName());
//    g_AppserverPort.SendMessage(-1, RSRegisterApplication.GetID(), m_SendBuffer, size);
//    RSRegisterApplication(g_AppserverPort, -1, GetPortID(), GetName());
//    Flush();    
    WaitForReply(-1, AppserverProtocol::REGISTER_APPLICATION_REPLY);
    
    
//    m_TopView = ptr_new<View>();
//    AddView(m_TopView);
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
    if (code == AppserverProtocol::REGISTER_APPLICATION_REPLY) {
        RSRegisterApplicationReply.Dispatch(data, length);
        return true;
    } else if (code == AppserverProtocol::CREATE_VIEW_REPLY) {
        RSCreateViewReply.Dispatch(data, length);
        return true;
    }
    return false;
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

bool Application::AddView(Ptr<View> view)
{
    AddHandler(view);
    Ptr<View> parent = view->GetParent();
    Post<ASCreateView>(GetPortID(), view->GetHandle(), (parent != nullptr) ? parent->m_ServerHandle : -1, view->GetName(), view->m_Frame);
    Flush();
    WaitForReply(-1, RSCreateViewReply.GetID());
    
    view->AttachedToScreen();
    for (Ptr<View> child : view->m_ChildrenList) {
        AddView(ptr_static_cast<View>(child));
    }
    view->AllAttachedToScreen();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::RemoveView(Ptr<View> view)
{
    if (view->m_ServerHandle == -1) {
        printf("ERROR: Application::RemoveView() attempt to remove a view with no server handle\n");
        return false;
    }
    if (view->GetParent() != nullptr) {
        return view->RemoveThis();
    }
    Post<ASDeleteView>(view->m_ServerHandle);
    view->HandleDetachedFromScreen();
    return RemoveHandler(view);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::Flush()
{
    if (m_UsedSendBufferSize > 0) {
        g_AppserverPort.SendMessage(m_ServerHandle, AppserverProtocol::MESSAGE_BUNDLE, m_SendBuffer, m_UsedSendBufferSize);
        m_UsedSendBufferSize = 0;
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
//        g_AppserverPort.SendMessage(m_ServerHandle, AppserverProtocol::MESSAGE_BUNDLE, m_SendBuffer, m_UsedSendBufferSize);
//        m_UsedSendBufferSize = 0;
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

/*bool Application::SlotTransmitRemoteSignal(int id, const void* data, size_t length)
{
    return g_AppserverPort.SendMessage(-1, id, data, length);
}*/

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::SlotCreateViewReply(handler_id clientHandle, handler_id serverHandle)
{
    Ptr<View> view = FindView(clientHandle);
    if (view != nullptr)
    {
        view->SetServerHandle(serverHandle);
    }
}

