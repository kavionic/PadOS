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
#include <utility>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/UserInput/UserInputManager.h>
#include <Kernel/USB/ClassDrivers/USBHIDMouseDriver.h>
#include <Kernel/USB/ClassDrivers/USBHostHIDInterface.h>
#include <Utils/String.h>

namespace kernel
{

namespace USBHIDMouseDriverInternal
{

static USBHIDReportField CreateBootMouseField(uint16_t usagePage, uint32_t usage, size_t bitOffset, size_t bitSize, int32_t logicalMinimum, int32_t logicalMaximum, uint32_t flags)
{
    USBHIDReportField field;
    field.ReportType = USBHIDReportType::Input;
    field.ReportID = 0;
    field.UsagePage = usagePage;
    field.Usage = usage;
    field.FullUsage = (uint32_t(usagePage) << 16) | usage;
    field.ApplicationUsagePage = USB_HID_USAGE_PAGE_GENERIC_DESKTOP;
    field.ApplicationUsage = USB_HID_USAGE_GENERIC_DESKTOP_MOUSE;
    field.BitOffset = bitOffset;
    field.BitSize = bitSize;
    field.LogicalMinimum = logicalMinimum;
    field.LogicalMaximum = logicalMaximum;
    field.Flags = flags;
    return field;
}

} // namespace USBHIDMouseDriverInternal

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHIDMouseDriver::Probe(const USBHIDInterfaceInfo& interfaceInfo)
{
    if (interfaceInfo.ReportDescriptor != nullptr && HasDescriptorReportLayout(*interfaceInfo.ReportDescriptor)) {
        return PROBE_SCORE_ENHANCED;
    }
    if (interfaceInfo.Subclass == USB_HID_SubclassCode::BOOT_INTERFACE && interfaceInfo.Protocol == USB_HID_ProtocolCode::MOUSE) {
        return PROBE_SCORE_BASIC;
    }
    return PROBE_SCORE_NONE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHIDMouseDriver::USBHIDMouseDriver(USBHostHIDInterface& hidInterface)
    : USBHIDDriver(hidInterface)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHIDMouseDriver::~USBHIDMouseDriver()
{
    Close();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::Startup()
{
    if (!BuildReportLayout()) {
        return;
    }
    if (m_SourceID == -1) {
        m_SourceID = KUserInputManager::Get().AddSource(PInputClass::Mouse);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::Close()
{
    if (m_SourceID != -1)
    {
        KUserInputManager::Get().RemoveSource(m_SourceID);
        m_SourceID = -1;
    }
    m_PreviousButtons = 0;
    m_HasReportLayout = false;
    m_ReportLayout = {};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::HandleReport(const uint8_t* report, size_t length)
{
    if (m_SourceID == -1) {
        return;
    }
    if (report == nullptr) {
        return;
    }
    if (!m_HasReportLayout) {
        return;
    }

    const MouseReportLayout& reportLayout = m_ReportLayout;
    if (reportLayout.ReportID != 0)
    {
        if (length == 0 || report[0] != reportLayout.ReportID) {
            return;
        }
    }

    LogReport(report, length);

    const uint32_t buttons = GetButtonFlags(reportLayout, report, length);
    const uint32_t changedButtons = buttons ^ m_PreviousButtons;
    const PPointerButtonMask pointerButtons = GetPointerButtons(buttons);

    for (uint32_t buttonUsage = 1; buttonUsage <= 8; ++buttonUsage)
    {
        const uint32_t buttonFlag = GetButtonFlag(buttonUsage);
        if ((changedButtons & buttonFlag) != 0)
        {
            const PMouseButton button = GetButton(buttonUsage);
            if (button != PMouseButton::None) {
                EmitButtonEvent(button, pointerButtons, (buttons & buttonFlag) != 0);
            }
        }
    }

    const int deltaPositionX = GetFieldValue(report, length, reportLayout.DeltaPositionXField);
    const int deltaPositionY = GetFieldValue(report, length, reportLayout.DeltaPositionYField);

    if (deltaPositionX != 0 || deltaPositionY != 0) {
        EmitMoveEvent(deltaPositionX, deltaPositionY, pointerButtons);
    }
    const int deltaWheel = GetFieldValue(report, length, reportLayout.WheelField);
    if (deltaWheel != 0) {
        EmitWheelEvent(deltaWheel, pointerButtons);
    }

    m_PreviousButtons = buttons;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::EmitButtonEvent(PMouseButton button, PPointerButtonMask buttons, bool pressed)
{
    PMouseEvent event;
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::MouseEvent;
    event.ClassID = PInputClass::Mouse;
    event.Timestamp = kget_monotonic_time();
    event.EventID = pressed ? PInputEventID::MouseDown : PInputEventID::MouseUp;
    event.SourceID = m_SourceID;
    event.Button = button;
    event.Buttons = buttons;
    event.Position = PPoint(0.0f);

    try {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(
            LogCategoryUSBHost,
            "HID mouse button event: source={} button={} {}.",
            event.SourceID,
            std::to_underlying(event.Button),
            pressed ? "down" : "up"
        );
        KUserInputManager::Get().AddEvent(event);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID mouse failed to queue button event: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::EmitMoveEvent(int deltaPositionX, int deltaPositionY, PPointerButtonMask buttons)
{
    PMouseEvent event;
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::MouseEvent;
    event.ClassID = PInputClass::Mouse;
    event.Timestamp = kget_monotonic_time();
    event.EventID = PInputEventID::MouseMove;
    event.SourceID = m_SourceID;
    event.Button = PMouseButton::None;
    event.Buttons = buttons;
    event.Position = PPoint(float(deltaPositionX), float(deltaPositionY));

    try {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(
            LogCategoryUSBHost,
            "HID mouse move event: source={} dx={} dy={}.",
            event.SourceID,
            deltaPositionX,
            deltaPositionY
        );
        KUserInputManager::Get().AddEvent(event);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID mouse failed to queue move event: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::EmitWheelEvent(int deltaWheel, PPointerButtonMask buttons)
{
    PMouseEvent event;
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::MouseEvent;
    event.ClassID = PInputClass::Mouse;
    event.Timestamp = kget_monotonic_time();
    event.EventID = PInputEventID::MouseWheel;
    event.SourceID = m_SourceID;
    event.Button = PMouseButton::None;
    event.Buttons = buttons;
    event.Position = PPoint(0.0f, float(deltaWheel));

    try {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(
            LogCategoryUSBHost,
            "HID mouse wheel event: source={} wheel={}.",
            event.SourceID,
            deltaWheel
        );
        KUserInputManager::Get().AddEvent(event);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID mouse failed to queue wheel event: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::BuildReportLayout()
{
    const USBHIDReportDescriptor& reportDescriptor = GetHIDInterface().GetReportDescriptor();
    if (reportDescriptor.IsValid() && BuildDescriptorReportLayout(reportDescriptor)) {
        return true;
    }

    const USBHIDInterfaceInfo interfaceInfo = GetHIDInterface().GetInterfaceInfo();
    if (interfaceInfo.Subclass == USB_HID_SubclassCode::BOOT_INTERFACE && interfaceInfo.Protocol == USB_HID_ProtocolCode::MOUSE)
    {
        BuildBootReportLayout();
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::BuildDescriptorReportLayout(const USBHIDReportDescriptor& reportDescriptor)
{
    std::vector<MouseReportLayout> reportLayouts;
    BuildDescriptorReportLayouts(reportDescriptor, reportLayouts);

    const MouseReportLayout* bestReportLayout = FindBestReportLayout(reportLayouts);
    if (bestReportLayout == nullptr) {
        return false;
    }

    m_ReportLayout = *bestReportLayout;
    m_HasReportLayout = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::HasDescriptorReportLayout(const USBHIDReportDescriptor& reportDescriptor)
{
    if (!reportDescriptor.IsValid()) {
        return false;
    }

    std::vector<MouseReportLayout> reportLayouts;
    BuildDescriptorReportLayouts(reportDescriptor, reportLayouts);
    return FindBestReportLayout(reportLayouts) != nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::BuildDescriptorReportLayouts(const USBHIDReportDescriptor& reportDescriptor, std::vector<MouseReportLayout>& reportLayouts)
{
    reportLayouts.clear();
    for (const USBHIDReportField& field : reportDescriptor.GetFields())
    {
        if (field.ReportType != USBHIDReportType::Input) {
            continue;
        }
        if (!IsMouseApplicationField(field)) {
            continue;
        }

        MouseReportLayout& reportLayout = GetOrCreateReportLayout(reportLayouts, field.ReportID);

        if (IsButtonField(field)) {
            reportLayout.ButtonFields.push_back(field);
        } else if (IsDeltaPositionXField(field)) {
            reportLayout.DeltaPositionXField = field;
        } else if (IsDeltaPositionYField(field)) {
            reportLayout.DeltaPositionYField = field;
        } else if (IsWheelField(field)) {
            reportLayout.WheelField = field;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::BuildBootReportLayout()
{
    MouseReportLayout reportLayout;
    reportLayout.ReportID = 0;
    reportLayout.ButtonFields.push_back(USBHIDMouseDriverInternal::CreateBootMouseField(USB_HID_USAGE_PAGE_BUTTON, 1, 0, 1, 0, 1, USB_HID_MAIN_ITEM_VARIABLE));
    reportLayout.ButtonFields.push_back(USBHIDMouseDriverInternal::CreateBootMouseField(USB_HID_USAGE_PAGE_BUTTON, 2, 1, 1, 0, 1, USB_HID_MAIN_ITEM_VARIABLE));
    reportLayout.ButtonFields.push_back(USBHIDMouseDriverInternal::CreateBootMouseField(USB_HID_USAGE_PAGE_BUTTON, 3, 2, 1, 0, 1, USB_HID_MAIN_ITEM_VARIABLE));
    reportLayout.DeltaPositionXField = USBHIDMouseDriverInternal::CreateBootMouseField(USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_X, 8, 8, -127, 127, USB_HID_MAIN_ITEM_VARIABLE | USB_HID_MAIN_ITEM_RELATIVE);
    reportLayout.DeltaPositionYField = USBHIDMouseDriverInternal::CreateBootMouseField(USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_Y, 16, 8, -127, 127, USB_HID_MAIN_ITEM_VARIABLE | USB_HID_MAIN_ITEM_RELATIVE);
    reportLayout.WheelField = USBHIDMouseDriverInternal::CreateBootMouseField(USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_WHEEL, 24, 8, -127, 127, USB_HID_MAIN_ITEM_VARIABLE | USB_HID_MAIN_ITEM_RELATIVE);

    m_ReportLayout = std::move(reportLayout);
    m_HasReportLayout = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USBHIDMouseDriver::MouseReportLayout* USBHIDMouseDriver::FindBestReportLayout(const std::vector<MouseReportLayout>& reportLayouts)
{
    const MouseReportLayout* bestReportLayout = nullptr;
    int bestReportLayoutScore = 0;

    for (const MouseReportLayout& reportLayout : reportLayouts)
    {
        if (!reportLayout.DeltaPositionXField.has_value() || !reportLayout.DeltaPositionYField.has_value()) {
            continue;
        }

        const int reportLayoutScore = GetReportLayoutScore(reportLayout);
        if (bestReportLayout == nullptr || reportLayoutScore > bestReportLayoutScore)
        {
            bestReportLayoutScore = reportLayoutScore;
            bestReportLayout = &reportLayout;
        }
    }
    return bestReportLayout;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHIDMouseDriver::GetReportLayoutScore(const MouseReportLayout& reportLayout)
{
    int reportLayoutScore = 0;

    if (!reportLayout.ButtonFields.empty()) {
        reportLayoutScore += 50;
    }
    if (reportLayout.WheelField.has_value()) {
        reportLayoutScore += 10;
    }

    return reportLayoutScore + int(reportLayout.ButtonFields.size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t USBHIDMouseDriver::GetButtonFlags(const MouseReportLayout& reportLayout, const uint8_t* report, size_t length) const
{
    uint32_t buttonFlags = 0;

    for (const USBHIDReportField& buttonField : reportLayout.ButtonFields)
    {
        uint32_t buttonValue = 0;
        if (USBHIDReportDescriptor::ExtractUnsignedValue(report, length, buttonField, buttonValue) && buttonValue != 0) {
            buttonFlags |= GetButtonFlag(buttonField.Usage);
        }
    }
    return buttonFlags;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPointerButtonMask USBHIDMouseDriver::GetPointerButtons(uint32_t buttonFlags) const
{
    PPointerButtonMask buttons = PPointerButtonMaskNone;

    for (uint32_t buttonUsage = 1; buttonUsage <= 8; ++buttonUsage)
    {
        if ((buttonFlags & GetButtonFlag(buttonUsage)) != 0) {
            buttons |= GetPointerButtonMask(GetButton(buttonUsage));
        }
    }
    return buttons;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHIDMouseDriver::GetFieldValue(const uint8_t* report, size_t length, const std::optional<USBHIDReportField>& field) const
{
    if (!field.has_value()) {
        return 0;
    }

    int32_t value = 0;
    if (USBHIDReportDescriptor::ExtractSignedValue(report, length, field.value(), value)) {
        return int(value);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::IsMouseApplicationField(const USBHIDReportField& field)
{
    if (field.ApplicationUsagePage != USB_HID_USAGE_PAGE_GENERIC_DESKTOP) {
        return false;
    }
    return field.ApplicationUsage == USB_HID_USAGE_GENERIC_DESKTOP_MOUSE || field.ApplicationUsage == USB_HID_USAGE_GENERIC_DESKTOP_POINTER;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::IsButtonField(const USBHIDReportField& field)
{
    return field.UsagePage == USB_HID_USAGE_PAGE_BUTTON && field.Usage >= 1 && field.Usage <= 8 && field.BitSize != 0 && !field.IsConstant() && field.IsVariable();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::IsDeltaPositionXField(const USBHIDReportField& field)
{
    return field.UsagePage == USB_HID_USAGE_PAGE_GENERIC_DESKTOP && field.Usage == USB_HID_USAGE_GENERIC_DESKTOP_X && field.BitSize != 0 && !field.IsConstant() && field.IsVariable() && field.IsRelative();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::IsDeltaPositionYField(const USBHIDReportField& field)
{
    return field.UsagePage == USB_HID_USAGE_PAGE_GENERIC_DESKTOP && field.Usage == USB_HID_USAGE_GENERIC_DESKTOP_Y && field.BitSize != 0 && !field.IsConstant() && field.IsVariable() && field.IsRelative();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDMouseDriver::IsWheelField(const USBHIDReportField& field)
{
    return field.UsagePage == USB_HID_USAGE_PAGE_GENERIC_DESKTOP && field.Usage == USB_HID_USAGE_GENERIC_DESKTOP_WHEEL && field.BitSize != 0 && !field.IsConstant() && field.IsVariable() && field.IsRelative();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMouseButton USBHIDMouseDriver::GetButton(uint32_t buttonUsage)
{
    switch (buttonUsage)
    {
        case 1:  return PMouseButton::Left;
        case 2:  return PMouseButton::Right;
        case 3:  return PMouseButton::Middle;
        case 4:  return PMouseButton::Button4;
        case 5:  return PMouseButton::Button5;
        case 6:  return PMouseButton::Button6;
        case 7:  return PMouseButton::Button7;
        case 8:  return PMouseButton::Button8;
        default: return PMouseButton::None;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t USBHIDMouseDriver::GetButtonFlag(uint32_t buttonUsage)
{
    if (buttonUsage < 1 || buttonUsage > 8) {
        return 0;
    }
    return uint32_t(1u << (buttonUsage - 1));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHIDMouseDriver::MouseReportLayout& USBHIDMouseDriver::GetOrCreateReportLayout(std::vector<MouseReportLayout>& reportLayouts, uint8_t reportID)
{
    for (MouseReportLayout& reportLayout : reportLayouts)
    {
        if (reportLayout.ReportID == reportID) {
            return reportLayout;
        }
    }

    MouseReportLayout reportLayout;
    reportLayout.ReportID = reportID;
    reportLayouts.push_back(reportLayout);
    return reportLayouts.back();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDMouseDriver::LogReport(const uint8_t* report, size_t length)
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
        "HID mouse report ({} bytes): {}",
        length,
        reportText.c_str()
    );
}

} // namespace kernel
