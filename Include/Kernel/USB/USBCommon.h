// This file is part of PadOS.
//
// Copyright (C) 2022-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 28.05.2022 15:00

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <Utils/Logging.h>
#include <Kernel/Kernel.h>

enum class USB_Speed : uint8_t;


namespace kernel
{
PDEFINE_LOG_CATEGORY(LogCategoryUSB,        "USB",  PLogSeverity::INFO_LOW_VOL,  PLogChannel::DebugPort);
PDEFINE_LOG_CATEGORY(LogCategoryUSBDevice,  "USBD", PLogSeverity::INFO_LOW_VOL, PLogChannel::DebugPort);
PDEFINE_LOG_CATEGORY(LogCategoryUSBHost,    "USBH", PLogSeverity::INFO_LOW_VOL,  PLogChannel::DebugPort);

enum class USB_ControlStage : int
{
    IDLE,
    SETUP,
    DATA,
    ACK
};

using USB_PipeIndex = int32_t;
static constexpr USB_PipeIndex USB_INVALID_PIPE = -1;
static constexpr uint8_t USB_INVALID_ENDPOINT = 0xff;

// A segment array describes one continuous USB data phase. At least one segment
// must be supplied. For nonzero transfers, consumed segments must be nonempty
// and together describe at least length bytes. The described buffers and arrays
// containing more than one segment must remain stable until the request
// completes. Intermediate boundaries within the transfer must align to the
// endpoint packet size.
struct USB_TransferSegment
{
    void*  Buffer = nullptr;
    size_t Length = 0;
};

const char* USB_GetSpeedName(USB_Speed speed);

} // namespace kernel
