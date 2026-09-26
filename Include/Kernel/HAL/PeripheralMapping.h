// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 02.04.2022 20:25

#pragma once

#if defined(STM32H7)
#include "STM32/PeripheralMapping_STM32H7.h"
#elif defined(STM32G0)
#include "STM32/PeripheralMapping_STM32G030xx.h"
#else
#error Unknown platform.
#endif
