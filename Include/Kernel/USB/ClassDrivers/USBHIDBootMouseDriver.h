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

#include <stdint.h>

#include <GUI/GUIEvent.h>
#include <Kernel/USB/ClassDrivers/USBHIDDriver.h>

namespace kernel
{

class USBHIDBootMouseDriver : public USBHIDDriver
{
public:
    static int Probe(const USBHIDInterfaceInfo& interfaceInfo);

    explicit USBHIDBootMouseDriver(USBHostHIDInterface& hidInterface);
    virtual ~USBHIDBootMouseDriver() override;

    virtual void Startup() override;
    virtual void Close() override;
    virtual void HandleReport(const uint8_t* report, size_t length) override;

private:
    void EmitButtonEvent(PMouseButton button, PPointerButtonMask buttons, bool pressed);
    void EmitMoveEvent(int deltaPositionX, int deltaPositionY, PPointerButtonMask buttons);
    void EmitWheelEvent(int deltaWheel, PPointerButtonMask buttons);

    static PMouseButton GetButton(uint8_t buttonFlag);
    static PPointerButtonMask GetButtons(uint8_t buttonFlags);
    static int GetWheelDelta(const uint8_t* report, size_t length);
    static void LogReport(const uint8_t* report, size_t length);

    int32_t m_SourceID = -1;
    uint8_t m_PreviousButtons = 0;
};

} // namespace kernel
