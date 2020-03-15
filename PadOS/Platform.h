/*
 * Platform.h
 *
 *  Created on: Oct 25, 2019
 *      Author: kurts
 */

#pragma once

#if defined(__SAME70N21__) || defined(__SAME70Q21__)

#include  "compiler.h"
#include "sam.h"
#include <component/tc.h>

#define IRQ_COUNT PERIPH_COUNT_IRQn

typedef TcChannel MCU_Timer16_t;
typedef Pio		  GPIO_Port_t

//#include "Kernel/HAL/SAME70System.h"
#elif defined(STM32H743xx)

#define IRQ_COUNT 150 // (WAKEUP_PIN_IRQn + 1)


#include <stm32h7xx.h>
#include <cmsis_gcc.h>
#include "core_cm7.h"

typedef TIM_TypeDef MCU_Timer16_t;
typedef GPIO_TypeDef GPIO_Port_t;
#else
#error Unknown platform
#endif

//typedef IRQn_Type IRQ_ID_t;
//typedef int32_t IRQ_ID_t;
