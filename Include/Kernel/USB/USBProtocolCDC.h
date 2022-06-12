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

#include <Kernel/USB/USBProtocol.h>

///////////////////////////////////////////////////////////////////////////////
/// Communications Class Subclass Codes

enum class USB_CDC_CommSubclassType : uint8_t
{
    DIRECT_LINE_CONTROL_MODEL      = 0x01, // Direct Line Control Model.         [USBPSTN1.2]
    ABSTRACT_CONTROL_MODEL         = 0x02, // Abstract Control Model.            [USBPSTN1.2]
    TELEPHONE_CONTROL_MODEL        = 0x03, // Telephone Control Model.           [USBPSTN1.2]
    MULTICHANNEL_CONTROL_MODEL     = 0x04, // Multi-Channel Control Model.       [USBISDN1.2]
    CAPI_CONTROL_MODEL             = 0x05, // CAPI Control Model.                [USBISDN1.2]
    ETHERNET_CONTROL_MODEL         = 0x06, // Ethernet Networking Control Model. [USBECM1.2]
    ATM_NETWORKING_CONTROL_MODEL   = 0x07, // ATM Networking Control Model.      [USBATM1.2]
    WIRELESS_HANDSET_CONTROL_MODEL = 0x08, // Wireless Handset Control Model.    [USBWMC1.1]
    DEVICE_MANAGEMENT              = 0x09, // Device Management.                 [USBWMC1.1]
    MOBILE_DIRECT_LINE_MODEL       = 0x0a, // Mobile Direct Line Model.          [USBWMC1.1]
    OBEX                           = 0x0b, // OBEX.                              [USBWMC1.1]
    ETHERNET_EMULATION_MODEL       = 0x0c, // Ethernet Emulation Model.          [USBEEM1.0]
    NETWORK_CONTROL_MODEL          = 0x0d  // Network Control Model.             [USBNCM1.0]
};

///////////////////////////////////////////////////////////////////////////////
/// Communications Class Protocol Codes

enum class USB_CDC_CommProtocol : uint8_t
{
    NONE                          = 0x00, // No specific protocol.
    ATCOMMAND                     = 0x01, // AT Commands: V.250 etc.
    ATCOMMAND_PCCA_101            = 0x02, // AT Commands defined by PCCA-101.
    ATCOMMAND_PCCA_101_AND_ANNEXO = 0x03, // AT Commands defined by PCCA-101 & Annex O.
    ATCOMMAND_GSM_707             = 0x04, // AT Commands defined by GSM 07.07.
    ATCOMMAND_3GPP_27007          = 0x05, // AT Commands defined by 3GPP 27.007.
    ATCOMMAND_CDMA                = 0x06, // AT Commands defined by TIA for CDMA.
    ETHERNET_EMULATION_MODEL      = 0x07  // Ethernet Emulation Model.
};

///////////////////////////////////////////////////////////////////////////////
/// Data Interface Class Protocol Codes
///
/// In certain types of USB communications devices, no protocol will need to be
/// specified in the Data Class interface descriptor. In these cases the value
/// of 00h should be used.

enum class USB_CDC_DataProtocol : uint8_t
{
    NO_CLASS_PROTOCOL                       = 0x00, // No class specific protocol required.
    ISDN_BRI                                = 0x30, // Physical interface protocol for ISDN BRI.
    HDLC                                    = 0x31, // HDLC.
    TRANSPARENT                             = 0x32, // Transparent.
    Q921_MANAGEMENT                         = 0x50, // Management protocol for Q.921 data link protocol.
    Q921_DATA_LINK                          = 0x51, // Data link protocol for Q.931.
    Q921_TEI_MULTIPLEXOR                    = 0x52, // TEI-multiplexor for Q.921 data link protocol.
    V42BIS_DATA_COMPRESSION                 = 0x90, // Data compression procedures.
    EURO_ISDN                               = 0x91, // Euro-ISDN protocol control.
    V24_RATE_ADAPTION_TO_ISDN               = 0x92, // V.24 rate adaptation to ISDN.
    CAPI_COMMAND                            = 0x93, // CAPI Commands.
    HOST_BASED_DRIVER                       = 0xfd, // Host based driver. Note: This protocol code should only be used in messages between host and device to identify the host driver portion of a protocol stack.
    IN_PROTOCOL_UNIT_FUNCTIONAL_DESCRIPTOR  = 0xfe  // The protocol(s) are described using a ProtocolUnit Functional Descriptors on Communications Class Interface.
};

