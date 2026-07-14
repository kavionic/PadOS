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

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace kernel
{

static constexpr uint8_t USB_HID_ITEM_LONG_PREFIX = 0xfe;

static constexpr uint8_t USB_HID_ITEM_TYPE_MAIN   = 0x00;
static constexpr uint8_t USB_HID_ITEM_TYPE_GLOBAL = 0x01;
static constexpr uint8_t USB_HID_ITEM_TYPE_LOCAL  = 0x02;

static constexpr uint8_t USB_HID_MAIN_ITEM_INPUT_TAG          = 0x08;
static constexpr uint8_t USB_HID_MAIN_ITEM_OUTPUT_TAG         = 0x09;
static constexpr uint8_t USB_HID_MAIN_ITEM_COLLECTION_TAG     = 0x0a;
static constexpr uint8_t USB_HID_MAIN_ITEM_FEATURE_TAG        = 0x0b;
static constexpr uint8_t USB_HID_MAIN_ITEM_END_COLLECTION_TAG = 0x0c;

static constexpr uint8_t USB_HID_GLOBAL_ITEM_USAGE_PAGE_TAG      = 0x00;
static constexpr uint8_t USB_HID_GLOBAL_ITEM_LOGICAL_MINIMUM_TAG = 0x01;
static constexpr uint8_t USB_HID_GLOBAL_ITEM_LOGICAL_MAXIMUM_TAG = 0x02;
static constexpr uint8_t USB_HID_GLOBAL_ITEM_REPORT_SIZE_TAG     = 0x07;
static constexpr uint8_t USB_HID_GLOBAL_ITEM_REPORT_ID_TAG       = 0x08;
static constexpr uint8_t USB_HID_GLOBAL_ITEM_REPORT_COUNT_TAG    = 0x09;
static constexpr uint8_t USB_HID_GLOBAL_ITEM_PUSH_TAG            = 0x0a;
static constexpr uint8_t USB_HID_GLOBAL_ITEM_POP_TAG             = 0x0b;

static constexpr uint8_t USB_HID_LOCAL_ITEM_USAGE_TAG         = 0x00;
static constexpr uint8_t USB_HID_LOCAL_ITEM_USAGE_MINIMUM_TAG = 0x01;
static constexpr uint8_t USB_HID_LOCAL_ITEM_USAGE_MAXIMUM_TAG = 0x02;

static constexpr uint16_t USB_HID_USAGE_PAGE_GENERIC_DESKTOP = 0x01;
static constexpr uint16_t USB_HID_USAGE_PAGE_BUTTON          = 0x09;

static constexpr uint32_t USB_HID_USAGE_GENERIC_DESKTOP_POINTER = 0x01;
static constexpr uint32_t USB_HID_USAGE_GENERIC_DESKTOP_MOUSE   = 0x02;
static constexpr uint32_t USB_HID_USAGE_GENERIC_DESKTOP_X       = 0x30;
static constexpr uint32_t USB_HID_USAGE_GENERIC_DESKTOP_Y       = 0x31;
static constexpr uint32_t USB_HID_USAGE_GENERIC_DESKTOP_WHEEL   = 0x38;

enum class USBHIDReportType : uint8_t
{
    Input,
    Output,
    Feature
};

enum class USBHIDCollectionType : uint8_t
{
    Physical      = 0x00,
    Application   = 0x01,
    Logical       = 0x02,
    Report        = 0x03,
    NamedArray    = 0x04,
    UsageSwitch   = 0x05,
    UsageModifier = 0x06
};

static constexpr uint32_t USB_HID_MAIN_ITEM_CONSTANT      = 0x0001;
static constexpr uint32_t USB_HID_MAIN_ITEM_VARIABLE      = 0x0002;
static constexpr uint32_t USB_HID_MAIN_ITEM_RELATIVE      = 0x0004;
static constexpr uint32_t USB_HID_MAIN_ITEM_WRAP          = 0x0008;
static constexpr uint32_t USB_HID_MAIN_ITEM_NON_LINEAR    = 0x0010;
static constexpr uint32_t USB_HID_MAIN_ITEM_NO_PREFERRED  = 0x0020;
static constexpr uint32_t USB_HID_MAIN_ITEM_NULL_STATE    = 0x0040;
static constexpr uint32_t USB_HID_MAIN_ITEM_VOLATILE      = 0x0080;
static constexpr uint32_t USB_HID_MAIN_ITEM_BUFFERED_BYTE = 0x0100;

struct USBHIDReportField
{
    USBHIDReportType ReportType = USBHIDReportType::Input;
    uint8_t          ReportID = 0;
    uint16_t         UsagePage = 0;
    uint32_t         Usage = 0;
    uint32_t         FullUsage = 0;
    uint16_t         ApplicationUsagePage = 0;
    uint32_t         ApplicationUsage = 0;
    size_t           BitOffset = 0;
    size_t           BitSize = 0;
    int32_t          LogicalMinimum = 0;
    int32_t          LogicalMaximum = 0;
    uint32_t         Flags = 0;

    bool IsConstant() const { return (Flags & USB_HID_MAIN_ITEM_CONSTANT) != 0; }
    bool IsVariable() const { return (Flags & USB_HID_MAIN_ITEM_VARIABLE) != 0; }
    bool IsRelative() const { return (Flags & USB_HID_MAIN_ITEM_RELATIVE) != 0; }
};

struct USBHIDReportBitSize
{
    USBHIDReportType ReportType = USBHIDReportType::Input;
    uint8_t          ReportID = 0;
    size_t           BitSize = 0;
};

class USBHIDReportDescriptor
{
public:
    bool Parse(const uint8_t* descriptor, size_t length);
    void Clear();

    const std::vector<USBHIDReportField>& GetFields() const { return m_Fields; }
    bool IsValid() const { return m_IsValid; }

    size_t GetReportBitSize(USBHIDReportType reportType, uint8_t reportID) const;

    static bool ExtractUnsignedValue(const uint8_t* report, size_t length, const USBHIDReportField& field, uint32_t& outValue);
    static bool ExtractSignedValue(const uint8_t* report, size_t length, const USBHIDReportField& field, int32_t& outValue);

private:
    std::vector<USBHIDReportField>   m_Fields;
    std::vector<USBHIDReportBitSize> m_ReportBitSizes;
    bool                             m_IsValid = false;
};

} // namespace kernel
