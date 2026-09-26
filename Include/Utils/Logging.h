// This file is part of PadOS.
//
// Copyright (c) 2025-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 08.11.2025 23:30

#pragma once

#include <sys/pados_syscalls.h>
#include <Utils/String.h>
#include <Utils/LogSeverity.h>

enum class PLogChannel : uint8_t
{
    DebugPort,
    SerialManager
};


#ifdef PADOS_OPT_MINIMUM_LOG_SEVERITY
static constexpr PLogSeverity PLogSeverity_Minimum = PLogSeverity::PADOS_OPT_MINIMUM_LOG_SEVERITY;
#else
static constexpr PLogSeverity PLogSeverity_Minimum = PLogSeverity::INFO_HIGH_VOL;
#endif

struct PLogCategoryRegistrator
{
    PLogCategoryRegistrator(uint32_t categoryHash, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel, PLogChannel channel = PLogChannel::SerialManager) {
        system_log_register_category(categoryHash, channel, categoryName, displayName, initialLogLevel);
    }
};

#define PDEFINE_LOG_CATEGORY(CATEGORY, DISPLAY_NAME, INITIAL_LEVEL, ...) \
    static constexpr uint32_t CATEGORY = PString::hash_string_literal(#CATEGORY, sizeof(#CATEGORY) - 1); \
    static constexpr const char* CATEGORY##_Name = #CATEGORY; \
    inline const PLogCategoryRegistrator CATEGORY##_CategoryRegistrator(CATEGORY, #CATEGORY, DISPLAY_NAME, INITIAL_LEVEL __VA_OPT__(,) __VA_ARGS__)

#define PGET_LOG_CATEGORY_NAME(CATEGORY) CATEGORY##_Name

template<PLogSeverity TSeverity, typename ...ARGS>
void p_system_log(uint32_t category, PFormatString<ARGS...>&& fmt, ARGS&&... args)
{
    if constexpr (TSeverity <= PLogSeverity_Minimum)
    {
        const PString text = PString::format_string(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...);
        system_log_add_message(category, TSeverity, text.c_str());
    }
}

template<PLogSeverity TSeverity, typename ...ARGS>
void p_system_vlog(uint32_t category, std::string_view fmt, ARGS&&... args)
{
    if constexpr (TSeverity <= PLogSeverity_Minimum)
    {
        const PString text = PString::vformat_string(fmt, std::forward<ARGS>(args)...);
        system_log_add_message(category, TSeverity, text.c_str());
    }
}

PDEFINE_LOG_CATEGORY(LogCat_General,    "GENERAL", PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCat_Threads,    "THREADS", PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCat_UnitTests,  "TESTS", PLogSeverity::INFO_HIGH_VOL);