///////////////////////////////////////////////////////////////////////////////
/// bDescriptor SubType in Communications Class Functional Descriptors

enum class USB_CDC_FuncDescType : uint8_t
{
    HEADER                                           = 0x00, // Header Functional Descriptor, which marks the beginning of the concatenated set of functional descriptors for the interface.
    CALL_MANAGEMENT                                  = 0x01, // Call Management Functional Descriptor.
    ABSTRACT_CONTROL_MANAGEMENT                      = 0x02, // Abstract Control Management Functional Descriptor.
    DIRECT_LINE_MANAGEMENT                           = 0x03, // Direct Line Management Functional Descriptor.
    TELEPHONE_RINGER                                 = 0x04, // Telephone Ringer Functional Descriptor.
    TELEPHONE_CALL_AND_LINE_STATE_REPORTING_CAPACITY = 0x05, // Telephone Call and Line State Reporting Capabilities Functional Descriptor.
    UNION                                            = 0x06, // Union Functional Descriptor.
    COUNTRY_SELECTION                                = 0x07, // Country Selection Functional Descriptor.
    TELEPHONE_OPERATIONAL_MODES                      = 0x08, // Telephone Operational ModesFunctional Descriptor.
    USB_TERMINAL                                     = 0x09, // USB Terminal Functional Descriptor.
    NETWORK_CHANNEL_TERMINAL                         = 0x0a, // Network Channel Terminal Descriptor.
    PROTOCOL_UNIT                                    = 0x0b, // Protocol Unit Functional Descriptor.
    EXTENSION_UNIT                                   = 0x0c, // Extension Unit Functional Descriptor.
    MULTICHANEL_MANAGEMENT                           = 0x0d, // Multi-Channel Management Functional Descriptor.
    CAPI_CONTROL_MANAGEMENT                          = 0x0e, // CAPI Control Management Functional Descriptor.
    ETHERNET_NETWORKING                              = 0x0f, // Ethernet Networking Functional Descriptor.
    ATM_NETWORKING                                   = 0x10, // ATM Networking Functional Descriptor.
    WIRELESS_HANDSET_CONTROL_MODEL                   = 0x11, // Wireless Handset Control Model Functional Descriptor.
    MOBILE_DIRECT_LINE_MODEL                         = 0x12, // Mobile Direct Line Model Functional Descriptor.
    MOBILE_DIRECT_LINE_MODEL_DETAIL                  = 0x13, // MDLM Detail Functional Descriptor.
    DEVICE_MANAGEMENT_MODEL                          = 0x14, // Device Management Model Functional Descriptor.
    OBEX                                             = 0x15, // OBEX Functional Descriptor.
    COMMAND_SET                                      = 0x16, // Command Set Functional Descriptor.
    COMMAND_SET_DETAIL                               = 0x17, // Command Set Detail Functional Descriptor.
    TELEPHONE_CONTROL_MODEL                          = 0x18, // Telephone Control Model Functional Descriptor.
    OBEX_SERVICE_IDENTIFIER                          = 0x19, // OBEX Service Identifier Functional Descriptor.
    NCM                                              = 0x1a  // NCM Functional Descriptor.
};

///////////////////////////////////////////////////////////////////////////////
/// Management Element Requests
/// Class-Specific Request Codes

