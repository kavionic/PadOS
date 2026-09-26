// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
