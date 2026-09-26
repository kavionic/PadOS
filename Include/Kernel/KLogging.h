// This file is part of PadOS.
//
// Copyright (c) 2025-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 04.11.2025 23:00

#pragma once


#include <Utils/Logging.h>
#include <Kernel/Logging/LogManager.h>

namespace kernel
{

PErrorCode  ksystem_log_register_category(uint32_t categoryHash, PLogChannel channel, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel);
PErrorCode  ksystem_log_set_category_minimum_severity(uint32_t categoryHash, PLogSeverity logLevel);
bool        ksystem_log_is_category_active(uint32_t categoryHash, PLogSeverity logLevel);
PLogChannel ksystem_log_get_category_channel(uint32_t categoryHash);

const char* ksystem_log_get_severity_name(PLogSeverity logLevel);
const PString& ksystem_log_get_category_name(uint32_t categoryHash);
const PString& ksystem_log_get_category_display_name(uint32_t categoryHash);

void            ksystem_log_add_message(uint32_t category, PLogSeverity severity, const PString& message);


PDEFINE_LOG_CATEGORY(LogCatKernel_General,      "KGENERL",  PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_VFS,          "VFS",      PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_PTY,          "PTY",      PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_Processes,    "Process",  PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_Signals,      "Signals",  PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_Drivers,      "DRIVERS",  PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_BlockCache,   "BCACHE",   PLogSeverity::INFO_LOW_VOL);
PDEFINE_LOG_CATEGORY(LogCatKernel_Scheduler,    "SCHEDUL",  PLogSeverity::INFO_HIGH_VOL);


template<PLogSeverity TSeverity, typename ...ARGS>
void kernel_log(uint32_t category, PFormatString<ARGS...>&& fmt, ARGS&&... args)
{
    if constexpr (TSeverity <= PLogSeverity_Minimum)
    {
        if (KLogManager::Get().IsCategoryActive(category, TSeverity))
        {
            const PString text = PString::format_string(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...);
            ksystem_log_add_message(category, TSeverity, text);
        }
    }
}


} // namespace kernel
