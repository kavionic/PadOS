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
// Created: 17.03.2018 20:45:16

#include "sam.h"

#include <fcntl.h>

#include "ApplicationServer.h"
#include "ServerApplication.h"
#include "ServerView.h"
#include "Protocol.h"
#include "System/GUI/View.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"
#include "System/SystemMessageIDs.h"
#include "DeviceControl/FT5x0x.h"

using namespace os;
using namespace kernel;

MessagePort os::g_AppserverPort(-1, false);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ApplicationServer::ApplicationServer() : Looper("appserver", 100, APPSERVER_MSG_BUFFER_SIZE)
{
    m_TopView = ptr_new<ServerView>("::topview::");
    m_TopView->SetFrame(GetScreenFrame());
    m_TopView->SetFrame(GetScreenFrame());

    AddHandler(m_TopView);

    RSRegisterApplication.Connect(this, &ApplicationServer::SlotRegisterApplication); 
    
    m_TouchInputDevice = Kernel::OpenFile("/dev/ft5x0x/0", O_RDWR);
    if (m_TouchInputDevice != -1)
    {
        FT5x0xIOCTL_SetTargetPort(m_TouchInputDevice, GetPortID());
    }
    else
    {
        printf("ERROR: ApplicationServer::ApplicationServer() failed to open touch device\n");
    }
    g_AppserverPort = GetPort();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ApplicationServer::~ApplicationServer()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ApplicationServer::HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    bool result = false;
    

    switch(code)
    {
        case AppserverProtocol::REGISTER_APPLICATION:
            RSRegisterApplication.Dispatch(data, length);
            result = true;
            break;        
        case MessageID::MOUSE_DOWN:
        {
            const MsgMouseEvent* msg = static_cast<const MsgMouseEvent*>(data);
            HandleMouseDown(msg->ButtonID, msg->Position);
            result = true;
            break;
        }            
        case MessageID::MOUSE_UP:
        {
            const MsgMouseEvent* msg = static_cast<const MsgMouseEvent*>(data);
            HandleMouseUp(msg->ButtonID, msg->Position);
            result = true;
            break;
        }            
        case MessageID::MOUSE_MOVE:
        {
            const MsgMouseEvent* msg = static_cast<const MsgMouseEvent*>(data);
            HandleMouseMove(msg->ButtonID, msg->Position);
            result = true;
            break;
        }
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect ApplicationServer::GetScreenFrame()
{
    return Rect(Point(0.0f), Point(GfxDriver::Instance.GetResolution() - IPoint(1, 1)));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect ApplicationServer::GetScreenIFrame()
{
    return IRect(IPoint(0), GfxDriver::Instance.GetResolution() - IPoint(1, 1));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ApplicationServer::RegisterView(Ptr<ServerView> view)
{
    return AddHandler(view);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ServerView> ApplicationServer::FindView(handler_id handle) const
{
    return ptr_static_cast<ServerView>(FindHandler(handle));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SlotRegisterApplication(port_id clientPort, const String& name)
{
    Ptr<ServerApplication> app = ptr_new<ServerApplication>(this, name, clientPort);
    
    AddHandler(app);

    ASRegisterApplicationReply::Sender::Emit(MessagePort(clientPort), -1, app->GetHandle());    
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseDown(MouseButton_e button, const Point& position)
{
    m_TopView->HandleMouseDown(button, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseUp(MouseButton_e button, const Point& position)
{
    if (m_MouseDownView.Lock() != nullptr)
    {
        m_MouseDownView.Lock()->HandleMouseUp(button, position - m_MouseDownView.Lock()->m_ScreenPos - m_MouseDownView.Lock()->m_ScrollOffset);
    }
    if (m_FocusView.Lock() != nullptr && m_FocusView != m_MouseDownView)
    {
        m_FocusView.Lock()->HandleMouseUp(button, position - m_FocusView.Lock()->m_ScreenPos - m_FocusView.Lock()->m_ScrollOffset);
    }
    m_MouseDownView = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseMove(MouseButton_e button, const Point& position)
{
    if (m_MouseDownView.Lock() != nullptr)
    {
        m_MouseDownView.Lock()->HandleMouseMove(button, position - m_MouseDownView.Lock()->m_ScreenPos - m_MouseDownView.Lock()->m_ScrollOffset);
    }
    if (m_FocusView.Lock() != nullptr && m_FocusView != m_MouseDownView)
    {
        m_FocusView.Lock()->HandleMouseMove(button, position - m_FocusView.Lock()->m_ScreenPos - m_FocusView.Lock()->m_ScrollOffset);
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetMouseDownView( Ptr<ServerView> view )
{
    m_MouseDownView = view;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetFocusView( Ptr<ServerView> view )
{
    m_FocusView = view;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ServerView> ApplicationServer::GetFocusView() const
{
    return m_FocusView.Lock();
}
