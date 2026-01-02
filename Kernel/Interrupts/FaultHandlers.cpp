// This file is part of PadOS.
//
// Copyright (C) 2025-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 15.09.2025 23:00

#include <Kernel/Kernel.h>

namespace kernel
{

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void NonMaskableInt_Handler()
{
    panic("NMI\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HardFault_Handler()
{
    panic("HardFault\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MemoryManagement_Handler()
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
    panic("MemManage\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void BusFault_Handler()
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

    panic("BusFault\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UsageFault_Handler()
{
    volatile uint32_t   cfsr = SCB->CFSR;
    volatile uint32_t   fpccr = FPU->FPCCR;

    volatile uint32_t   faultAddress = 0xff00ffff;
    volatile bool       fpuLazyStatePreservation = false;
    volatile bool       exceptionStackingError = false;
    volatile bool       exceptionUnstackingError = false;
    volatile bool		impreciseError = false;
    volatile bool		preciseError = false;
    volatile bool       dataAccessError = false;
    volatile bool       instrBusError = false;
    if (cfsr & SCB_CFSR_BFARVALID_Msk) {
        faultAddress = SCB->BFAR;
    }
    if (cfsr & SCB_CFSR_LSPERR_Msk) {
        fpuLazyStatePreservation = true;
    }
    if (cfsr & SCB_CFSR_STKERR_Msk) {
        exceptionStackingError = true;
    }
    if (cfsr & SCB_CFSR_UNSTKERR_Msk) {
        exceptionUnstackingError = true;
    }
    if (cfsr & SCB_CFSR_IMPRECISERR_Msk) {
        impreciseError = true;
    }
    if (cfsr & SCB_CFSR_PRECISERR_Msk) {
        preciseError = true;
    }
    if (cfsr & SCB_CFSR_IBUSERR_Msk) {
        instrBusError = true;
    }
    (void)fpccr;
    (void)faultAddress;
    (void)fpuLazyStatePreservation;
    (void)exceptionStackingError;
    (void)exceptionUnstackingError;
    (void)impreciseError;
    (void)preciseError;
    (void)dataAccessError;
    (void)instrBusError;

    panic("UsageFault\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DebugMonitor_Handler()
{
}

} // extern "C"

} // namespace kernel
