// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#if defined(STM32H7)
#include "STM32/DMA_STM32.h"
#else
#error Unknown platform
#endif
