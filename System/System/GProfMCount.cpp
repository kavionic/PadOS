// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 21.09.2026 00:00

#include <sys/pados_syscalls.h>

#include <System/GProf.h>

#if !defined(STM32H7)
#error GNU call-graph profiling currently requires STM32H7.
#endif

extern "C"
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

__attribute__((no_instrument_function)) void p_gprof_notify_waiter()
{
    condition_var_wakeup_all(g_PGProfCallGraph.DrainCondition);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

// Only capture shutdown enters this path. Preserve floating-point arguments
// across the condition-variable notification, including a possible syscall.
__attribute__((naked, no_instrument_function)) void p_gprof_wakeup()
{
    __asm volatile
    (
        "push    {r4, lr}\n"
        ".save   {r4, lr}\n"
        "vpush   {d0-d7}\n"
        ".vsave  {d0-d7}\n"
        "vmrs    r4, fpscr\n"
        "push    {r4, r12}\n"
        ".pad    #8\n"
        "bl      p_gprof_notify_waiter\n"
        "pop     {r4, r12}\n"
        "vmsr    fpscr, r4\n"
        "vpop    {d0-d7}\n"
        "pop     {r4, pc}\n"
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

// GCC pushes the caller's LR before BL __gnu_mcount_nc. Preserve argument
// registers, restore that LR, and remove the compiler's extra stack word.
// The normal collector and its thread-state accessor use only integer registers.
// PadOS_SystemIFlash keeps the disabled path usable before RAM relocation.
__attribute__((naked, no_instrument_function)) void __gnu_mcount_nc()
{
    __asm volatile
    (
        "push    {r0-r3, r12, lr}\n"
        "ldr     r0, [sp, #24]\n"
        "str     r0, [sp, #20]\n"
        "str     lr, [sp, #24]\n"
        ".save   {r0-r3, r12, lr, pc}\n"
        "sub     sp, sp, #4\n"
        ".pad    #4\n"
        "mrs     r0, ipsr\n"
        "cbnz    r0, 1f\n"
        "ldr     r0, =g_PGProfCallGraphEnabled\n"
        "ldr     r0, [r0]\n"
        "cbz     r0, 1f\n"
        "ldr     r0, [sp, #24]\n"
        "mov     r1, lr\n"
        "bl      p_gprof_record_arc\n"
        "cbz     r0, 1f\n"
        "bl      p_gprof_wakeup\n"
        "1:\n"
        "add     sp, sp, #4\n"
        "pop     {r0-r3, r12, lr}\n"
        "pop     {pc}\n"
    );
}

} // extern "C"
