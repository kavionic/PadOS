// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.05.2022 23:00

#pragma once

#include <stdint.h>
#include <Kernel/HAL/DigitalPort.h>

enum class HWTimerID : int32_t;

namespace kernel
{

bool setup_beeper(HWTimerID timerID, PinMuxTarget beeperPin);

void kbeep_seconds(float duration);

} //namespace kernel
