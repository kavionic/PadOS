// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 20.07.2020 23:40

#include <sys/errno.h>

#include "Kernel/IRQDispatcher.h"
#include "Kernel/Scheduler.h"
#include "System/Platform.h"
#include "System/System.h"
#include "Threads/Threads.h"


namespace kernel
{

static KIRQAction* gk_IRQHandlers[IRQ_COUNT];
static TimeValNanos gk_TotalIRQTime;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int register_irq_handler(IRQn_Type irqNum, KIRQHandler* handler, void* userData)
{
    if (irqNum < 0 || irqNum >= IRQ_COUNT || handler == nullptr)
    {
        set_last_error(EINVAL);
        return -1;
    }
    KIRQAction* action = new KIRQAction;
    if (action == nullptr) {
        set_last_error(ENOMEM);
        return -1;
    }
    static int currentHandle = 0;
    int handle = ++currentHandle;

    action->m_Handle = handle;
    action->m_Handler = handler;
    action->m_UserData = userData;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        bool needEnabled = gk_IRQHandlers[irqNum] == nullptr;

        action->m_Next = gk_IRQHandlers[irqNum];
        gk_IRQHandlers[irqNum] = action;

        if (needEnabled) {
            NVIC_SetPriority(irqNum, KIRQ_PRI_NORMAL_LATENCY2);
            NVIC_EnableIRQ(irqNum);
        }
    } CRITICAL_END;
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int unregister_irq_handler(IRQn_Type irqNum, int handle)
{
    if (irqNum < 0 || irqNum >= IRQ_COUNT)
    {
        set_last_error(EINVAL);
        return -1;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        KIRQAction* prev = nullptr;
        for (KIRQAction* action = gk_IRQHandlers[irqNum]; action != nullptr; action = action->m_Next)
        {
            if (action->m_Handle == handle)
            {
                if (prev != nullptr) {
                    prev->m_Next = action->m_Next;
                } else {
                    gk_IRQHandlers[irqNum] = action->m_Next;
                }
                delete action;
                if (gk_IRQHandlers[irqNum] == nullptr) {
                    NVIC_DisableIRQ(irqNum);
                }
                return 0;
            }
        }
    } CRITICAL_END;
    set_last_error(EINVAL);
    return -1;
}

} // namespace


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos get_total_irq_time()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    return kernel::gk_TotalIRQTime;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" void KernelHandleIRQ()
{
    TimeValNanos start = get_system_time_hires();
    volatile int vector = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;


    if (vector >= 16)
    {
        IRQn_Type irqNum = IRQn_Type(vector - 16);

        if (irqNum < IRQ_COUNT)
        {
            TimeValNanos handlerStart = start;
            for (kernel::KIRQAction* action = kernel::gk_IRQHandlers[irqNum]; action != nullptr; action = action->m_Next)
            {
                kernel::IRQResult result = action->m_Handler(irqNum, action->m_UserData);
                TimeValNanos curTime = get_system_time_hires();
                TimeValNanos delta = curTime - handlerStart;
                handlerStart = curTime;
                action->m_RunTime += delta;
                if (result == kernel::IRQResult::HANDLED) break;
            }
        }

    }
    else
    {
        kernel::panic("Unhandled exception.");
    }
    TimeValNanos end = get_system_time_hires();
    TimeValNanos delta = end - start;

    kernel::gk_CurrentThread->m_StartTime += delta; // Don't blame the current thread for the time spent handling interrupts.
    kernel::gk_TotalIRQTime += delta;
}
