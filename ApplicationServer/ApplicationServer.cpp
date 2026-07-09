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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <System/AppDefinition.h>
#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/Protocol.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <ApplicationServer/ServerView.h>
#include <ApplicationServer/Drivers/RA8875GfxDriver.h>
#include <Utils/Utils.h>
#include <GUI/View.h>

#include <System/SystemMessageIDs.h>

#include "ServerApplication.h"


static volatile port_id g_AppserverPort = -1;

namespace
{

PMotionToolType GetDefaultMotionToolType(const PMotionEvent& motionEvent)
{
    switch (motionEvent.ClassID)
    {
        case PInputClass::Mouse:
            return PMotionToolType::Mouse;

        case PInputClass::TouchScreen:
            return PMotionToolType::Finger;

        default:
            return GetMotionToolType(motionEvent.ButtonID);
    }
}

PMouseButton GetPointerButton(const PMotionEvent& motionEvent)
{
    switch (motionEvent.EventID)
    {
        case PInputEventID::MouseDown:
        case PInputEventID::MouseUp:
            return motionEvent.ButtonID;

        case PInputEventID::TouchDown:
        case PInputEventID::TouchUp:
            return PMouseButton::Left;

        default:
            return PMouseButton::None;
    }
}

PPointerEvent CreatePointerEvent(const PMotionEvent& motionEvent, PPointerID pointerID, PMouseButton button, PPointerButtonMask buttons)
{
    PPointerEvent pointerEvent;
    pointerEvent.PointerID = pointerID;
    pointerEvent.Timestamp = motionEvent.Timestamp;
    pointerEvent.ToolType = GetDefaultMotionToolType(motionEvent);
    pointerEvent.Button = button;
    pointerEvent.Buttons = buttons;
    pointerEvent.Pressure = (buttons != PPointerButtonMaskNone) ? 1.0f : 0.0f;
    pointerEvent.Position = motionEvent.Position;
    return pointerEvent;
}

} // namespace

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
    m_TopView = ptr_new<PServerView>(
        ptr_raw_pointer_cast(s_ScreenBitmap),
        "::topview::",
        GetScreenFrame(),
        PPoint(0.0f, 0.0f),
        PViewDockType::TopLevelView,
        0,
        0,
        PFocusKeyboardMode::None,
        PDrawingMode::Copy,
        1.0f,
        PCapStyle::Square,
        PJointStyle::Bevel,
        4.0f,   // Miter limit
        std::vector<float>{},     // Dash pattern
        0.0f,   // Dash offset
        PFontID::e_FontLarge,
        PColor(0xffffffff),
        PColor(0xffffffff),
        PColor(0)
    );

    AddHandler(m_TopView);

    RSRegisterApplication.Connect(this, &ApplicationServer::SlotRegisterApplication); 
    
    m_TouchInputDevice = open("/dev/input/touch", O_RDONLY | O_NONBLOCK);
    if (m_TouchInputDevice != -1)
    {
        if (!GetWaitGroup().AddFile(m_TouchInputDevice)) {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ApplicationServer() failed to add touch input device to wait group: {}", strerror(errno));
        }
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ApplicationServer() failed to open touch input device: {}", strerror(errno));
    }
    g_AppserverPort = GetPortID();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ApplicationServer::~ApplicationServer()
{
    if (m_TouchInputDevice != -1)
    {
        GetWaitGroup().RemoveFile(m_TouchInputDevice);
        close(m_TouchInputDevice);
        m_TouchInputDevice = -1;
    }
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
    ReadInputEvents();

    while(!m_MotionEventQueue.empty())
    {
        const PMotionEvent& motionEvent = m_MotionEventQueue.front();
        const PPointerID pointerID = GetPointerID(motionEvent.ButtonID);
        const PMouseButton button = GetPointerButton(motionEvent);
        const PPointerButtonMask buttons = UpdatePointerButtonState(pointerID, motionEvent.EventID, button);
        PPointerEvent pointerEvent = CreatePointerEvent(motionEvent, pointerID, button, buttons);
        switch(motionEvent.EventID)
        {
            case PInputEventID::MouseDown:
            case PInputEventID::TouchDown:
            {
                HandlePointerDown(pointerID, pointerEvent.Position, pointerEvent);
                break;
            }            
            case PInputEventID::MouseUp:
            case PInputEventID::TouchUp:
            {
                HandlePointerUp(pointerID, pointerEvent.Position, pointerEvent);
                break;
            }            
            case PInputEventID::MouseMove:
            case PInputEventID::TouchMove:
            {
                HandlePointerMove(pointerID, pointerEvent.Position, pointerEvent);
                break;
            }
            default:
                break;
        }
        m_MotionEventQueue.pop();
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
    for (auto i = m_PointerViewMap.begin(); i != m_PointerViewMap.end(); )
    {
        if (i->second == view) {
            i = m_PointerViewMap.erase(i);
        } else {
            ++i;
        }
    }
    for (auto i = m_PointerFocusMap.begin(); i != m_PointerFocusMap.end(); )
    {
        if (i->second == view) {
            i = m_PointerFocusMap.erase(i);
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

void ApplicationServer::ReadInputEvents()
{
    if (m_TouchInputDevice != -1)
    {
        for (;;)
        {
            const ssize_t bytesRead = read(m_TouchInputDevice, m_InputEventBuffer.data(), m_InputEventBuffer.size());

            if (bytesRead > 0)
            {
                const uint8_t* currentEventData = m_InputEventBuffer.data();
                size_t remainingLength = size_t(bytesRead);

                while (remainingLength > 0)
                {
                    if (remainingLength < sizeof(PInputEvent)) {
                        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received truncated input event header: {}", remainingLength);
                        break;
                    }

                    const PInputEvent* eventHeader = reinterpret_cast<const PInputEvent*>(currentEventData);
                    if (eventHeader->EventSize < sizeof(PInputEvent) || eventHeader->EventSize > remainingLength) {
                        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received invalid input event size: {} of {}", eventHeader->EventSize, remainingLength);
                        break;
                    }

                    if (eventHeader->EventType == PInputEventType::MotionEvent)
                    {
                        if (eventHeader->EventSize < sizeof(PMotionEvent)) {
                            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received truncated motion event: {}", eventHeader->EventSize);
                        } else {
                            QueueMotionEvent(*reinterpret_cast<const PMotionEvent*>(currentEventData));
                        }
                    }

                    currentEventData += eventHeader->EventSize;
                    remainingLength -= eventHeader->EventSize;
                }
                continue;
            }
            if (bytesRead < 0)
            {
                if (errno == EINTR) {
                    continue;
                }
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() failed to read touch input: {}", strerror(errno));
                }
                break;
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::QueueMotionEvent(const PMotionEvent& event)
{
    const bool isMoveEvent = event.EventID == PInputEventID::MouseMove || event.EventID == PInputEventID::TouchMove;

    if (isMoveEvent && !m_MotionEventQueue.empty())
    {
        PMotionEvent& queuedEvent = m_MotionEventQueue.back();
        if (queuedEvent.EventID == event.EventID && queuedEvent.SourceID == event.SourceID && queuedEvent.ButtonID == event.ButtonID)
        {
            queuedEvent = event;
            return;
        }
    }
    m_MotionEventQueue.push(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerButtonMask ApplicationServer::UpdatePointerButtonState(PPointerID pointerID, PInputEventID eventID, PMouseButton button)
{
    PPointerButtonMask buttons = PPointerButtonMaskNone;
    auto iterator = m_PointerButtonsMap.find(pointerID);
    if (iterator != m_PointerButtonsMap.end()) {
        buttons = iterator->second;
    }

    const PPointerButtonMask buttonMask = GetPointerButtonMask(button);
    switch (eventID)
    {
        case PInputEventID::MouseDown:
        case PInputEventID::TouchDown:
            buttons |= buttonMask;
            break;

        case PInputEventID::MouseUp:
        case PInputEventID::TouchUp:
            buttons &= ~buttonMask;
            break;

        default:
            break;
    }

    if (buttons != PPointerButtonMaskNone) {
        m_PointerButtonsMap[pointerID] = buttons;
    } else if (iterator != m_PointerButtonsMap.end()) {
        m_PointerButtonsMap.erase(iterator);
    }
    return buttons;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandlePointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event)
{
    m_TopView->HandlePointerDown(pointerID, position, event);
//    if (m_KeyboardFocusView != nullptr) {
//        m_KeyboardFocusView->HandlePointerDown(pointerID, m_KeyboardFocusView->ConvertFromRoot(position), event);
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandlePointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event)
{
    Ptr<PServerView> pointerDownView = GetPointerDownView(pointerID);

    if (pointerDownView != nullptr)
    {
        pointerDownView->HandlePointerUp(pointerID, pointerDownView->ConvertFromRoot(position), event);
        SetPointerDownView(pointerID, nullptr);
    }
    Ptr<PServerView> focusView = GetFocusView(pointerID);
    if (focusView != nullptr && focusView != pointerDownView)
    {
        focusView->HandlePointerUp(pointerID, focusView->ConvertFromRoot(position), event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandlePointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event)
{
//    Ptr<ServerView> pointerDownView = GetPointerDownView(pointerID);
//    if (mouseView != nullptr)
//    {
//        pointerDownView->HandlePointerMove(pointerID, pointerDownView->ConvertFromRoot(position));
//    }
    Ptr<PServerView> focusView = GetFocusView(pointerID);
    if (focusView != nullptr /*&& focusView != mouseView*/)
    {
        focusView->HandlePointerMove(pointerID, focusView->ConvertFromRoot(position), event);
    }
    if (m_KeyboardFocusView != nullptr && m_KeyboardFocusView != ptr_raw_pointer_cast(focusView)) {
        m_KeyboardFocusView->HandlePointerMove(pointerID, m_KeyboardFocusView->ConvertFromRoot(position), event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetPointerDownView(PPointerID pointerID, Ptr<PServerView> view)
{
    if (view != nullptr)
    {
        m_PointerViewMap[pointerID] = ptr_raw_pointer_cast(view);
    }
    else
    {
        auto iterator = m_PointerViewMap.find(pointerID);
        if (iterator != m_PointerViewMap.end()) {
            m_PointerViewMap.erase(iterator);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PServerView> ApplicationServer::GetPointerDownView(PPointerID pointerID) const
{
    auto iterator = m_PointerViewMap.find(pointerID);
    if (iterator != m_PointerViewMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetFocusView(PPointerID pointerID, Ptr<PServerView> view, bool focus)
{
    if (view != nullptr)
    {
        if (focus)
        {
            m_PointerFocusMap[pointerID] = ptr_raw_pointer_cast(view);
        }
        else
        {
            auto iterator = m_PointerFocusMap.find(pointerID);
            if (iterator != m_PointerFocusMap.end() && iterator->second == ptr_raw_pointer_cast(view)) {
                m_PointerFocusMap.erase(iterator);
            }
        }
    }
    else
    {
        auto iterator = m_PointerFocusMap.find(pointerID);
        if (iterator != m_PointerFocusMap.end()) {
            m_PointerFocusMap.erase(iterator);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PServerView> ApplicationServer::GetFocusView(PPointerID pointerID) const
{
    auto iterator = m_PointerFocusMap.find(pointerID);
    if (iterator != m_PointerFocusMap.end()) {
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
    RA8875GfxDriverParameters driverConfig;
    if (argc > 1) {
        Pjson::parse(argv[1]).get_to(driverConfig);
    }
    ApplicationServer* applicationServer = new ApplicationServer(ptr_new<RA8875GfxDriver>(driverConfig));
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCat_General, "Application server started.");
    applicationServer->Adopt();
    return 0;
}

static PAppDefinition g_AppServerAppDef("appserver", "Server providing GUI and other services to applications.", appserver_main);
