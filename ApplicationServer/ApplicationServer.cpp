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


#include <algorithm>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <string_view>
#include <unistd.h>
#include <utility>

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

Ptr<PDisplayDriver>  ApplicationServer::s_DisplayDriver;
Ptr<PSrvBitmap>          ApplicationServer::s_ScreenBitmap;

namespace
{
    constexpr PMouseCursorPixel GetMouseCursorPixel(char value)
    {
        switch (value)
        {
            case '1':
                return PMouseCursorPixel::Color1;
            case '2':
                return PMouseCursorPixel::Color2;
            case 'i':
            case 'I':
                return PMouseCursorPixel::Invert;
            default:
                return PMouseCursorPixel::Transparent;
        }
    }

    template<size_t Width, size_t Height>
    constexpr std::array<PMouseCursorPixel, Width * Height> MakeMouseCursorRaster(const std::array<std::string_view, Height>& rows)
    {
        std::array<PMouseCursorPixel, Width * Height> raster{};

        for (size_t y = 0; y < Height; ++y)
        {
            const size_t rowWidth = std::min(Width, rows[y].size());
            for (size_t x = 0; x < rowWidth; ++x) {
                raster[y * Width + x] = GetMouseCursorPixel(rows[y][x]);
            }
        }
        return raster;
    }

    static constexpr int32_t STANDARD_MOUSE_CURSOR_WIDTH = 32;
    static constexpr int32_t STANDARD_MOUSE_CURSOR_HEIGHT = 32;

