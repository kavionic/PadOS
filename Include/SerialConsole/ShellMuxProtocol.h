// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    OpenChannel      = 1,  // PC→Device: request a new channel
    OpenChannelAck   = 2,  // Device→PC: channel ready, ChannelID holds the assigned ID
    CloseChannel     = 3,  // Either direction: tear down channel
    WindowSizeChange = 4,  // PC→Device: SSH client resized its terminal window
};

struct ShellMuxControlPayload
{
    ShellMuxCommand Command;
    uint16_t        ChannelID;  // Assigned ID in OpenChannelAck; 0 in OpenChannel
};

struct ShellMuxWindowSizePayload
{
    ShellMuxCommand Command;   // = WindowSizeChange
    uint16_t        ChannelID;
    uint16_t        Width;
    uint16_t        Height;
    uint16_t        PixelWidth;
    uint16_t        PixelHeight;
};
