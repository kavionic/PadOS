// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 26.11.2025 21:00

#include <sys/pados_syscalls.h>
#include <Utils/Beep.h>


void p_beep(PBeepLength length)
{
    float duration = 0.1f;
    switch (length)
    {
        case PBeepLength::Short:     duration = 0.02f;  break;
        case PBeepLength::Medium:    duration = 0.2f;   break;
        case PBeepLength::Long:      duration = 1.0f;   break;
        case PBeepLength::VeryLong:  duration = 3.0f;   break;
    }
    beep_seconds(duration);
}
