// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>

namespace kernel
{

class KThreadCB;

inline constexpr uint32_t KDEBUGGER_VERSION = 2;
inline constexpr uint32_t KDEBUGGER_ARCH_UNSUPPORTED = 0;
inline constexpr uint32_t KDEBUGGER_ARCH_CORTEX_M = 1;
inline constexpr uint32_t KDEBUGGER_CORE_REGISTER_COUNT = 17;
inline constexpr uint32_t KDEBUGGER_FLOATING_POINT_REGISTER_COUNT = 17;
inline constexpr uint16_t KDEBUGGER_STACK_POINTER_REGISTER = UINT16_MAX;

struct KDebuggerThreadState
{
    uint32_t Value;
    const char* Name;
};

// Version 2 uses target-endian 32-bit fields and pointers. Field offsets are
// measured from the beginning of KThreadCB, including its base classes, except
// ThreadListFirstOffset, which is relative to Threads. The first and next links
// point to embedded PIntrusiveListNode objects; subtract ThreadNodeOffset to
// obtain KThreadCB. Traverse forward until null, with an independent safety limit.
// Readers inspect a halted target and do not depend on the list's count.
// Register tables contain uint16_t byte offsets in R0-R12, SP, LR, PC, xPSR
// order. The SP entry is KDEBUGGER_STACK_POINTER_REGISTER; recover SP from
// the frame size and the alignment bit in the saved xPSR. The floating-point
// table describes D0-D15 (consecutive S-register pairs), followed by FPSCR,
// in an extended frame. Each pair stores its low word before its high word.
// qGetTLSAddr uses lm = 1 for kernel TLS and lm = 2 for user-space TLS.
// The domain belongs to the variable's ELF image, independent of the stopped
// thread's execution mode. An allocated _kernel_debugger_info identifies a
// kernel image; otherwise, an allocated __app_definition identifies a user image.
// TLS fields give the offsets of the thread's TLS pointers. TLSDataOffset is
// the byte offset from either pointer to the TLS data addressed by DWARF.
// ThreadUserspaceTLSOffset is UINT32_MAX when user space is not configured.
struct KDebuggerInfo
{
    constexpr KDebuggerInfo() noexcept;

    uint32_t Version;
    uint32_t Size;
    uint32_t Architecture;
    uint32_t PointerSize;
    const void* Threads;
    uint32_t ThreadListFirstOffset;
    uint32_t ThreadNodeOffset;
    KThreadCB* const volatile* CurrentThread;
    uint32_t ThreadNextOffset;
    uint32_t ThreadIDOffset;
    uint32_t ThreadNameOffset;
    uint32_t ThreadNameSize;
    uint32_t ThreadStateOffset;
    uint32_t ThreadStateSize;
    uint32_t ThreadPriorityOffset;
    uint32_t ThreadStackOffset;
    uint32_t ThreadSyscallReturnOffset;
    int32_t PriorityMinimum;
    uint32_t ThreadStateCount;
    const KDebuggerThreadState* ThreadStates;
    uint32_t StackPointerMask;
    uint32_t ExceptionReturnOffset;
    uint32_t BasicFrameSize;
    uint32_t ExtendedFrameSize;
    uint32_t CoreRegisterCount;
    const uint16_t* BasicRegisterOffsets;
    const uint16_t* ExtendedRegisterOffsets;
    uint32_t FloatingPointRegisterCount;
    const uint16_t* FloatingPointRegisterOffsets;
    uint32_t ThreadKernelTLSOffset;
    uint32_t ThreadUserspaceTLSOffset;
    uint32_t TLSDataOffset;
};

extern "C" const KDebuggerInfo _kernel_debugger_info;

} // namespace kernel
