#include <gtest/gtest.h>

#include <Kernel/USB/ClassDrivers/USBHIDReportParser.h>

using namespace kernel;

namespace USBHIDReportParserTest
{

const USBHIDReportField* FindInputField(const USBHIDReportDescriptor& reportDescriptor, uint16_t usagePage, uint32_t usage)
{
    for (const USBHIDReportField& field : reportDescriptor.GetFields())
    {
        if (field.ReportType == USBHIDReportType::Input && field.UsagePage == usagePage && field.Usage == usage) {
            return &field;
        }
    }
    return nullptr;
}

} // namespace USBHIDReportParserTest

TEST(USBHIDReportParser, ParsesBootMouseDescriptor)
{
    static constexpr uint8_t reportDescriptorBytes[] =
    {
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x02,       // Usage (Mouse)
        0xa1, 0x01,       // Collection (Application)
        0x09, 0x01,       //   Usage (Pointer)
        0xa1, 0x00,       //   Collection (Physical)
        0x05, 0x09,       //     Usage Page (Button)
        0x19, 0x01,       //     Usage Minimum (1)
        0x29, 0x03,       //     Usage Maximum (3)
        0x15, 0x00,       //     Logical Minimum (0)
        0x25, 0x01,       //     Logical Maximum (1)
        0x75, 0x01,       //     Report Size (1)
        0x95, 0x03,       //     Report Count (3)
        0x81, 0x02,       //     Input (Data, Variable, Absolute)
        0x75, 0x05,       //     Report Size (5)
        0x95, 0x01,       //     Report Count (1)
        0x81, 0x01,       //     Input (Constant)
        0x05, 0x01,       //     Usage Page (Generic Desktop)
        0x09, 0x30,       //     Usage (X)
        0x09, 0x31,       //     Usage (Y)
        0x09, 0x38,       //     Usage (Wheel)
        0x15, 0x81,       //     Logical Minimum (-127)
        0x25, 0x7f,       //     Logical Maximum (127)
        0x75, 0x08,       //     Report Size (8)
        0x95, 0x03,       //     Report Count (3)
        0x81, 0x06,       //     Input (Data, Variable, Relative)
        0xc0,             //   End Collection
        0xc0              // End Collection
    };

    USBHIDReportDescriptor reportDescriptor;
    ASSERT_TRUE(reportDescriptor.Parse(reportDescriptorBytes, sizeof(reportDescriptorBytes)));
    EXPECT_TRUE(reportDescriptor.IsValid());
    EXPECT_EQ(reportDescriptor.GetFields().size(), 6u);
    EXPECT_EQ(reportDescriptor.GetReportBitSize(USBHIDReportType::Input, 0), 32u);

    const USBHIDReportField* button1Field = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_BUTTON, 1);
    const USBHIDReportField* deltaPositionXField = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_X);
    const USBHIDReportField* deltaPositionYField = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_Y);
    const USBHIDReportField* wheelField = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_WHEEL);

    ASSERT_NE(button1Field, nullptr);
    ASSERT_NE(deltaPositionXField, nullptr);
    ASSERT_NE(deltaPositionYField, nullptr);
    ASSERT_NE(wheelField, nullptr);

    EXPECT_EQ(button1Field->ApplicationUsagePage, USB_HID_USAGE_PAGE_GENERIC_DESKTOP);
    EXPECT_EQ(button1Field->ApplicationUsage, USB_HID_USAGE_GENERIC_DESKTOP_MOUSE);
    EXPECT_EQ(button1Field->BitOffset, 0u);
    EXPECT_EQ(deltaPositionXField->BitOffset, 8u);
    EXPECT_EQ(deltaPositionYField->BitOffset, 16u);
    EXPECT_EQ(wheelField->BitOffset, 24u);

    static constexpr uint8_t reportBytes[] = { 0x05, 0xfe, 0x02, 0xff };

    uint32_t buttonValue = 0;
    int32_t deltaPositionX = 0;
    int32_t deltaPositionY = 0;
    int32_t wheel = 0;

    ASSERT_TRUE(USBHIDReportDescriptor::ExtractUnsignedValue(reportBytes, sizeof(reportBytes), *button1Field, buttonValue));
    ASSERT_TRUE(USBHIDReportDescriptor::ExtractSignedValue(reportBytes, sizeof(reportBytes), *deltaPositionXField, deltaPositionX));
    ASSERT_TRUE(USBHIDReportDescriptor::ExtractSignedValue(reportBytes, sizeof(reportBytes), *deltaPositionYField, deltaPositionY));
    ASSERT_TRUE(USBHIDReportDescriptor::ExtractSignedValue(reportBytes, sizeof(reportBytes), *wheelField, wheel));

    EXPECT_EQ(buttonValue, 1u);
    EXPECT_EQ(deltaPositionX, -2);
    EXPECT_EQ(deltaPositionY, 2);
    EXPECT_EQ(wheel, -1);
}

