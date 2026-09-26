// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <stdint.h>

#include <GUI/GUIEvent.h>
#include <Kernel/USB/ClassDrivers/USBHIDDriver.h>

namespace kernel
{

class USBHIDBootKeyboardDriver : public USBHIDDriver
{
public:
    static int Probe(const USBHIDInterfaceInfo& interfaceInfo);

    explicit USBHIDBootKeyboardDriver(USBHostHIDInterface& hidInterface);
    virtual ~USBHIDBootKeyboardDriver() override;

    virtual void Startup() override;
    virtual void Close() override;
    virtual void HandleReport(const uint8_t* report, size_t length) override;

private:
    static constexpr size_t BOOT_REPORT_SIZE = 8;
    static constexpr size_t FIRST_KEY_INDEX = 2;

    void EmitModifierChanges(uint8_t modifiers);
    void EmitKeyChanges(const uint8_t* report, uint8_t modifiers);
    void EmitKeyEvent(bool pressed, uint8_t usageCode, uint8_t modifiers);

    static bool ReportContains(const uint8_t* report, uint8_t usageCode);
    static PKeyCodes GetKeyCode(uint8_t usageCode);
    static char GetTextCharacter(uint8_t usageCode, uint8_t modifiers);
    static bool IsShiftPressed(uint8_t modifiers);
    static bool IsModifierUsage(uint8_t usageCode);
    static uint8_t GetModifierUsageCode(uint8_t modifierMask);

    int32_t                 m_SourceID = -1;
    std::array<uint8_t, 8>  m_PreviousReport = {};
};

} // namespace kernel