    static constexpr auto s_PointerCursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "11..............................",
            "111.............................",
            "1111............................",
            "11111...........................",
            "112111..........................",
            "1122111.........................",
            "11222111........................",
            "112222111.......................",
            "1122222111......................",
            "11222222111.....................",
            "112222222111....................",
            "1122222222111...................",
            "11222222222111..................",
            "112222222222111.................",
            "1122222222222111................",
            "11222222222222111...............",
            "112222222222222111..............",
            "1122222222222222111.............",
            "11222222221111111111............",
            "112222222221111111111...........",
            "11222211222211..................",
            "112221111122211.................",
            "1122111.1122211.................",
            "112111...1122211................",
            "11111....1122211................",
            "1111......11222211..............",
            "111.......11222211..............",
            "11.........11222211.............",
            "...........11222211.............",
            "............11222211............",
            "............1112211.............",
            "..............1111.............."
        }
    );

    static constexpr auto s_TextSelectCursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "................................",
            "................................",
            "....111111111111111111111111....",
            "....111111111111111111111111....",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "..............1111..............",
            "....111111111111111111111111....",
            "....111111111111111111111111....",
            "................................",
            "................................"
        }
    );

    static constexpr auto s_BusyCursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "................................",
            "................................",
            "...11111111111111111111111111...",
            "...12222222222222222222222221...",
            "....122222222222222222222221....",
            ".....1222222222222222222221.....",
            "......12222222222222222221......",
            ".......122222222222222221.......",
            "........1222222222222221........",
            ".........12222222222221.........",
            "..........122222222221..........",
            "...........1222222221...........",
            "............12222221............",
            ".............122221.............",
            "..............1221..............",
            "...............11...............",
            "...............11...............",
            "..............1221..............",
            ".............12..21.............",
            "............12....21............",
            "...........12......21...........",
            "..........12........21..........",
            ".........12..........21.........",
            "........12............21........",
            ".......12..............21.......",
            "......12................21......",
            ".....12..................21.....",
            "....12....................21....",
            "...12222222222222222222222221...",
            "...11111111111111111111111111...",
            "................................",
            "................................"
        }
    );

    static constexpr auto s_CrosshairCursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "11111111111111111111111111111111",
            "11111111111111111111111111111111",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11...............",
            "...............11..............."
        }
    );

    static constexpr auto s_ResizeHorizontalCursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "1..............................1",
            "11............................11",
            "121..........................121",
            "1221........................1221",
            "12221......................12221",
            "12222111111111111111111111222221",
            "12222222222222222222222222222221",
            "12222111111111111111111111222221",
            "12221......................12221",
            "1221........................1221",
            "121..........................121",
            "11............................11",
            "1..............................1",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................",
            "................................"
        }
    );

    static constexpr auto s_ResizeVerticalCursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "...............11...............",
            "..............1221..............",
            ".............122221.............",
            "............12222221............",
            "...........1222222221...........",
            "..........122222222221..........",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..............1221..............",
            "..........122222222221..........",
            "...........1222222221...........",
            "............12222221............",
            ".............122221.............",
            "..............1221..............",
            "...............11..............."
        }
    );

    static constexpr auto s_ResizeDiagonalNWSECursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "11..............................",
            "111.............................",
            "11221...........................",
            ".11221..........................",
            "..11221.........................",
            "...11221........................",
            "....11221.......................",
            ".....11221......................",
            "......11221.....................",
            ".......11221....................",
            "........11221...................",
            ".........11221..................",
            "..........11221.................",
            "...........11221................",
            "............11221...............",
            ".............11221..............",
            "..............11221.............",
            "...............11221............",
            "................11221...........",
            ".................11221..........",
            "..................11221.........",
            "...................11221........",
            "....................11221.......",
            ".....................11221......",
            "......................11221.....",
            ".......................11221....",
            "........................11221...",
            ".........................11221..",
            "..........................11221.",
            "............................121.",
            ".............................111",
            "..............................11"
        }
    );

    static constexpr auto s_ResizeDiagonalNESWCursorRaster = MakeMouseCursorRaster<STANDARD_MOUSE_CURSOR_WIDTH, STANDARD_MOUSE_CURSOR_HEIGHT>(
        std::array<std::string_view, STANDARD_MOUSE_CURSOR_HEIGHT>{
            "..............................11",
            ".............................111",
            "...........................12211",
            "..........................12211.",
            ".........................12211..",
            "........................12211...",
            ".......................12211....",
            "......................12211.....",
            ".....................12211......",
            "....................12211.......",
            "...................12211........",
            "..................12211.........",
            ".................12211..........",
            "................12211...........",
            "...............12211............",
            "..............12211.............",
            ".............12211..............",
            "............12211...............",
            "...........12211................",
            "..........12211.................",
            ".........12211..................",
            "........12211...................",
            ".......12211....................",
            "......12211.....................",
            ".....12211......................",
            "....12211.......................",
            "...12211........................",
            "..12211.........................",
            ".12211..........................",
            ".121............................",
            "111.............................",
            "11.............................."
        }
    );

    template<size_t PixelCount>
    PMouseCursorBitmap MakeStandardMouseCursorBitmap(const std::array<PMouseCursorPixel, PixelCount>& raster, PIPoint hotSpot)
    {
        return PMouseCursorBitmap{
            .Width = STANDARD_MOUSE_CURSOR_WIDTH,
            .Height = STANDARD_MOUSE_CURSOR_HEIGHT,
            .HotSpot = hotSpot,
            .Color1 = PColor(0xff000000),
            .Color2 = PColor(0xffffffff),
            .Raster = std::span<const PMouseCursorPixel>(raster.data(), raster.size())
        };
    }

    bool GetStandardMouseCursorBitmap(PMouseCursorID cursorID, PMouseCursorBitmap& outBitmap, bool& outVisible)
    {
        if (!IsStandardMouseCursorID(cursorID)) {
            return false;
        }

        outVisible = true;

        switch (PStandardMouseCursor(cursorID))
        {
            case PStandardMouseCursor::Pointer:
                outBitmap = MakeStandardMouseCursorBitmap(s_PointerCursorRaster, PIPoint(0, 0));
                return true;
            case PStandardMouseCursor::TextSelect:
                outBitmap = MakeStandardMouseCursorBitmap(s_TextSelectCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::Busy:
                outBitmap = MakeStandardMouseCursorBitmap(s_BusyCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::Crosshair:
                outBitmap = MakeStandardMouseCursorBitmap(s_CrosshairCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::ResizeHorizontal:
                outBitmap = MakeStandardMouseCursorBitmap(s_ResizeHorizontalCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::ResizeVertical:
                outBitmap = MakeStandardMouseCursorBitmap(s_ResizeVerticalCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::ResizeDiagonalNWSE:
                outBitmap = MakeStandardMouseCursorBitmap(s_ResizeDiagonalNWSECursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::ResizeDiagonalNESW:
                outBitmap = MakeStandardMouseCursorBitmap(s_ResizeDiagonalNESWCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::Move:
                outBitmap = MakeStandardMouseCursorBitmap(s_CrosshairCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::Hand:
                outBitmap = MakeStandardMouseCursorBitmap(s_PointerCursorRaster, PIPoint(0, 0));
                return true;
            case PStandardMouseCursor::NotAllowed:
                outBitmap = MakeStandardMouseCursorBitmap(s_BusyCursorRaster, PIPoint(15, 15));
                return true;
            case PStandardMouseCursor::Hidden:
                outVisible = false;
                outBitmap = PMouseCursorBitmap{};
                return true;
            case PStandardMouseCursor::Count:
                break;
        }
        return false;
    }
}

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

    m_MousePosition = GetScreenFrame().Center();
    s_DisplayDriver->SetMousePos(PIPoint(m_MousePosition));
    ApplyMouseCursor(ToMouseCursorID(PStandardMouseCursor::Pointer));

    m_MouseInputDevice = open("/dev/input/mouse", O_RDONLY | O_NONBLOCK);
    if (m_MouseInputDevice != -1)
    {
        if (!GetWaitGroup().AddFile(m_MouseInputDevice)) {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ApplicationServer() failed to add mouse input device to wait group: {}", strerror(errno));
        }
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ApplicationServer() failed to open mouse input device: {}", strerror(errno));
    }

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
    if (m_MouseInputDevice != -1)
    {
        GetWaitGroup().RemoveFile(m_MouseInputDevice);
        close(m_MouseInputDevice);
        m_MouseInputDevice = -1;
    }
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

    while(!m_PointerEventQueue.empty())
    {
        const QueuedPointerEvent& queuedEvent = m_PointerEventQueue.front();
        switch(queuedEvent.EventID)
        {
            case PInputEventID::MouseDown:
            case PInputEventID::TouchDown:
            {
                HandlePointerDown(queuedEvent.PointerEvent.PointerID, queuedEvent.PointerEvent.Position, queuedEvent.PointerEvent);
                break;
            }            
            case PInputEventID::MouseUp:
            case PInputEventID::TouchUp:
            {
                HandlePointerUp(queuedEvent.PointerEvent.PointerID, queuedEvent.PointerEvent.Position, queuedEvent.PointerEvent);
                break;
            }            
            case PInputEventID::MouseMove:
            case PInputEventID::TouchMove:
            {
                HandlePointerMove(queuedEvent.PointerEvent.PointerID, queuedEvent.PointerEvent.Position, queuedEvent.PointerEvent);
                break;
            }
            default:
                break;
        }
        m_PointerEventQueue.pop();
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

void ApplicationServer::PushMouseCursor(ServerApplication* owner, PMouseCursorToken token, PMouseCursorID cursorID)
{
    if (owner == nullptr || token == PInvalidMouseCursorToken) {
        return;
    }
    if (!IsStandardMouseCursorID(cursorID))
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::PushMouseCursor() invalid cursor ID: {}", cursorID);
        return;
    }

    const auto iterator = std::find_if(m_MouseCursorStack.begin(), m_MouseCursorStack.end(),
        [owner, token](const MouseCursorStackEntry& entry)
        {
            return entry.Owner == owner && entry.Token == token;
        });
    if (iterator != m_MouseCursorStack.end())
    {
        iterator->CursorID = cursorID;
        UpdateMouseCursor();
        return;
    }

    m_MouseCursorStack.push_back(MouseCursorStackEntry{ .Owner = owner, .Token = token, .CursorID = cursorID });
    UpdateMouseCursor();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::PopMouseCursor(ServerApplication* owner, PMouseCursorToken token)
{
    if (owner == nullptr || token == PInvalidMouseCursorToken) {
        return;
    }

    for (auto iterator = m_MouseCursorStack.begin(); iterator != m_MouseCursorStack.end(); ++iterator)
    {
        if (iterator->Owner == owner && iterator->Token == token)
        {
            m_MouseCursorStack.erase(iterator);
            UpdateMouseCursor();
            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::RemoveMouseCursorStackEntries(ServerApplication* owner)
{
    if (owner == nullptr) {
        return;
    }

    bool wasChanged = false;
    for (auto iterator = m_MouseCursorStack.begin(); iterator != m_MouseCursorStack.end(); )
    {
        if (iterator->Owner == owner)
        {
            iterator = m_MouseCursorStack.erase(iterator);
            wasChanged = true;
        }
        else
        {
            ++iterator;
        }
    }
    if (wasChanged) {
        UpdateMouseCursor();
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
    ReadInputEvents(m_MouseInputDevice, PInputClass::Mouse, "mouse");
    ReadInputEvents(m_TouchInputDevice, PInputClass::TouchScreen, "touch");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ReadInputEvents(int inputDevice, PInputClass inputClass, const char* deviceName)
{
    if (inputDevice == -1) {
        return;
    }

    for (;;)
    {
        const ssize_t bytesRead = read(inputDevice, m_InputEventBuffer.data(), m_InputEventBuffer.size());

        if (bytesRead > 0)
        {
            const uint8_t* currentEventData = m_InputEventBuffer.data();
            size_t remainingLength = size_t(bytesRead);

            while (remainingLength > 0)
            {
                if (remainingLength < sizeof(PInputEvent))
                {
                    p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received truncated {} input event header: {}", deviceName, remainingLength);
                    break;
                }

                const PInputEvent* eventHeader = reinterpret_cast<const PInputEvent*>(currentEventData);
                if (eventHeader->EventSize < sizeof(PInputEvent) || eventHeader->EventSize > remainingLength)
                {
                    p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received invalid {} input event size: {} of {}", deviceName, eventHeader->EventSize, remainingLength);
                    break;
                }

                if (eventHeader->ClassID != inputClass)
                {
                    p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received unexpected {} input class: {}", deviceName, std::to_underlying(eventHeader->ClassID));
                }
                else
                {
                    switch (eventHeader->EventType)
                    {
                        case PInputEventType::MouseEvent:
                            if (inputClass != PInputClass::Mouse)
                            {
                                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received non-mouse event from {} input.", deviceName);
                            }
                            else if (eventHeader->EventSize < sizeof(PMouseEvent))
                            {
                                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received truncated mouse event: {}", eventHeader->EventSize);
                            }
                            else
                            {
                                QueueMouseEvent(*reinterpret_cast<const PMouseEvent*>(currentEventData));
                            }
                            break;

                        case PInputEventType::TouchEvent:
                            if (inputClass != PInputClass::TouchScreen)
                            {
                                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received non-touch event from {} input.", deviceName);
                            }
                            else if (eventHeader->EventSize < sizeof(PTouchEvent))
                            {
                                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received truncated touch event: {}", eventHeader->EventSize);
                            }
                            else
                            {
                                QueueTouchEvent(*reinterpret_cast<const PTouchEvent*>(currentEventData));
                            }
                            break;

                        default:
                            break;
                    }
                }

                currentEventData += eventHeader->EventSize;
                remainingLength -= eventHeader->EventSize;
            }
            continue;
        }
        else if (bytesRead < 0)
        {
            if (errno == EINTR) {
                continue;
            }
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() failed to read {} input: {}", deviceName, strerror(errno));
            }
            break;
        }
        else
        {
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::QueuePointerEvent(PInputEventID eventID, const PPointerEvent& event)
{
    const bool isMoveEvent = eventID == PInputEventID::MouseMove || eventID == PInputEventID::TouchMove;

    if (isMoveEvent && !m_PointerEventQueue.empty())
    {
        QueuedPointerEvent& queuedEvent = m_PointerEventQueue.back();
        if (queuedEvent.EventID == eventID && queuedEvent.PointerEvent.PointerID == event.PointerID)
        {
            queuedEvent.PointerEvent = event;
            return;
        }
    }
    m_PointerEventQueue.push(QueuedPointerEvent{ .EventID = eventID, .PointerEvent = event });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::QueueMouseEvent(const PMouseEvent& event)
{
    const PPoint position = UpdateMousePosition(event);
    QueuePointerEvent(event.EventID, CreatePointerEvent(event, position));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::QueueTouchEvent(const PTouchEvent& event)
{
    QueuePointerEvent(event.EventID, CreatePointerEvent(event));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint ApplicationServer::UpdateMousePosition(const PMouseEvent& event)
{
    if (event.EventID == PInputEventID::MouseMove)
    {
        m_MousePosition = ClampMousePosition(m_MousePosition + event.Position);
        s_DisplayDriver->SetMousePos(PIPoint(m_MousePosition));
    }
    return m_MousePosition;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint ApplicationServer::ClampMousePosition(const PPoint& position) const
{
    const PRect screenFrame = GetScreenFrame();
    return PPoint(
        std::clamp(position.x, screenFrame.left, std::max(screenFrame.left, screenFrame.right - 1.0f)),
        std::clamp(position.y, screenFrame.top, std::max(screenFrame.top, screenFrame.bottom - 1.0f))
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMouseButton ApplicationServer::GetTouchPointerButton(const PTouchEvent& touchEvent)
{
    switch (touchEvent.EventID)
    {
        case PInputEventID::TouchDown:
        case PInputEventID::TouchUp:
            return TOUCH_POINTER_BUTTON;

        default:
            return PMouseButton::None;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerButtonMask ApplicationServer::GetTouchPointerButtons(const PTouchEvent& touchEvent)
{
    return (touchEvent.EventID == PInputEventID::TouchUp)
        ? PPointerButtonMaskNone
        : GetPointerButtonMask(TOUCH_POINTER_BUTTON);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerEvent ApplicationServer::CreatePointerEvent(const PMouseEvent& mouseEvent, const PPoint& position)
{
    PPointerEvent pointerEvent;
    pointerEvent.PointerID = PMousePointerID;
    pointerEvent.Timestamp = mouseEvent.Timestamp;
    pointerEvent.ToolType = PMotionToolType::Mouse;
    pointerEvent.Button = mouseEvent.Button;
    pointerEvent.Buttons = mouseEvent.Buttons;
    pointerEvent.Pressure = (mouseEvent.Buttons != PPointerButtonMaskNone) ? 1.0f : 0.0f;
    pointerEvent.Position = position;
    return pointerEvent;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerEvent ApplicationServer::CreatePointerEvent(const PTouchEvent& touchEvent)
{
    PPointerEvent pointerEvent;
    pointerEvent.PointerID = GetTouchPointerID(touchEvent.TouchID);
    pointerEvent.Timestamp = touchEvent.Timestamp;
    pointerEvent.ToolType = touchEvent.ToolType;
    pointerEvent.Button = GetTouchPointerButton(touchEvent);
    pointerEvent.Buttons = GetTouchPointerButtons(touchEvent);
    pointerEvent.Pressure = touchEvent.Pressure;
    pointerEvent.Position = touchEvent.Position;
    return pointerEvent;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ApplicationServer::ApplyMouseCursor(PMouseCursorID cursorID)
{
    if (cursorID == m_CurrentMouseCursorID) {
        return true;
    }

    PMouseCursorBitmap cursorBitmap;
    bool isVisible = false;
    if (!GetStandardMouseCursorBitmap(cursorID, cursorBitmap, isVisible))
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ApplyMouseCursor() invalid cursor ID: {}", cursorID);
        return false;
    }

    if (!isVisible)
    {
        s_DisplayDriver->SetMouseCursorVisible(false);
        m_CurrentMouseCursorID = cursorID;
        return true;
    }

    if (s_DisplayDriver->SetMouseCursorBitmap(cursorBitmap))
    {
        s_DisplayDriver->SetMouseCursorVisible(true);
        m_CurrentMouseCursorID = cursorID;
        return true;
    }

    p_system_log<PLogSeverity::WARNING>(LogCategoryAppServer, "ApplicationServer::ApplyMouseCursor() display driver rejected cursor ID: {}", cursorID);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::UpdateMouseCursor()
{
    PMouseCursorID cursorID = ToMouseCursorID(PStandardMouseCursor::Pointer);
    if (!m_MouseCursorStack.empty()) {
        cursorID = m_MouseCursorStack.back().CursorID;
    }
    ApplyMouseCursor(cursorID);
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
