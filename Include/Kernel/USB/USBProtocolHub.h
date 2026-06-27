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
// Created: 27.06.2026 14:00

#pragma once

#include <stdint.h>

#include <System/Platform.h>
#include <Kernel/USB/USBProtocol.h>

enum class USB_HubFeatureSelector : uint16_t
{
    PORT_CONNECTION        = 0,
    PORT_ENABLE            = 1,
    PORT_SUSPEND           = 2,
    PORT_OVER_CURRENT      = 3,
    PORT_RESET             = 4,
    PORT_POWER             = 8,
    PORT_LOW_SPEED         = 9,
    C_PORT_CONNECTION      = 16,
    C_PORT_ENABLE          = 17,
    C_PORT_SUSPEND         = 18,
    C_PORT_OVER_CURRENT    = 19,
    C_PORT_RESET           = 20
};

struct USB_DescHub : USB_DescriptorHeader
{
    static constexpr USB_DescriptorType DESCRIPTOR_TYPE = USB_DescriptorType::HUB;

    uint16_t GetPowerOnDelayMS() const { return uint16_t(uint16_t(bPwrOn2PwrGood) * 2u); }

    uint8_t     bNbrPorts = 0;
    uint16_t    wHubCharacteristics = 0;
    uint8_t     bPwrOn2PwrGood = 0;
    uint8_t     bHubContrCurrent = 0;
} ATTR_PACKED;

static_assert(sizeof(USB_DescHub) == 7);

struct USB_HubPortStatus
{
    static constexpr uint16_t PORT_STATUS_CONNECTION       = 1 << 0;
    static constexpr uint16_t PORT_STATUS_ENABLE           = 1 << 1;
    static constexpr uint16_t PORT_STATUS_SUSPEND          = 1 << 2;
    static constexpr uint16_t PORT_STATUS_OVER_CURRENT     = 1 << 3;
    static constexpr uint16_t PORT_STATUS_RESET            = 1 << 4;
    static constexpr uint16_t PORT_STATUS_POWER            = 1 << 8;
    static constexpr uint16_t PORT_STATUS_LOW_SPEED        = 1 << 9;
    static constexpr uint16_t PORT_STATUS_HIGH_SPEED       = 1 << 10;
    static constexpr uint16_t PORT_STATUS_TEST             = 1 << 11;
    static constexpr uint16_t PORT_STATUS_INDICATOR        = 1 << 12;

    static constexpr uint16_t PORT_CHANGE_CONNECTION       = 1 << 0;
    static constexpr uint16_t PORT_CHANGE_ENABLE           = 1 << 1;
    static constexpr uint16_t PORT_CHANGE_SUSPEND          = 1 << 2;
    static constexpr uint16_t PORT_CHANGE_OVER_CURRENT     = 1 << 3;
    static constexpr uint16_t PORT_CHANGE_RESET            = 1 << 4;

    uint16_t    wPortStatus = 0;
    uint16_t    wPortChange = 0;
} ATTR_PACKED;

static_assert(sizeof(USB_HubPortStatus) == 4);
