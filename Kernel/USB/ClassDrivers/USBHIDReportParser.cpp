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

#include <algorithm>
#include <limits>

#include <Kernel/USB/ClassDrivers/USBHIDReportParser.h>

namespace kernel
{

namespace USBHIDReportParserInternal
{

struct HIDGlobalState
{
    uint16_t UsagePage = 0;
    int32_t  LogicalMinimum = 0;
    int32_t  LogicalMaximum = 0;
    size_t   ReportSize = 0;
    size_t   ReportCount = 0;
    uint8_t  ReportID = 0;
};

struct HIDLocalState
{
    std::vector<uint32_t> Usages;
    uint32_t              UsageMinimum = 0;
    uint32_t              UsageMaximum = 0;
    bool                  HasUsageMinimum = false;
    bool                  HasUsageMaximum = false;

    void Clear()
    {
        Usages.clear();
        UsageMinimum = 0;
        UsageMaximum = 0;
        HasUsageMinimum = false;
        HasUsageMaximum = false;
    }
};

struct HIDCollectionState
{
    USBHIDCollectionType Type = USBHIDCollectionType::Physical;
    uint16_t             UsagePage = 0;
    uint32_t             Usage = 0;
};

static uint32_t ReadUnsignedItemData(const uint8_t* data, size_t size)
{
    uint32_t value = 0;

    for (size_t byteIndex = 0; byteIndex < size; ++byteIndex) {
        value |= uint32_t(data[byteIndex]) << (byteIndex * 8);
    }
    return value;
}

static int32_t ReadSignedItemData(const uint8_t* data, size_t size)
{
    const uint32_t value = ReadUnsignedItemData(data, size);

    switch (size)
    {
        case 1:  return int32_t(int8_t(value));
        case 2:  return int32_t(int16_t(value));
        case 4:  return int32_t(value);
        default: return int32_t(value);
    }
}

static size_t GetShortItemDataSize(uint8_t itemPrefix)
{
    const uint8_t sizeCode = itemPrefix & 0x03;
    return (sizeCode == 0x03) ? 4 : sizeCode;
}

static uint32_t MakeFullUsage(uint16_t usagePage, uint32_t usage)
{
    if (usage > 0xffff) {
        return usage;
    }
    return (uint32_t(usagePage) << 16) | usage;
}

static bool AddWithOverflowCheck(size_t lhs, size_t rhs, size_t& outResult)
{
    if (lhs > std::numeric_limits<size_t>::max() - rhs) {
        return false;
    }
    outResult = lhs + rhs;
    return true;
}

static bool MultiplyWithOverflowCheck(size_t lhs, size_t rhs, size_t& outResult)
{
    if (lhs != 0 && rhs > std::numeric_limits<size_t>::max() / lhs) {
        return false;
    }
    outResult = lhs * rhs;
    return true;
}

class HIDReportDescriptorParser
{
public:
    bool Parse(const uint8_t* descriptor, size_t length);

    const std::vector<USBHIDReportField>& GetFields() const { return m_Fields; }
    const std::vector<USBHIDReportBitSize>& GetReportBitSizes() const { return m_ReportBitSizes; }

private:
    bool ParseShortItem(uint8_t itemType, uint8_t itemTag, const uint8_t* itemData, size_t itemSize);
    bool ParseMainItem(uint8_t itemTag, uint32_t itemData);
    bool ParseGlobalItem(uint8_t itemTag, const uint8_t* itemData, size_t itemSize);
    bool ParseLocalItem(uint8_t itemTag, const uint8_t* itemData, size_t itemSize);

    bool AddReportFields(USBHIDReportType reportType, uint32_t flags);
    bool PushCollection(uint32_t collectionType);
    bool PopCollection();

    uint32_t GetUsage(size_t fieldIndex) const;
    HIDCollectionState GetApplicationCollection() const;
    size_t GetReportBitSize(USBHIDReportType reportType, uint8_t reportID) const;
    bool SetReportBitSize(USBHIDReportType reportType, uint8_t reportID, size_t bitSize);

