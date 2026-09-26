// This file is part of PadOS.
//
// Copyright (c) 2025-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 29.12.2025 15:30

#pragma once

#include <stdint.h>

struct KExceptionStackFrame
{
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;    // LR/R14
    uint32_t PC;    // PC/R15
    uint32_t xPSR;
};

struct KExceptionStackFrameFPU
{
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
    uint32_t S0;
    uint32_t S1;
    uint32_t S2;
    uint32_t S3;
    uint32_t S4;
    uint32_t S5;
    uint32_t S6;
    uint32_t S7;
    uint32_t S8;
    uint32_t S9;
    uint32_t S10;
    uint32_t S11;
    uint32_t S12;
    uint32_t S13;
    uint32_t S14;
    uint32_t S15;
    uint32_t FPSCR;
    uint32_t reserved;
};

struct KCtxSwitchKernelStackFrame
{
    uint32_t R4;
    uint32_t R5;
    uint32_t R6;
    uint32_t R7;
    uint32_t R8;
    uint32_t R9;
    uint32_t R10;
    uint32_t R11;
    uint32_t EXEC_RETURN;
    uint32_t padding;
};
static_assert(sizeof(KCtxSwitchKernelStackFrame) % 8 == 0);

struct KCtxSwitchKernelStackFrameFPU
{
    uint32_t R4;
    uint32_t R5;
    uint32_t R6;
    uint32_t R7;
    uint32_t R8;
    uint32_t R9;
    uint32_t R10;
    uint32_t R11;
    uint32_t EXEC_RETURN;
    uint32_t padding;
    uint32_t S16;
    uint32_t S17;
    uint32_t S18;
    uint32_t S19;
    uint32_t S20;
    uint32_t S21;
    uint32_t S22;
    uint32_t S23;
    uint32_t S24;
    uint32_t S25;
    uint32_t S26;
    uint32_t S27;
    uint32_t S28;
    uint32_t S29;
    uint32_t S30;
    uint32_t S31;
};
static_assert(sizeof(KCtxSwitchKernelStackFrameFPU) % 8 == 0);

struct KCtxSwitchStackFrame
{
    KCtxSwitchKernelStackFrame  KernelFrame;
    KExceptionStackFrame        ExceptionFrame;
};

struct KCtxSwitchStackFrameFPU
{
    KCtxSwitchKernelStackFrameFPU   KernelFrame;
    KExceptionStackFrameFPU         ExceptionFrame;
};

struct KSignalStackFrame
{
    uintptr_t   PreSignalPSPAndPrivilege;
    uint32_t    Padding;
    sigset_t    SignalMask;
    siginfo_t   SigInfo;
};
static_assert(sizeof(KSignalStackFrame) % 8 == 0);

static inline bool exception_has_fpu_frame(uint32_t execReturn)
{
    return (execReturn & 0x10) == 0;
}

#define ASM_STORE_SCHED_CONTEXT(base_reg) \
    "   tst     lr, #0x10\n"                    /* Test bit 4 in EXEC_RETURN to check if the thread use the FPU context. */ \
    "   it eq\n" \
    "   vstmdbeq " #base_reg "!, {s16-s31}\n"   /* If bit 4 not set, push the high FPU registers. */ \
    "" \
    "   sub    " #base_reg ", #4\n"             /* Skipp padding word after KCtxSwitchKernelStackFrame[FPU]::EXEC_RETURN. */ \
    "   stmdb  " #base_reg "!, {r4-r11, lr}\n"  /* Push high core registers. */

#define ASM_LOAD_SCHED_CONTEXT(base_reg) \
    "   ldmia " #base_reg "!, {r4-r11, lr}\n"    /* Pop high core registers. */ \
    "   add   " #base_reg ", #4\n"               /* Skipp padding word after KCtxSwitchKernelStackFrame[FPU]::EXEC_RETURN. */ \
    "" \
    "   tst     lr, #0x10\n"            /* Test bit 4 in EXEC_RETURN to check if the thread use the FPU context. */ \
    "   it eq\n" \
    "   vldmiaeq r0!, {s16-s31}\n"      /* If bit 4 not set, pop the high FPU registers. */
