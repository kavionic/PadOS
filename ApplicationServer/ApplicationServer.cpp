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
// Created: 17.03.2018 20:45:16

#include "System/Platform.h"

#include <string.h>
#include <fcntl.h>

#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/Protocol.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <ApplicationServer/ServerView.h>
#include <Utils/Utils.h>
#include <GUI/View.h>

#include <System/SystemMessageIDs.h>
#include <DeviceControl/HID.h>

#include "ServerApplication.h"

using namespace os;
using namespace kernel;

static port_id g_AppserverPort = -1;

Ptr<os::DisplayDriver>  ApplicationServer::s_DisplayDriver;
Ptr<SrvBitmap>          ApplicationServer::s_ScreenBitmap;

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

ApplicationServer::ApplicationServer(Ptr<os::DisplayDriver> displayDriver)
    : Looper("Appserver", 10, APPSERVER_MSG_BUFFER_SIZE)
    , m_ReplyPort("appserver_reply", 100)
{
    REGISTER_KERNEL_LOG_CATEGORY(LogCategoryAppServer, kernel::KLogSeverity::INFO_HIGH_VOL);
    
    set_input_event_port(GetPortID());

    s_DisplayDriver = displayDriver;
    s_DisplayDriver->Open();
    s_ScreenBitmap = s_DisplayDriver->GetScreenBitmap();
    m_TopView = ptr_new<ServerView>(ptr_raw_pointer_cast(s_ScreenBitmap), "::topview::", GetScreenFrame(), Point(0.0f, 0.0f), ViewDockType::TopLevelView, 0, 0, FocusKeyboardMode::None, DrawingMode::Copy, Font_e::e_FontLarge, Color(0xffffffff), Color(0xffffffff), Color(0));

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
            
        case int32_t(MessageID::MOUSE_DOWN):
        case int32_t(MessageID::MOUSE_UP):
            m_MouseEventQueue.push(*static_cast<const MotionEvent*>(data));
            return true;
        case int32_t(MessageID::MOUSE_MOVE):
            if (!m_MouseEventQueue.empty() && m_MouseEventQueue.back().EventID == MessageID::MOUSE_MOVE) {
                m_MouseEventQueue.back() = *static_cast<const MotionEvent*>(data);
            } else {
                m_MouseEventQueue.push(*static_cast<const MotionEvent*>(data));
            }
            return true;
        case int32_t(MessageID::KEY_DOWN):
        case int32_t(MessageID::KEY_UP):
        {
            Ptr<ServerView> focusView = GetKeyboardFocus();
            if (focusView != nullptr)
            {
                send_message(focusView->GetClientPort(), focusView->GetClientHandle(), code, data, length, TimeValMicros::FromMilliseconds(500).AsMicroSeconds());
            }
            return true;
        }
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
        const MotionEvent& event = m_MouseEventQueue.front();
        switch(event.EventID)
        {
            case MessageID::MOUSE_DOWN:
            {
                HandleMouseDown(event.ButtonID, event.Position, event);
                break;
            }            
            case MessageID::MOUSE_UP:
            {
                HandleMouseUp(event.ButtonID, event.Position, event);
                break;
            }            
            case MessageID::MOUSE_MOVE:
            {
                HandleMouseMove(event.ButtonID, event.Position, event);
                break;
            }
            default:
                break;
        }
        m_MouseEventQueue.pop();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Rect ApplicationServer::GetScreenFrame()
{
    return Rect(Point(0.0f), Point(s_DisplayDriver->GetResolution()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect ApplicationServer::GetScreenIFrame()
{
    return IRect(IPoint(0), s_DisplayDriver->GetResolution());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DisplayDriver* ApplicationServer::GetDisplayDriver()
{
    return ptr_raw_pointer_cast(s_DisplayDriver);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ServerView> ApplicationServer::GetTopView()
{
    return m_TopView;
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
    if (view == m_KeyboardFocusView) {
        SetKeyboardFocus(ptr_tmp_cast(view), false);
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

void ApplicationServer::HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    m_TopView->HandleMouseDown(button, position, event);
//    if (m_KeyboardFocusView != nullptr) {
//        m_KeyboardFocusView->HandleMouseDown(button, m_KeyboardFocusView->ConvertFromRoot(position), event);
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    Ptr<ServerView> mouseView = GetMouseDownView(button);

    if (mouseView != nullptr)
    {
        mouseView->HandleMouseUp(button, mouseView->ConvertFromRoot(position), event);
        SetMouseDownView(button, nullptr);
    }
    Ptr<ServerView> focusView = GetFocusView(button);
    if (focusView != nullptr && focusView != mouseView)
    {
        focusView->HandleMouseUp(button, focusView->ConvertFromRoot(position), event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
//    Ptr<ServerView> mouseView = GetMouseDownView(button);
//    if (mouseView != nullptr)
//    {
//        mouseView->HandleMouseMove(button, mouseView->ConvertFromRoot(position));
//    }
    Ptr<ServerView> focusView = GetFocusView(button);
    if (focusView != nullptr /*&& focusView != mouseView*/)
    {
        focusView->HandleMouseMove(button, focusView->ConvertFromRoot(position), event);
    }
    if (m_KeyboardFocusView != nullptr && m_KeyboardFocusView != ptr_raw_pointer_cast(focusView)) {
        m_KeyboardFocusView->HandleMouseMove(button, m_KeyboardFocusView->ConvertFromRoot(position), event);
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetKeyboardFocus(Ptr<ServerView> view, bool focus)
{
    if (focus)
    {
        m_KeyboardFocusView = ptr_raw_pointer_cast(view);
        if (m_KeyboardFocusView != nullptr && m_KeyboardFocusView->GetFocusKeyboardMode() != FocusKeyboardMode::None)
        {
            post_to_window_manager<ASWindowManagerEnableVKeyboard>(INVALID_HANDLE, view->ConvertToRoot(view->GetFrame()), m_KeyboardFocusView->GetFocusKeyboardMode() == FocusKeyboardMode::Numeric);
        }
    }
    else
    {
        if (view == m_KeyboardFocusView) {
            m_KeyboardFocusView = nullptr;
            post_to_window_manager<ASWindowManagerDisableVKeyboard>(INVALID_HANDLE);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ServerView> ApplicationServer::GetKeyboardFocus() const
{
    return ptr_tmp_cast(m_KeyboardFocusView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::UpdateViewFocusMode(ServerView* view)
{
    if (view == m_KeyboardFocusView)
    {
        if (view->GetFocusKeyboardMode() != FocusKeyboardMode::None)
        {
            post_to_window_manager<ASWindowManagerEnableVKeyboard>(INVALID_HANDLE, view->ConvertToRoot(view->GetFrame()), view->GetFocusKeyboardMode() == FocusKeyboardMode::Numeric);
        }
        else
        {
            post_to_window_manager<ASWindowManagerDisableVKeyboard>(INVALID_HANDLE);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::PowerLost(bool hasPower)
{
    s_DisplayDriver->PowerLost(hasPower);
}
