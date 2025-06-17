// This file is part of PadOS.
//
// Copyright (C) 2019-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 25.10.2019

#pragma once


#if defined(__SAME70N21__) || defined(__SAME70Q21__)

#include "compiler.h"
#include "sam.h"
#include <component/tc.h>

#define IRQ_COUNT PERIPH_COUNT_IRQn

typedef TcChannel MCU_Timer16_t;
typedef Pio		  GPIO_Port_t

//#include "Kernel/HAL/SAME70System.h"
#elif defined(STM32H7)

#include <sys/cdefs.h>

#define IRQ_COUNT 150 // (WAKEUP_PIN_IRQn + 1)

#include <stm32h7xx.h>
#include "core_cm7.h"

#include <cmsis_gcc.h>

typedef TIM_TypeDef MCU_Timer16_t;
typedef GPIO_TypeDef GPIO_Port_t;

#elif defined(STM32G0)

#define IRQ_COUNT 29 // (USART2_IRQn + 1)

#include <stm32g0xx.h>
#include "core_cm0plus.h"

#include <cmsis_compiler.h>

#elif _WIN32
#else
#error Unknown platform
#endif

#ifdef __GNUC__
#define ATTR_PACKED __attribute__ ((packed))
#define PALWAYS_INLINE __attribute__ ((always_inline))

#define PSET_OPTIMIZATION(level)  _Pragma("GCC push_options") _Pragma(__XSTRING( GCC optimize (#level) ))
#define PRESET_OPTIMIZATION() _Pragma("GCC pop_options")
#elif _WIN32
#define PALWAYS_INLINE __forceinline
#endif

