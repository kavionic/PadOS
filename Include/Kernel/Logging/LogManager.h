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

#include <deque>
#include <format>
#include <print>

#include <stdint.h>
#include <Utils/String.h>
#include <Threads/Mutex.h>
#include <Threads/ConditionVariable.h>
#include <Kernel/KThread.h>


enum class PLogSeverity
{
    INFO_HIGH_VOL,
    INFO_LOW_VOL,
    WARNING,
    CRITICAL,
    ERROR,
    FATAL,
    NONE
};


namespace kernel
{

class KLogManager : public KThread
{
public:
    static KLogManager& Get();

    KLogManager();

    void Setup(int threadPriority, size_t threadStackSize);

    virtual void* Run() override;


    bool RegisterCategory(uint32_t categoryHash, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel);
    void SetCategoryMinimumSeverity(uint32_t categoryHash, PLogSeverity logLevel);
    bool IsCategoryActive(uint32_t categoryHash, PLogSeverity logLevel);
    bool IsCategoryActive_pl(uint32_t categoryHash, PLogSeverity logLevel);


    const char* GetLogSeverityName(PLogSeverity logLevel);
    const PString& GetCategoryName(uint32_t categoryHash);
    const PString& GetCategoryDisplayName(uint32_t categoryHash);
    const PString& GetCategoryDisplayName_pl(uint32_t categoryHash);

    void AddLogMessage(uint32_t category, PLogSeverity severity, const PString& message);

private:
    struct CategoryDesc
    {
        CategoryDesc(PLogSeverity minSeverity, const PString& categoryName, const PString& displayName) : MinSeverity(minSeverity), CategoryName(categoryName), DisplayName(displayName) {}

        PLogSeverity    MinSeverity;
        PString         CategoryName;
        PString         DisplayName;
    };
    struct LogEntry
    {
        TimeValNanos    Timestamp;
        uint32_t        CategoryHash;
        PLogSeverity    Severity;
        PString         Message;
    };

    CategoryDesc*       FindCategoryDesc(uint32_t categoryHash);
    const CategoryDesc* FindCategoryDesc(uint32_t categoryHash) const { return const_cast<KLogManager*>(this)->FindCategoryDesc(categoryHash); }
    const CategoryDesc& GetCategoryDesc(uint32_t categoryHash) const;

    PMutex m_Mutex;
    PConditionVariable m_ConditionVar;
    std::map<int, CategoryDesc>	m_LogCategories;
    std::deque<LogEntry>        m_LogEntries;
};


#define DEFINE_KERNEL_LOG_CATEGORY(CATEGORY)   static constexpr uint32_t CATEGORY = PString::hash_string_literal(#CATEGORY, sizeof(#CATEGORY) - 1); static constexpr const char* CATEGORY##_Name = #CATEGORY
#define GET_KERNEL_LOG_CATEGORY_NAME(CATEGORY) CATEGORY##_Name
#define REGISTER_KERNEL_LOG_CATEGORY(CATEGORY, DISPLAY_NAME, INITIAL_LEVEL) KLogManager::Get().RegisterCategory(CATEGORY, #CATEGORY, DISPLAY_NAME, INITIAL_LEVEL)
#define PREGISTER_LOG_CATEGORY(CATEGORY, DISPLAY_NAME, INITIAL_LEVEL) kernel::KLogManager::Get().RegisterCategory(CATEGORY, #CATEGORY, DISPLAY_NAME, INITIAL_LEVEL)

DEFINE_KERNEL_LOG_CATEGORY(LogCatKernel_General);
DEFINE_KERNEL_LOG_CATEGORY(LogCatKernel_VFS);
DEFINE_KERNEL_LOG_CATEGORY(LogCatKernel_Drivers);
DEFINE_KERNEL_LOG_CATEGORY(LogCatKernel_BlockCache);
DEFINE_KERNEL_LOG_CATEGORY(LogCatKernel_Scheduler);

template<typename ...ARGS>
void kernel_log(uint32_t category, PLogSeverity severity, std::format_string<ARGS...> fmt, ARGS&&... args)
{
    if (kernel::KLogManager::Get().IsCategoryActive(category, severity))
    {
        PString text = std::format(fmt, std::forward<ARGS>(args)...);
        KLogManager::Get().AddLogMessage(category, severity, text);
    }
}

} // namespace

DEFINE_KERNEL_LOG_CATEGORY(LogCat_General);


template<typename ...ARGS>
void p_system_log(uint32_t category, PLogSeverity severity, std::format_string<ARGS...> fmt, ARGS&&... args)
{
    if (kernel::KLogManager::Get().IsCategoryActive(category, severity))
    {
        PString text = std::format(fmt, std::forward<ARGS>(args)...);
        kernel::KLogManager::Get().AddLogMessage(category, severity, text);
    }
}

template<typename ...ARGS>
void p_system_vlog(uint32_t category, PLogSeverity severity, std::string_view fmt, ARGS&&... args)
{
    if (kernel::KLogManager::Get().IsCategoryActive(category, severity))
    {
        PString text = std::vformat(fmt, std::make_format_args(args...));
        kernel::KLogManager::Get().AddLogMessage(category, severity, text);
    }
}
