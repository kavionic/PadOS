// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 26.05.2022 23:00

#pragma once

#include <stdint.h>
#include <System/Endian.h>
#include <System/Platform.h>


///////////////////////////////////////////////////////////////////////////////
/// Address Field

static constexpr uint32_t   USB_ADDRESS_EPNUM_Pos       = 0;
static constexpr uint8_t    USB_ADDRESS_EPNUM_Msk       = 0x0f << USB_ADDRESS_EPNUM_Pos;
static constexpr uint8_t    USB_ADDRESS_MAX_EP_COUNT    = 16;

static constexpr uint32_t   USB_ADDRESS_DIR_Pos         = 7;
static constexpr uint8_t    USB_ADDRESS_DIR_Msk         = 1 << USB_ADDRESS_DIR_Pos;
static constexpr uint8_t    USB_ADDRESS_DIR_IN          = USB_ADDRESS_DIR_Msk;

inline constexpr uint8_t    USB_ADDRESS_EPNUM(uint8_t addr)         { return (addr & USB_ADDRESS_EPNUM_Msk) >> USB_ADDRESS_EPNUM_Pos; }
inline constexpr uint8_t    USB_MK_IN_ADDRESS(uint8_t endpointNum)  { return endpointNum | USB_ADDRESS_DIR_IN; }
inline constexpr uint8_t    USB_MK_OUT_ADDRESS(uint8_t endpointNum) { return endpointNum; }

///////////////////////////////////////////////////////////////////////////////
/// Bit definition for the respond to
/// USB_RequestRecipient::DEVICE / USB_RequestCode::GET_STATUS.

static constexpr uint16_t USB_DEVICE_STATUS_SELF_POWERED_Pos             = 0;
static constexpr uint16_t USB_DEVICE_STATUS_SELF_POWERED                 = 1 << USB_DEVICE_STATUS_SELF_POWERED_Pos;
static constexpr uint16_t USB_DEVICE_STATUS_REMOTE_WAKEUP_ENABLED_Pos    = 1;
static constexpr uint16_t USB_DEVICE_STATUS_REMOTE_WAKEUP_ENABLED        = 1 << USB_DEVICE_STATUS_REMOTE_WAKEUP_ENABLED_Pos;

///////////////////////////////////////////////////////////////////////////////
/// Bit definition for the respond to
/// USB_RequestRecipient::ENDPOINT / USB_RequestCode::GET_STATUS.

static constexpr uint16_t USB_ENDPOINT_STATUS_HALT_Pos  = 0;
static constexpr uint16_t USB_ENDPOINT_STATUS_HALT      = 1 << USB_ENDPOINT_STATUS_HALT_Pos;

///////////////////////////////////////////////////////////////////////////////
/// defined base on EHCI specs value for Endpoint Speed.

enum class USB_Speed : uint8_t
{
    FULL = 0,
    LOW  = 1,
    HIGH = 2
};

///////////////////////////////////////////////////////////////////////////////
/// Standard Request Codes.

enum class USB_RequestCode : uint8_t
{
    GET_STATUS          = 0,
    CLEAR_FEATURE       = 1,
    reserved1           = 2,
    SET_FEATURE         = 3,
    reserved2           = 4,
    SET_ADDRESS         = 5,
    GET_DESCRIPTOR      = 6,
    SET_DESCRIPTOR      = 7,
    GET_CONFIGURATION   = 8,
    SET_CONFIGURATION   = 9,
    GET_INTERFACE       = 10,
    SET_INTERFACE       = 11,
    SYNCH_FRAME         = 12
};

///////////////////////////////////////////////////////////////////////////////
/// Descriptor Types.

enum class USB_DescriptorType : uint8_t
{
    UNDEFINED                   = 0x00,
    DEVICE                      = 0x01,
    CONFIGURATION               = 0x02,
    STRING                      = 0x03,
    INTERFACE                   = 0x04,
    ENDPOINT                    = 0x05,
    DEVICE_QUALIFIER            = 0x06,
    OTHER_SPEED_CONFIGURATION   = 0x07,
    INTERFACE_POWER             = 0x08, // Descriptor is defined in the current revision of the USB Interface Power Management Specification.
    OTG                         = 0x09,
    DEBUG_DESC                  = 0x0a,
    INTERFACE_ASSOCIATION       = 0x0b,
    BOS                         = 0x0f,
    DEVICE_CAPABILITY           = 0x10,
    FUNCTIONAL                  = 0x21,

