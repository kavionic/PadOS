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

#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <vector>

#include <System/ExceptionHandling.h>

#include <PadOS/Time.h>

#include <Kernel/KLogging.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/Logging/LogManager.h>
#include <SerialConsole/SerialProtocol.h>
#include <SerialConsole/SerialCommandHandler.h>


namespace kernel
{

const std::deque<KLogManager::LogEntry>* g_LogEntries;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KLogManager& KLogManager::Get()
{
    static KLogManager instance;
    return instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KLogManager::KLogManager() : KThread("log_manager"), m_Mutex("log_manager", PEMutexRecursionMode_RaiseError), m_ConditionVar("log_manager")
{
    g_LogEntries = &m_LogEntries;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::Setup(int threadPriority, size_t threadStackSize,
                        const char* logFilePath, size_t maxLogFileSize, int maxLogFiles)
{
    m_LogFilePath = logFilePath;
    m_MaxLogFileSize = maxLogFileSize;
    m_MaxLogFiles    = maxLogFiles;

    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        kcreate_directory(KLocateFlags(KLocateFlag::KernelCtx), "/var/logs");
        OpenLogFile();
        RegisterSerialHandlers();
        Start_trw(KSpawnThreadFlag::None, PThreadDetachState_Detached, threadPriority, threadStackSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* KLogManager::Run()
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        CRITICAL_SCOPE(m_Mutex);

        for (;;)
        {
            while (m_LogEntries.empty()) {
                m_ConditionVar.Wait(m_Mutex);
            }
            while (!m_LogEntries.empty())
            {
                const LogEntry& entry = m_LogEntries.front();
                if (!IsCategoryActive_pl(entry.CategoryHash, entry.Severity))
                {
                    m_LogEntries.pop_front();
                    m_ConditionVar.WakeupAll();
                    continue;
                }
                const PLogChannel   channel      = GetCategoryChannel_pl(entry.CategoryHash);
                const int64_t       timestamp    = entry.Timestamp.AsNanoseconds();
                const uint32_t      categoryHash = entry.CategoryHash;
                const uint8_t       severity     = uint8_t(entry.Severity);
                const PString       categoryName = GetCategoryName_pl(entry.CategoryHash);
                const char*         severityName = GetLogSeverityName(entry.Severity);

                const PString message = entry.Message;
                m_LogEntries.pop_front();
                m_ConditionVar.WakeupAll();

                SerialProtocol::LogMessage msgHeader;
                msgHeader.InitMsg(msgHeader, timestamp, categoryHash, severity);
                msgHeader.PackageLength = sizeof(msgHeader) + message.size();

                m_Mutex.Unlock();

                WriteEntryToFile(timestamp, categoryName, severityName, message);

                if (channel == PLogChannel::SerialManager)
                {
                    for (;;)
                    {
                        if (SerialCommandHandler::Get().SendSerialData(&msgHeader, sizeof(msgHeader), message.data(), message.size())) {
                            break;
                        } else {
                            snooze_ms(100);
                        }
                    }
                }
                else
                {
                    const PString text = PString::format_string("[{:<8}: {:<7.7}]: {}\n", GetCategoryDisplayName(categoryHash), severityName, message);
                    kwrite(1, text.data(), text.size());
                }
                m_Mutex.Lock();
            }
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KLogManager::RegisterCategory(uint32_t categoryHash, PLogChannel channel, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    m_LogCategories.emplace(categoryHash, CategoryDesc(channel, initialLogLevel, categoryName, displayName));
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode KLogManager::SetCategoryMinimumSeverity(uint32_t categoryHash, PLogSeverity logLevel)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    CategoryDesc* desc = FindCategoryDesc(categoryHash);

    if (desc != nullptr)
    {
        desc->MinSeverity = logLevel;
        return PErrorCode::Success;
    }
    else
    {
        kprintf("ERROR: kernel_log_set_category_log_level() called with unknown categoryHash %08x\n", categoryHash);
        return PErrorCode::NOENT;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KLogManager::IsCategoryActive(uint32_t categoryHash, PLogSeverity logLevel)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return IsCategoryActive_pl(categoryHash, logLevel);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KLogManager::IsCategoryActive_pl(uint32_t categoryHash, PLogSeverity logLevel)
{
    kassert(m_Mutex.IsLocked());
    const CategoryDesc& desc = GetCategoryDesc(categoryHash);
    return logLevel <= desc.MinSeverity;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PLogChannel KLogManager::GetCategoryChannel(uint32_t categoryHash) const
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return GetCategoryChannel_pl(categoryHash);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PLogChannel KLogManager::GetCategoryChannel_pl(uint32_t categoryHash) const
{
    kassert(m_Mutex.IsLocked());
    const CategoryDesc& desc = GetCategoryDesc(categoryHash);
    return desc.Channel;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const char* KLogManager::GetLogSeverityName(PLogSeverity logLevel)
{
    return PLogSeverity_names[logLevel];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& KLogManager::GetCategoryName(uint32_t categoryHash)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return GetCategoryName_pl(categoryHash);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& KLogManager::GetCategoryName_pl(uint32_t categoryHash)
{
    kassert(m_Mutex.IsLocked());
    return GetCategoryDesc(categoryHash).CategoryName;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& KLogManager::GetCategoryDisplayName(uint32_t categoryHash)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return GetCategoryDisplayName_pl(categoryHash);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& KLogManager::GetCategoryDisplayName_pl(uint32_t categoryHash)
{
    kassert(m_Mutex.IsLocked());
    return GetCategoryDesc(categoryHash).DisplayName;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<std::pair<uint32_t, KLogManager::CategoryDesc>> KLogManager::GetCategoryList()
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return std::vector<std::pair<uint32_t, CategoryDesc>>(m_LogCategories.begin(), m_LogCategories.end());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::AddLogMessage(uint32_t category, PLogSeverity severity, const PString& message)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        kassert(!m_Mutex.IsLocked());
        CRITICAL_SCOPE(m_Mutex);

        if (m_LogEntries.size() < 1000)
        {
            m_LogEntries.push_back(LogEntry{ .Timestamp = kget_real_time(), .CategoryHash = category, .Severity = severity, .Message = message });
            m_ConditionVar.WakeupAll();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::FlushMessages(TimeValNanos timeout)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    TimeValNanos deadline = timeout.IsInfinit() ? TimeValNanos::infinit : (kget_monotonic_time() + timeout);
    while (!m_LogEntries.empty() && kget_monotonic_time() <= deadline)
    {
        m_ConditionVar.Wait(m_Mutex);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::RegisterSerialHandlers()
{
    SerialCommandHandler::Get().RegisterPacketHandler<SerialProtocol::RequestLogCategories>(this, &KLogManager::HandleRequestLogCategories);
    SerialCommandHandler::Get().RegisterPacketHandler<SerialProtocol::RequestLogSeverities>(this, &KLogManager::HandleRequestLogSeverities);
    SerialCommandHandler::Get().RegisterPacketHandler<SerialProtocol::RequestLogHistory>(this, &KLogManager::HandleRequestLogHistory);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::HandleRequestLogCategories(const SerialProtocol::RequestLogCategories& packet)
{
    const auto categories = GetCategoryList();

    std::vector<SerialProtocol::LogCategoryEntry> entries;

    entries.reserve(categories.size());

    for (const auto& [hash, desc] : categories)
    {
        SerialProtocol::LogCategoryEntry& entry = entries.emplace_back();

        entry = {};

        entry.CategoryHash = hash;
        entry.MinSeverity = std::to_underlying(desc.MinSeverity);

        strncpy(entry.CategoryName, desc.CategoryName.c_str(), sizeof(entry.CategoryName) - 1);
        strncpy(entry.DisplayName, desc.DisplayName.c_str(), sizeof(entry.DisplayName) - 1);
    }

    SerialProtocol::LogCategoriesReply reply;
    SerialProtocol::LogCategoriesReply::InitMsg(reply, uint32_t(entries.size()));
    reply.PackageLength += uint32_t(entries.size() * sizeof(SerialProtocol::LogCategoryEntry));

    SerialCommandHandler::Get().SendSerialData(&reply, sizeof(reply), entries.data(), entries.size() * sizeof(SerialProtocol::LogCategoryEntry));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::HandleRequestLogSeverities(const SerialProtocol::RequestLogSeverities& packet)
{
    std::vector<SerialProtocol::LogSeverityEntry> entries;

    for (const auto& [severityEnum, name] : PLogSeverity_names.Names)
    {
        if (severityEnum != PLogSeverity::NONE)
        {
            SerialProtocol::LogSeverityEntry& entry = entries.emplace_back();

            entry = {};
            entry.SeverityID = std::to_underlying(severityEnum);

            strncpy(entry.Name, name, sizeof(entry.Name) - 1);
        }
    }

    SerialProtocol::LogSeveritiesReply reply;
    SerialProtocol::LogSeveritiesReply::InitMsg(reply, uint32_t(entries.size()));
    reply.PackageLength += uint32_t(entries.size() * sizeof(SerialProtocol::LogSeverityEntry));

    SerialCommandHandler::Get().SendSerialData(&reply, sizeof(reply), entries.data(), entries.size() * sizeof(SerialProtocol::LogSeverityEntry));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int64_t KLogManager::ParseLineTimestamp(const char* line, size_t length)
{
    const char* const end = line + length;
    const char* p = line;

    // Skip token 1: human timestamp.
    while (p < end && *p != ' ') { ++p; }
    if (p >= end) { return INT64_MAX; }
    ++p;

    // Parse token 2: nanosecond timestamp.
    char* parseEnd;
    const int64_t nanos = strtoll(p, &parseEnd, 10);
    if (parseEnd == p || (parseEnd < end && *parseEnd != ' ')) { return INT64_MAX; }

    return nanos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int64_t KLogManager::ReadFileFirstTimestamp(const PString& filename)
{
    try
    {
        const int fileHandle = kopen_trw(filename.c_str(), O_RDONLY);
        PScopeExit closeHandle([fileHandle]() { kclose(fileHandle); });

        static constexpr size_t READ_SIZE = 256;
        char buf[READ_SIZE];
        size_t bytesRead = 0;
        kread(fileHandle, buf, READ_SIZE - 1, bytesRead);
        if (bytesRead == 0) { return INT64_MAX; }
        buf[bytesRead] = '\0';

        const char* newline = static_cast<const char*>(memchr(buf, '\n', bytesRead));
        const size_t lineLen = (newline != nullptr) ? size_t(newline - buf) : bytesRead;

        return ParseLineTimestamp(buf, lineLen);
    }
    catch (...) { return INT64_MAX; }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off_t KLogManager::FindEndPositionBeforeTimestamp(int fileHandle, off_t fileSize, int64_t targetTimestamp)
{
    static constexpr size_t LINE_BUF_SIZE = 512;
    char lineBuf[LINE_BUF_SIZE];

    off_t lo     = 0;
    off_t hi     = fileSize;
    off_t result = 0;

    while (lo < hi)
    {
        const off_t mid = lo + (hi - lo) / 2;
        klseek_trw(fileHandle, mid, SEEK_SET);

        // Skip partial line (scan for newline), unless we are at start of file.
        off_t lineStart = mid;
        if (mid > 0)
        {
            size_t bytesRead = 0;
            kread(fileHandle, lineBuf, size_t(std::min(off_t(LINE_BUF_SIZE), hi - mid)), bytesRead);
            const char* newline = static_cast<const char*>(memchr(lineBuf, '\n', bytesRead));
            if (newline == nullptr)
            {
                hi = mid;
                continue;
            }
            lineStart = mid + off_t(newline + 1 - lineBuf);
        }

        if (lineStart >= hi)
        {
            hi = mid;
            continue;
        }

        klseek_trw(fileHandle, lineStart, SEEK_SET);
        size_t bytesRead = 0;
        kread(fileHandle, lineBuf, size_t(std::min(off_t(LINE_BUF_SIZE - 1), hi - lineStart)), bytesRead);
        if (bytesRead == 0)
        {
            hi = mid;
            continue;
        }
        lineBuf[bytesRead] = '\0';

        const char* newline = static_cast<const char*>(memchr(lineBuf, '\n', bytesRead));
        const size_t lineLen = (newline != nullptr) ? size_t(newline - lineBuf) : bytesRead;

        const int64_t timestamp = ParseLineTimestamp(lineBuf, lineLen);

        if (timestamp < targetTimestamp)
        {
            result = lineStart + off_t(lineLen) + (newline != nullptr ? 1 : 0);
            lo = result;
        }
        else
        {
            if (mid <= lo) { break; }
            hi = mid;
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KLogManager::FindFilePositionForTimestamp(int64_t targetTimestamp, int& outPageIndex, off_t& outFilePosition)
{
    // Check current (newest) log file first.
    if (ReadFileFirstTimestamp(m_LogFilePath) < targetTimestamp)
    {
        try
        {
            const int fileHandle = kopen_trw(m_LogFilePath.c_str(), O_RDONLY);
            PScopeExit closeHandle([fileHandle]() { kclose(fileHandle); });
            const off_t fileSize = klseek_trw(fileHandle, 0, SEEK_END);
            outPageIndex    = -1;
            outFilePosition = FindEndPositionBeforeTimestamp(fileHandle, fileSize, targetTimestamp);
            return true;
        }
        catch (...) {}
    }

    // Find max rotated file index.
    int maxIndex = -1;
    for (int i = 0; i < m_MaxLogFiles; ++i)
    {
        try
        {
            const int testHandle = kopen_trw(PString::format_string("{}.{}", m_LogFilePath, i).c_str(), O_RDONLY);
            kclose(testHandle);
            maxIndex = i;
        }
        catch (...) { break; }
    }

    if (maxIndex < 0) { return false; }

    // Binary search among rotated files for the lowest index k where firstTimestamp < targetTimestamp.
    // firstTimestamp decreases as the index increases (higher index = older file).
    int lo     = 0;
    int hi     = maxIndex;
    int result = -1;
    while (lo <= hi)
    {
        const int mid = lo + (hi - lo) / 2;
        const int64_t firstTs = ReadFileFirstTimestamp(PString::format_string("{}.{}", m_LogFilePath, mid));
        if (firstTs < targetTimestamp)
        {
            result = mid;
            hi = mid - 1;
        }
        else
        {
            lo = mid + 1;
        }
    }

    if (result < 0) { return false; }

    try
    {
        const PString filename = PString::format_string("{}.{}", m_LogFilePath, result);
        const int fileHandle = kopen_trw(filename.c_str(), O_RDONLY);
        PScopeExit closeHandle([fileHandle]() { kclose(fileHandle); });
        const off_t fileSize = klseek_trw(fileHandle, 0, SEEK_END);
        outPageIndex    = result;
        outFilePosition = FindEndPositionBeforeTimestamp(fileHandle, fileSize, targetTimestamp);
        return true;
    }
    catch (...) { return false; }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::HandleRequestLogHistory(const SerialProtocol::RequestLogHistory& packet)
{
    int   pageIndex;
    off_t filePosition;
    bool  needsSearch = false;
    {
        kassert(!m_Mutex.IsLocked());
        CRITICAL_SCOPE(m_Mutex);
        if (packet.LastTimestamp == 0)
        {
            m_LogHistoryPageIndex       = -1;
            m_LogHistoryFilePosition    = -1;
            m_LogHistoryOldestTimestamp = INT64_MAX;
            m_LogHistoryActive          = true;
        }
        else if (packet.LastTimestamp != m_LogHistoryOldestTimestamp || !m_LogHistoryActive)
        {
            needsSearch = true;
        }
        pageIndex    = m_LogHistoryPageIndex;
        filePosition = m_LogHistoryFilePosition;
    }

    if (needsSearch)
    {
        int   searchPageIndex;
        off_t searchFilePosition;
        if (!FindFilePositionForTimestamp(packet.LastTimestamp, searchPageIndex, searchFilePosition))
        {
            SerialProtocol::LogHistoryComplete reply;
            SerialProtocol::LogHistoryComplete::InitMsg(reply, false);
            SerialCommandHandler::Get().SendSerialData(&reply, sizeof(reply), nullptr, 0);
            return;
        }
        {
            kassert(!m_Mutex.IsLocked());
            CRITICAL_SCOPE(m_Mutex);
            m_LogHistoryPageIndex    = searchPageIndex;
            m_LogHistoryFilePosition = searchFilePosition;
            m_LogHistoryActive       = true;
            pageIndex    = m_LogHistoryPageIndex;
            filePosition = m_LogHistoryFilePosition;
        }
    }

    const PString filename = (pageIndex < 0) ? m_LogFilePath : PString::format_string("{}.{}", m_LogFilePath, pageIndex);
    off_t   actualStart          = 0;
    int64_t batchOldestTimestamp = INT64_MAX;
    try
    {
        const int fileHandle = kopen_trw(filename.c_str(), O_RDONLY);
        PScopeExit closeHandle([fileHandle]() { kclose(fileHandle); });

        if (filePosition == -1) {
            filePosition = klseek_trw(fileHandle, 0, SEEK_END);
        }
        const off_t readEnd = filePosition;
        const off_t seekPos = std::max(off_t(0), readEnd - off_t(packet.PageSize));
        klseek_trw(fileHandle, seekPos, SEEK_SET);

        const auto categories = GetCategoryList();

        static constexpr size_t CHUNK_SIZE = 512;
        char chunk[CHUNK_SIZE];
        PString lineBuffer;

        actualStart = seekPos;
        bool  skipToNewline  = (seekPos > 0);
        off_t bytesRemaining = readEnd - seekPos;

        for (;;)
        {
            if (bytesRemaining <= 0) {
                break;
            }

            size_t bytesRead = 0;
            size_t toRead = size_t(std::min(off_t(CHUNK_SIZE), bytesRemaining));
            kread(fileHandle, chunk, toRead, bytesRead);

            if (bytesRead == 0) {
                break;
            }

            bytesRemaining -= off_t(bytesRead);

            const char* segmentStart = chunk;
            const char* chunkEnd = chunk + bytesRead;

            if (skipToNewline)
            {
                const char* newline = static_cast<const char*>(memchr(segmentStart, '\n', chunkEnd - segmentStart));
                if (newline == nullptr)
                {
                    actualStart += off_t(bytesRead);
                    continue;
                }
                actualStart += off_t(newline + 1 - segmentStart);
                segmentStart = newline + 1;
                skipToNewline = false;
            }

            while (segmentStart < chunkEnd)
            {
                const char* newline = static_cast<const char*>(memchr(segmentStart, '\n', chunkEnd - segmentStart));
                if (newline != nullptr)
                {
                    lineBuffer.append(segmentStart, newline - segmentStart);
                    segmentStart = newline + 1;

                    SerialProtocol::LogMessage msgHeader;
                    PString message;
                    if (ParseLogfileLine(lineBuffer, msgHeader, message)) {
                        batchOldestTimestamp = std::min(batchOldestTimestamp, msgHeader.Timestamp);
                        SerialCommandHandler::Get().SendSerialData(&msgHeader, sizeof(msgHeader), message.data(), message.size());
                    }
                    lineBuffer.clear();
                }
                else
                {
                    lineBuffer.append(segmentStart, chunkEnd - segmentStart);
                    break;
                }
            }
        }
    }
    catch (...)
    {
        SerialProtocol::LogHistoryComplete reply;
        SerialProtocol::LogHistoryComplete::InitMsg(reply, false);
        SerialCommandHandler::Get().SendSerialData(&reply, sizeof(reply), nullptr, 0);
        return;
    }

    {
        kassert(!m_Mutex.IsLocked());
        CRITICAL_SCOPE(m_Mutex);
        m_LogHistoryFilePosition    = actualStart;
        m_LogHistoryOldestTimestamp = batchOldestTimestamp;
        if (actualStart == 0)
        {
            m_LogHistoryPageIndex    = (m_LogHistoryPageIndex < 0) ? 0 : m_LogHistoryPageIndex + 1;
            m_LogHistoryFilePosition = -1;
        }
        pageIndex    = m_LogHistoryPageIndex;
        filePosition = m_LogHistoryFilePosition;
    }

    bool hasMorePages = (filePosition > 0);
    if (!hasMorePages)
    {
        try
        {
            const int nextHandle = kopen_trw(PString::format_string("{}.{}", m_LogFilePath, pageIndex).c_str(), O_RDONLY);
            kclose(nextHandle);
            hasMorePages = true;
        }
        catch (...) {}
    }

    SerialProtocol::LogHistoryComplete reply;
    SerialProtocol::LogHistoryComplete::InitMsg(reply, hasMorePages);
    SerialCommandHandler::Get().SendSerialData(&reply, sizeof(reply), nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::OpenLogFile()
{
    try
    {
        m_LogFileHandle = kopen_trw(m_LogFilePath.c_str(), O_WRONLY | O_CREAT | O_APPEND);
        struct stat fileStats;
        if (kread_stat(m_LogFileHandle, &fileStats) == PErrorCode::Success) {
            m_CurrentFileSize = static_cast<size_t>(fileStats.st_size);
        } else {
            m_CurrentFileSize = 0;
        }
    }
    catch (...)
    {
        m_LogFileHandle   = -1;
        m_CurrentFileSize = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::RotateLogFiles()
{
    kclose(m_LogFileHandle);
    m_LogFileHandle = -1;

    for (int i = m_MaxLogFiles - 2; i >= 0; --i)
    {
        krename(KLocateFlags(KLocateFlag::KernelCtx),
                PString::format_string("{}.{}", m_LogFilePath, i).c_str(),
                PString::format_string("{}.{}", m_LogFilePath, i + 1).c_str());
    }
    krename(KLocateFlags(KLocateFlag::KernelCtx),
            m_LogFilePath.c_str(),
            PString::format_string("{}.0", m_LogFilePath).c_str());

    {
        kassert(!m_Mutex.IsLocked());
        CRITICAL_SCOPE(m_Mutex);
        if (m_LogHistoryActive) {
            ++m_LogHistoryPageIndex;
        }
    }

    OpenLogFile();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::WriteEntryToFile(int64_t timestamp,
                                   const PString& categoryName, const char* severityName,
                                   const PString& message)
{
    if (m_LogFileHandle < 0) {
        return;
    }
    const time_t seconds      = static_cast<time_t>(timestamp / 1'000'000'000LL);
    const int    centiseconds = static_cast<int>((timestamp % 1'000'000'000LL) / 10'000'000LL);
    const tm*    timeInfo     = gmtime(&seconds);

    const PString humanTS = PString::format_string("{:04}{:02}{:02}.{:02}:{:02}:{:02}.{:02}",
        timeInfo->tm_year + 1900,
        timeInfo->tm_mon + 1,
        timeInfo->tm_mday,
        timeInfo->tm_hour,
        timeInfo->tm_min,
        timeInfo->tm_sec,
        centiseconds);

    PString escaped;
    escaped.reserve(message.size());
    for (const char character : message)
    {
        if (character == '\\')      { escaped += "\\\\"; }
        else if (character == '\n') { escaped += "\\n"; }
        else if (character == '"')  { escaped += "\\\""; }
        else                        { escaped += character; }
    }

    const PString line = PString::format_string("{} {} {} {} \"{}\"\n",
        humanTS, timestamp, categoryName, severityName, escaped);

    size_t written = 0;
    kwrite(m_LogFileHandle, line.data(), line.size(), written);
    m_CurrentFileSize += written;

    if (m_CurrentFileSize >= m_MaxLogFileSize) {
        RotateLogFiles();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KLogManager::ParseLogfileLine(const PString& lineBuffer, SerialProtocol::LogMessage& msgHeader, PString& message)
{
    const char* p = lineBuffer.c_str();

    // Token 1: skip human timestamp.
    while (*p != '\0' && *p != ' ') { ++p; }
    if (*p != ' ') { return false; }
    ++p;

    // Token 2: nanosecond timestamp.
    char* end;
    const int64_t nanos = strtoll(p, &end, 10);
    if (end == p || *end != ' ') { return false; }
    p = end + 1;

    // Token 3: category name.
    const char* catStart = p;
    while (*p != '\0' && *p != ' ') { ++p; }
    if (*p != ' ') { return false; }
    const PString categoryName(catStart, p - catStart);
    ++p;

    // Token 4: severity name.
    const char* sevStart = p;
    while (*p != '\0' && *p != ' ') { ++p; }
    if (*p != ' ') { return false; }
    const PString severityName(sevStart, p - sevStart);
    ++p;

    // Token 5: double-quoted message.
    if (*p != '"') { return false; }
    ++p;

    message.reserve(lineBuffer.size());
    while (*p != '\0')
    {
        if (*p == '\\')
        {
            ++p;
            if (*p == '\0')     { break; }
            if (*p == '\\')     { message += '\\'; }
            else if (*p == 'n') { message += '\n'; }
            else if (*p == '"') { message += '"'; }
            else                { message += '\\'; message += *p; }
        }
        else if (*p == '"')
        {
            break;
        }
        else
        {
            message += *p;
        }
        ++p;
    }

    // Calculate category hash.
    const uint32_t categoryHash = PString::hash_string_literal(categoryName.c_str());

    // Look up severity enum.
    PLogSeverity severity = PLogSeverity::NONE;
    for (const auto& [severityEnum, name] : PLogSeverity_names.Names)
    {
        if (severityName == name)
        {
            severity = severityEnum;
            break;
        }
    }

    msgHeader.InitMsg(msgHeader, nanos, categoryHash, uint8_t(severity));
    msgHeader.PackageLength = sizeof(msgHeader) + uint32_t(message.size());

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KLogManager::CategoryDesc* KLogManager::FindCategoryDesc(uint32_t categoryHash)
{
    kassert(m_Mutex.IsLocked());
    auto i = m_LogCategories.find(categoryHash);
    if (i != m_LogCategories.end()) [[likely]]
    {
        return &i->second;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const KLogManager::CategoryDesc& KLogManager::GetCategoryDesc(uint32_t categoryHash) const
{
    kassert(m_Mutex.IsLocked());
    const CategoryDesc* desc = FindCategoryDesc(categoryHash);

    if (desc != nullptr) [[likely]]
    {
        return *desc;
    }
    else
    {
        static CategoryDesc unknownDesc(PLogChannel::DebugPort, PLogSeverity::NONE, "*invalid*", "*invalid*");
        return unknownDesc;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksystem_log_register_category(uint32_t categoryHash, PLogChannel channel, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        return KLogManager::Get().RegisterCategory(categoryHash, channel, categoryName, displayName, initialLogLevel);
    }
    return PErrorCode::NOSYS;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode ksystem_log_set_category_minimum_severity(uint32_t categoryHash, PLogSeverity logLevel)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        return KLogManager::Get().SetCategoryMinimumSeverity(categoryHash, logLevel);
    }
    return PErrorCode::NOSYS;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ksystem_log_is_category_active(uint32_t categoryHash, PLogSeverity logLevel)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        return KLogManager::Get().IsCategoryActive(categoryHash, logLevel);
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PLogChannel ksystem_log_get_category_channel(uint32_t categoryHash)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        return KLogManager::Get().GetCategoryChannel(categoryHash);
    }
    return PLogChannel::DebugPort;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const char* ksystem_log_get_severity_name(PLogSeverity logLevel)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        return KLogManager::Get().GetLogSeverityName(logLevel);
    }
    return "";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& ksystem_log_get_category_name(uint32_t categoryHash)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        return KLogManager::Get().GetCategoryName(categoryHash);
    }
    return PString::zero;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PString& ksystem_log_get_category_display_name(uint32_t categoryHash)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        return KLogManager::Get().GetCategoryDisplayName(categoryHash);
    }
    return PString::zero;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksystem_log_add_message(uint32_t category, PLogSeverity severity, const PString& message)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        KLogManager::Get().AddLogMessage(category, severity, message);
    }
}

} // namespace kernel