    HIDGlobalState                  m_GlobalState;
    HIDLocalState                   m_LocalState;
    std::vector<HIDGlobalState>     m_GlobalStack;
    std::vector<HIDCollectionState> m_CollectionStack;
    std::vector<USBHIDReportField>  m_Fields;
    std::vector<USBHIDReportBitSize> m_ReportBitSizes;
};

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::Parse(const uint8_t* descriptor, size_t length)
{
    if (descriptor == nullptr && length != 0) {
        return false;
    }

    size_t descriptorOffset = 0;

    while (descriptorOffset < length)
    {
        const uint8_t itemPrefix = descriptor[descriptorOffset++];

        if (itemPrefix == USB_HID_ITEM_LONG_PREFIX)
        {
            if (length - descriptorOffset < 2) {
                return false;
            }

            const size_t itemSize = descriptor[descriptorOffset];
            descriptorOffset += 2;

            if (itemSize > length - descriptorOffset) {
                return false;
            }
            descriptorOffset += itemSize;
            continue;
        }

        const size_t itemSize = GetShortItemDataSize(itemPrefix);
        if (itemSize > length - descriptorOffset) {
            return false;
        }

        const uint8_t itemType = (itemPrefix >> 2) & 0x03;
        const uint8_t itemTag = (itemPrefix >> 4) & 0x0f;
        const uint8_t* itemData = descriptor + descriptorOffset;

        if (!ParseShortItem(itemType, itemTag, itemData, itemSize)) {
            return false;
        }
        descriptorOffset += itemSize;
    }
    return m_CollectionStack.empty();
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::ParseShortItem(uint8_t itemType, uint8_t itemTag, const uint8_t* itemData, size_t itemSize)
{
    switch (itemType)
    {
        case USB_HID_ITEM_TYPE_MAIN:
            return ParseMainItem(itemTag, ReadUnsignedItemData(itemData, itemSize));

        case USB_HID_ITEM_TYPE_GLOBAL:
            return ParseGlobalItem(itemTag, itemData, itemSize);

        case USB_HID_ITEM_TYPE_LOCAL:
            return ParseLocalItem(itemTag, itemData, itemSize);

        default:
            return true;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::ParseMainItem(uint8_t itemTag, uint32_t itemData)
{
    bool result = true;

    switch (itemTag)
    {
        case USB_HID_MAIN_ITEM_INPUT_TAG:
            result = AddReportFields(USBHIDReportType::Input, itemData);
            break;

        case USB_HID_MAIN_ITEM_OUTPUT_TAG:
            result = AddReportFields(USBHIDReportType::Output, itemData);
            break;

        case USB_HID_MAIN_ITEM_COLLECTION_TAG:
            result = PushCollection(itemData);
            break;

        case USB_HID_MAIN_ITEM_FEATURE_TAG:
            result = AddReportFields(USBHIDReportType::Feature, itemData);
            break;

        case USB_HID_MAIN_ITEM_END_COLLECTION_TAG:
            result = PopCollection();
            break;

        default:
            break;
    }

    m_LocalState.Clear();
    return result;
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::ParseGlobalItem(uint8_t itemTag, const uint8_t* itemData, size_t itemSize)
{
    switch (itemTag)
    {
        case USB_HID_GLOBAL_ITEM_USAGE_PAGE_TAG:
            m_GlobalState.UsagePage = uint16_t(ReadUnsignedItemData(itemData, itemSize));
            break;

        case USB_HID_GLOBAL_ITEM_LOGICAL_MINIMUM_TAG:
            m_GlobalState.LogicalMinimum = ReadSignedItemData(itemData, itemSize);
            break;

        case USB_HID_GLOBAL_ITEM_LOGICAL_MAXIMUM_TAG:
            m_GlobalState.LogicalMaximum = ReadSignedItemData(itemData, itemSize);
            break;

        case USB_HID_GLOBAL_ITEM_REPORT_SIZE_TAG:
            m_GlobalState.ReportSize = ReadUnsignedItemData(itemData, itemSize);
            break;

        case USB_HID_GLOBAL_ITEM_REPORT_ID_TAG:
            m_GlobalState.ReportID = uint8_t(ReadUnsignedItemData(itemData, itemSize));
            break;

        case USB_HID_GLOBAL_ITEM_REPORT_COUNT_TAG:
            m_GlobalState.ReportCount = ReadUnsignedItemData(itemData, itemSize);
            break;

        case USB_HID_GLOBAL_ITEM_PUSH_TAG:
            m_GlobalStack.push_back(m_GlobalState);
            break;

        case USB_HID_GLOBAL_ITEM_POP_TAG:
            if (m_GlobalStack.empty()) {
                return false;
            }
            m_GlobalState = m_GlobalStack.back();
            m_GlobalStack.pop_back();
            break;

        default:
            break;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::ParseLocalItem(uint8_t itemTag, const uint8_t* itemData, size_t itemSize)
{
    switch (itemTag)
    {
        case USB_HID_LOCAL_ITEM_USAGE_TAG:
            m_LocalState.Usages.push_back(MakeFullUsage(m_GlobalState.UsagePage, ReadUnsignedItemData(itemData, itemSize)));
            break;

        case USB_HID_LOCAL_ITEM_USAGE_MINIMUM_TAG:
            m_LocalState.UsageMinimum = MakeFullUsage(m_GlobalState.UsagePage, ReadUnsignedItemData(itemData, itemSize));
            m_LocalState.HasUsageMinimum = true;
            break;

        case USB_HID_LOCAL_ITEM_USAGE_MAXIMUM_TAG:
            m_LocalState.UsageMaximum = MakeFullUsage(m_GlobalState.UsagePage, ReadUnsignedItemData(itemData, itemSize));
            m_LocalState.HasUsageMaximum = true;
            break;

        default:
            break;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::AddReportFields(USBHIDReportType reportType, uint32_t flags)
{
    size_t fieldsBitSize = 0;
    if (!MultiplyWithOverflowCheck(m_GlobalState.ReportSize, m_GlobalState.ReportCount, fieldsBitSize)) {
        return false;
    }

    const uint8_t reportID = m_GlobalState.ReportID;
    const size_t reportBitOffset = GetReportBitSize(reportType, reportID);
    size_t nextReportBitSize = 0;
    if (!AddWithOverflowCheck(reportBitOffset, fieldsBitSize, nextReportBitSize)) {
        return false;
    }

    if ((flags & USB_HID_MAIN_ITEM_CONSTANT) == 0)
    {
        const HIDCollectionState applicationCollection = GetApplicationCollection();

        for (size_t fieldIndex = 0; fieldIndex < m_GlobalState.ReportCount; ++fieldIndex)
        {
            size_t fieldBitOffset = 0;
            if (!MultiplyWithOverflowCheck(fieldIndex, m_GlobalState.ReportSize, fieldBitOffset)) {
                return false;
            }
            if (!AddWithOverflowCheck(reportBitOffset, fieldBitOffset, fieldBitOffset)) {
                return false;
            }

            const uint32_t fullUsage = GetUsage(fieldIndex);

            USBHIDReportField field;
            field.ReportType = reportType;
            field.ReportID = reportID;
            field.UsagePage = uint16_t(fullUsage >> 16);
            field.Usage = fullUsage & 0xffff;
            field.FullUsage = fullUsage;
            field.ApplicationUsagePage = applicationCollection.UsagePage;
            field.ApplicationUsage = applicationCollection.Usage;
            field.BitOffset = fieldBitOffset;
            field.BitSize = m_GlobalState.ReportSize;
            field.LogicalMinimum = m_GlobalState.LogicalMinimum;
            field.LogicalMaximum = m_GlobalState.LogicalMaximum;
            field.Flags = flags;
            m_Fields.push_back(field);
        }
    }

    return SetReportBitSize(reportType, reportID, nextReportBitSize);
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::PushCollection(uint32_t collectionType)
{
    HIDCollectionState collection;
    collection.Type = USBHIDCollectionType(uint8_t(collectionType));

    const uint32_t fullUsage = GetUsage(0);
    collection.UsagePage = uint16_t(fullUsage >> 16);
    collection.Usage = fullUsage & 0xffff;

    m_CollectionStack.push_back(collection);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::PopCollection()
{
    if (m_CollectionStack.empty()) {
        return false;
    }
    m_CollectionStack.pop_back();
    return true;
}

///////////////////////////////////////////////////////////////////////////////

uint32_t HIDReportDescriptorParser::GetUsage(size_t fieldIndex) const
{
    if (!m_LocalState.Usages.empty())
    {
        if (fieldIndex < m_LocalState.Usages.size()) {
            return m_LocalState.Usages[fieldIndex];
        }
        return m_LocalState.Usages.back();
    }

    if (m_LocalState.HasUsageMinimum && m_LocalState.HasUsageMaximum && m_LocalState.UsageMinimum <= m_LocalState.UsageMaximum)
    {
        const uint32_t usage = m_LocalState.UsageMinimum + uint32_t(fieldIndex);
        return std::min(usage, m_LocalState.UsageMaximum);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

HIDCollectionState HIDReportDescriptorParser::GetApplicationCollection() const
{
    for (auto collectionIterator = m_CollectionStack.rbegin(); collectionIterator != m_CollectionStack.rend(); ++collectionIterator)
    {
        if (collectionIterator->Type == USBHIDCollectionType::Application) {
            return *collectionIterator;
        }
    }
    return HIDCollectionState();
}

///////////////////////////////////////////////////////////////////////////////

size_t HIDReportDescriptorParser::GetReportBitSize(USBHIDReportType reportType, uint8_t reportID) const
{
    for (const USBHIDReportBitSize& reportBitSize : m_ReportBitSizes)
    {
        if (reportBitSize.ReportType == reportType && reportBitSize.ReportID == reportID) {
            return reportBitSize.BitSize;
        }
    }
    return (reportID != 0) ? 8 : 0;
}

///////////////////////////////////////////////////////////////////////////////

bool HIDReportDescriptorParser::SetReportBitSize(USBHIDReportType reportType, uint8_t reportID, size_t bitSize)
{
    for (USBHIDReportBitSize& reportBitSize : m_ReportBitSizes)
    {
        if (reportBitSize.ReportType == reportType && reportBitSize.ReportID == reportID)
        {
            reportBitSize.BitSize = bitSize;
            return true;
        }
    }

    USBHIDReportBitSize reportBitSize;
    reportBitSize.ReportType = reportType;
    reportBitSize.ReportID = reportID;
    reportBitSize.BitSize = bitSize;
    m_ReportBitSizes.push_back(reportBitSize);
    return true;
}

static int32_t SignExtendValue(uint32_t value, size_t bitSize)
{
    if (bitSize == 0) {
        return 0;
    }
    if (bitSize >= 32) {
        return int32_t(value);
    }

    const uint32_t signBit = uint32_t(1u << (bitSize - 1));
    const uint32_t valueMask = signBit | (signBit - 1);

    if ((value & signBit) != 0) {
        return int32_t(value | ~valueMask);
    }
    return int32_t(value & valueMask);
}

} // namespace USBHIDReportParserInternal

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDReportDescriptor::Parse(const uint8_t* descriptor, size_t length)
{
    Clear();

    USBHIDReportParserInternal::HIDReportDescriptorParser parser;
    if (!parser.Parse(descriptor, length)) {
        return false;
    }

    m_Fields = parser.GetFields();
    m_ReportBitSizes = parser.GetReportBitSizes();
    m_IsValid = true;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHIDReportDescriptor::Clear()
{
    m_Fields.clear();
    m_ReportBitSizes.clear();
    m_IsValid = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t USBHIDReportDescriptor::GetReportBitSize(USBHIDReportType reportType, uint8_t reportID) const
{
    for (const USBHIDReportBitSize& reportBitSize : m_ReportBitSizes)
    {
        if (reportBitSize.ReportType == reportType && reportBitSize.ReportID == reportID) {
            return reportBitSize.BitSize;
        }
    }
    return (reportID != 0) ? 8 : 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDReportDescriptor::ExtractUnsignedValue(const uint8_t* report, size_t length, const USBHIDReportField& field, uint32_t& outValue)
{
    outValue = 0;

    if (report == nullptr) {
        return false;
    }
    if (field.BitSize == 0 || field.BitSize > 32) {
        return false;
    }

    const size_t reportBitSize = length * 8;
    if (field.BitOffset > reportBitSize) {
        return false;
    }
    if (field.BitSize > reportBitSize - field.BitOffset) {
        return false;
    }

    for (size_t fieldBitIndex = 0; fieldBitIndex < field.BitSize; ++fieldBitIndex)
    {
        const size_t reportBitIndex = field.BitOffset + fieldBitIndex;
        const size_t reportByteIndex = reportBitIndex / 8;
        const uint8_t reportBitMask = uint8_t(1u << (reportBitIndex % 8));

        if ((report[reportByteIndex] & reportBitMask) != 0) {
            outValue |= uint32_t(1u << fieldBitIndex);
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHIDReportDescriptor::ExtractSignedValue(const uint8_t* report, size_t length, const USBHIDReportField& field, int32_t& outValue)
{
    uint32_t unsignedValue = 0;
    if (!ExtractUnsignedValue(report, length, field, unsignedValue)) {
        return false;
    }

    if (field.LogicalMinimum < 0) {
        outValue = USBHIDReportParserInternal::SignExtendValue(unsignedValue, field.BitSize);
    } else {
        outValue = int32_t(unsignedValue);
    }
    return true;
}

} // namespace kernel