    // Class specific descriptors.
    CS_DEVICE                   = 0x21,
    CS_CONFIGURATION            = 0x22,
    CS_STRING                   = 0x23,
    CS_INTERFACE                = 0x24,
    CS_ENDPOINT                 = 0x25,

    SUPERSPEED_ENDPOINT_COMPANION     = 0x30,
    SUPERSPEED_ISO_ENDPOINT_COMPANION = 0x31
};

///////////////////////////////////////////////////////////////////////////////
/// Standard Feature Selectors

enum class USB_RequestFeatureSelector : uint8_t
{
    ENDPOINT_HALT           = 0, // Recipient: Endpoint.
    DEVICE_REMOTE_WAKEUP    = 1, // Recipient: Device.
    TEST_MODE               = 2  // Recipient: Device.
};


///////////////////////////////////////////////////////////////////////////////
/// Test Mode Selectors

enum class USB_TestModeSelector : uint8_t
{
    TEST_J              = 0x01, // Test_J.
    TEST_K              = 0x02, // Test_K.
    TEST_SE0_NAK        = 0x03, // Test_SE0_NAK.
    TEST_PACKET         = 0x04, // Test_Packet.
    TEST_FORCE_ENABLE   = 0x05, // Test_Force_Enable.
    TEST_VENDOR_FIRST   = 0xc0, // Reserved for vendor-specific test modes.
    TEST_VENDOR_LAST    = 0xff 
};

enum class USB_RequestRecipient : uint8_t
{
    DEVICE = 0,
    INTERFACE = 1,
    ENDPOINT = 2,
    OTHER = 3
};

enum class USB_RequestType : uint8_t
{
    STANDARD  = 0,
    CLASS     = 1,
    VENDOR    = 2,
    INVALID   = 3
};

enum class USB_RequestDirection : uint8_t
{
    HOST_TO_DEVICE = 0,
    DEVICE_TO_HOST = 1
};


enum class USB_ClassCode : uint8_t
{
    UNSPECIFIED         = 0,
    AUDIO               = 1,
    CDC                 = 2,
    HID                 = 3,
    RESERVED_4          = 4,
    PHYSICAL            = 5,
    IMAGE               = 6,
    PRINTER             = 7,
    MSC                 = 8,
    HUB                 = 9,
    CDC_DATA            = 10,
    SMART_CARD          = 11,
    RESERVED_12         = 12,
    CONTENT_SECURITY    = 13,
    VIDEO               = 14,
    PERSONAL_HEALTHCARE = 15,
    AUDIO_VIDEO         = 16,

    DIAGNOSTIC          = 0xdc,
    WIRELESS_CONTROLLER = 0xe0,
    MISC                = 0xef,
    APPLICATION_SPECIFIC= 0xfe,
    VENDOR_SPECIFIC     = 0xff
};

enum class USB_MiscSubclassType : uint8_t
{
    COMMON = 2
};

enum class USB_MiscProtocolType : uint8_t
{
    IAD = 1
};

enum class USB_AppSubclassType : uint8_t
{
    DFU_RUNTIME = 0x01,
    IRDA_BRIDGE = 0x02,
    USBTMC      = 0x03
};

enum class USB_DeviceCapabilityType : uint8_t
{
    WIRELESS_USB                = 0x01,
    USB20_EXTENSION             = 0x02,
    SUPERSPEED_USB              = 0x03,
    CONTAINER_id                = 0x04,
    PLATFORM                    = 0x05,
    POWER_DELIVERY              = 0x06,
    BATTERY_INFO                = 0x07,
    PD_CONSUMER_PORT            = 0x08,
    PD_PROVIDER_PORT            = 0x09,
    SUPERSPEED_PLUS             = 0x0a,
    PRECESION_TIME_MEASUREMENT  = 0x0b,
    WIRELESS_USB_EXT            = 0x0c,
    BILLBOARD                   = 0x0d,
    AUTHENTICATION              = 0x0e,
    BILLBOARD_EX                = 0x0f,
    CONFIGURATION_SUMMARY       = 0x10
};

///////////////////////////////////////////////////////////////////////////////
/// Microsoft OS 2.0 descriptor wDescriptorType values

