// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>

#include <System/Platform.h>
#include <Kernel/USB/USBProtocol.h>

enum class USB_HID_SubclassCode : uint8_t
{
    NONE           = 0x00,
    BOOT_INTERFACE = 0x01
};

enum class USB_HID_ProtocolCode : uint8_t
{
    NONE     = 0x00,
    KEYBOARD = 0x01,
    MOUSE    = 0x02
};

enum class USB_HID_Request : uint8_t
{
    GET_REPORT   = 0x01,
    GET_IDLE     = 0x02,
    GET_PROTOCOL = 0x03,
    SET_REPORT   = 0x09,
    SET_IDLE     = 0x0a,
    SET_PROTOCOL = 0x0b
};

enum class USB_HID_Protocol : uint8_t
{
    BOOT   = 0,
    REPORT = 1
};

enum class USB_HID_DescriptorType : uint8_t
{
    HID      = 0x21,
    REPORT   = 0x22,
    PHYSICAL = 0x23
};

struct USB_HID_DescriptorInfo
{
    USB_HID_DescriptorType  DescriptorType = USB_HID_DescriptorType::REPORT;
    uint16_t                DescriptorLength = 0;
} ATTR_PACKED;

static_assert(sizeof(USB_HID_DescriptorInfo) == 3);

struct USB_HID_DescHID : USB_DescriptorHeader
{
    uint16_t                    HIDVersion = 0;
    uint8_t                     CountryCode = 0;
    uint8_t                     DescriptorCount = 0;
    USB_HID_DescriptorInfo      Descriptors[];
} ATTR_PACKED;

static_assert(sizeof(USB_HID_DescHID) == 6);
