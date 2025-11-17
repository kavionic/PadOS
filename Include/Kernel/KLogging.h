// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.11.2025 23:00

#pragma once


#include <Utils/Logging.h>

namespace kernel
{

PDEFINE_LOG_CATEGORY(LogCatKernel_General,      "KGENERL",  PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_VFS,          "VFS",      PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_Drivers,      "DRIVERS",  PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_BlockCache,   "BCACHE",   PLogSeverity::INFO_LOW_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_Scheduler,    "SCHEDUL",  PLogSeverity::INFO_HIGH_VOL);


template<PLogSeverity TSeverity, typename ...ARGS>
void kernel_log(uint32_t category, PFormatString<ARGS...>&& fmt, ARGS&&... args)
{
    if constexpr (TSeverity <= PLogSeverity_Minimum)
    {
        const PString text = PString::format_string(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...);
        KLogManager::Get().AddLogMessage(category, TSeverity, text);
    }
}


} // namespace kernel
