// This file is part of PadOS.
//
// Copyright (C) 2025-2026 Kurt Skauen <http://kavionic.com/>
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
#include <vector>

#include <SerialConsole/LogMessages.h>

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

    static KLogManager& Get();

    KLogManager();

    void Setup(int threadPriority, size_t threadStackSize,
               const char* logFilePath    = "/var/logs/system.log",
               size_t      maxLogFileSize = 512 * 1024,
               int         maxLogFiles    = 100);

    virtual void* Run() override;


    PErrorCode  RegisterCategory(uint32_t categoryHash, PLogChannel channel, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel);
    PErrorCode  SetCategoryMinimumSeverity(uint32_t categoryHash, PLogSeverity logLevel);
    
    bool        IsCategoryActive(uint32_t categoryHash, PLogSeverity logLevel);
    bool        IsCategoryActive_pl(uint32_t categoryHash, PLogSeverity logLevel);
    
    PLogChannel GetCategoryChannel(uint32_t categoryHash) const;
    PLogChannel GetCategoryChannel_pl(uint32_t categoryHash) const;

    const char*     GetLogSeverityName(PLogSeverity logLevel);
    const PString&  GetCategoryName(uint32_t categoryHash);
    const PString&  GetCategoryName_pl(uint32_t categoryHash);
    const PString&  GetCategoryDisplayName(uint32_t categoryHash);
    const PString&  GetCategoryDisplayName_pl(uint32_t categoryHash);

    std::vector<std::pair<uint32_t, CategoryDesc>> GetCategoryList();

    void AddLogMessage(uint32_t category, PLogSeverity severity, const PString& message);

    void FlushMessages(TimeValNanos timeout);

private:
    void RegisterSerialHandlers();

    void HandleRequestLogCategories(const SerialProtocol::RequestLogCategories& packet);
    void HandleRequestLogSeverities(const SerialProtocol::RequestLogSeverities& packet);
    void HandleRequestLogHistory(const SerialProtocol::RequestLogHistory& packet);

    void    OpenLogFile();
    void    RotateLogFiles();
    void    WriteEntryToFile(int64_t timestamp,
                             const PString& categoryName, const char* severityName,
                             const PString& message);

    bool ParseLogfileLine(const PString& lineBuffer, SerialProtocol::LogMessage& msgHeader, PString& payload);

    static int64_t  ParseLineTimestamp(const char* line, size_t length);
    static int64_t  ReadFileFirstTimestamp(const PString& filename);
    static off_t    FindEndPositionBeforeTimestamp(int fileHandle, off_t fileSize, int64_t targetTimestamp);
    bool            FindFilePositionForTimestamp(int64_t targetTimestamp, int& outPageIndex, off_t& outFilePosition);

    CategoryDesc*       FindCategoryDesc(uint32_t categoryHash);
    const CategoryDesc* FindCategoryDesc(uint32_t categoryHash) const { return const_cast<KLogManager*>(this)->FindCategoryDesc(categoryHash); }
    const CategoryDesc& GetCategoryDesc(uint32_t categoryHash) const;

    mutable KMutex      m_Mutex;
    KConditionVariable  m_ConditionVar;

    std::map<int, CategoryDesc> m_LogCategories;
    std::deque<LogEntry>        m_LogEntries;
    TimeValNanos                m_PreviousTimestamp;

    PString     m_LogFilePath;
    size_t      m_MaxLogFileSize      = 10 * 1024;
    int         m_MaxLogFiles         = 5;
    int         m_LogFileHandle       = -1;
    size_t      m_CurrentFileSize     = 0;
    int         m_LogHistoryPageIndex       = 0;
    off_t       m_LogHistoryFilePosition    = -1;
    bool        m_LogHistoryActive          = false;
    int64_t     m_LogHistoryOldestTimestamp = 0;
};

} // namespace

