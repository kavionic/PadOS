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
