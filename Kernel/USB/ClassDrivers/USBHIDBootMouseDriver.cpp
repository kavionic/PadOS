// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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

#include <exception>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/UserInput/UserInputManager.h>
#include <Kernel/USB/ClassDrivers/USBHIDBootMouseDriver.h>
#include <Utils/String.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHIDBootMouseDriver::Probe(const USBHIDInterfaceInfo& interfaceInfo)
{
    if (interfaceInfo.Subclass == USB_HID_SubclassCode::BOOT_INTERFACE && interfaceInfo.Protocol == USB_HID_ProtocolCode::MOUSE) {
        return PROBE_SCORE_BASIC;
    }
    return PROBE_SCORE_NONE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHIDBootMouseDriver::USBHIDBootMouseDriver(USBHostHIDInterface& hidInterface)
    : USBHIDDriver(hidInterface)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHIDBootMouseDriver::~USBHIDBootMouseDriver()
{
    Close();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootMouseDriver::Startup()
{
    if (m_SourceID == -1) {
        m_SourceID = KUserInputManager::Get().AddSource(PInputClass::Mouse);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootMouseDriver::Close()
{
    if (m_SourceID != -1)
    {
        KUserInputManager::Get().RemoveSource(m_SourceID);
        m_SourceID = -1;
    }
    m_PreviousButtons = 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootMouseDriver::HandleReport(const uint8_t* report, size_t length)
{
    if (m_SourceID == -1) {
        return;
    }
    if (report == nullptr) {
        return;
    }
    if (length < 3)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID boot mouse short report: {} bytes.", length);
        return;
    }

    LogReport(report, length);

    const uint8_t buttons = report[0];
    const uint8_t changedButtons = buttons ^ m_PreviousButtons;

    for (uint8_t buttonFlag = 1; buttonFlag != 0; buttonFlag = uint8_t(buttonFlag << 1))
    {
        if ((changedButtons & buttonFlag) != 0)
        {
            const PMouseButton button = GetButton(buttonFlag);
            if (button != PMouseButton::None) {
                EmitButtonEvent(button, (buttons & buttonFlag) != 0);
            }
        }
    }

    const int deltaPositionX = int(int8_t(report[1]));
    const int deltaPositionY = int(int8_t(report[2]));

    if (deltaPositionX != 0 || deltaPositionY != 0) {
        EmitMoveEvent(deltaPositionX, deltaPositionY);
    }
    const int deltaWheel = GetWheelDelta(report, length);
    if (deltaWheel != 0) {
        EmitWheelEvent(deltaWheel);
    }

    m_PreviousButtons = buttons;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootMouseDriver::EmitButtonEvent(PMouseButton button, bool pressed)
{
    PMotionEvent event;
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::MotionEvent;
    event.ClassID = PInputClass::Mouse;
    event.Timestamp = kget_monotonic_time();
    event.EventID = pressed ? PInputEventID::MouseDown : PInputEventID::MouseUp;
    event.SourceID = m_SourceID;
    event.ToolType = PMotionToolType::Mouse;
    event.ButtonID = button;
    event.Position = PPoint(0.0f);

    try {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(
            LogCategoryUSBHost,
            "HID boot mouse button event: source={} button={} {}.",
            event.SourceID,
            int(event.ButtonID),
            pressed ? "down" : "up"
        );
        KUserInputManager::Get().AddEvent(event);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID boot mouse failed to queue button event: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootMouseDriver::EmitMoveEvent(int deltaPositionX, int deltaPositionY)
{
    PMotionEvent event;
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::MotionEvent;
    event.ClassID = PInputClass::Mouse;
    event.Timestamp = kget_monotonic_time();
    event.EventID = PInputEventID::MouseMove;
    event.SourceID = m_SourceID;
    event.ToolType = PMotionToolType::Mouse;
    event.ButtonID = PMouseButton::None;
    event.Position = PPoint(float(deltaPositionX), float(deltaPositionY));

    try {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(
            LogCategoryUSBHost,
            "HID boot mouse move event: source={} dx={} dy={}.",
            event.SourceID,
            deltaPositionX,
            deltaPositionY
        );
        KUserInputManager::Get().AddEvent(event);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID boot mouse failed to queue move event: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootMouseDriver::EmitWheelEvent(int deltaWheel)
{
    PMotionEvent event;
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::MotionEvent;
    event.ClassID = PInputClass::Mouse;
    event.Timestamp = kget_monotonic_time();
    event.EventID = PInputEventID::MouseWheel;
    event.SourceID = m_SourceID;
    event.ToolType = PMotionToolType::Mouse;
    event.ButtonID = PMouseButton::None;
    event.Position = PPoint(0.0f, float(deltaWheel));

    try {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(
            LogCategoryUSBHost,
            "HID boot mouse wheel event: source={} wheel={}.",
            event.SourceID,
            deltaWheel
        );
        KUserInputManager::Get().AddEvent(event);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID boot mouse failed to queue wheel event: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMouseButton USBHIDBootMouseDriver::GetButton(uint8_t buttonFlag)
{
    switch (buttonFlag)
    {
        case 0x01: return PMouseButton::Left;
        case 0x02: return PMouseButton::Right;
        case 0x04: return PMouseButton::Middle;
        case 0x08: return PMouseButton::Button4;
        case 0x10: return PMouseButton::Button5;
        case 0x20: return PMouseButton::Button6;
        case 0x40: return PMouseButton::Button7;
        case 0x80: return PMouseButton::Button8;
        default:   return PMouseButton::None;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHIDBootMouseDriver::GetWheelDelta(const uint8_t* report, size_t length)
{
    if (length >= 7 && report[6] != 0) {
        return int(int8_t(report[6]));
    }
    if (length >= 4) {
        return int(int8_t(report[3]));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootMouseDriver::LogReport(const uint8_t* report, size_t length)
{
    PString reportText;

    for (size_t reportIndex = 0; reportIndex < length; ++reportIndex)
    {
        if (!reportText.empty()) {
            reportText += " ";
        }
        reportText += PString::format_string("{:02x}", int(report[reportIndex]));
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(
        LogCategoryUSBHost,
        "HID boot mouse report ({} bytes): {}",
        length,
        reportText.c_str()
    );
}

} // namespace kernel
