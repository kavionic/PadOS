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

#include <PadOS/Time.h>

#include <Kernel/Logging/LogManager.h>
#include <SerialConsole/SerialProtocol.h>
#include <SerialConsole/SerialCommandHandler.h>

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KLogManager::KLogManager() : KThread("log_manager"), m_Mutex("log_manager", PEMutexRecursionMode_RaiseError), m_ConditionVar("log_manager")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::Setup(int threadPriority, size_t threadStackSize)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        Start_trw(PThreadDetachState_Detached, threadPriority, threadStackSize);
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
                    continue;
                }
                const PString text = std::format("[{:<8}: {:<7.7}]: {}\n", GetCategoryDisplayName_pl(entry.CategoryHash), GetLogSeverityName(entry.Severity), entry.Message);
                m_LogEntries.pop_front();

                SerialProtocol::LogMessage header;
                header.InitMsg(header);
                header.PackageLength = sizeof(header) + text.size();

                m_Mutex.Unlock();
                for (;;)
                {
                    if (SerialCommandHandler::Get().SendSerialData(&header, sizeof(header), text.data(), text.size())) {
                        break;
                    } else {
                        snooze_ms(100);
                    }
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

bool KLogManager::RegisterCategory(uint32_t categoryHash, const char* categoryName, const char* displayName, PLogSeverity initialLogLevel)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    m_LogCategories.emplace(categoryHash, CategoryDesc(initialLogLevel, categoryName, displayName));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KLogManager::SetCategoryMinimumSeverity(uint32_t categoryHash, PLogSeverity logLevel)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    CategoryDesc* desc = FindCategoryDesc(categoryHash);

    if (desc != nullptr) {
        desc->MinSeverity = logLevel;
    } else {
        kprintf("ERROR: kernel_log_set_category_log_level() called with unknown categoryHash %08x\n", categoryHash);
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

void KLogManager::AddLogMessage(uint32_t category, PLogSeverity severity, const PString& message)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        kassert(!m_Mutex.IsLocked());
        CRITICAL_SCOPE(m_Mutex);

        if (m_LogEntries.size() < 1000)
        {
            m_LogEntries.push_back(LogEntry{ .Timestamp = get_real_time(), .CategoryHash = category, .Severity = severity, .Message = message });
            m_ConditionVar.WakeupAll();
        }
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
        static CategoryDesc unknownDesc(PLogSeverity::NONE, "*invalid*", "*invalid*");
        return unknownDesc;
    }
}

void kadd_log_message(uint32_t category, PLogSeverity severity, const PString& message)
{
    if constexpr (PLogSeverity_Minimum != PLogSeverity::NONE)
    {
        kernel::KLogManager::Get().AddLogMessage(category, severity, message);
    }
}

} // namespace kernel
