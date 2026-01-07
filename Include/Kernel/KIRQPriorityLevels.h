// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.01.2026 21:00

#pragma once

#include <cmsis-core/core_cm7.h>

namespace kernel
{

enum KIRQPriorityLevels
{
    KIRQ_PRI_LOW_LATENCY_MAX = 1,
    KIRQ_PRI_LOW_LATENCY3 = KIRQ_PRI_LOW_LATENCY_MAX,
    KIRQ_PRI_LOW_LATENCY2,
    KIRQ_PRI_LOW_LATENCY1,
    KIRQ_PRI_NORMAL_LATENCY_MAX,
    KIRQ_PRI_NORMAL_LATENCY4 = KIRQ_PRI_NORMAL_LATENCY_MAX,
    KIRQ_PRI_NORMAL_LATENCY3,
    KIRQ_PRI_NORMAL_LATENCY2,
#if	__NVIC_PRIO_BITS == 3
#elif __NVIC_PRIO_BITS == 4
    KIRQP_PRI_unused1, KIRQP_PRI_unused2, KIRQP_PRI_unused3, KIRQP_PRI_unused4, KIRQP_PRI_unused5, KIRQP_PRI_unused6, KIRQP_PRI_unused7, KIRQP_PRI_unused8,
#else
#endif
    KIRQ_PRI_NORMAL_LATENCY1,
    KIRQ_PRI_KERNEL = KIRQ_PRI_NORMAL_LATENCY1
};


} // namespace
