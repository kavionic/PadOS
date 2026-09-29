// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <System/Platform.h>

#include <cstddef>
#include <iterator>
#include <utility>

#include <Kernel/KDebugger.h>
#include <Kernel/KNamedObject.h>
#include <Kernel/KStackFrames.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/Scheduler.h>

namespace kernel
{

#if defined(STM32H7)
static constexpr uint32_t DEBUGGER_ARCHITECTURE = KDEBUGGER_ARCH_CORTEX_M;
#else
static constexpr uint32_t DEBUGGER_ARCHITECTURE = KDEBUGGER_ARCH_UNSUPPORTED;
#endif

static constexpr KDebuggerThreadState DEBUGGER_THREAD_STATES[] =
{
    { std::to_underlying(ThreadState_Running), "running" },
    { std::to_underlying(ThreadState_Ready), "ready" },
    { std::to_underlying(ThreadState_Sleeping), "sleeping" },
    { std::to_underlying(ThreadState_Waiting), "waiting" },
    { std::to_underlying(ThreadState_Stopped), "stopped" },
    { std::to_underlying(ThreadState_Zombie), "zombie" },
    { std::to_underlying(ThreadState_Deleted), "deleted" }
};

static constexpr uint16_t DEBUGGER_BASIC_REGISTER_OFFSETS[] =
{
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.R0),
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.R1),
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.R2),
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.R3),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R4),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R5),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R6),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R7),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R8),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R9),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R10),
    offsetof(KCtxSwitchStackFrame, KernelFrame.R11),
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.R12),
    KDEBUGGER_STACK_POINTER_REGISTER,
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.LR),
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.PC),
    offsetof(KCtxSwitchStackFrame, ExceptionFrame.xPSR)
};

static constexpr uint16_t DEBUGGER_EXTENDED_REGISTER_OFFSETS[] =
{
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.R0),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.R1),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.R2),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.R3),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R4),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R5),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R6),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R7),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R8),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R9),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R10),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.R11),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.R12),
    KDEBUGGER_STACK_POINTER_REGISTER,
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.LR),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.PC),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.xPSR)
};

static constexpr uint16_t DEBUGGER_FLOATING_POINT_REGISTER_OFFSETS[] =
{
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S0),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S2),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S4),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S6),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S8),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S10),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S12),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.S14),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S16),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S18),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S20),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S22),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S24),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S26),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S28),
    offsetof(KCtxSwitchStackFrameFPU, KernelFrame.S30),
    offsetof(KCtxSwitchStackFrameFPU, ExceptionFrame.FPSCR)
};

static_assert(sizeof(void*) == sizeof(uint32_t));
static_assert(sizeof(ThreadState) <= sizeof(uint32_t));
static_assert(sizeof(KDebuggerThreadState) == 2 * sizeof(uint32_t));
static_assert(sizeof(KDebuggerInfo) == 32 * sizeof(uint32_t));
static_assert(std::size(DEBUGGER_BASIC_REGISTER_OFFSETS) == KDEBUGGER_CORE_REGISTER_COUNT);
static_assert(std::size(DEBUGGER_EXTENDED_REGISTER_OFFSETS) == KDEBUGGER_CORE_REGISTER_COUNT);
static_assert(std::size(DEBUGGER_FLOATING_POINT_REGISTER_OFFSETS) == KDEBUGGER_FLOATING_POINT_REGISTER_COUNT);
static_assert(offsetof(KCtxSwitchKernelStackFrame, EXEC_RETURN) == offsetof(KCtxSwitchKernelStackFrameFPU, EXEC_RETURN));

constexpr KDebuggerInfo::KDebuggerInfo() noexcept
    : Version(KDEBUGGER_VERSION)
    , Size(sizeof(KDebuggerInfo))
    , Architecture(DEBUGGER_ARCHITECTURE)
    , PointerSize(sizeof(void*))
    , Threads(&KThreadCB::s_DebuggerThreads)
    , ThreadListFirstOffset(KThreadCB::DebuggerThreadList::GetFirstNodeOffset())
    , ThreadNodeOffset(offsetof(KThreadCB, m_DebuggerListNode))
    , CurrentThread(&gk_CurrentThread)
    , ThreadNextOffset(offsetof(KThreadCB, m_DebuggerListNode) + KThreadCB::DebuggerThreadList::GetNextNodeOffset())
    , ThreadIDOffset(offsetof(KThreadCB, m_Handle))
    , ThreadNameOffset(offsetof(KThreadCB, m_Name))
    , ThreadNameSize(sizeof(KNamedObject::m_Name))
    , ThreadStateOffset(offsetof(KThreadCB, m_ThreadState))
    , ThreadStateSize(sizeof(ThreadState))
    , ThreadPriorityOffset(offsetof(KThreadCB, m_PriorityLevel))
    , ThreadStackOffset(offsetof(KThreadCB, m_CurrentStackAndPrivilege))
    , ThreadSyscallReturnOffset(offsetof(KThreadCB, m_SyscallReturn))
    , PriorityMinimum(KTHREAD_PRIORITY_MIN)
    , ThreadStateCount(std::size(DEBUGGER_THREAD_STATES))
    , ThreadStates(DEBUGGER_THREAD_STATES)
    , StackPointerMask(~uint32_t(1))
    , ExceptionReturnOffset(offsetof(KCtxSwitchKernelStackFrame, EXEC_RETURN))
    , BasicFrameSize(sizeof(KCtxSwitchStackFrame))
    , ExtendedFrameSize(sizeof(KCtxSwitchStackFrameFPU))
    , CoreRegisterCount(KDEBUGGER_CORE_REGISTER_COUNT)
    , BasicRegisterOffsets(DEBUGGER_BASIC_REGISTER_OFFSETS)
    , ExtendedRegisterOffsets(DEBUGGER_EXTENDED_REGISTER_OFFSETS)
    , FloatingPointRegisterCount(KDEBUGGER_FLOATING_POINT_REGISTER_COUNT)
    , FloatingPointRegisterOffsets(DEBUGGER_FLOATING_POINT_REGISTER_OFFSETS)
    , ThreadKernelTLSOffset(offsetof(KThreadCB, m_KernelTLS))
#ifdef PADOS_MODULE_USER_SPACE
    , ThreadUserspaceTLSOffset(offsetof(KThreadCB, m_UserspaceTLS))
#else
    , ThreadUserspaceTLSOffset(UINT32_MAX)
#endif
    , TLSDataOffset(sizeof(PThreadControlBlock))
{
}

// GDB discovers this symbol by name; there need not be a firmware reference.
[[gnu::used]] constinit const KDebuggerInfo _kernel_debugger_info;

} // namespace kernel
