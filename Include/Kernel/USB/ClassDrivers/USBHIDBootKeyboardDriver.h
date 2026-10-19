// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bitset>
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

    void EmitKeyChanges(const std::bitset<KEY_CODE_LIMIT>& pressedKeys, size_t firstKeyIndex, size_t endKeyIndex);
    void EmitKeyEvent(bool pressed, PKeyCodes keyCode);

    std::bitset<KEY_CODE_LIMIT> m_PressedKeys;
    int32_t m_SourceID = -1;
};

} // namespace kernel
