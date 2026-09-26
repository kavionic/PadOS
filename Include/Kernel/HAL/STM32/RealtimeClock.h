// This file is part of PadOS.
//
// Copyright (c) 2022-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 14.04.2022 21:00

#pragma once

#include <System/TimeValue.h>

class RealtimeClock
{
public:
    static void SetClock(TimeValNanos time);
    static TimeValNanos GetClock();

    static void     WriteBackupRegister_trw(size_t registerIndex, uint32_t value);
    static uint32_t ReadBackupRegister_trw(size_t registerIndex);
};


