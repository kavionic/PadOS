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