enum class USB_MSOS20Descriptors : uint8_t
{
    SET_HEADER_DESCRIPTOR          = 0x00,
    SUBSET_HEADER_CONFIGURATION    = 0x01,
    SUBSET_HEADER_FUNCTION         = 0x02,
    FEATURE_COMPATBLE_ID           = 0x03,
    FEATURE_REG_PROPERTY           = 0x04,
    FEATURE_MIN_RESUME_TIME        = 0x05,
    FEATURE_MODEL_ID               = 0x06,
    FEATURE_CCGP_DEVICE            = 0x07,
    FEATURE_VENDOR_REVISION        = 0x08
};

///////////////////////////////////////////////////////////////////////////////
/// USB Device Requests

struct USB_ControlRequest
{
    // Bit definitions for bmRequestType.
    static constexpr uint8_t REQUESTTYPE_RECIPIENT_Pos  = 0;
    static constexpr uint8_t REQUESTTYPE_RECIPIENT_Msk  = 0x1f << REQUESTTYPE_RECIPIENT_Pos;
    static constexpr uint8_t REQUESTTYPE_TYPE_Pos       = 5;
    static constexpr uint8_t REQUESTTYPE_TYPE_Msk       = 0x03 << REQUESTTYPE_TYPE_Pos;
    static constexpr uint8_t REQUESTTYPE_DIR_Pos        = 7;
    static constexpr uint8_t REQUESTTYPE_DIR            = 0x01 << REQUESTTYPE_DIR_Pos;  // 0: Host-to-device, 1: Device-to-host
    static constexpr uint8_t REQUESTTYPE_DIR_IN         = 0x01 << REQUESTTYPE_DIR_Pos;
    static constexpr uint8_t REQUESTTYPE_DIR_OUT        = 0x00 << REQUESTTYPE_DIR_Pos;

    USB_ControlRequest() {}
    USB_ControlRequest(
        USB_RequestRecipient    recipient,
        USB_RequestType         type,
        USB_RequestDirection    direction,
        uint8_t                 request,
        uint16_t                value,
        uint16_t                index,
        uint16_t                length
    )
        : bmRequestType(uint8_t((uint8_t(recipient) << REQUESTTYPE_RECIPIENT_Pos) | (uint8_t(type) << REQUESTTYPE_TYPE_Pos) | (uint8_t(direction) << REQUESTTYPE_DIR_Pos)))
        , bRequest(request)
        , wValue(value)
        , wIndex(index)
        , wLength(length)
    {}
    static uint8_t ComposeType(USB_RequestRecipient recipient, USB_RequestType type, USB_RequestDirection direction) { return uint8_t((uint8_t(recipient) << REQUESTTYPE_RECIPIENT_Pos) | (uint8_t(type) << REQUESTTYPE_TYPE_Pos) | (uint8_t(direction) << REQUESTTYPE_DIR_Pos)); }

    uint8_t     bmRequestType = 0;
    uint8_t     bRequest = 0;   // Specific request.
    uint16_t    wValue = 0;     // Word-sized field that varies according to request.
    uint16_t    wIndex = 0;     // Word-sized field that varies according to request; typically used to pass an index or offset.
    uint16_t    wLength = 0;    // Number of bytes to transfer if there is a Data stage.
} ATTR_PACKED;

static_assert(sizeof(USB_ControlRequest) == 8);

///////////////////////////////////////////////////////////////////////////////
/// USB Descriptors.

struct USB_DescriptorHeader
{
    USB_DescriptorHeader(uint8_t length = 0, USB_DescriptorType type = USB_DescriptorType::UNDEFINED) : bLength(length), bDescriptorType(type) {}

    const USB_DescriptorHeader* GetNext() const { return reinterpret_cast<const USB_DescriptorHeader*>(reinterpret_cast<const uint8_t*>(this) + bLength); }

    uint8_t             bLength;           // Descriptor size in bytes.
    USB_DescriptorType  bDescriptorType;   // Descriptor Type.
} ATTR_PACKED;

static_assert(sizeof(USB_DescriptorHeader) == 2);

///////////////////////////////////////////////////////////////////////////////
/// Standard Device Descriptor

