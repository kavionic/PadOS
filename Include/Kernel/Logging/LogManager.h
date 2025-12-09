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
#include <print>

#include <stdint.h>
#include <Utils/String.h>
#include <Utils/EnumUtils.h>
#include <Kernel/KThread.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>


enum class PLogSeverity : uint8_t;
enum class PLogChannel : uint8_t;


namespace kernel
{

class KLogManager : public KThread
{
public:
    static KLogManager& Get();

    KLogManager();

    void Setup(int threadPriority, size_t threadStackSize);

    virtual void* Run() override;


    PErrorCode  RegisterCategory(uint32_t categoryHash, PLogChannel channel, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel);
    PErrorCode  SetCategoryMinimumSeverity(uint32_t categoryHash, PLogSeverity logLevel);
    bool        IsCategoryActive(uint32_t categoryHash, PLogSeverity logLevel);
    bool        IsCategoryActive_pl(uint32_t categoryHash, PLogSeverity logLevel);
    
    PLogChannel GetCategoryChannel(uint32_t categoryHash) const;
    PLogChannel GetCategoryChannel_pl(uint32_t categoryHash) const;

    const char*     GetLogSeverityName(PLogSeverity logLevel);
    const PString&  GetCategoryName(uint32_t categoryHash);
    const PString&  GetCategoryDisplayName(uint32_t categoryHash);
    const PString&  GetCategoryDisplayName_pl(uint32_t categoryHash);

    void AddLogMessage(uint32_t category, PLogSeverity severity, const PString& message);

    void FlushMessages(TimeValNanos timeout);

    struct CategoryDesc
    {
        CategoryDesc(PLogChannel channel, PLogSeverity minSeverity, const PString& categoryName, const PString& displayName) : Channel(channel), MinSeverity(minSeverity), CategoryName(categoryName), DisplayName(displayName) {}

        PLogChannel     Channel;
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

private:

    CategoryDesc*       FindCategoryDesc(uint32_t categoryHash);
    const CategoryDesc* FindCategoryDesc(uint32_t categoryHash) const { return const_cast<KLogManager*>(this)->FindCategoryDesc(categoryHash); }
    const CategoryDesc& GetCategoryDesc(uint32_t categoryHash) const;

    mutable KMutex      m_Mutex;
    KConditionVariable  m_ConditionVar;

    std::map<int, CategoryDesc>	m_LogCategories;
    std::deque<LogEntry>        m_LogEntries;
};

PErrorCode  ksystem_log_register_category(uint32_t categoryHash, PLogChannel channel, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel);
PErrorCode  ksystem_log_set_category_minimum_severity(uint32_t categoryHash, PLogSeverity logLevel);
bool        ksystem_log_is_category_active(uint32_t categoryHash, PLogSeverity logLevel);
PLogChannel ksystem_log_get_category_channel(uint32_t categoryHash);

const char*     ksystem_log_get_severity_name(PLogSeverity logLevel);
const PString&  ksystem_log_get_category_name(uint32_t categoryHash);
const PString&  ksystem_log_get_category_display_name(uint32_t categoryHash);

void            ksystem_log_add_message(uint32_t category, PLogSeverity severity, const PString& message);

} // namespace

