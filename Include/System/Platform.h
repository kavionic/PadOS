// This file is part of PadOS.
//
// Copyright (c) 2019-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