enum class USB_CDC_ManagementRequest : uint8_t
{
    SEND_ENCAPSULATED_COMMAND                       = 0x00, // 6.2.1
    GET_ENCAPSULATED_RESPONSE                       = 0x01, // 6.2.2
    SET_COMM_FEATURE                                = 0x02, // [USBPSTN1.2]
    GET_COMM_FEATURE                                = 0x03, // [USBPSTN1.2]
    CLEAR_COMM_FEATURE                              = 0x04, // [USBPSTN1.2]
    SET_AUX_LINE_STATE                              = 0x10, // [USBPSTN1.2]
    SET_HOOK_STATE                                  = 0x11, // [USBPSTN1.2]
    PULSE_SETUP                                     = 0x12, // [USBPSTN1.2]
    SEND_PULSE                                      = 0x13, // [USBPSTN1.2]
    SET_PULSE_TIME                                  = 0x14, // [USBPSTN1.2]
    RING_AUX_JACK                                   = 0x15, // [USBPSTN1.2]
    SET_LINE_CODING                                 = 0x20, // [USBPSTN1.2]
    GET_LINE_CODING                                 = 0x21, // [USBPSTN1.2]
    SET_CONTROL_LINE_STATE                          = 0x22, // [USBPSTN1.2]
    SEND_BREAK                                      = 0x23, // [USBPSTN1.2]
    SET_RINGER_PARMS                                = 0x30, // [USBPSTN1.2]
    GET_RINGER_PARMS                                = 0x31, // [USBPSTN1.2]
    SET_OPERATION_PARMS                             = 0x32, // [USBPSTN1.2]
    GET_OPERATION_PARMS                             = 0x33, // [USBPSTN1.2]
    SET_LINE_PARMS                                  = 0x34, // [USBPSTN1.2]
    GET_LINE_PARMS                                  = 0x35, // [USBPSTN1.2]
    DIAL_DIGITS                                     = 0x36, // [USBPSTN1.2]
    SET_UNIT_PARAMETER                              = 0x37, // [USBISDN1.2]
    GET_UNIT_PARAMETER                              = 0x38, // [USBISDN1.2]
    CLEAR_UNIT_PARAMETER                            = 0x39, // [USBISDN1.2]
    GET_PROFILE                                     = 0x3a, // [USBISDN1.2]
    SET_ETHERNET_MULTICAST_FILTERS                  = 0x40, // [USBECM1.2]
    SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER    = 0x41, // [USBECM1.2]
    GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER    = 0x42, // [USBECM1.2]
    SET_ETHERNET_PACKET_FILTER                      = 0x43, // [USBECM1.2]
    GET_ETHERNET_STATISTIC                          = 0x44, // [USBECM1.2]
    SET_ATM_DATA_FORMAT                             = 0x50, // [USBATM1.2]
    GET_ATM_DEVICE_STATISTICS                       = 0x51, // [USBATM1.2]
    SET_ATM_DEFAULT_VC                              = 0x52, // [USBATM1.2]
    GET_ATM_VC_STATISTICS                           = 0x53, // [USBATM1.2]
    MDLM_SEMANTIC_MODEL                             = 0x60, // [USBWMC1.2]
    GET_NTB_PARAMETERS                              = 0x80, // [USBNCM1.0]
    GET_NET_ADDRESS                                 = 0x81, // [USBNCM1.0]
    SET_NET_ADDRESS                                 = 0x82, // [USBNCM1.0]
    GET_NTB_FORMAT                                  = 0x83, // [USBNCM1.0]
    SET_NTB_FORMAT                                  = 0x84, // [USBNCM1.0]
    GET_NTB_INPUT_SIZE                              = 0x85, // [USBNCM1.0]
    SET_NTB_INPUT_SIZE                              = 0x86, // [USBNCM1.0]
    GET_MAX_DATAGRAM_SIZE                           = 0x87, // [USBNCM1.0]
    SET_MAX_DATAGRAM_SIZE                           = 0x88, // [USBNCM1.0]
    GET_CRC_MODE                                    = 0x89, // [USBNCM1.0]
    SET_CRC_MODE                                    = 0x8a  // [USBNCM1.0
};

///////////////////////////////////////////////////////////////////////////////
/// Management Element Notification
/// Class-Specific Notification Codes (Notification Endpoint)