struct USB_DescDeviceHeader : USB_DescriptorHeader
{
    USB_DescDeviceHeader(uint8_t length = sizeof(USB_DescDeviceHeader)) : USB_DescriptorHeader(length, USB_DescriptorType::DEVICE) {}

    uint16_t        bcdUSB = 0;             // BUSB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H). This field identifies the release of the USB Specification with which the device and its descriptors are compliant.

    USB_ClassCode   bDeviceClass = USB_ClassCode::UNSPECIFIED; // Class code (assigned by the USB-IF). If this field is reset to zero, each interface within a configuration specifies its own class information and the various interfaces operate independently. If this field is set to a value between 1 and FEH, the device supports different class specifications on different interfaces and the interfaces may not operate independently. This value identifies the class definition used for the aggregate interfaces. If this field is set to FFH, the device class is vendor-specific.
    uint8_t         bDeviceSubClass = 0;    // Subclass code (assigned by the USB-IF). These codes are qualified by the value of the bDeviceClass field. If the bDeviceClass field is reset to zero, this field must also be reset to zero. If the bDeviceClass field is not set to FFH, all values are reserved for assignment by the USB-IF.
    uint8_t         bDeviceProtocol = 0;    // Protocol code (assigned by the USB-IF). These codes are qualified by the value of the bDeviceClass and the bDeviceSubClass fields. If a device supports class-specific protocols on a device basis as opposed to an interface basis, this code identifies the protocols that the device uses as defined by the specification of the device class. If this field is reset to zero, the device does not use class-specific protocols on a device basis. However, it may use class-specific protocols on an interface basis. If this field is set to FFH, the device uses a vendor-specific protocol on a device basis.
    uint8_t         bMaxPacketSize0 = 0;    // Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid). For HS devices is fixed to 64.
};

static_assert(sizeof(USB_DescDeviceHeader) == 8);

struct USB_DescDevice : USB_DescDeviceHeader
{
    USB_DescDevice() : USB_DescDeviceHeader(sizeof(*this)) {}

    uint16_t        idVendor        = 0;    // Vendor ID (assigned by the USB-IF).
    uint16_t        idProduct       = 0;    // Product ID (assigned by the manufacturer).
    uint16_t        bcdDevice       = 0;    // Device release number in binary-coded decimal.
    uint8_t         iManufacturer   = 0;    // Index of string descriptor describing manufacturer.
    uint8_t         iProduct        = 0;    // Index of string descriptor describing product.
    uint8_t         iSerialNumber   = 0;    // Index of string descriptor describing the device's serial number.

    uint8_t         bNumConfigurations = 0; // Number of possible configurations.
} ATTR_PACKED;

static_assert(sizeof(USB_DescDevice) == 18);


///////////////////////////////////////////////////////////////////////////////
/// Device_Qualifier Descriptor.

struct USB_DescDeviceQualifier : USB_DescriptorHeader
{
    USB_DescDeviceQualifier(
        uint16_t        usbVersion,
        USB_ClassCode   deviceClass,
        uint8_t         deviceSubClass,
        uint8_t         deviceProtocol,
        uint8_t         maxPacketSize0,
        uint8_t         numConfigurations
    )
        : USB_DescriptorHeader(sizeof(*this), USB_DescriptorType::DEVICE_QUALIFIER)
        , bcdUSB(usbVersion)
        , bDeviceClass(deviceClass)
        , bDeviceSubClass(deviceSubClass)
        , bDeviceProtocol(deviceProtocol)
        , bMaxPacketSize0(maxPacketSize0)
        , bNumConfigurations(numConfigurations)
        , bReserved(0)
    {}
    uint16_t        bcdUSB;             // USB specification version number (e.g., 0200H for V2.00).

    USB_ClassCode   bDeviceClass;       // Class Code.
    uint8_t         bDeviceSubClass;    // SubClass Code.
    uint8_t         bDeviceProtocol;    // Protocol Code.

    uint8_t         bMaxPacketSize0;    // Maximum packet size for other speed.
    uint8_t         bNumConfigurations; // Number of Other-speed Configurations.
    uint8_t         bReserved;          // Reserved for future use, must be zero.
} ATTR_PACKED;

static_assert(sizeof(USB_DescDeviceQualifier) == 10);


///////////////////////////////////////////////////////////////////////////////
/// Standard Configuration Descriptor.

struct USB_DescConfiguration : USB_DescriptorHeader
{
    // Bit definitions for bmAttributes.

