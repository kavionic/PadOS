// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <exception>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/UserInput/UserInputManager.h>
#include <Kernel/USB/ClassDrivers/USBHIDBootKeyboardDriver.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int USBHIDBootKeyboardDriver::Probe(const USBHIDInterfaceInfo& interfaceInfo)
{
    if (interfaceInfo.Subclass == USB_HID_SubclassCode::BOOT_INTERFACE
        && interfaceInfo.Protocol == USB_HID_ProtocolCode::KEYBOARD) {
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
    m_PressedKeys.reset();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::HandleReport(const uint8_t* report, size_t length)
{
    if (m_SourceID == -1 || report == nullptr) {
        return;
    }
    if (length < BOOT_REPORT_SIZE)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "HID boot keyboard short report: {} bytes.", length);
        return;
    }

    // Keyboard-page usage IDs map directly to PadOS physical key codes.
    std::bitset<KEY_CODE_LIMIT> pressedKeys;
    bool rollover = false;
    for (size_t index = FIRST_KEY_INDEX; index < BOOT_REPORT_SIZE; ++index)
    {
        const uint8_t usageID = report[index];
        if (usageID >= 1 && usageID <= 3) {
            rollover = true;
        } else if (usageID >= std::to_underlying(PKeyCodes::A) && usageID < KEY_FIRST_MODIFIER_INDEX) {
            pressedKeys.set(usageID);
        }
    }
    // ErrorRollOver, POSTFail and ErrorUndefined do not describe released keys.
    if (rollover) {
        pressedKeys = m_PressedKeys;
    }
    for (size_t index = 0; index < 8; ++index) {
        pressedKeys.set(KEY_FIRST_MODIFIER_INDEX + index, (report[0] & (1U << index)) != 0);
    }

    EmitKeyChanges(pressedKeys, KEY_FIRST_MODIFIER_INDEX, KEY_LAST_MODIFIER_INDEX + 1);
    EmitKeyChanges(pressedKeys, std::to_underlying(PKeyCodes::A), KEY_FIRST_MODIFIER_INDEX);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::EmitKeyChanges(
    const std::bitset<KEY_CODE_LIMIT>& pressedKeys,
    size_t firstKeyIndex,
    size_t endKeyIndex)
{
    for (size_t keyIndex = firstKeyIndex; keyIndex < endKeyIndex; ++keyIndex)
    {
        if (pressedKeys[keyIndex] != m_PressedKeys[keyIndex])
        {
            try
            {
                EmitKeyEvent(pressedKeys[keyIndex], static_cast<PKeyCodes>(keyIndex));
                m_PressedKeys.set(keyIndex, pressedKeys[keyIndex]);
            }
            catch (const std::exception& exception)
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to queue keyboard event: {}", exception.what());
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDBootKeyboardDriver::EmitKeyEvent(bool pressed, PKeyCodes keyCode)
{
    PRawKeyEvent event{};
    event.EventSize = sizeof(event);
    event.EventType = PInputEventType::RawKeyEvent;
    event.ClassID = PInputClass::Keyboard;
    event.Timestamp = kget_monotonic_time();
    event.EventID = pressed ? PInputEventID::KeyDown : PInputEventID::KeyUp;
    event.SourceID = m_SourceID;
    event.KeyCode = keyCode;
    KUserInputManager::Get().AddEvent(event);
}

} // namespace kernel
