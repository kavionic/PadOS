// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 30.05.2026 24:30

#pragma once

#include <Utils/EnumUtils.h>

enum class PLogSeverity : uint8_t
{
    NONE,
    FATAL,
    CRITICAL,
    ERROR,
    WARNING,
    NOTICE,
    INFO_LOW_VOL,
    INFO_HIGH_VOL,
    INFO_FLOODING,
};

inline const PEnumNames<PLogSeverity> PLogSeverity_names(
    {
        PENUM_ENTRY_NAME(PLogSeverity, NONE),
        PENUM_ENTRY_NAME(PLogSeverity, FATAL),
        PENUM_ENTRY_NAME(PLogSeverity, CRITICAL),
        PENUM_ENTRY_NAME(PLogSeverity, ERROR),
        PENUM_ENTRY_NAME(PLogSeverity, WARNING),
        PENUM_ENTRY_NAME(PLogSeverity, NOTICE),
        PENUM_ENTRY_NAME(PLogSeverity, INFO_LOW_VOL),
        PENUM_ENTRY_NAME(PLogSeverity, INFO_HIGH_VOL),
        PENUM_ENTRY_NAME(PLogSeverity, INFO_FLOODING)
    }
);
