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

#pragma once

#include "System/Platform.h"
#include <System/TimeValue.h>

namespace kernel
{

enum class IRQResult : int
{
    UNHANDLED,
    HANDLED
};

typedef IRQResult KIRQHandler(IRQn_Type irq, void* userData);

struct KIRQAction
{
    KIRQHandler*    m_Handler;
    void*           m_UserData;
    TimeValNanos    m_RunTime;
    int32_t         m_Handle;
    KIRQAction*     m_Next;
};

int register_irq_handler(IRQn_Type irqNum, KIRQHandler* handler, void* userData);
int unregister_irq_handler(IRQn_Type irqNum, int handle);
} // namespace

TimeValNanos kget_total_irq_time();
