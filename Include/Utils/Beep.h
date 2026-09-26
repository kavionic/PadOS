// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 26.11.2025 21:00

#pragma once

#include <stdint.h>


enum class PBeepLength : uint32_t
{
    Short,
    Medium,
    Long,
    VeryLong
};

void p_beep(PBeepLength length);