enum class USB_CDC_NotificationRequest : uint8_t
{
    NETWORK_CONNECTION               = 0x00,    // 6.3.1
    RESPONSE_AVAILABLE               = 0x01,    // 6.3.2
    AUX_JACK_HOOK_STATE              = 0x08,    // [USBPSTN1.2]
    RING_DETECT                      = 0x09,    // [USBPSTN1.2]
    SERIAL_STATE                     = 0x20,    // [USBPSTN1.2]
    CALL_STATE_CHANGE                = 0x28,    // [USBPSTN1.2]
    LINE_STATE_CHANGE                = 0x29,    // [USBPSTN1.2]
    CONNECTION_SPEED_CHANGE          = 0x2a,    // 6.3.3
    MDLM_SEMANTIC_MODEL_NOTIFICATION = 0x40     // [USBWMC1.1]
};

///////////////////////////////////////////////////////////////////////////////
/// Class Specific Functional Descriptor

struct USB_CDC_DescriptorHeader : USB_DescriptorHeader
{
    USB_CDC_DescriptorHeader(uint8_t length, USB_CDC_FuncDescType descriptorSubType)
        : USB_DescriptorHeader(length, USB_DescriptorType::CS_INTERFACE)
        , bDescriptorSubType(descriptorSubType)
    {}
    USB_CDC_FuncDescType    bDescriptorSubType; // Descriptor SubType (from USB_CDC_FuncDescType).
} ATTR_PACKED;


///////////////////////////////////////////////////////////////////////////////
/// Class-Specific Descriptor Header.

struct USB_CDC_DescFuncHeader : USB_CDC_DescriptorHeader
{
    USB_CDC_DescFuncHeader(uint16_t releaseNumber) : USB_CDC_DescriptorHeader(sizeof(*this), USB_CDC_FuncDescType::HEADER), bcdCDC(releaseNumber) {}

    uint16_t bcdCDC; // USB Class Definitions for Communications Devices Specification release number in binary-coded decimal.
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncHeader) == 5);


///////////////////////////////////////////////////////////////////////////////
/// Union Interface Functional Descriptor

template<int SUBORDINATE_ITF_COUNT = 1>
struct USB_CDC_DescFuncUnion : USB_CDC_DescriptorHeader
{
    template<typename ...SUBORDINATE_INTERFACES>
    USB_CDC_DescFuncUnion(uint8_t controlInterface, SUBORDINATE_INTERFACES ...subordinateInterfaces)
        : USB_CDC_DescriptorHeader(sizeof(*this), USB_CDC_FuncDescType::UNION)
        , bControlInterface(controlInterface)
        , bSubordinateInterfaces{ uint8_t(subordinateInterfaces)... }
    {
    }
    uint8_t bControlInterface;                              // Interface number of Communication Interface (Zero based index of the interface in this configuration (bInterfaceNum)).
    uint8_t bSubordinateInterfaces[SUBORDINATE_ITF_COUNT];  // Array of Interface number of Data Interface (Zero based index of the interface in this configuration (bInterfaceNum)).
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncUnion<1>) == 5);
static_assert(sizeof(USB_CDC_DescFuncUnion<2>) == 6);

///////////////////////////////////////////////////////////////////////////////
/// Country Selection Functional Descriptor.

template<int COUNTRY_COUNT>
struct USB_CDC_DescFuncCountrySelection : USB_CDC_DescriptorHeader
{
    uint8_t     iCountryCodeRelDate;            // Index of a string giving the release date for the implemented ISO 3166 Country Codes (Date shall be presented as ddmmyyyy with dd=day, mm=month, and yyyy=year).
    uint16_t    wCountryCodes[COUNTRY_COUNT];   // Country code in the format as defined in [ISO3166], release date as specified in offset 3 for the supported country codes.
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncCountrySelection<1>) == 6);
static_assert(sizeof(USB_CDC_DescFuncCountrySelection<2>) == 8);

///////////////////////////////////////////////////////////////////////////////
/// Call Management Functional Descriptor

struct USB_CDC_DescFuncCallManagement : USB_CDC_DescriptorHeader
{
    // Bit definitions for bmCapabilities.
    
    // 0: Device sends/receives call management information only over the Communications Class interface. 1: Device can send/receive call management information over a Data Class interface.
    static constexpr uint8_t CAPABILITIES_HANDLE_CALL_Pos       = 0;
    static constexpr uint8_t CAPABILITIES_HANDLE_CALL           = 1 << CAPABILITIES_HANDLE_CALL_Pos;

