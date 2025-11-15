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
#include <Utils/EnumUtils.h>
#include <Threads/Mutex.h>
#include <Threads/ConditionVariable.h>
#include <Kernel/KThread.h>


enum class PLogSeverity : uint8_t;


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

    const char*     GetLogSeverityName(PLogSeverity logLevel);
    const PString&  GetCategoryName(uint32_t categoryHash);
    const PString&  GetCategoryDisplayName(uint32_t categoryHash);
    const PString&  GetCategoryDisplayName_pl(uint32_t categoryHash);

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

    PMutex              m_Mutex;
    PConditionVariable  m_ConditionVar;

    std::map<int, CategoryDesc>	m_LogCategories;
    std::deque<LogEntry>        m_LogEntries;
};

void kadd_log_message(uint32_t category, PLogSeverity severity, const PString& message);

} // namespace