    // Set to 1 if device support remote wakeup.
    static constexpr uint8_t ATTRIBUTES_REMOTE_WAKEUP_Pos   = 5;
    static constexpr uint8_t ATTRIBUTES_REMOTE_WAKEUP       = 1 << ATTRIBUTES_REMOTE_WAKEUP_Pos;
    // Set to 1 if the device is not powered from the bus.
    static constexpr uint8_t ATTRIBUTES_SELF_POWERED_Pos    = 6;
    static constexpr uint8_t ATTRIBUTES_SELF_POWERED        = 1 << ATTRIBUTES_SELF_POWERED_Pos;
    // Bit 7 is reserved, and should always be 1.
    static constexpr uint8_t ATTRIBUTES_RESERVED_HIGH_Pos   = 7;
    static constexpr uint8_t ATTRIBUTES_RESERVED_HIGH       = 1 << ATTRIBUTES_RESERVED_HIGH_Pos;

    USB_DescConfiguration() {}
    USB_DescConfiguration(
        uint16_t totalLength,
        uint8_t  numInterfaces,
        uint8_t  configurationValue,
        uint8_t  configStringIndex,
        uint8_t  attributes,
        uint8_t  powerMilliAmps
    )
        : USB_DescriptorHeader(sizeof(*this), USB_DescriptorType::CONFIGURATION)
        , wTotalLength(totalLength)
        , bNumInterfaces(numInterfaces)
        , bConfigurationValue(configurationValue)
        , iConfiguration(configStringIndex)
        , bmAttributes(attributes)
        , bMaxPower(powerMilliAmps / 2)
    {}

    uint16_t wTotalLength;          // Total length of data returned for this configuration. Includes the combined length of all descriptors (configuration, interface, endpoint, and class- or vendor-specific) returned for this configuration.
    uint8_t  bNumInterfaces;        // Number of interfaces supported by this configuration.
    uint8_t  bConfigurationValue;   // Value to use as an argument to the SetConfiguration() request to select this configuration.
    uint8_t  iConfiguration;        // Index of string descriptor describing this configuration.
    uint8_t  bmAttributes;          // Configuration characteristics \n D7: Reserved (set to one)\n D6: Self-powered \n D5: Remote Wakeup \n D4...0: Reserved (reset to zero) \n D7 is reserved and must be set to one for historical reasons. \n A device configuration that uses power from the bus and a local source reports a non-zero value in bMaxPower to indicate the amount of bus power required and sets D6. The actual power source at runtime may be determined using the GetStatus(DEVICE) request (see USB 2.0 spec Section 9.4.5). \n If a device configuration supports remote wakeup, D5 is set to one.
    uint8_t  bMaxPower;             // Maximum power consumption of the USB device from the bus in this specific configuration when the device is fully operational. Expressed in 2 mA units (i.e., 50 = 100 mA).
} ATTR_PACKED;

static_assert(sizeof(USB_DescConfiguration) == 9);

///////////////////////////////////////////////////////////////////////////////
/// Other_Speed_Configuration Descriptor.

struct USB_DescOtherSpeed : USB_DescriptorHeader
{
    uint16_t wTotalLength;          // Total length of data returned.
    uint8_t  bNumInterfaces;        // Number of interfaces supported by this speed configuration.
    uint8_t  bConfigurationValue;   // Value to use to select configuration.
    uint8_t  iConfiguration;        // Index of string descriptor.
    uint8_t  bmAttributes;          // Same as Configuration descriptor.
    uint8_t  bMaxPower;             // Same as Configuration descriptor.
} ATTR_PACKED;

static_assert(sizeof(USB_DescOtherSpeed) == 9);

///////////////////////////////////////////////////////////////////////////////
/// Standard Interface Descriptor.

struct USB_DescInterface : USB_DescriptorHeader
{
    USB_DescInterface() {}
    USB_DescInterface(
        uint8_t         interfaceNumber,
        uint8_t         alternateSetting,
        uint8_t         numEndpoints,
        USB_ClassCode   interfaceClass,
        uint8_t         interfaceSubClass,
        uint8_t         interfaceProtocol,
        uint8_t         interfaceStringIndex
    )
        : USB_DescriptorHeader(sizeof(*this), USB_DescriptorType::INTERFACE)
        , bInterfaceNumber(interfaceNumber)
        , bAlternateSetting(alternateSetting)
        , bNumEndpoints(numEndpoints)
        , bInterfaceClass(interfaceClass)
        , bInterfaceSubClass(interfaceSubClass)
        , bInterfaceProtocol(interfaceProtocol)
        , iInterface(interfaceStringIndex)
    {}

