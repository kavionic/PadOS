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
