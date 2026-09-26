// This file is part of PadOS.
//
// Copyright (c) 2022-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.05.2022 23:00

#include <Kernel/Misc.h>
#include <Kernel/HAL/STM32/PiezoBuzzer_STM32.h>

namespace kernel
{

static PiezoBuzzer_STM32 g_BuzzerDriver;

bool setup_beeper(HWTimerID timerID, PinMuxTarget beeperPin)
{
    return g_BuzzerDriver.Setup(timerID, beeperPin);
}

void kbeep_seconds(float duration)
{
    g_BuzzerDriver.Beep(duration);
}

} // namespace kernel

