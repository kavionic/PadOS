// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
#include <DeviceControl/InputDevice.h>
#include <Storage/Directory.h>
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
        PViewHitMode::HitTest,
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

    RegisterRemoteSignal(&m_RSRegisterApplication, &ApplicationServer::SlotRegisterApplication);
    RegisterRemoteSignal(&m_RSVirtualKeyboardEvent, &ApplicationServer::SlotVirtualKeyboardEvent);

    m_MousePosition = GetScreenFrame().Center();
    s_DisplayDriver->SetMousePos(PIPoint(m_MousePosition));

    OpenInputDevices();
    UpdateMouseCursor();
    g_AppserverPort = GetPortID();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ApplicationServer::~ApplicationServer()
{
    for (InputDevice& inputDevice : m_InputDevices)
    {
        GetWaitGroup().RemoveFile(inputDevice.FileDescriptor);
        close(inputDevice.FileDescriptor);
        inputDevice.FileDescriptor = -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::Idle()
{
    ReadInputEvents();

    while (!m_PointerEventQueue.empty())
    {
        HandlePointerEvent(m_PointerEventQueue.front());
        m_PointerEventQueue.pop();
    }
    RefreshPointerRoutes();
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

void ApplicationServer::ViewDetached(PServerView* view)
{
    if (view == m_KeyboardFocusView) {
        SetKeyboardFocus(ptr_tmp_cast(view), false);
    }

    for (auto& [pointerID, pointerState] : m_PointerRouteMap)
    {
        if (pointerState.DeliveredRootView == view) {
            pointerState.DeliveredRootView = nullptr;
        }
        if (pointerState.Capture.RootView == view)
        {
            const PointerCaptureState captureState = pointerState.Capture;
            pointerState.Capture = PointerCaptureState();
            if (captureState.Application != nullptr) {
                captureState.Application->HandlePointerCaptureLost(
                    pointerID, captureState.CaptureID, PPointerCaptureLostReason::ViewDetached);
            }
        }
    }
    InvalidatePointerRoutes();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ServerApplicationDestructed(ServerApplication* application)
{
    RemoveMouseCursorStackEntries(application);

    for (auto& pointerEntry : m_PointerRouteMap)
    {
        PointerRouteState& pointerState = pointerEntry.second;
        if (pointerState.Capture.Application == application) {
            pointerState.Capture = PointerCaptureState();
        }
        if (pointerState.DeliveredRootView != nullptr
            && pointerState.DeliveredRootView->GetOwnerApplication() == application) {
            pointerState.DeliveredRootView = nullptr;
        }
    }
    InvalidatePointerRoutes();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::InvalidatePointerRoutes()
{
    m_PointerRoutesInvalid = true;
    WakeupLooper();
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

void ApplicationServer::SlotVirtualKeyboardEvent(const PKeyEvent& keyEvent)
{
    Ptr<PServerView> focusView = GetKeyboardFocus();
    if (focusView != nullptr) {
        focusView->SendKeyboardEvent(keyEvent);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::OpenInputDevices()
{
    PDirectory inputDirectory("/dev/input");
    if (!inputDirectory.IsValid())
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::OpenInputDevices() failed to open /dev/input: {}", strerror(errno));
        return;
    }

    PString deviceName;
    while (inputDirectory.GetNextEntry(deviceName))
    {
        if (deviceName.is_dot_or_dot_dot()) {
            continue;
        }

        const int inputDevice = openat(inputDirectory.GetFileDescriptor(), deviceName.c_str(), O_RDONLY | O_NONBLOCK);
        if (inputDevice == -1)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::OpenInputDevices() failed to open {}: {}", deviceName, strerror(errno));
            continue;
        }

        if (!GetWaitGroup().AddFile(inputDevice))
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::OpenInputDevices() failed to add {} to wait group: {}", deviceName, strerror(errno));
            close(inputDevice);
            continue;
        }

        m_InputDevices.push_back(InputDevice{deviceName, inputDevice});
        ReadRegisteredInputDevices(inputDevice, deviceName);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ReadRegisteredInputDevices(int inputDevice, const PString& deviceName)
{
    try
    {
        PInputDeviceControl deviceControl(inputDevice);
        std::vector<PInputDeviceInfo> registeredDevices;

        for (;;)
        {
            const size_t registeredDeviceCount = deviceControl.GetRegisteredDevices(registeredDevices.data(), registeredDevices.size());
            if (registeredDeviceCount <= registeredDevices.size())
            {
                registeredDevices.resize(registeredDeviceCount);
                break;
            }
            registeredDevices.resize(registeredDeviceCount);
        }

        for (const PInputDeviceInfo& device : registeredDevices)
        {
            m_RegisteredInputDevices[device.SourceID] = device.ClassID;
        }
    }
    catch (const std::exception& error)
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadRegisteredInputDevices() failed to query {}: {}", deviceName, error.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ReadInputEvents()
{
    for (const InputDevice& inputDevice : m_InputDevices) {
        ReadInputEvents(inputDevice.FileDescriptor, inputDevice.Name);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ReadInputEvents(int inputDevice, const PString& deviceName)
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

                switch (eventHeader->EventType)
                {
                    case PInputEventType::DeviceEvent:
                        HandleInputDeviceEvent(*eventHeader, deviceName);
                        break;

                    case PInputEventType::MouseEvent:
                        if (eventHeader->ClassID != PInputClass::Mouse)
                        {
                            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received mouse event with input class {} from {}.", std::to_underlying(eventHeader->ClassID), deviceName);
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
                        if (eventHeader->ClassID != PInputClass::TouchScreen)
                        {
                            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received touch event with input class {} from {}.", std::to_underlying(eventHeader->ClassID), deviceName);
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

                    case PInputEventType::KeyEvent:
                        if (eventHeader->ClassID != PInputClass::Keyboard)
                        {
                            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received key event with input class {} from {}.", std::to_underlying(eventHeader->ClassID), deviceName);
                        }
                        else if (eventHeader->EventSize < sizeof(PKeyEvent))
                        {
                            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received truncated key event: {}", eventHeader->EventSize);
                        }
                        else
                        {
                            const PKeyEvent& keyEvent = *reinterpret_cast<const PKeyEvent*>(currentEventData);
                            if (keyEvent.EventID == PInputEventID::KeyDown || keyEvent.EventID == PInputEventID::KeyUp)
                            {
                                Ptr<PServerView> focusView = GetKeyboardFocus();
                                if (focusView != nullptr) {
                                    focusView->SendKeyboardEvent(keyEvent);
                                }
                            }
                            else
                            {
                                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::ReadInputEvents() received invalid key event ID {} from {}.", std::to_underlying(keyEvent.EventID), deviceName);
                            }
                        }
                        break;

                    default:
                        break;
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

void ApplicationServer::HandleInputDeviceEvent(const PInputEvent& event, const PString& deviceName)
{
    bool deviceListChanged = false;

    if (event.EventID == PInputEventID::DeviceAdded)
    {
        const auto [deviceIterator, inserted] = m_RegisteredInputDevices.emplace(event.SourceID, event.ClassID);
        deviceListChanged = inserted;

        if (!inserted && deviceIterator->second != event.ClassID)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::HandleInputDeviceEvent() source {} changed class from {} to {} on {}.", event.SourceID, std::to_underlying(deviceIterator->second), std::to_underlying(event.ClassID), deviceName);
            deviceIterator->second = event.ClassID;
            deviceListChanged = true;
        }
    }
    else if (event.EventID == PInputEventID::DeviceRemoved)
    {
        const auto deviceIterator = m_RegisteredInputDevices.find(event.SourceID);
        if (deviceIterator != m_RegisteredInputDevices.end())
        {
            if (deviceIterator->second != event.ClassID)
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::HandleInputDeviceEvent() source {} removal has class {} instead of {} on {}.", event.SourceID, std::to_underlying(event.ClassID), std::to_underlying(deviceIterator->second), deviceName);
            }
            m_RegisteredInputDevices.erase(deviceIterator);
            deviceListChanged = true;
        }
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ApplicationServer::HandleInputDeviceEvent() received invalid device event ID {} from {}.", std::to_underlying(event.EventID), deviceName);
    }

    if (deviceListChanged)
    {
        switch (event.ClassID)
        {
            case PInputClass::Keyboard: UpdateVirtualKeyboard();        break;
            case PInputClass::Mouse:    UpdateMouseCursorVisibility();  break;
            default: break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ApplicationServer::HasInputDevice(PInputClass classID) const
{
    return std::any_of(
        m_RegisteredInputDevices.begin(),
        m_RegisteredInputDevices.end(),
        [classID](const auto& device) { return device.second == classID; }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::UpdateVirtualKeyboard()
{
    if (m_KeyboardFocusView != nullptr && m_KeyboardFocusView->GetFocusKeyboardMode() != PFocusKeyboardMode::None && !HasInputDevice(PInputClass::Keyboard)) {
        p_post_to_window_manager<ASWindowManagerEnableVKeyboard>(INVALID_HANDLE, m_KeyboardFocusView->ConvertToScreen(m_KeyboardFocusView->GetFrame()), m_KeyboardFocusView->GetFocusKeyboardMode() == PFocusKeyboardMode::Numeric);
    } else {
        p_post_to_window_manager<ASWindowManagerDisableVKeyboard>(INVALID_HANDLE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::QueuePointerEvent(const PPointerEvent& event)
{
    if (event.EventType == PPointerEventType::Invalid) {
        return;
    }

    const bool isMoveEvent = event.EventType == PPointerEventType::Move;

    if (isMoveEvent && !m_PointerEventQueue.empty())
    {
        PPointerEvent& queuedEvent = m_PointerEventQueue.back();
        if (queuedEvent.EventType == event.EventType && queuedEvent.PointerID == event.PointerID)
        {
            queuedEvent = event;
            return;
        }
    }
    m_PointerEventQueue.push(event);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::QueueMouseEvent(const PMouseEvent& event)
{
    const PPoint position = UpdateMousePosition(event);
    QueuePointerEvent(CreatePointerEvent(event, position));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::QueueTouchEvent(const PTouchEvent& event)
{
    QueuePointerEvent(CreatePointerEvent(event));
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

void ApplicationServer::BeginPointerGesture(
    PointerRouteState& pointerState, const PPointerEvent& event)
{
    PointerGestureState& gesture = pointerState.Gesture;
    gesture.StartPosition = event.ScreenPosition;
    gesture.StartTime = event.Timestamp;
    gesture.Button = event.Button;
    gesture.TapEligible = true;
    gesture.LongPressEligible = true;
    gesture.LongPressTimer.Start(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::UpdatePointerGesture(
    PointerRouteState& pointerState, const PPointerEvent& event)
{
    const PointerGestureState& gesture = pointerState.Gesture;
    if ((gesture.TapEligible || gesture.LongPressEligible)
        && (event.ScreenPosition - gesture.StartPosition).LengthSqr()
            > BEGIN_DRAG_OFFSET * BEGIN_DRAG_OFFSET)
    {
        CancelPointerGesture(pointerState);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ApplicationServer::EndPointerGesture(
    PointerRouteState& pointerState, const PPointerEvent& event)
{
    const PointerGestureState& gesture = pointerState.Gesture;
    const TimeValNanos gestureDuration =
        event.Timestamp - gesture.StartTime;
    const bool generateTap =
        gesture.TapEligible
        && gestureDuration >= TimeValNanos::zero
        && gestureDuration <= TimeValNanos::FromSeconds(TAP_MAX_DURATION);

    CancelPointerGesture(pointerState);
    return generateTap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::CancelPointerGesture(PointerRouteState& pointerState)
{
    PointerGestureState& gesture = pointerState.Gesture;
    gesture.LongPressTimer.Stop();
    gesture.Button = PMouseButton::None;
    gesture.TapEligible = false;
    gesture.LongPressEligible = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SlotLongPressTimer(PEventTimer* timer)
{
    const PPointerID pointerID = PPointerID(timer->GetID());
    auto iterator = m_PointerRouteMap.find(pointerID);
    if (iterator != m_PointerRouteMap.end()
        && iterator->second.Gesture.LongPressEligible)
    {
        PointerRouteState& pointerState = iterator->second;
        PointerGestureState& gesture = pointerState.Gesture;
        gesture.TapEligible = false;
        gesture.LongPressEligible = false;

        PPointerEvent longPressEvent = pointerState.LastEvent;
        longPressEvent.EventType = PPointerEventType::LongPress;
        longPressEvent.Timestamp = get_monotonic_time();
        longPressEvent.Button = gesture.Button;
        HandlePointerEvent(longPressEvent);
    }
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

PPointerEventType ApplicationServer::GetPointerEventType(PInputEventID eventID)
{
    switch (eventID)
    {
        case PInputEventID::MouseDown:
        case PInputEventID::TouchDown:
            return PPointerEventType::Down;

        case PInputEventID::MouseUp:
        case PInputEventID::TouchUp:
            return PPointerEventType::Up;

        case PInputEventID::MouseMove:
        case PInputEventID::TouchMove:
            return PPointerEventType::Move;

        default:
            return PPointerEventType::Invalid;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerEvent ApplicationServer::CreatePointerEvent(const PMouseEvent& mouseEvent, const PPoint& position)
{
    PPointerEvent pointerEvent;
    pointerEvent.PointerID = PMousePointerID;
    pointerEvent.EventType = GetPointerEventType(mouseEvent.EventID);
    pointerEvent.Timestamp = mouseEvent.Timestamp;
    pointerEvent.ToolType = PMotionToolType::Mouse;
    pointerEvent.SupportsHover = true;
    pointerEvent.Button = mouseEvent.Button;
    pointerEvent.Buttons = mouseEvent.Buttons;
    pointerEvent.Pressure = (mouseEvent.Buttons != PPointerButtonMaskNone) ? 1.0f : 0.0f;
    pointerEvent.ScreenPosition = position;
    pointerEvent.ViewPosition = position;
    return pointerEvent;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerEvent ApplicationServer::CreatePointerEvent(const PTouchEvent& touchEvent)
{
    PPointerEvent pointerEvent;
    pointerEvent.PointerID = GetTouchPointerID(touchEvent.TouchID);
    pointerEvent.EventType = GetPointerEventType(touchEvent.EventID);
    pointerEvent.Timestamp = touchEvent.Timestamp;
    pointerEvent.ToolType = touchEvent.ToolType;
    pointerEvent.Button = GetTouchPointerButton(touchEvent);
    pointerEvent.Buttons = GetTouchPointerButtons(touchEvent);
    pointerEvent.Pressure = touchEvent.Pressure;
    pointerEvent.ScreenPosition = touchEvent.Position;
    pointerEvent.ViewPosition = touchEvent.Position;
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
        m_IsMouseCursorRequestedVisible = false;
        UpdateMouseCursorVisibility();
        m_CurrentMouseCursorID = cursorID;
        return true;
    }

    if (s_DisplayDriver->SetMouseCursorBitmap(cursorBitmap))
    {
        m_IsMouseCursorRequestedVisible = true;
        UpdateMouseCursorVisibility();
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

void ApplicationServer::UpdateMouseCursorVisibility()
{
    s_DisplayDriver->SetMouseCursorVisible(m_IsMouseCursorRequestedVisible && HasInputDevice(PInputClass::Mouse));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandlePointerEvent(const PPointerEvent& event)
{
    const PPointerID pointerID = event.PointerID;
    PointerRouteState& pointerState = GetPointerRouteState(pointerID);
    bool generateTap = false;
    switch (event.EventType)
    {
        case PPointerEventType::Down:
            BeginPointerGesture(pointerState, event);
            break;

        case PPointerEventType::Move:
            UpdatePointerGesture(pointerState, event);
            break;

        case PPointerEventType::Up:
            generateTap = EndPointerGesture(pointerState, event);
            break;

        case PPointerEventType::Cancel:
            CancelPointerGesture(pointerState);
            break;

        case PPointerEventType::Invalid:
        case PPointerEventType::LongPress:
        case PPointerEventType::Tap:
        case PPointerEventType::Enter:
        case PPointerEventType::Leave:
        case PPointerEventType::Over:
        case PPointerEventType::Out:
            break;
    }
    pointerState.LastEvent = event;

    if (generateTap)
    {
        PPointerEvent tapEvent = event;
        tapEvent.EventType = PPointerEventType::Tap;
        RoutePointerEvent(pointerState, tapEvent);
    }
    RoutePointerEvent(pointerState, event);

    if (!event.SupportsHover
        && (event.EventType == PPointerEventType::Up
            || event.EventType == PPointerEventType::Cancel))
    {
        m_PointerRouteMap.erase(pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::RoutePointerEvent(
    PointerRouteState& pointerState, const PPointerEvent& event)
{
    if (event.SupportsHover) {
        HandleHoverPointerEvent(pointerState, event);
    } else {
        HandleNonHoverPointerEvent(pointerState, event);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleHoverPointerEvent(
    PointerRouteState& pointerState, const PPointerEvent& event)
{
    const PointerCaptureState captureState = pointerState.Capture;
    const Ptr<PServerView> rootView = (captureState.RootView != nullptr)
        ? ptr_tmp_cast(captureState.RootView)
        : m_TopView->FindPointerEventRootView(event.ScreenPosition);
    DeliverPointerEvent(pointerState, rootView, event, captureState.CaptureID);

    if (event.EventType == PPointerEventType::Up
        || event.EventType == PPointerEventType::Cancel)
    {
        pointerState.Capture = PointerCaptureState();
        if (event.EventType == PPointerEventType::Cancel) {
            NotifyPointerExitedRootView(pointerState, event);
        } else {
            ReevaluatePointerRoute(pointerState);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::HandleNonHoverPointerEvent(
    PointerRouteState& pointerState, const PPointerEvent& event)
{
    if (event.EventType == PPointerEventType::Down)
    {
        const Ptr<PServerView> rootView =
            m_TopView->FindPointerEventRootView(event.ScreenPosition);
        BeginImplicitPointerCapture(pointerState, rootView);
        DeliverPointerEvent(
            pointerState, rootView, event, pointerState.Capture.CaptureID);
    }
    else if (pointerState.Capture.RootView != nullptr)
    {
        DeliverPointerEvent(
            pointerState,
            ptr_tmp_cast(pointerState.Capture.RootView),
            event,
            pointerState.Capture.CaptureID);
    }

    if (event.EventType == PPointerEventType::Up
        || event.EventType == PPointerEventType::Cancel)
    {
        pointerState.Capture = PointerCaptureState();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ApplicationServer::PointerRouteState& ApplicationServer::GetPointerRouteState(PPointerID pointerID)
{
    auto [iterator, inserted] = m_PointerRouteMap.try_emplace(pointerID);
    if (inserted)
    {
        PointerRouteState& pointerState = iterator->second;
        pointerState.Gesture.LongPressTimer.Set(LONG_PRESS_DELAY, true);
        pointerState.Gesture.LongPressTimer.SetID(int32_t(pointerID));
        pointerState.Gesture.LongPressTimer.SignalTrigged.Connect(
            this, &ApplicationServer::SlotLongPressTimer);
    }
    return iterator->second;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::NotifyPointerExitedRootView(PointerRouteState& pointerState, const PPointerEvent& event)
{
    PServerView* deliveredRootView = pointerState.DeliveredRootView;
    pointerState.DeliveredRootView = nullptr;

    if (deliveredRootView != nullptr) {
        deliveredRootView->SendPointerRootViewUpdate(event, PPointerRootViewUpdateType::Exited);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::DeliverPointerEvent(
    PointerRouteState& pointerState,
    Ptr<PServerView> rootView,
    const PPointerEvent& event,
    PPointerCaptureID captureID)
{
    if (event.SupportsHover && pointerState.DeliveredRootView != ptr_raw_pointer_cast(rootView)) {
        NotifyPointerExitedRootView(pointerState, event);
    }
    if (rootView != nullptr && rootView->SendPointerEvent(event, captureID)) {
        pointerState.DeliveredRootView = ptr_raw_pointer_cast(rootView);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ReevaluatePointerRoute(PointerRouteState& pointerState)
{
    const PPointerEvent& event = pointerState.LastEvent;
    const Ptr<PServerView> rootView = (pointerState.Capture.RootView != nullptr)
        ? ptr_tmp_cast(pointerState.Capture.RootView)
        : m_TopView->FindPointerEventRootView(event.ScreenPosition);

    if (rootView != pointerState.DeliveredRootView) {
        NotifyPointerExitedRootView(pointerState, event);
    }
    if (rootView != nullptr
        && rootView->SendPointerRootViewUpdate(event, PPointerRootViewUpdateType::Reevaluate))
    {
        pointerState.DeliveredRootView = ptr_raw_pointer_cast(rootView);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::RefreshPointerRoutes()
{
    if (m_PointerRoutesInvalid)
    {
        m_PointerRoutesInvalid = false;
        for (auto& pointerEntry : m_PointerRouteMap)
        {
            PointerRouteState& pointerState = pointerEntry.second;
            if (pointerState.LastEvent.SupportsHover && pointerState.Capture.Application == nullptr) {
                ReevaluatePointerRoute(pointerState);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::BeginImplicitPointerCapture(
    PointerRouteState& pointerState, Ptr<PServerView> rootView)
{
    if (pointerState.Capture.Application != nullptr)
    {
        const PointerCaptureState previousCapture = pointerState.Capture;
        pointerState.Capture = PointerCaptureState();
        previousCapture.Application->HandlePointerCaptureLost(
            pointerState.LastEvent.PointerID,
            previousCapture.CaptureID,
            PPointerCaptureLostReason::PointerCancel);
    }

    if (rootView != nullptr && rootView->GetOwnerApplication() != nullptr)
    {
        pointerState.Capture = PointerCaptureState
        {
            .Application = rootView->GetOwnerApplication(),
            .RootView = ptr_raw_pointer_cast(rootView),
            .CaptureID = AllocatePointerCaptureID(),
            .Mode = PPointerCaptureMode::Preemptible
        };
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::GrantPointerCapture(
    PPointerID pointerID,
    PointerRouteState& pointerState,
    ServerApplication* application,
    Ptr<PServerView> rootView,
    PPointerCaptureRequestID requestID,
    PPointerCaptureMode mode)
{
    const PointerCaptureState previousCapture = pointerState.Capture;
    const bool rootViewChanged = pointerState.DeliveredRootView != ptr_raw_pointer_cast(rootView);
    const PPointerCaptureID captureID = AllocatePointerCaptureID();

    pointerState.Capture = PointerCaptureState
    {
        .Application = application,
        .RootView = ptr_raw_pointer_cast(rootView),
        .CaptureID = captureID,
        .Mode = mode
    };

    if (previousCapture.Application != nullptr) {
        previousCapture.Application->HandlePointerCaptureLost(
            pointerID,
            previousCapture.CaptureID,
            PPointerCaptureLostReason::Stolen);
    }

    application->HandlePointerCaptureRequestReply(
        pointerID, requestID, rootView, captureID, pointerState.LastEvent);

    if (rootViewChanged)
    {
        if (pointerState.LastEvent.SupportsHover)
        {
            NotifyPointerExitedRootView(pointerState, pointerState.LastEvent);
            ReevaluatePointerRoute(pointerState);
        }
        else
        {
            pointerState.DeliveredRootView = ptr_raw_pointer_cast(rootView);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::RejectPointerCaptureRequest(
    PPointerID pointerID,
    PointerRouteState& pointerState,
    ServerApplication* application,
    Ptr<PServerView> rootView,
    PPointerCaptureRequestID requestID)
{
    application->HandlePointerCaptureRequestReply(
        pointerID,
        requestID,
        rootView,
        PInvalidPointerCaptureID,
        pointerState.LastEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetPointerCapture(
    PPointerID pointerID,
    ServerApplication* application,
    Ptr<PServerView> rootView,
    PPointerCaptureID captureID,
    PPointerCaptureRequestID requestID,
    PPointerCaptureMode mode)
{
    if (application == nullptr || rootView == nullptr
        || rootView->GetOwnerApplication() != application) {
        return;
    }

    auto pointerIterator = m_PointerRouteMap.find(pointerID);
    if (pointerIterator == m_PointerRouteMap.end())
    {
        if (captureID != PInvalidPointerCaptureID)
        {
            application->HandlePointerCaptureLost(
                pointerID, captureID, PPointerCaptureLostReason::Stolen);
        }
        else if (requestID != PInvalidPointerCaptureRequestID)
        {
            PPointerEvent invalidEvent;
            application->HandlePointerCaptureRequestReply(
                pointerID,
                requestID,
                rootView,
                PInvalidPointerCaptureID,
                invalidEvent);
        }
        return;
    }

    PointerRouteState& pointerState = pointerIterator->second;
    PointerCaptureState& currentCapture = pointerState.Capture;

    if (captureID != PInvalidPointerCaptureID)
    {
        if (currentCapture.Application == application && currentCapture.CaptureID == captureID)
        {
            currentCapture.RootView = ptr_raw_pointer_cast(rootView);
            currentCapture.Mode = mode;
            pointerState.DeliveredRootView = ptr_raw_pointer_cast(rootView);
        }
        else
        {
            application->HandlePointerCaptureLost(
                pointerID, captureID, PPointerCaptureLostReason::Stolen);
        }
        return;
    }

    if (requestID != PInvalidPointerCaptureRequestID)
    {
        const bool captureCanBeStolen = currentCapture.Application == nullptr
            || currentCapture.Application == application
            || currentCapture.Mode == PPointerCaptureMode::Preemptible;
        if (pointerState.LastEvent.Buttons != PPointerButtonMaskNone && captureCanBeStolen) {
            GrantPointerCapture(pointerID, pointerState, application, rootView, requestID, mode);
        } else {
            RejectPointerCaptureRequest(pointerID, pointerState, application, rootView, requestID);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::ReleasePointerCapture(
    PPointerID pointerID,
    ServerApplication* application,
    PPointerCaptureID captureID)
{
    auto iterator = m_PointerRouteMap.find(pointerID);
    if (iterator == m_PointerRouteMap.end()) {
        return;
    }

    PointerCaptureState& captureState = iterator->second.Capture;
    if (application != captureState.Application) {
        return;
    }
    if (captureID != captureState.CaptureID) {
        return;
    }

    captureState = PointerCaptureState();
    if (iterator->second.LastEvent.SupportsHover) {
        ReevaluatePointerRoute(iterator->second);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerCaptureID ApplicationServer::AllocatePointerCaptureID()
{
    const PPointerCaptureID captureID = m_NextPointerCaptureID;
    m_NextPointerCaptureID++;
    if (m_NextPointerCaptureID == PInvalidPointerCaptureID) {
        m_NextPointerCaptureID = PFirstPointerCaptureID;
    }
    return captureID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ApplicationServer::SetKeyboardFocus(Ptr<PServerView> view, bool focus)
{
    if (focus)
    {
        m_KeyboardFocusView = ptr_raw_pointer_cast(view);
        UpdateVirtualKeyboard();
    }
    else if (view == m_KeyboardFocusView)
    {
        m_KeyboardFocusView = nullptr;
        UpdateVirtualKeyboard();
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
    if (view == m_KeyboardFocusView) {
        UpdateVirtualKeyboard();
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
