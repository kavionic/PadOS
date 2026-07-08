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
#include <string.h>
#include <utility>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/UserInput/UserInputManager.h>
#include <Kernel/USB/ClassDrivers/USBHIDBootKeyboardDriver.h>

namespace kernel
{

namespace
{

static constexpr uint8_t HID_USAGE_KEY_A = 0x04;
static constexpr uint8_t HID_USAGE_KEY_Z = 0x1d;
static constexpr uint8_t HID_USAGE_LEFT_CONTROL = 0xe0;
static constexpr uint8_t HID_USAGE_RIGHT_GUI = 0xe7;
static constexpr uint8_t HID_MODIFIER_LEFT_SHIFT = 0x02;
static constexpr uint8_t HID_MODIFIER_RIGHT_SHIFT = 0x20;

struct UsageKeyMap
{
    uint8_t     UsageCode;
    PKeyCodes   KeyCode;
    char        NormalText;
    char        ShiftedText;
};

static constexpr UsageKeyMap g_UsageKeyMap[] =
{
    {0x1e, PKeyCodes::NUM_1,    '1', '!'},
    {0x1f, PKeyCodes::NUM_2,    '2', '@'},
    {0x20, PKeyCodes::NUM_3,    '3', '#'},
    {0x21, PKeyCodes::NUM_4,    '4', '$'},
    {0x22, PKeyCodes::NUM_5,    '5', '%'},
    {0x23, PKeyCodes::NUM_6,    '6', '^'},
    {0x24, PKeyCodes::NUM_7,    '7', '&'},
    {0x25, PKeyCodes::NUM_8,    '8', '*'},
    {0x26, PKeyCodes::NUM_9,    '9', '('},
    {0x27, PKeyCodes::NUM_0,    '0', ')'},
    {0x28, PKeyCodes::ENTER,    '\n', '\n'},
    {0x29, PKeyCodes::ESCAPE,   '\0', '\0'},
    {0x2a, PKeyCodes::BACKSPACE,'\0', '\0'},
    {0x2b, PKeyCodes::TAB,      '\t', '\t'},
    {0x2c, PKeyCodes::SPACE,    ' ', ' '},
    {0x2d, PKeyCodes('-'),      '-', '_'},
    {0x2e, PKeyCodes('='),      '=', '+'},
    {0x2f, PKeyCodes('['),      '[', '{'},
    {0x30, PKeyCodes(']'),      ']', '}'},
    {0x31, PKeyCodes('\\'),     '\\', '|'},
    {0x33, PKeyCodes(';'),      ';', ':'},
    {0x34, PKeyCodes('\''),     '\'', '"'},
    {0x35, PKeyCodes('`'),      '`', '~'},
    {0x36, PKeyCodes(','),      ',', '<'},
    {0x37, PKeyCodes('.'),      '.', '>'},
    {0x38, PKeyCodes('/'),      '/', '?'},
    {0x4a, PKeyCodes::HOME,     '\0', '\0'},
    {0x4c, PKeyCodes::DELETE,   '\0', '\0'},
    {0x4d, PKeyCodes::END,      '\0', '\0'},
    {0x4f, PKeyCodes::CURSOR_RIGHT, '\0', '\0'},
    {0x50, PKeyCodes::CURSOR_LEFT,  '\0', '\0'},
    {0x51, PKeyCodes::CURSOR_DOWN,  '\0', '\0'},
    {0x52, PKeyCodes::CURSOR_UP,    '\0', '\0'}
};

} // namespace

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHIDBootKeyboardDriver::Probe(const USBHIDInterfaceInfo& interfaceInfo)
{
    if (interfaceInfo.Subclass == USB_HID_SubclassCode::BOOT_INTERFACE && interfaceInfo.Protocol == USB_HID_ProtocolCode::KEYBOARD) {
        return PROBE_SCORE_BASIC;
    }
    return PROBE_SCORE_NONE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHIDBootKeyboardDriver::USBHIDBootKeyboardDriver(USBHostHIDInterface& hidInterface)
    : USBHIDDriver(hidInterface)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHIDBootKeyboardDriver::~USBHIDBootKeyboardDriver()
{
    Close();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::Startup()
{
    if (m_SourceID == -1) {
        m_SourceID = KUserInputManager::Get().AddSource(PInputClass::Keyboard);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::Close()
{
    if (m_SourceID != -1)
    {
        KUserInputManager::Get().RemoveSource(m_SourceID);
        m_SourceID = -1;
    }
    m_PreviousReport = {};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::HandleReport(const uint8_t* report, size_t length)
{
    if (m_SourceID == -1) {
        return;
    }
    if (report == nullptr) {
        return;
    }
    if (length < BOOT_REPORT_SIZE)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID boot keyboard short report: {} bytes.", length);
        return;
    }

    const uint8_t modifiers = report[0];

    EmitModifierChanges(modifiers);
    EmitKeyChanges(report, modifiers);

    for (size_t reportIndex = 0; reportIndex < m_PreviousReport.size(); ++reportIndex) {
        m_PreviousReport[reportIndex] = report[reportIndex];
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::EmitModifierChanges(uint8_t modifiers)
{
    const uint8_t previousModifiers = m_PreviousReport[0];
    const uint8_t changedModifiers = modifiers ^ previousModifiers;

    for (uint8_t modifierIndex = 0; modifierIndex < 8; ++modifierIndex)
    {
        const uint8_t modifierMask = uint8_t(1 << modifierIndex);
        if ((changedModifiers & modifierMask) != 0)
        {
            const uint8_t usageCode = GetModifierUsageCode(modifierMask);
            EmitKeyEvent((modifiers & modifierMask) != 0, usageCode, modifiers);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::EmitKeyChanges(const uint8_t* report, uint8_t modifiers)
{
    for (size_t keyIndex = FIRST_KEY_INDEX; keyIndex < m_PreviousReport.size(); ++keyIndex)
    {
        const uint8_t usageCode = m_PreviousReport[keyIndex];
        if (usageCode != 0 && !ReportContains(report, usageCode)) {
            EmitKeyEvent(false, usageCode, modifiers);
        }
    }

    for (size_t keyIndex = FIRST_KEY_INDEX; keyIndex < m_PreviousReport.size(); ++keyIndex)
    {
        const uint8_t usageCode = report[keyIndex];
        if (usageCode != 0 && !ReportContains(m_PreviousReport.data(), usageCode)) {
            EmitKeyEvent(true, usageCode, modifiers);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::EmitKeyEvent(bool pressed, uint8_t usageCode, uint8_t modifiers)
{
    const PKeyCodes keyCode = GetKeyCode(usageCode);
    if (keyCode == PKeyCodes::NONE) {
        return;
    }

    PKeyEvent event;
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::KeyEvent;
    event.ClassID = PInputClass::Keyboard;
    event.Timestamp = kget_monotonic_time();
    event.EventID = pressed ? PInputEventID::KeyDown : PInputEventID::KeyUp;
    event.SourceID = m_SourceID;
    event.m_KeyCode = keyCode;
    memset(event.m_Text, 0, sizeof(event.m_Text));

    if (pressed)
    {
        const char textCharacter = GetTextCharacter(usageCode, modifiers);
        if (textCharacter != '\0') {
            event.m_Text[0] = textCharacter;
        }
    }

    try {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(
            LogCategoryUSBHost,
            "HID boot keyboard event: source={} key={} usage={:02x} modifiers={:02x} {} text='{}'.",
            event.SourceID,
            int(std::to_underlying(event.m_KeyCode)),
            int(usageCode),
            int(modifiers),
            pressed ? "down" : "up",
            event.m_Text
        );
        KUserInputManager::Get().AddEvent(event);
    } catch (const std::exception& exception) {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "HID boot keyboard failed to queue key event: {}", exception.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDBootKeyboardDriver::ReportContains(const uint8_t* report, uint8_t usageCode)
{
    for (size_t keyIndex = FIRST_KEY_INDEX; keyIndex < BOOT_REPORT_SIZE; ++keyIndex) {
        if (report[keyIndex] == usageCode) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PKeyCodes USBHIDBootKeyboardDriver::GetKeyCode(uint8_t usageCode)
{
    if (usageCode >= HID_USAGE_KEY_A && usageCode <= HID_USAGE_KEY_Z) {
        return PKeyCodes(std::to_underlying(PKeyCodes::A) + usageCode - HID_USAGE_KEY_A);
    }
    if (IsModifierUsage(usageCode))
    {
        switch (usageCode)
        {
            case 0xe0:
            case 0xe4:
                return PKeyCodes::CTRL;
            case 0xe1:
            case 0xe5:
                return PKeyCodes::SHIFT;
            case 0xe2:
            case 0xe6:
                return PKeyCodes::ALT;
            default:
                return PKeyCodes::NONE;
        }
    }
    for (const UsageKeyMap& entry : g_UsageKeyMap) {
        if (entry.UsageCode == usageCode) {
            return entry.KeyCode;
        }
    }
    return PKeyCodes::NONE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

char USBHIDBootKeyboardDriver::GetTextCharacter(uint8_t usageCode, uint8_t modifiers)
{
    const bool shiftPressed = IsShiftPressed(modifiers);

    if (usageCode >= HID_USAGE_KEY_A && usageCode <= HID_USAGE_KEY_Z)
    {
        const char baseCharacter = shiftPressed ? 'A' : 'a';
        return char(baseCharacter + usageCode - HID_USAGE_KEY_A);
    }
    for (const UsageKeyMap& entry : g_UsageKeyMap)
    {
        if (entry.UsageCode == usageCode) {
            return shiftPressed ? entry.ShiftedText : entry.NormalText;
        }
    }
    return '\0';
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDBootKeyboardDriver::IsShiftPressed(uint8_t modifiers)
{
    return (modifiers & (HID_MODIFIER_LEFT_SHIFT | HID_MODIFIER_RIGHT_SHIFT)) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDBootKeyboardDriver::IsModifierUsage(uint8_t usageCode)
{
    return usageCode >= HID_USAGE_LEFT_CONTROL && usageCode <= HID_USAGE_RIGHT_GUI;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t USBHIDBootKeyboardDriver::GetModifierUsageCode(uint8_t modifierMask)
{
    for (uint8_t modifierIndex = 0; modifierIndex < 8; ++modifierIndex) {
        if (modifierMask == uint8_t(1 << modifierIndex)) {
            return HID_USAGE_LEFT_CONTROL + modifierIndex;
        }
    }
    return 0;
}

} // namespace kernel