    uint8_t         bInterfaceNumber;   // Number of this interface. Zero-based value identifying the index in the array of concurrent interfaces supported by this configuration.
    uint8_t         bAlternateSetting;  // Value used to select this alternate setting for the interface identified in the prior field.
    uint8_t         bNumEndpoints;      // Number of endpoints used by this interface (excluding endpoint zero). If this value is zero, this interface only uses the Default Control Pipe.
    USB_ClassCode   bInterfaceClass;    // Class code (assigned by the USB-IF). A value of zero is reserved for future standardization. If this field is set to FFH, the interface class is vendor-specific. All other values are reserved for assignment by the USB-IF.
    uint8_t         bInterfaceSubClass; // Subclass code (assigned by the USB-IF). These codes are qualified by the value of the bInterfaceClass field. If the bInterfaceClass field is reset to zero, this field must also be reset to zero. If the bInterfaceClass field is not set to FFH, all values are reserved for assignment by the USB-IF.
    uint8_t         bInterfaceProtocol; // Protocol code (assigned by the USB). These codes are qualified by the value of the bInterfaceClass and the bInterfaceSubClass fields. If an interface supports class-specific requests, this code identifies the protocols that the device uses as defined by the specification of the device class. If this field is reset to zero, the device does not use a class-specific protocol on this interface. If this field is set to FFH, the device uses a vendor-specific protocol for this interface.
    uint8_t         iInterface;         // Index of string descriptor describing this interface.
} ATTR_PACKED;

static_assert(sizeof(USB_DescInterface) == 9);


///////////////////////////////////////////////////////////////////////////////
/// Values matches the TRANSFER_TYPE field of USB_DescEndpoint::bmAttributes.

enum class USB_TransferType : uint8_t
{
    CONTROL     = 0,
    ISOCHRONOUS = 1,
    BULK        = 2,
    INTERRUPT   = 3
};

///////////////////////////////////////////////////////////////////////////////
/// Values matches the SYNC_TYPE field of USB_DescEndpoint::bmAttributes.

enum class USB_IsoEndpointSyncType : uint8_t
{
    NONE        = 0,
    ASYNC       = 1,
    ADAPTIVE    = 2,
    SYNC        = 3
};

///////////////////////////////////////////////////////////////////////////////
/// Values matches the USAGE_TYPE field of USB_DescEndpoint::bmAttributes.

enum class USB_EndpointUsageType : uint8_t
{
    DATA                    = 0,
    FEEDBACK                = 1,
    EXPLICIT_FEEDBACK_DATA  = 2
};

///////////////////////////////////////////////////////////////////////////////
/// Standard Endpoint Descriptor.

struct USB_DescEndpoint : USB_DescriptorHeader
{
    // Bit definitions for bmAttributes.
    static constexpr uint8_t ATTRIBUTES_TRANSFER_TYPE_Pos   = 0;
    static constexpr uint8_t ATTRIBUTES_TRANSFER_TYPE_Msk   = 0x03 << ATTRIBUTES_TRANSFER_TYPE_Pos;
    static constexpr uint8_t ATTRIBUTES_SYNC_TYPE_Pos       = 2;
    static constexpr uint8_t ATTRIBUTES_SYNC_TYPE_Msk       = 0x03 << ATTRIBUTES_SYNC_TYPE_Pos;
    static constexpr uint8_t ATTRIBUTES_USAGE_TYPE_Pos      = 4;
    static constexpr uint8_t ATTRIBUTES_USAGE_TYPE_Msk      = 0x03 << ATTRIBUTES_USAGE_TYPE_Pos;

    // Bit definitions for wMaxPacketSize.
    static constexpr uint16_t MAX_PACKET_SIZE_LEN_Pos                   = 0;     // Maximum allowed packet size.
    static constexpr uint16_t MAX_PACKET_SIZE_LEN_Msk                   = 0x7ff << MAX_PACKET_SIZE_LEN_Pos;
    static constexpr uint16_t MAX_PACKET_SIZE_EXTRA_TRANSACTIONS_Pos    = 11;    // Maximum allowed extra transactions per micro frame.
    static constexpr uint16_t MAX_PACKET_SIZE_EXTRA_TRANSACTIONS_Msk    = 0x03 << MAX_PACKET_SIZE_EXTRA_TRANSACTIONS_Pos;

