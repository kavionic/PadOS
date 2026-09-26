// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 07.06.2020 14:25:38

#pragma once

#if defined(STM32H7)
#include "STM32/Peripherals_STM32H7.h"
#elif defined(STM32G0)
#else
#error Unknown platform
#endif
