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
#include <Kernel/KStackFrames.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static siginfo_t classify_memmanage_fault()
{
    const uint32_t cfsr = SCB->CFSR;
    void* const faultAddress = (cfsr & SCB_CFSR_MMARVALID_Msk) ? reinterpret_cast<void*>(SCB->MMFAR) : nullptr;

    if (cfsr & (SCB_CFSR_IACCVIOL_Msk | SCB_CFSR_DACCVIOL_Msk))
    {
        siginfo_t si = {};
        si.si_signo = SIGSEGV;
        si.si_code  = SEGV_ACCERR;
        si.si_addr  = faultAddress;
        return si;
    }

    if (cfsr & (SCB_CFSR_MSTKERR_Msk | SCB_CFSR_MUNSTKERR_Msk | SCB_CFSR_MLSPERR_Msk)) {
        siginfo_t si = {};
        si.si_signo = SIGBUS;
        si.si_code  = BUS_OBJERR;
        si.si_addr  = faultAddress;
        return si;
    }
    return {};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static siginfo_t classify_busfault_fault()
{
    const uint32_t cfsr = SCB->CFSR;
    void* const faultAddress = (cfsr & SCB_CFSR_BFARVALID_Msk) ? reinterpret_cast<void*>(SCB->BFAR) : nullptr;

    if (cfsr & SCB_CFSR_PRECISERR_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGBUS;
        si.si_code  = BUS_ADRERR;
        si.si_addr  = faultAddress;
        return si;
    }
    if (cfsr & SCB_CFSR_IMPRECISERR_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGBUS;
        si.si_code  = BUS_OBJERR;
        si.si_addr  = nullptr;
        return si;
    }
    if (cfsr & (SCB_CFSR_STKERR_Msk | SCB_CFSR_UNSTKERR_Msk | SCB_CFSR_LSPERR_Msk))
    {
        siginfo_t si = {};
        si.si_signo = SIGBUS;
        si.si_code  = BUS_OBJERR;
        si.si_addr  = faultAddress;
        return si;
    }

    return {};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static siginfo_t classify_usagefault_fault()
{
    const uint32_t cfsr = SCB->CFSR;

    if (cfsr & SCB_CFSR_DIVBYZERO_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGFPE;
        si.si_code  = FPE_INTDIV;
        return si;
    }
    if (cfsr & SCB_CFSR_UNALIGNED_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGBUS;
        si.si_code  = BUS_ADRALN;
        return si;
    }
    if (cfsr & SCB_CFSR_NOCP_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGILL;
        si.si_code  = ILL_COPROC;
        return si;
    }
    if (cfsr & SCB_CFSR_UNDEFINSTR_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGILL;
        si.si_code  = ILL_ILLOPC;
        return si;
    }
    if (cfsr & SCB_CFSR_INVPC_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGILL;
        si.si_code  = ILL_ILLADR;
        return si;
    }
    if (cfsr & SCB_CFSR_INVSTATE_Msk)
    {
        siginfo_t si = {};
        si.si_signo = SIGILL;
        si.si_code  = ILL_ILLADR;
        return si;
    }
    return {};
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static siginfo_t classify_fault(IRQn_Type irqNum)
{
    siginfo_t si = {};

    switch (irqNum)
    {
        case MemoryManagement_IRQn:
            si = classify_memmanage_fault();
            break;
        case BusFault_IRQn:
            si = classify_busfault_fault();
            break;
        case UsageFault_IRQn:
            si = classify_usagefault_fault();
            break;
        default:
            break;
    }
    if (si.si_signo == 0)
    {
        si.si_signo = SIGBUS;   // Failed to classify. Default to SIGBUS.
        si.si_code = BUS_OBJERR;
    }
    return si;
}

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t handle_fault(void* currentStack, uint32_t controlReg)
{
    const int vectorIndex = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
    const IRQn_Type irqNum = IRQn_Type(vectorIndex - 16);

    // Faults triggered from privileged code leads to kernel panic.
    if ((controlReg & 0x01) == 0)
    {
        const char* faultName;
        switch(irqNum)
        {
            case NonMaskableInt_IRQn:   faultName = "NMI";          break;
            case HardFault_IRQn:        faultName = "HardFault";    break;
            case MemoryManagement_IRQn: faultName = "MemManage";    break;
            case BusFault_IRQn:         faultName = "BusFault";     break;
            case UsageFault_IRQn:       faultName = "UsageFault";   break;
            default: faultName = "Unknown fault"; break;
        }
        panic(faultName);
    }

    KThreadCB& thread = *gk_CurrentThread;
    const uint32_t stackAddrInt = intptr_t(currentStack);
    thread.m_CurrentStackAndPrivilege = stackAddrInt | (controlReg & 0x01); // Store nPRIV in bit 0 of stack address.

    siginfo_t sigInfo = classify_fault(irqNum);
    if (sigInfo.si_signo == SIGFPE || sigInfo.si_signo == SIGILL) {
        sigInfo.si_addr = reinterpret_cast<void*>(get_ctxswitch_frame_pc(currentStack));
    }

    // Clear fault status:
    SCB->CFSR = SCB->CFSR;
    SCB->HFSR = SCB->HFSR;

    thread.SetPendingSignal(sigInfo.si_signo);
    if (thread.m_CurrentStackAndPrivilege & 0x01)
    {
        const uintptr_t newStackPtr = kprocess_signal(sigInfo.si_signo, thread.m_CurrentStackAndPrivilege & ~0x01, /*userMode*/ true, /*fromFault*/ true, &sigInfo);
        thread.m_CurrentStackAndPrivilege = (thread.m_CurrentStackAndPrivilege & 0x01) | newStackPtr;
    }
    else
    {
        panic("Signal-generating fault from privileged code.");
    }
    return thread.m_CurrentStackAndPrivilege;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void NonMaskableInt_Handler()
{
    panic("NMI");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void HardFault_Handler()
{
    volatile uint32_t   faultAddress = 0xff00ffff;
    volatile bool       fpuLazyStatePreservation = false;
    volatile bool       exceptionStackingError = false;
    volatile bool       exceptionUnstackingError = false;
    volatile bool       impreciseError = false;
    volatile bool       preciseError = false;
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

    panic("HardFault");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

__attribute__((naked)) void Fault_Handler()
{
    __asm volatile
    (
        "   tst     lr, #4\n"       // EXC_RETURN bit2: 0=MSP, 1=PSP
        "   ite     eq\n"
        "   mrseq   r0, msp\n"      // r0 = exception frame (arg 0)
        "   mrsne   r0, psp\n"
        "   mrs     r1, CONTROL\n"  // r1 = CONTROL (arg 1)
        ""
        ASM_STORE_SCHED_CONTEXT(r0)
        "   bl      handle_fault\n"        // Convert the fault to a POSIX signal if appropriate, else to a kernel panic.
        ""
        "   mrs     r1, CONTROL\n"
        "   bfi     r1, r0, #0, #1\n"       // Set nPRIV to bit 0 from the stack address returned by handle_fault().
        "   msr     CONTROL, r1\n"
        "   isb\n"                          // Flush instruction pipeline.
        "   bic     r0, r0, #1\n"           // Clear bit 0 (nPRIV) from the stack address.
        ""
        ASM_LOAD_SCHED_CONTEXT(r0)
        ""
        "   msr     psp, r0\n"
        "   bx      lr\n"
        );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DebugMonitor_Handler()
{
}

} // extern "C"

} // namespace kernel
