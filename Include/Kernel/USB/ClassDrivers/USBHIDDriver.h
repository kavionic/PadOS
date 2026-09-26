// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <Ptr/PtrTarget.h>
#include <Kernel/USB/USBCommon.h>
#include <Kernel/USB/USBProtocolHID.h>

namespace kernel
{

class USBHIDReportDescriptor;
class USBHostHIDInterface;

struct USBHIDInterfaceInfo
{
    uint8_t                 DeviceAddress = 0;
    uint32_t                InterfaceIndex = 0;
    uint8_t                 InterfaceNumber = 0;
    USB_HID_SubclassCode    Subclass = USB_HID_SubclassCode::NONE;
    USB_HID_ProtocolCode    Protocol = USB_HID_ProtocolCode::NONE;
    uint16_t                ReportDescriptorLength = 0;
    const USBHIDReportDescriptor* ReportDescriptor = nullptr;
    uint8_t                 ReportEndpointIn = USB_INVALID_ENDPOINT;
    size_t                  ReportEndpointInSize = 0;
};

class USBHIDDriver : public PtrTarget
{
public:
    static constexpr int PROBE_SCORE_NONE = 0;
    static constexpr int PROBE_SCORE_BASIC = 1;
    static constexpr int PROBE_SCORE_ENHANCED = 1000;
    static constexpr int PROBE_SCORE_VENDOR = 2000;
    static constexpr int PROBE_SCORE_MAX_STANDARD = 10000;

    explicit USBHIDDriver(USBHostHIDInterface& hidInterface);
    virtual ~USBHIDDriver() override;

    virtual void Startup() {}
    virtual void Close() {}
    virtual void HandleReport(const uint8_t* report, size_t length) = 0;

protected:
    USBHostHIDInterface& GetHIDInterface() const { return m_HIDInterface; }

private:
    USBHostHIDInterface& m_HIDInterface;
};

} // namespace kernel
