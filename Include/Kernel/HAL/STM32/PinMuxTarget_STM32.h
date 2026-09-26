// This file is part of PadOS.
//
// Copyright (c) 2023 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(STM32H7)
#include <Kernel/HAL/STM32/PinMuxTarget_STM32H7.h>
#elif defined(STM32G0)
#include <Kernel/HAL/STM32/PinMuxTarget_STM32G030xx.h>
#else
#error Unknown platform.
#endif