    USB_DescEndpoint() {}
    USB_DescEndpoint(
        uint8_t                 endpointAddress,
        USB_TransferType        transferType,
        USB_IsoEndpointSyncType syncType,
        USB_EndpointUsageType   usageType,
        uint16_t                maxPacketSize,
        uint8_t                 interval
    )
        : USB_DescriptorHeader(sizeof(*this), USB_DescriptorType::ENDPOINT)
        , bEndpointAddress(endpointAddress)
        , bmAttributes(uint8_t((uint8_t(transferType) << ATTRIBUTES_TRANSFER_TYPE_Pos) | (uint8_t(syncType) << ATTRIBUTES_SYNC_TYPE_Pos) | (uint8_t(usageType) << ATTRIBUTES_USAGE_TYPE_Pos)))
        , wMaxPacketSize(maxPacketSize)
        , bInterval(interval)
    {}

    USB_TransferType        GetTransferType() const { return USB_TransferType((bmAttributes & ATTRIBUTES_TRANSFER_TYPE_Msk) >> ATTRIBUTES_TRANSFER_TYPE_Pos); }
    USB_IsoEndpointSyncType GetSyncType() const { return USB_IsoEndpointSyncType((bmAttributes & ATTRIBUTES_SYNC_TYPE_Msk) >> ATTRIBUTES_SYNC_TYPE_Pos); }
    USB_EndpointUsageType   GetUsageType() const { return USB_EndpointUsageType((bmAttributes & ATTRIBUTES_USAGE_TYPE_Msk) >> ATTRIBUTES_USAGE_TYPE_Pos); }

    uint16_t                GetMaxPacketSize() const { return (wMaxPacketSize & MAX_PACKET_SIZE_LEN_Msk) >> MAX_PACKET_SIZE_LEN_Pos; }
    uint16_t                GetExtraTransactions() const { return (wMaxPacketSize & MAX_PACKET_SIZE_EXTRA_TRANSACTIONS_Msk) >> MAX_PACKET_SIZE_EXTRA_TRANSACTIONS_Pos; }

    bool Validate(USB_Speed speed) const
    {
        const uint16_t maxPacketSize = GetMaxPacketSize();
        switch (GetTransferType())
        {
            case USB_TransferType::ISOCHRONOUS:
            {
                const uint16_t maxAllowedSize = (speed == USB_Speed::HIGH ? 1024 : 1023);
                return maxPacketSize <= maxAllowedSize;
            }
            case USB_TransferType::BULK:
                if (speed == USB_Speed::HIGH) {
                    return maxPacketSize == 512; // Bulk high-speed can only be 512.
                }
                else {
                    return maxPacketSize == 8 || maxPacketSize == 16 || maxPacketSize == 32 || maxPacketSize == 64; // Bulk full-speed can only be 8, 16, 32 or 64.
                }
            case USB_TransferType::INTERRUPT:
            {
                uint16_t const maxAllowedSize = (speed == USB_Speed::HIGH ? 1024 : 64);
                return maxPacketSize <= maxAllowedSize;
            }
            default: return false;
        }
    }

    uint8_t  bEndpointAddress;  // The address of the endpoint on the USB device described by this descriptor (layout defined by USB_ADDRESS_xxx above).
    uint8_t  bmAttributes;      // This field describes the endpoint’s attributes when it is configured using the bConfigurationValue (see definition above).
    uint16_t wMaxPacketSize;    // Bit 0-10 : max packet size, bit 11-12 additional transaction per high-speed micro-frame (see bit definitions above).
    uint8_t  bInterval;         // Interval for polling endpoint for data transfers. Expressed in frames or micro-frames depending on the device operating speed (i.e., either 1 millisecond or 125 μs units).
} ATTR_PACKED;

static_assert(sizeof(USB_DescEndpoint) == 7);

///////////////////////////////////////////////////////////////////////////////
/// UNICODE String Descriptor.