TEST(USBHIDReportParser, HandlesReportIDPaddingAndSigned16BitAxes)
{
    static constexpr uint8_t reportDescriptorBytes[] =
    {
        0x05, 0x01,             // Usage Page (Generic Desktop)
        0x09, 0x02,             // Usage (Mouse)
        0xa1, 0x01,             // Collection (Application)
        0x85, 0x02,             //   Report ID (2)
        0x05, 0x09,             //   Usage Page (Button)
        0x19, 0x01,             //   Usage Minimum (1)
        0x29, 0x05,             //   Usage Maximum (5)
        0x15, 0x00,             //   Logical Minimum (0)
        0x25, 0x01,             //   Logical Maximum (1)
        0x75, 0x01,             //   Report Size (1)
        0x95, 0x05,             //   Report Count (5)
        0x81, 0x02,             //   Input (Data, Variable, Absolute)
        0x75, 0x03,             //   Report Size (3)
        0x95, 0x01,             //   Report Count (1)
        0x81, 0x01,             //   Input (Constant)
        0x05, 0x01,             //   Usage Page (Generic Desktop)
        0x09, 0x30,             //   Usage (X)
        0x09, 0x31,             //   Usage (Y)
        0x16, 0x01, 0x80,       //   Logical Minimum (-32767)
        0x26, 0xff, 0x7f,       //   Logical Maximum (32767)
        0x75, 0x10,             //   Report Size (16)
        0x95, 0x02,             //   Report Count (2)
        0x81, 0x06,             //   Input (Data, Variable, Relative)
        0x09, 0x38,             //   Usage (Wheel)
        0x15, 0x81,             //   Logical Minimum (-127)
        0x25, 0x7f,             //   Logical Maximum (127)
        0x75, 0x08,             //   Report Size (8)
        0x95, 0x01,             //   Report Count (1)
        0x81, 0x06,             //   Input (Data, Variable, Relative)
        0xc0                    // End Collection
    };

    USBHIDReportDescriptor reportDescriptor;
    ASSERT_TRUE(reportDescriptor.Parse(reportDescriptorBytes, sizeof(reportDescriptorBytes)));
    EXPECT_EQ(reportDescriptor.GetReportBitSize(USBHIDReportType::Input, 2), 56u);

    const USBHIDReportField* button5Field = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_BUTTON, 5);
    const USBHIDReportField* deltaPositionXField = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_X);
    const USBHIDReportField* deltaPositionYField = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_Y);
    const USBHIDReportField* wheelField = USBHIDReportParserTest::FindInputField(reportDescriptor, USB_HID_USAGE_PAGE_GENERIC_DESKTOP, USB_HID_USAGE_GENERIC_DESKTOP_WHEEL);

    ASSERT_NE(button5Field, nullptr);
    ASSERT_NE(deltaPositionXField, nullptr);
    ASSERT_NE(deltaPositionYField, nullptr);
    ASSERT_NE(wheelField, nullptr);

    EXPECT_EQ(button5Field->BitOffset, 12u);
    EXPECT_EQ(deltaPositionXField->BitOffset, 16u);
    EXPECT_EQ(deltaPositionYField->BitOffset, 32u);
    EXPECT_EQ(wheelField->BitOffset, 48u);

    static constexpr uint8_t reportBytes[] = { 0x02, 0x11, 0x34, 0x12, 0x00, 0xff, 0x7f };

    uint32_t buttonValue = 0;
    int32_t deltaPositionX = 0;
    int32_t deltaPositionY = 0;
    int32_t wheel = 0;

    ASSERT_TRUE(USBHIDReportDescriptor::ExtractUnsignedValue(reportBytes, sizeof(reportBytes), *button5Field, buttonValue));
    ASSERT_TRUE(USBHIDReportDescriptor::ExtractSignedValue(reportBytes, sizeof(reportBytes), *deltaPositionXField, deltaPositionX));
    ASSERT_TRUE(USBHIDReportDescriptor::ExtractSignedValue(reportBytes, sizeof(reportBytes), *deltaPositionYField, deltaPositionY));
    ASSERT_TRUE(USBHIDReportDescriptor::ExtractSignedValue(reportBytes, sizeof(reportBytes), *wheelField, wheel));

    EXPECT_EQ(buttonValue, 1u);
    EXPECT_EQ(deltaPositionX, 0x1234);
    EXPECT_EQ(deltaPositionY, -256);
    EXPECT_EQ(wheel, 127);
}
