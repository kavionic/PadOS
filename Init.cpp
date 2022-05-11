// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 21.10.2017 00:18:50

#include "System/Platform.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>

#include <vector>
#include <deque>
#include <algorithm>

#include "Signals/Signal.h"
#include "Kernel/HAL/SAME70System.h"
#include "Threads/EventTimer.h"
#include "Threads/Threads.h"
#include "Kernel/Kernel.h"
#include "Kernel/Scheduler.h"
#include "Kernel/VFS/KBlockCache.h"

extern "C" IFLASHC void InitializeNewLibMutexes();

extern "C" IFLASHC void NonMaskableInt_Handler()
{
    kernel::panic("NMI\n");
}
extern "C" IFLASHC void HardFault_Handler()
{
    kernel::panic("HardFault\n");
}
extern "C" IFLASHC void MemoryManagement_Handler()
{
    volatile uint32_t   faultAddress = 0xff00ffff;
    volatile bool       fpuLazyStatePreservation = false;
    volatile bool       exceptionStackingError = false;
	volatile bool       exceptionUnstackingError = false;
    volatile bool       dataAccessError = false;
	volatile bool       instrAccessError = false;
    if (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) {
        faultAddress = SCB->MMFAR;
    }
    if (SCB->CFSR & SCB_CFSR_MLSPERR_Msk) {
        fpuLazyStatePreservation = true;
    }
    if (SCB->CFSR & SCB_CFSR_MSTKERR_Msk) {
        exceptionStackingError = true;
    }
	if (SCB->CFSR & SCB_CFSR_MUNSTKERR_Msk) {
		exceptionUnstackingError = true;
	}
	if (SCB->CFSR & SCB_CFSR_DACCVIOL_Msk) {
		dataAccessError = true;
	}
	if (SCB->CFSR & SCB_CFSR_IACCVIOL_Msk) {
		instrAccessError = true;
	}
	(void)faultAddress;
	(void)fpuLazyStatePreservation;
	(void)exceptionStackingError;
	(void)exceptionUnstackingError;
	(void)dataAccessError;
	(void)instrAccessError;
	kernel::panic("MemManage\n");
}
extern "C" IFLASHC void BusFault_Handler()
{
	volatile uint32_t   faultAddress = 0xff00ffff;
	volatile bool       fpuLazyStatePreservation = false;
	volatile bool       exceptionStackingError = false;
	volatile bool       exceptionUnstackingError = false;
	volatile bool		impreciseError = false;
	volatile bool		preciseError = false;
	volatile bool       dataAccessError = false;
	volatile bool       instrBusError = false;
	if (SCB->CFSR & SCB_CFSR_BFARVALID_Msk) {
		faultAddress = SCB->BFAR;
	}
	if (SCB->CFSR & SCB_CFSR_LSPERR_Msk) {
		fpuLazyStatePreservation = true;
	}
	if (SCB->CFSR & SCB_CFSR_STKERR_Msk) {
		exceptionStackingError = true;
	}
	if (SCB->CFSR & SCB_CFSR_UNSTKERR_Msk) {
		exceptionUnstackingError = true;
	}
	if (SCB->CFSR & SCB_CFSR_IMPRECISERR_Msk) {
		impreciseError = true;
	}
	if (SCB->CFSR & SCB_CFSR_PRECISERR_Msk) {
		preciseError = true;
	}
	if (SCB->CFSR & SCB_CFSR_IBUSERR_Msk) {
		instrBusError = true;
	}
	(void)faultAddress;
	(void)fpuLazyStatePreservation;
	(void)exceptionStackingError;
	(void)exceptionUnstackingError;
	(void)impreciseError;
	(void)preciseError;
	(void)dataAccessError;
	(void)instrBusError;

	kernel::panic("BusFault\n");
}
extern "C" IFLASHC void UsageFault_Handler()
{
    kernel::panic("UsageFault\n");
}
extern "C" IFLASHC void DebugMonitor_Handler()
{   
}

/*!
 * \brief Initialize the SWO trace port for debug message printing
 * \param portBits Port bit mask to be configured
 * \param cpuCoreFreqHz CPU core clock frequency in Hz
 */
#if 0
void IFLASHC SWO_Init(uint32_t portBits, uint32_t cpuCoreFreqHz)
{
  uint32_t SWOSpeed = 2000000; /* default 64k baud rate */
  uint32_t SWOPrescaler = (cpuCoreFreqHz / SWOSpeed) - 1; /* SWOSpeed in Hz, note that cpuCoreFreqHz is expected to be match the CPU core clock */
 
#if 0
  CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; /* enable trace in core debug */
//  *((volatile unsigned *)(ITM_BASE + 0x400F0)) = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
//  *((volatile unsigned *)(ITM_BASE + 0x40010)) = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
//  *((volatile unsigned *)(ITM_BASE + 0x00FB0)) = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */

  TPI->SPPR = 0x00000002; /* "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO NRZ, 1: SWO Manchester encoding) */
  TPI->FFCR = 0x100;
  TPI->ACPR = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output */
  ITM->LAR  = 0xC5ACCE55; /* ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC */


  ITM->TCR = 0x00010000 /*ITM_TCR_TraceBusID_Msk*/ | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_ITMENA_Msk; /* ITM Trace Control Register */
  ITM->TER = portBits; /* ITM Trace Enable Register. Enabled tracing on stimulus ports. One bit per stimulus port. */
  ITM->TPR = ITM_TPR_PRIVMASK_Msk; /* ITM Trace Privilege Register */
  ITM->PORT[0].u32 = 0;

  // *((volatile unsigned *)(ITM_BASE + 0x01000)) = 0x400003FE; /* DWT_CTRL */
  // *((volatile unsigned *)(ITM_BASE + 0x40304)) = 0x00000100; /* Formatter and Flush Control Register */

//  DWT->CTRL = 0x400003FE; /* DWT_CTRL */
//  DWT->CTRL = 0x40000000; /* DWT_CTRL */
//  TPI->FFCR = 0x00000100; /* Formatter and Flush Control Register */
#endif
}

void IFLASHC SWO_PrintChar(char c, uint8_t portNo)
{
  volatile int timeout;
 
  /* Check if Trace Control Register (ITM->TCR at 0xE0000E80) is set */
  if ((ITM->TCR&ITM_TCR_ITMENA_Msk) == 0) { /* check Trace Control Register if ITM trace is enabled*/
    return; /* not enabled? */
  }
  /* Check if the requested channel stimulus port (ITM->TER at 0xE0000E00) is enabled */
  if ((ITM->TER & (1ul<<portNo))==0) { /* check Trace Enable Register if requested port is enabled */
    return; /* requested port not enabled? */
  }
  timeout = 5000; /* arbitrary timeout value */
  while (ITM->PORT[0].u32 == 0) {
    /* Wait until STIMx is ready, then send data */
    timeout--;
    if (timeout==0) {
      return; /* not able to send */
    }
  }
  ITM->PORT[0].u16 = 0x08 | (int(c)<<8);
}
#endif

void ApplicationMain();
void IFLASHC kernel::InitThreadMain(void* argument)
{
    InitializeNewLibMutexes();
    kernel::KBlockCache::Initialize();
    ApplicationMain();
}
