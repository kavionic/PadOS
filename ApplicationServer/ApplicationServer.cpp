// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <string.h>
#include <fcntl.h>

#include "ApplicationServer.h"
#include "ServerApplication.h"
#include "ServerView.h"
#include "Protocol.h"
#include "GUI/View.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"
#include "System/SystemMessageIDs.h"
#include "DeviceControl/HID.h"

using namespace os;
using namespace kernel;

static port_id g_AppserverPort = -1;

namespace os
{
    port_id get_appserver_port()
    {
        return g_AppserverPort;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ApplicationServer::ApplicationServer() : Looper("appserver", 10, APPSERVER_MSG_BUFFER_SIZE), m_ReplyPort("appserver_reply", 100) //, m_WindowsManagerPort(-1, false)
{
    m_TopView = ptr_new<ServerView>("::topview::", GetScreenFrame(), Point(0.0f, 0.0f), 0, 0, Color(0xffffffff), Color(0xffffffff), Color(0));

    AddHandler(m_TopView);

    RSRegisterApplication.Connect(this, &ApplicationServer::SlotRegisterApplication); 
    
    m_TouchInputDevice = FileIO::Open("/dev/touchscreen/0", O_RDWR);
    if (m_TouchInputDevice != -1)
    {
        HIDIOCTL_SetTargetPort(m_TouchInputDevice, GetPortID());
    }
    else
    {
        printf("ERROR: ApplicationServer::ApplicationServer() failed to open touch device\n");
    }
    g_AppserverPort = GetPortID();
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
    DEBUG_TRACK_FUNCTION();
    switch(code)
    {
        case AppserverProtocol::REGISTER_APPLICATION:
            RSRegisterApplication.Dispatch(data, length);
            return true;
            
        case MessageID::MOUSE_DOWN:
        case MessageID::MOUSE_UP:
            m_MouseEventQueue.push(*static_cast<const MsgMouseEvent*>(data));
            return true;
        case MessageID::MOUSE_MOVE:
            if (!m_MouseEventQueue.empty() && m_MouseEventQueue.back().EventID == MessageID::MOUSE_MOVE) {
                m_MouseEventQueue.back() = *static_cast<const MsgMouseEvent*>(data);
            } else {
                m_MouseEventQueue.push(*static_cast<const MsgMouseEvent*>(data));
            }
            return true;
            
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::Idle()
{
    DEBUG_TRACK_FUNCTION();
    while(!m_MouseEventQueue.empty())
    {
        const MsgMouseEvent& event = m_MouseEventQueue.front();
        switch(event.EventID)
        {
            case MessageID::MOUSE_DOWN:
            {
                HandleMouseDown(event.ButtonID, event.Position);
                break;
            }            
            case MessageID::MOUSE_UP:
            {
                HandleMouseUp(event.ButtonID, event.Position);
                break;
            }            
            case MessageID::MOUSE_MOVE:
            {
                HandleMouseMove(event.ButtonID, event.Position);
                break;
            }
        }
        m_MouseEventQueue.pop();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect ApplicationServer::GetScreenFrame()
{
    return Rect(Point(0.0f), Point(GfxDriver::Instance.GetResolution()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect ApplicationServer::GetScreenIFrame()
{
    return IRect(IPoint(0), GfxDriver::Instance.GetResolution());
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

void ApplicationServer::ViewDestructed(ServerView* view)
{
    for (auto i = m_MouseViewMap.begin(); i != m_MouseViewMap.end(); )
    {
        if (i->second == view) {
            i = m_MouseViewMap.erase(i);
        } else {
            ++i;
        }
    }
    for (auto i = m_MouseFocusMap.begin(); i != m_MouseFocusMap.end(); )
    {
        if (i->second == view) {
            i = m_MouseFocusMap.erase(i);
        } else {
            ++i;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SlotRegisterApplication(port_id replyPort, port_id clientPort, const String& name)
{
    Ptr<ServerApplication> app = ptr_new<ServerApplication>(this, name, clientPort);
    
    AddHandler(app);

    MsgRegisterApplicationReply reply;
    reply.m_ServerHandle = app->GetHandle();
    
    if (send_message(replyPort, -1, AppserverProtocol::REGISTER_APPLICATION_REPLY, &reply, sizeof(reply), 0) < 0) {
        printf("ERROR: ApplicationServer::SlotRegisterApplication() failed to send message: %s\n", strerror(get_last_error()));
    }
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
    Ptr<ServerView> mouseView = GetMouseDownView(button);

    if (mouseView != nullptr)
    {
        mouseView->HandleMouseUp(button, position - mouseView->m_ScreenPos - mouseView->m_ScrollOffset);
        SetMouseDownView(button, nullptr);
    }
    Ptr<ServerView> focusView = GetFocusView(button);
    if (focusView != nullptr && focusView != mouseView)
    {
        focusView->HandleMouseUp(button, position - focusView->m_ScreenPos - focusView->m_ScrollOffset);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseMove(MouseButton_e button, const Point& position)
{
//    Ptr<ServerView> mouseView = GetMouseDownView(button);
//    if (mouseView != nullptr)
//    {
//        mouseView->HandleMouseMove(button, position - mouseView->m_ScreenPos - mouseView->m_ScrollOffset);
//    }
    Ptr<ServerView> focusView = GetFocusView(button);
    if (focusView != nullptr /*&& focusView != mouseView*/)
    {
        focusView->HandleMouseMove(button, position - focusView->m_ScreenPos - focusView->m_ScrollOffset);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetMouseDownView(MouseButton_e button, Ptr<ServerView> view)
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    if (view != nullptr)
    {
        m_MouseViewMap[deviceID] = ptr_raw_pointer_cast(view);
    }
    else
    {
        auto iterator = m_MouseViewMap.find(deviceID);
        if (iterator != m_MouseViewMap.end()) {
            m_MouseViewMap.erase(iterator);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ServerView> ApplicationServer::GetMouseDownView(MouseButton_e button) const
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseViewMap.find(deviceID);
    if (iterator != m_MouseViewMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetFocusView(MouseButton_e button, Ptr<ServerView> view, bool focus)
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    if (view != nullptr)
    {
        if (focus)
        {
            m_MouseFocusMap[deviceID] = ptr_raw_pointer_cast(view);
        }
        else
        {
            auto iterator = m_MouseFocusMap.find(deviceID);
            if (iterator != m_MouseFocusMap.end() && iterator->second == ptr_raw_pointer_cast(view)) {
                m_MouseFocusMap.erase(iterator);
            }
        }
    }
    else
    {
        auto iterator = m_MouseFocusMap.find(deviceID);
        if (iterator != m_MouseFocusMap.end()) {
            m_MouseFocusMap.erase(iterator);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ServerView> ApplicationServer::GetFocusView(MouseButton_e button) const
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseFocusMap.find(deviceID);
    if (iterator != m_MouseFocusMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}
