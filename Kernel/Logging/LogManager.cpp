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

#include <string.h>
#include <vector>

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

KLogManager::KLogManager() : KThread("log_manager"), m_Mutex("log_manager", PEMutexRecursionMode_RaiseError), m_ConditionVar("log_manager")
{
    g_LogEntries = &m_LogEntries;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::Setup(int threadPriority, size_t threadStackSize)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
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

                const PString message = entry.Message;
                m_LogEntries.pop_front();
                m_ConditionVar.WakeupAll();

                SerialProtocol::LogMessage msgHeader;
                msgHeader.InitMsg(msgHeader, timestamp, categoryHash, severity);
                msgHeader.PackageLength = sizeof(msgHeader) + message.size();

                m_Mutex.Unlock();

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
                    const PString text = PString::format_string("[{:<8}: {:<7.7}]: {}\n", GetCategoryDisplayName(entry.CategoryHash), GetLogSeverityName(entry.Severity), entry.Message);
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

KLogManager& KLogManager::Get()
{
    static KLogManager instance;
    return instance;
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

void KLogManager::RegisterSerialHandlers()
{
    SerialCommandHandler::Get().RegisterPacketHandler<SerialProtocol::RequestLogCategories>(this, &KLogManager::HandleRequestLogCategories);
    SerialCommandHandler::Get().RegisterPacketHandler<SerialProtocol::RequestLogSeverities>(this, &KLogManager::HandleRequestLogSeverities);
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
        entry.MinSeverity  = std::to_underlying(desc.MinSeverity);

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

void KLogManager::FlushMessages(TimeValNanos timeout)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    TimeValNanos deadline = timeout.IsInfinit() ? TimeValNanos::infinit : (kget_monotonic_time() + timeout);
    while(!m_LogEntries.empty() && kget_monotonic_time() <= deadline)
    {
        m_ConditionVar.Wait(m_Mutex);
    }
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