struct USB_DescString : USB_DescriptorHeader
{
    uint16_t bString[]; // UNICODE encoded string.
} ATTR_PACKED;

static_assert(sizeof(USB_DescString) == 2);

///////////////////////////////////////////////////////////////////////////////
/// Interface Association Descriptor (IAD ECN)

struct USB_DescInterfaceAssociation : USB_DescriptorHeader
{
    USB_DescInterfaceAssociation(
        uint8_t         firstInterface,
        uint8_t         interfaceCount,
        USB_ClassCode   functionClass,
        uint8_t         functionSubClass,
        uint8_t         functionProtocol,
        uint8_t         functionString
    )
        : USB_DescriptorHeader(sizeof(*this), USB_DescriptorType::INTERFACE_ASSOCIATION)
        , bFirstInterface(firstInterface)
        , bInterfaceCount(interfaceCount)
        , bFunctionClass(functionClass)
        , bFunctionSubClass(functionSubClass)
        , bFunctionProtocol(functionProtocol)
        , iFunction(functionString)
    {}
    uint8_t         bFirstInterface;    // Index of the first associated interface.
    uint8_t         bInterfaceCount;    // Total number of associated interfaces.

    USB_ClassCode   bFunctionClass;     // Interface class ID.
    uint8_t         bFunctionSubClass;  // Interface subclass ID.
    uint8_t         bFunctionProtocol;  // Interface protocol ID.

    uint8_t         iFunction;          // Index of the string descriptor describing the interface association.
} ATTR_PACKED;

static_assert(sizeof(USB_DescInterfaceAssociation) == 8);

///////////////////////////////////////////////////////////////////////////////
/// DFU Functional Descriptor.

struct USB_DescDFUFunctional : USB_DescriptorHeader
{
    // Bit definitions for bmAttributes.
    static constexpr uint8_t ATTRIBUTES_CAN_DOWNLOAD_Pos            = 0;
    static constexpr uint8_t ATTRIBUTES_CAN_DOWNLOAD                = 1 << ATTRIBUTES_CAN_DOWNLOAD_Pos;
    static constexpr uint8_t ATTRIBUTES_CAN_UPLOAD_Pos              = 1;
    static constexpr uint8_t ATTRIBUTES_CAN_UPLOAD                  = 1 << ATTRIBUTES_CAN_UPLOAD_Pos;
    static constexpr uint8_t ATTRIBUTES_MANIFISTATION_TOLERANT_Pos  = 2;
    static constexpr uint8_t ATTRIBUTES_MANIFISTATION_TOLERANT      = 1 << ATTRIBUTES_MANIFISTATION_TOLERANT_Pos;
    static constexpr uint8_t ATTRIBUTES_WILL_DETACH_Pos             = 3;
    static constexpr uint8_t ATTRIBUTES_WILL_DETACH                 = 1 << ATTRIBUTES_WILL_DETACH_Pos;

    uint8_t     bmAttributes;
    uint16_t    wDetachTimeOut;
    uint16_t    wTransferSize;
    uint16_t    bcdDFUVersion;
} ATTR_PACKED;

static_assert(sizeof(USB_DescDFUFunctional) == 9);

///////////////////////////////////////////////////////////////////////////////
/// Binary Device Object Store (BOS) Descriptor.

struct USB_DescBOS : USB_DescriptorHeader
{
    uint16_t wTotalLength;      // Total length of data returned for this descriptor.
    uint8_t  bNumDeviceCaps;    // Number of device capability descriptors in the BOS.
} ATTR_PACKED;

static_assert(sizeof(USB_DescBOS) == 5);

///////////////////////////////////////////////////////////////////////////////
/// Binary Device Object Store (BOS)

struct USB_DescBOSPlatform : USB_DescriptorHeader
{
    uint8_t bDevCapabilityType;
    uint8_t bReserved;
    uint8_t PlatformCapabilityUUID[16];
    uint8_t CapabilityData[];
} ATTR_PACKED;

static_assert(sizeof(USB_DescBOSPlatform) == 20);

///////////////////////////////////////////////////////////////////////////////
/// WebUSB URL Descriptor.

struct USB_DescWebUSBURL : USB_DescriptorHeader
{
    uint8_t bScheme;
    char    url[];
} ATTR_PACKED;

static_assert(sizeof(USB_DescWebUSBURL) == 3);

