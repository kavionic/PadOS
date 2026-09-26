// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include <System/AppDefinition.h>
#include <Kernel/KPosixSignals.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KStackFrames.h>

namespace kernel
{

template<typename T>
static void setup_exit_handler_exception_frame(T& frame, void* returnValue, PThreadUserData* threadUserData)
{
    frame.ExceptionFrame.xPSR &= xPSR_T_Msk;
    frame.ExceptionFrame.R0 = uintptr_t(returnValue);
    frame.ExceptionFrame.R1 = uintptr_t(threadUserData);
    frame.ExceptionFrame.PC = uintptr_t(__app_definition.thread_terminated);
}

#ifdef PADOS_MODULE_USER_SPACE
extern "C" uintptr_t kprocess_thread_exit(uintptr_t prevStackPtr, void* returnValue)
{
    KCtxSwitchKernelStackFrame* prevStackFrame = reinterpret_cast<KCtxSwitchKernelStackFrame*>(prevStackPtr);

    const bool   hasFPUFrame = exception_has_fpu_frame(prevStackFrame->EXEC_RETURN);
    const size_t frameSize = hasFPUFrame ? sizeof(KCtxSwitchStackFrameFPU) : sizeof(KCtxSwitchStackFrame);

    const uintptr_t newStackPtr = prevStackPtr - frameSize;

    memcpy(reinterpret_cast<void*>(newStackPtr), reinterpret_cast<const void*>(prevStackPtr), frameSize);

    KThreadCB& thread = kget_current_thread();

    if (hasFPUFrame) {
        setup_exit_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrameFPU*>(newStackPtr), returnValue, thread.m_ThreadUserData);
    } else {
        setup_exit_handler_exception_frame(*reinterpret_cast<KCtxSwitchStackFrame*>(newStackPtr), returnValue, thread.m_ThreadUserData);
    }
    return newStackPtr;
}
#endif // PADOS_MODULE_USER_SPACE

} // namespace kernel