    // 0: Device does not handle call management itself. 1: Device handles call management itself.
    static constexpr uint8_t CAPABILITIES_SEND_RECV_CALL_Pos    = 1;
    static constexpr uint8_t CAPABILITIES_SEND_RECV_CALL        = 1 << CAPABILITIES_SEND_RECV_CALL_Pos;

    USB_CDC_DescFuncCallManagement(uint8_t capabilities, uint8_t dataInterface)
        : USB_CDC_DescriptorHeader(sizeof(*this), USB_CDC_FuncDescType::CALL_MANAGEMENT)
        , bmCapabilities(capabilities)
        , bDataInterface(dataInterface)
    {}

    uint8_t bmCapabilities;
    uint8_t bDataInterface; // Interface number of Data Class interface optionally used for call management (Zero based index of the interface in this configuration (bInterfaceNum)).
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncCallManagement) == 5);

///////////////////////////////////////////////////////////////////////////////
/// Abstract Control Management Functional Descriptor
///
/// This functional descriptor describes the commands supported by the
/// Communications Class interface with SubClass code of
/// USB_CDC_CommSubclassType::ABSTRACT_CONTROL_MODEL.

struct USB_CDC_DescFuncACM : USB_CDC_DescriptorHeader
{
    // Bit definitions for bmCapabilities.

    // D0 : 1 - Device supports the request combination of SET_COMM_FEATURE, CLEAR_COMM_FEATURE, and GET_COMM_FEATURE.
    static constexpr uint8_t CAPABILITIES_FEATURE_REQUESTS_Pos  = 0;
    static constexpr uint8_t CAPABILITIES_FEATURE_REQUESTS      = 1 << CAPABILITIES_FEATURE_REQUESTS_Pos;

    // D1 : 1 - Device supports the request combination of SET_LINE_CODING, SET_CONTROL_LINE_STATE, GET_LINE_CODING, and the notification SERIAL_STATE.
    static constexpr uint8_t CAPABILITIES_LINE_REQUESTS_Pos     = 1;
    static constexpr uint8_t CAPABILITIES_LINE_REQUESTS         = 1 << CAPABILITIES_LINE_REQUESTS_Pos;

    // D2 : 1 - Device supports the request SEND_BREAK.
    static constexpr uint8_t CAPABILITIES_SENDBREAK_REQUESTS_Pos= 2;
    static constexpr uint8_t CAPABILITIES_SENDBREAK_REQUESTS    = 1 << CAPABILITIES_SENDBREAK_REQUESTS_Pos;

    // D3: 1 - Device supports the notification NETWORK_CONNECTION.
    static constexpr uint8_t CAPABILITIES_NOTIFY_REQEUSTS_Pos   = 3;
    static constexpr uint8_t CAPABILITIES_NOTIFY_REQEUSTS       = 1 << CAPABILITIES_NOTIFY_REQEUSTS_Pos;

    USB_CDC_DescFuncACM(uint8_t capabilities) : USB_CDC_DescriptorHeader(sizeof(*this), USB_CDC_FuncDescType::ABSTRACT_CONTROL_MANAGEMENT), bmCapabilities(capabilities) {}

    uint8_t bmCapabilities;
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncACM) == 4);


///////////////////////////////////////////////////////////////////////////////
/// Direct Line Management Functional Descriptor
///
/// This functional descriptor describes the commands supported by the
/// Communications Class interface with SubClass code of 
/// USB_CDC_FuncDescType::DIRECT_LINE_MANAGEMENT.

struct USB_CDC_DescFuncDirectLineManagement : USB_CDC_DescriptorHeader
{
    // Bit definitions for bmCapabilities.

    // D0: 1 - Device supports the request combination of PULSE_SETUP, SEND_PULSE, and SET_PULSE_TIME.
    static constexpr uint8_t CAPABILITIES_SUPPORT_PULSE_REQUEST_Pos = 0;
    static constexpr uint8_t CAPABILITIES_SUPPORT_PULSE_REQUEST     = 1 << CAPABILITIES_SUPPORT_PULSE_REQUEST_Pos;

