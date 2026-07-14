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

#include <optional>
#include <stdint.h>
#include <vector>

#include <GUI/GUIEvent.h>
#include <Kernel/USB/ClassDrivers/USBHIDDriver.h>
#include <Kernel/USB/ClassDrivers/USBHIDReportParser.h>

namespace kernel
{

class USBHIDMouseDriver : public USBHIDDriver
{
public:
    static int Probe(const USBHIDInterfaceInfo& interfaceInfo);

    explicit USBHIDMouseDriver(USBHostHIDInterface& hidInterface);
    virtual ~USBHIDMouseDriver() override;

    virtual void Startup() override;
    virtual void Close() override;
    virtual void HandleReport(const uint8_t* report, size_t length) override;

private:
    struct MouseReportLayout
    {
        uint8_t                         ReportID = 0;
        std::vector<USBHIDReportField>  ButtonFields;
        std::optional<USBHIDReportField> DeltaPositionXField;
        std::optional<USBHIDReportField> DeltaPositionYField;
        std::optional<USBHIDReportField> WheelField;
    };

    void EmitButtonEvent(PMouseButton button, PPointerButtonMask buttons, bool pressed);
    void EmitMoveEvent(int deltaPositionX, int deltaPositionY, PPointerButtonMask buttons);
    void EmitWheelEvent(int deltaWheel, PPointerButtonMask buttons);

    bool BuildReportLayout();
    bool BuildDescriptorReportLayout(const USBHIDReportDescriptor& reportDescriptor);
    static bool HasDescriptorReportLayout(const USBHIDReportDescriptor& reportDescriptor);
    static void BuildDescriptorReportLayouts(const USBHIDReportDescriptor& reportDescriptor, std::vector<MouseReportLayout>& reportLayouts);
    void BuildBootReportLayout();
    static const MouseReportLayout* FindBestReportLayout(const std::vector<MouseReportLayout>& reportLayouts);
    static int GetReportLayoutScore(const MouseReportLayout& reportLayout);

    uint32_t GetButtonFlags(const MouseReportLayout& reportLayout, const uint8_t* report, size_t length) const;
    PPointerButtonMask GetPointerButtons(uint32_t buttonFlags) const;
    int GetFieldValue(const uint8_t* report, size_t length, const std::optional<USBHIDReportField>& field) const;

    static bool IsMouseApplicationField(const USBHIDReportField& field);
    static bool IsButtonField(const USBHIDReportField& field);
    static bool IsDeltaPositionXField(const USBHIDReportField& field);
    static bool IsDeltaPositionYField(const USBHIDReportField& field);
    static bool IsWheelField(const USBHIDReportField& field);
    static PMouseButton GetButton(uint32_t buttonUsage);
    static uint32_t GetButtonFlag(uint32_t buttonUsage);
    static MouseReportLayout& GetOrCreateReportLayout(std::vector<MouseReportLayout>& reportLayouts, uint8_t reportID);
    static void LogReport(const uint8_t* report, size_t length);

    int32_t             m_SourceID = -1;
    uint32_t            m_PreviousButtons = 0;
    bool                m_HasReportLayout = false;
    MouseReportLayout   m_ReportLayout;
};

} // namespace kernel
