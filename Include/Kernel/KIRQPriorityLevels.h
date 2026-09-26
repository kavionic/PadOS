// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 06.01.2026 21:00

#pragma once

#include <cmsis-core/core_cm7.h>

namespace kernel
{

enum KIRQPriorityLevels
{
    KIRQ_PRI_LOW_LATENCY_MAX = 1,
    KIRQ_PRI_LOW_LATENCY4 = KIRQ_PRI_LOW_LATENCY_MAX,
    KIRQ_PRI_LOW_LATENCY3,
    KIRQ_PRI_LOW_LATENCY2,
    KIRQ_PRI_LOW_LATENCY1,
    KIRQ_PRI_NORMAL_LATENCY_MAX,
    KIRQ_PRI_NORMAL_LATENCY3 = KIRQ_PRI_NORMAL_LATENCY_MAX,
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