    // D1: 1 - Device supports the request combination of SET_AUX_LINE_STATE, RING_AUX_JACK, and notification AUX_JACK_HOOK_STATE.
    static constexpr uint8_t CAPABILITIES_SUPPORT_AUX_REQUEST_Pos   = 1;
    static constexpr uint8_t CAPABILITIES_SUPPORT_AUX_REQUEST       = 1 << CAPABILITIES_SUPPORT_AUX_REQUEST_Pos;

    // D2: 1 - Device requires extra PULSE_SETUP request during pulse dialing sequence to disengage holding circuit. (see Section 6.3.6)
    static constexpr uint8_t CAPABILITIES_REQUIRE_PULSE_SETUP_Pos   = 2;
    static constexpr uint8_t CAPABILITIES_REQUIRE_PULSE_SETUP       = 1 << CAPABILITIES_REQUIRE_PULSE_SETUP_Pos;

    uint8_t bmCapabilities;
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncDirectLineManagement) == 4);

///////////////////////////////////////////////////////////////////////////////
/// Telephone Ringer Functional Descriptor
///
/// This functional descriptor describes the ringer capabilities supported by
/// the Communications Class interface with the SubClass code of
/// USB_CDC_CommSubclassType::TELEPHONE_CONTROL_MODEL.

struct USB_CDC_DescFuncTelephoneRinger : USB_CDC_DescriptorHeader
{
  uint8_t   bRingerVolSteps;    // Number of discrete steps in volume supported by the ringer (consult specs for format).
  uint8_t   bNumRingerPatterns; // Number of ringer patterns supported, values of 1 to 255 with a value of 0 being reserved for future use.
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncTelephoneRinger) == 5);

///////////////////////////////////////////////////////////////////////////////
/// Telephone Operational Modes Functional Descriptor
///
/// This functional descriptor describes the operational modes supported by
/// the Communications Class interface with the SubClass code of
/// USB_CDC_CommSubclassType::TELEPHONE_CONTROL_MODEL.

struct USB_CDC_DescFuncTelephoneOperationalModes : USB_CDC_DescriptorHeader
{
    // Bit definitions for bmCapabilities.

    // D0: 0 - Does not support Simple mode. 1 - Supports Simple mode.
    static constexpr uint8_t CAPABILITIES_SIMPLE_MODE_Pos           = 0;
    static constexpr uint8_t CAPABILITIES_SIMPLE_MODE               = 1 << CAPABILITIES_SIMPLE_MODE_Pos;

    // D1: 0 - Does not support Standalone mode. 1 - Supports Standalone mode.
    static constexpr uint8_t CAPABILITIES_STANDALONE_MODE_Pos       = 1;
    static constexpr uint8_t CAPABILITIES_STANDALONE_MODE           = 1 << CAPABILITIES_STANDALONE_MODE_Pos;

    // D2: 0 - Does not support Computer Centric mode. 1 - Supports Computer Centric mode.
    static constexpr uint8_t CAPABILITIES_COMPUTER_CENTRIC_MODE_Pos = 2;
    static constexpr uint8_t CAPABILITIES_COMPUTER_CENTRIC_MODE     = 1 << CAPABILITIES_COMPUTER_CENTRIC_MODE_Pos;
    uint8_t bmCapabilities;
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncTelephoneOperationalModes) == 4);


///////////////////////////////////////////////////////////////////////////////
/// Telephone Call State Reporting Capabilities Descriptor
///
/// This functional descriptor describes the call and line state reporting
/// capabilities of the device.

struct USB_CDC_DescFuncTelephoneCallStateReportingCapabilities : USB_CDC_DescriptorHeader
{
    // Bit definitions for bmCapabilities

    // 0: Reports only dialtone(does not differentiate between normal and interrupted dialtone). 1: Reports interrupted dialtone in addition to normal dialtone.
    static constexpr uint32_t CAPABILITIES_INTERRUPTED_DIALTONE_Pos     = 0;
    static constexpr uint32_t CAPABILITIES_INTERRUPTED_DIALTONE         = 1 << CAPABILITIES_INTERRUPTED_DIALTONE_Pos;

