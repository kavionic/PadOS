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
// Created: 05.05.2026

#pragma once

#include <stdint.h>
#include <stddef.h>

// Multiplexing protocol used on the USB shell serial port.
// Each packet starts with a ShellMuxHeader, followed by Length payload bytes.
// ChannelID 0 is reserved for control messages (open/close channels).
// ChannelID 1-N are data channels, one per active shell session.

struct ShellMuxHeader
{
    static constexpr uint16_t MAGIC = 0xA55A;
    uint16_t Magic;      // Always 0xA55A — enables resync after corruption
    uint16_t ChannelID;  // 0 = control channel, 1-N = data channels
    uint16_t Length;     // Payload byte count immediately following this header
};

static constexpr uint16_t SHELL_MUX_CONTROL_CHANNEL = 0;
static constexpr size_t   SHELL_MUX_MAX_PAYLOAD      = 4096;

enum class ShellMuxCommand : uint8_t
{
    OpenChannel    = 1,  // PC→Device: request a new channel
    OpenChannelAck = 2,  // Device→PC: channel ready, ChannelID holds the assigned ID
    CloseChannel   = 3,  // Either direction: tear down channel
};

struct ShellMuxControlPayload
{
    ShellMuxCommand Command;
    uint16_t        ChannelID;  // Assigned ID in OpenChannelAck; 0 in OpenChannel
};
