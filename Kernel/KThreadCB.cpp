// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.03.2018 19:30:41

#include "System/Platform.h"

#include <algorithm>
#include <stdint.h>
#include <string.h>

#include "Kernel/KThreadCB.h"
#include "Kernel/Scheduler.h"
#include "Utils/Utils.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void invalid_return_handler()
{
    panic("Invalid return from thread\n");
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThreadCB::KThreadCB(const char* name, int priority, bool joinable, int stackSize) : KNamedObject(name, KNamedObjectType::Thread)
{
    if (stackSize == 0) stackSize = THREAD_DEFAULT_STACK_SIZE;
    stackSize += THREAD_STACK_PADDING + THREAD_MAX_TLS_SLOTS * sizeof(void*);

    m_IsJoinable = joinable;

    m_StackSize = stackSize & ~3;
    m_StackBuffer = new uint8_t[m_StackSize + KSTACK_ALIGNMENT];
    memset(m_StackBuffer, 0, THREAD_MAX_TLS_SLOTS * sizeof(void*));
    m_CurrentStack  = (uint32_t*) ((intptr_t(m_StackBuffer) - 4 + m_StackSize) & ~(KSTACK_ALIGNMENT - 1));
    m_State         = ThreadState::Ready;
    m_PriorityLevel = PriToLevel(priority);
    _REENT_INIT_PTR(&m_NewLibreent);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThreadCB::~KThreadCB()
{
    // Hack to prevent _reclaim_reent from closing std in/out/err:
#define CLEAR_IF_STDF(f) if ((f) == 0 || (f) == 1 || (f) == 2) { (f) = -1; }
    CLEAR_IF_STDF(m_NewLibreent._stdin->_file);
    CLEAR_IF_STDF(m_NewLibreent._stdout->_file);
    CLEAR_IF_STDF(m_NewLibreent._stderr->_file);
#undef CLEAR_IF_STDF
    _reclaim_reent(&m_NewLibreent);
    delete [] m_StackBuffer;
}

///////////////////////////////////////////////////////////////////////////////
/// Setup a stack frame identical to the one produced during a context switch.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KThreadCB::InitializeStack(ThreadEntryPoint_t entryPoint, void* arguments)
{
    m_CurrentStack = (uint32_t*) ((intptr_t(m_StackBuffer) - 4 + m_StackSize) & ~(KSTACK_ALIGNMENT - 1));

    KCtxSwitchStackFrame* stackFrame = reinterpret_cast<KCtxSwitchStackFrame*>(m_CurrentStack) - 1;

    memset(stackFrame, 0, sizeof(*stackFrame));
    stackFrame->m_EXEC_RETURN = 0xfffffffd; // Return to Thread mode, exception return uses non-floating-point state from the PSP and execution uses PSP after return
    stackFrame->m_R0 = uint32_t(arguments);
    stackFrame->m_LR = uint32_t(invalid_return_handler) & ~1; // Clear the thump flag from the function pointer.
    stackFrame->m_PC = uint32_t(entryPoint) & ~1; // Clear the thump flag from the function pointer.
    stackFrame->m_xPSR = xPSR_T_Msk; // Always in Thumb state.

    m_CurrentStack = reinterpret_cast<uint32_t*>(stackFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KThreadCB::PriToLevel(int priority)
{
    int level = priority - KTHREAD_PRIORITY_MIN;
    return std::clamp(level, 0, KTHREAD_PRIORITY_LEVELS - 1);
//    return clamp(0, KTHREAD_PRIORITY_LEVELS - 1, level);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KThreadCB::LevelToPri(int level)
{
    return level + KTHREAD_PRIORITY_MIN;
}
