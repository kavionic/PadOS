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
// Created: 08.11.2025 23:30

#pragma once

#include <Utils/String.h>
#include <Kernel/Logging/LogManager.h>

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

enum class PLogChannel : uint8_t
{
    DebugPort,
    SerialManager
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

#ifdef PADOS_OPT_MINIMUM_LOG_SEVERITY
static constexpr PLogSeverity PLogSeverity_Minimum = PLogSeverity::PADOS_OPT_MINIMUM_LOG_SEVERITY;
#else
static constexpr PLogSeverity PLogSeverity_Minimum = PLogSeverity::INFO_HIGH_VOL;
#endif

struct PLogCategoryRegistrator
{
    PLogCategoryRegistrator(uint32_t categoryHash, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel, PLogChannel channel = PLogChannel::SerialManager) {
        kernel::KLogManager::Get().RegisterCategory(categoryHash, channel, categoryName, displayName, initialLogLevel);
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
        add_system_log_message(category, TSeverity, text.c_str());
    }
}

template<PLogSeverity TSeverity, typename ...ARGS>
void p_system_vlog(uint32_t category, std::string_view fmt, ARGS&&... args)
{
    if constexpr (TSeverity <= PLogSeverity_Minimum)
    {
        const PString text = PString::vformat_string(fmt, std::forward<ARGS>(args)...);
        add_system_log_message(category, TSeverity, text.c_str());
    }
}

PDEFINE_LOG_CATEGORY(LogCat_General, "GENERAL", PLogSeverity::INFO_HIGH_VOL);
PDEFINE_LOG_CATEGORY(LogCat_Threads, "THREADS", PLogSeverity::INFO_HIGH_VOL);