    // 0: Reports only dialing state. 1: Reports ringback, busy, and fast busy states.
    static constexpr uint32_t CAPABILITIES_RINGBACK_BUSY_FASTBUSY_Pos   = 1;
    static constexpr uint32_t CAPABILITIES_RINGBACK_BUSY_FASTBUSY       = 1 << CAPABILITIES_RINGBACK_BUSY_FASTBUSY_Pos;

    // 0: Does not report caller ID. 1: Reports caller ID information.
    static constexpr uint32_t CAPABILITIES_CALLER_ID_Pos                = 2;
    static constexpr uint32_t CAPABILITIES_CALLER_ID                    = 1 << CAPABILITIES_CALLER_ID_Pos;

    // 0: Reports only incoming ringing. 1: Reports incoming distinctive ringing patterns.
    static constexpr uint32_t CAPABILITIES_INCOMING_DISTINCTIVE_Pos     = 3;
    static constexpr uint32_t CAPABILITIES_INCOMING_DISTINCTIVE         = 1 << CAPABILITIES_INCOMING_DISTINCTIVE_Pos;

    // 0: Cannot report dual tone multi-frequency (DTMF) digits input remotely over the telephone line. 1: Can report DTMF digits input remotely over the telephone line.
    static constexpr uint32_t CAPABILITIES_DUAL_TONE_MULTI_FREQ_Pos     = 4;
    static constexpr uint32_t CAPABILITIES_DUAL_TONE_MULTI_FREQ         = 1 << CAPABILITIES_DUAL_TONE_MULTI_FREQ_Pos;

    // 0: Does not support line state change notification. 1: Does support line state change notification.
    static constexpr uint32_t CAPABILITIES_LINE_STATE_CHANGE_Pos        = 5;
    static constexpr uint32_t CAPABILITIES_LINE_STATE_CHANGE            = 1 << CAPABILITIES_LINE_STATE_CHANGE_Pos;

    uint32_t bmCapabilities;
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_DescFuncTelephoneCallStateReportingCapabilities) == 7);


enum class USB_CDC_LineCodingStopBits : uint8_t
{
    StopBits1   = 0, // 1 stop bit.
    StopBits1_5 = 1, // 1.5 stop bits.
    StopBits2   = 2, // 2 stop bits.
};

enum class USB_CDC_LineCodingParity : uint8_t
{
    None    = 0,
    Odd     = 1,
    Even    = 2,
    Mark    = 3,
    Space   = 4
};

///////////////////////////////////////////////////////////////////////////////
/// Line Coding Structure

struct USB_CDC_LineCoding
{
    uint32_t                    dwDTERate;      // Data terminal rate, in bits per second.
    USB_CDC_LineCodingStopBits  bCharFormat;    // Stop bits (from USB_CDC_LineCodingStopBits).
    USB_CDC_LineCodingParity    bParityType;    // Parity (from USB_CDC_LineCodingParity).
    uint8_t                     bDataBits;      // Data bits (5, 6, 7, 8 or 16).
} ATTR_PACKED;

static_assert(sizeof(USB_CDC_LineCoding) == 7);

// Indicates to DCE if DTE is present or not. This signal corresponds to V.24 signal 108 / 2 and RS232 signal DTR.
static constexpr uint16_t USB_DTE_LINE_CONTROL_STATE_DTE_PRESENT_Pos    = 0;
static constexpr uint16_t USB_DTE_LINE_CONTROL_STATE_DTE_PRESENT        = 1 << USB_DTE_LINE_CONTROL_STATE_DTE_PRESENT_Pos;

// Carrier control for half duplex modems. This signal corresponds to V.24 signal 105 and RS232 signal RTS.
static constexpr uint16_t USB_DTE_LINE_CONTROL_STATE_CARRIER_ACTIVE_Pos = 1;
static constexpr uint16_t USB_DTE_LINE_CONTROL_STATE_CARRIER_ACTIVE     = 1 << USB_DTE_LINE_CONTROL_STATE_CARRIER_ACTIVE_Pos;
