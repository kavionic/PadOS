// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    // Receive-only storage extent from Buffer. Zero preserves the existing payload-only DMA policy.
    // A nonzero capacity must cover Length. The caller grants exclusive DMA/cache-maintenance ownership
    // of every affected cache line until completion or synchronous cancellation, including padding.
    // Each segment grants ownership independently; adjacent segments do not imply shared cache-line ownership.
    // Capacity never increases the requested payload, packet count, or successful completion length.
    size_t ReceiveCapacity = 0;
};

const char* USB_GetSpeedName(USB_Speed speed);

} // namespace kernel
