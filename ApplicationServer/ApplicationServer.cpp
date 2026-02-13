// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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


#include <string.h>
#include <fcntl.h>

#include <System/AppDefinition.h>
#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/Protocol.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <ApplicationServer/ServerView.h>
#include <ApplicationServer/Drivers/RA8875Driver.h>
#include <Utils/Utils.h>
#include <GUI/View.h>

#include <System/SystemMessageIDs.h>
#include <DeviceControl/HID.h>

#include "ServerApplication.h"


static volatile port_id g_AppserverPort = -1;

Ptr<PDisplayDriver>  ApplicationServer::s_DisplayDriver;
Ptr<PSrvBitmap>          ApplicationServer::s_ScreenBitmap;

port_id p_get_appserver_port()
{
    while(g_AppserverPort == INVALID_HANDLE) {
        snooze_ms(100);
    }
    return g_AppserverPort;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ApplicationServer::ApplicationServer(Ptr<PDisplayDriver> displayDriver)
    : PLooper("Appserver", 10, PAPPSERVER_MSG_BUFFER_SIZE)
    , m_ReplyPort("appserver_reply", 100)
{
    set_input_event_port(GetPortID());

    s_DisplayDriver = displayDriver;
    s_DisplayDriver->Open();
    s_ScreenBitmap = s_DisplayDriver->GetScreenBitmap();
    m_TopView = ptr_new<PServerView>(ptr_raw_pointer_cast(s_ScreenBitmap), "::topview::", GetScreenFrame(), PPoint(0.0f, 0.0f), PViewDockType::TopLevelView, 0, 0, PFocusKeyboardMode::None, PDrawingMode::Copy, 1.0f, PFontID::e_FontLarge, PColor(0xffffffff), PColor(0xffffffff), PColor(0));

    AddHandler(m_TopView);

    RSRegisterApplication.Connect(this, &ApplicationServer::SlotRegisterApplication); 
    
    m_TouchInputDevice = open("/dev/touchscreen/0", O_RDWR);
    if (m_TouchInputDevice != -1)
    {
        HIDIOCTL_SetTargetPort(m_TouchInputDevice, GetPortID());
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ApplicationServer() failed to open touch device.");
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
    switch(code)
    {
        case PAppserverProtocol::REGISTER_APPLICATION:
            RSRegisterApplication.Dispatch(data, length);
            return true;
            
        case int32_t(PMessageID::MOUSE_DOWN):
        case int32_t(PMessageID::MOUSE_UP):
            m_MouseEventQueue.push(*static_cast<const PMotionEvent*>(data));
            return true;
        case int32_t(PMessageID::MOUSE_MOVE):
            if (!m_MouseEventQueue.empty() && m_MouseEventQueue.back().EventID == PMessageID::MOUSE_MOVE) {
                m_MouseEventQueue.back() = *static_cast<const PMotionEvent*>(data);
            } else {
                m_MouseEventQueue.push(*static_cast<const PMotionEvent*>(data));
            }
            return true;
        case int32_t(PMessageID::KEY_DOWN):
        case int32_t(PMessageID::KEY_UP):
        {
            Ptr<PServerView> focusView = GetKeyboardFocus();
            if (focusView != nullptr)
            {
                message_port_send_timeout_ns(focusView->GetClientPort(), focusView->GetClientHandle(), code, data, length, TimeValNanos::FromMilliseconds(500).AsNanoseconds());
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
    while(!m_MouseEventQueue.empty())
    {
        const PMotionEvent& event = m_MouseEventQueue.front();
        switch(event.EventID)
        {
            case PMessageID::MOUSE_DOWN:
            {
                HandleMouseDown(event.ButtonID, event.Position, event);
                break;
            }            
            case PMessageID::MOUSE_UP:
            {
                HandleMouseUp(event.ButtonID, event.Position, event);
                break;
            }            
            case PMessageID::MOUSE_MOVE:
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

PRect ApplicationServer::GetScreenFrame()
{
    return PRect(PPoint(0.0f), PPoint(s_DisplayDriver->GetResolution()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIRect ApplicationServer::GetScreenIFrame()
{
    return PIRect(PIPoint(0), s_DisplayDriver->GetResolution());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayDriver* ApplicationServer::GetDisplayDriver()
{
    return ptr_raw_pointer_cast(s_DisplayDriver);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PServerView> ApplicationServer::GetTopView()
{
    return m_TopView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ApplicationServer::RegisterView(Ptr<PServerView> view)
{
    return AddHandler(view);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PServerView> ApplicationServer::FindView(handler_id handle) const
{
    return ptr_static_cast<PServerView>(FindHandler(handle));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ViewDestructed(PServerView* view)
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

void ApplicationServer::SlotRegisterApplication(port_id replyPort, port_id clientPort, const PString& name)
{
    Ptr<ServerApplication> app = ptr_new<ServerApplication>(this, name, clientPort);
    
    AddHandler(app);

    MsgRegisterApplicationReply reply;
    reply.m_ServerHandle = app->GetHandle();
    
    const PErrorCode result = message_port_send_timeout_ns(replyPort, -1, PAppserverProtocol::REGISTER_APPLICATION_REPLY, &reply, sizeof(reply), 0);
    if (result != PErrorCode::Success) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::SlotRegisterApplication() failed to send message: {}", strerror(std::to_underlying(result)));
    }
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    m_TopView->HandleMouseDown(button, position, event);
//    if (m_KeyboardFocusView != nullptr) {
//        m_KeyboardFocusView->HandleMouseDown(button, m_KeyboardFocusView->ConvertFromRoot(position), event);
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    Ptr<PServerView> mouseView = GetMouseDownView(button);

    if (mouseView != nullptr)
    {
        mouseView->HandleMouseUp(button, mouseView->ConvertFromRoot(position), event);
        SetMouseDownView(button, nullptr);
    }
    Ptr<PServerView> focusView = GetFocusView(button);
    if (focusView != nullptr && focusView != mouseView)
    {
        focusView->HandleMouseUp(button, focusView->ConvertFromRoot(position), event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
//    Ptr<ServerView> mouseView = GetMouseDownView(button);
//    if (mouseView != nullptr)
//    {
//        mouseView->HandleMouseMove(button, mouseView->ConvertFromRoot(position));
//    }
    Ptr<PServerView> focusView = GetFocusView(button);
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

void ApplicationServer::SetMouseDownView(PMouseButton button, Ptr<PServerView> view)
{
    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

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

Ptr<PServerView> ApplicationServer::GetMouseDownView(PMouseButton button) const
{
    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseViewMap.find(deviceID);
    if (iterator != m_MouseViewMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetFocusView(PMouseButton button, Ptr<PServerView> view, bool focus)
{
    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

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

Ptr<PServerView> ApplicationServer::GetFocusView(PMouseButton button) const
{
    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseFocusMap.find(deviceID);
    if (iterator != m_MouseFocusMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetKeyboardFocus(Ptr<PServerView> view, bool focus)
{
    if (focus)
    {
        m_KeyboardFocusView = ptr_raw_pointer_cast(view);
        if (m_KeyboardFocusView != nullptr && m_KeyboardFocusView->GetFocusKeyboardMode() != PFocusKeyboardMode::None)
        {
            p_post_to_window_manager<ASWindowManagerEnableVKeyboard>(INVALID_HANDLE, view->ConvertToRoot(view->GetFrame()), m_KeyboardFocusView->GetFocusKeyboardMode() == PFocusKeyboardMode::Numeric);
        }
    }
    else
    {
        if (view == m_KeyboardFocusView) {
            m_KeyboardFocusView = nullptr;
            p_post_to_window_manager<ASWindowManagerDisableVKeyboard>(INVALID_HANDLE);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PServerView> ApplicationServer::GetKeyboardFocus() const
{
    return ptr_tmp_cast(m_KeyboardFocusView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::UpdateViewFocusMode(PServerView* view)
{
    if (view == m_KeyboardFocusView)
    {
        if (view->GetFocusKeyboardMode() != PFocusKeyboardMode::None)
        {
            p_post_to_window_manager<ASWindowManagerEnableVKeyboard>(INVALID_HANDLE, view->ConvertToRoot(view->GetFrame()), view->GetFocusKeyboardMode() == PFocusKeyboardMode::Numeric);
        }
        else
        {
            p_post_to_window_manager<ASWindowManagerDisableVKeyboard>(INVALID_HANDLE);
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int appserver_main(int argc, char* argv[])
{
    RA8875DriverParameters driverConfig;
    if (argc > 1) {
        Pjson::parse(argv[1]).get_to(driverConfig);
    }
    ApplicationServer* applicationServer = new ApplicationServer(ptr_new<RA8875Driver>(driverConfig));
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCat_General, "Application server started.");
    applicationServer->Adopt();
    return 0;
}

static PAppDefinition g_AppServerAppDef("appserver", "Server providing GUI and other services to applications.", appserver_main);
