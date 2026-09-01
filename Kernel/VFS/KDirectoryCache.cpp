// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 01.09.2026

#include "System/Platform.h"

#include <stddef.h>
#include <stdint.h>
#include <malloc.h>
#include <new>
#include <string.h>

#include <Kernel/VFS/KDirectoryCache.h>
#include <Utils/HashCalculator.h>

namespace kernel
{

struct KDirectoryCache::Entry
{
    char* GetName() noexcept { return reinterpret_cast<char*>(this + 1); }
    const char* GetName() const noexcept { return reinterpret_cast<const char*>(this + 1); }

    KDirectoryCache* Owner = nullptr;
    Entry* HashNext = nullptr;
    Entry* LRUPrevious = nullptr;
    Entry* LRUNext = nullptr;
    ino_t  ParentInodeID = 0;
    ino_t  ResultInodeID = 0;
    uint32_t Hash = 0;
    uint16_t NameLength = 0;
    uint16_t AllocationSize = 0;
};

std::map<fs_id, KDirectoryCache*> KDirectoryCache::s_VolumeMap;
KMutex KDirectoryCache::s_Mutex("directory_cache_mutex", PEMutexRecursionMode_RaiseError);
KDirectoryCache::Entry* KDirectoryCache::s_LRUFirst = nullptr;
KDirectoryCache::Entry* KDirectoryCache::s_LRULast = nullptr;
size_t KDirectoryCache::s_MemoryUsage = 0;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDirectoryCache::KDirectoryCache() = default;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDirectoryCache::~KDirectoryCache()
{
    const bool volumeWasCleared = SetVolume(-1);
    kassert(volumeWasCleared);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KDirectoryCache::SetVolume(fs_id volumeID)
{
    CRITICAL_SCOPE(s_Mutex);

    auto previousVolume = s_VolumeMap.end();
    bool previousVolumeRegistrationIsValid = m_VolumeID == -1;
    if (m_VolumeID != -1)
    {
        previousVolume = s_VolumeMap.find(m_VolumeID);
        previousVolumeRegistrationIsValid = previousVolume != s_VolumeMap.end() && previousVolume->second == this;
        kassert(previousVolumeRegistrationIsValid);
    }

    if (m_VolumeID == volumeID && previousVolumeRegistrationIsValid) {
        return true;
    }

    bool volumeRegistrationSucceeded = true;
    if (volumeID != -1) {
        try
        {
            const auto volumeInsertionResult = s_VolumeMap.try_emplace(volumeID, this);
            volumeRegistrationSucceeded = volumeInsertionResult.second;
        }
        catch (const std::bad_alloc&) {
            volumeRegistrationSucceeded = false;
        }
    }

    if (!volumeRegistrationSucceeded)
    {
        if (!previousVolumeRegistrationIsValid)
        {
            RemoveAllEntries_pl();
            m_VolumeID = -1;
        }
        return false;
    }

    RemoveAllEntries_pl();
    if (previousVolumeRegistrationIsValid && previousVolume != s_VolumeMap.end()) {
        s_VolumeMap.erase(previousVolume);
    }
    m_VolumeID = volumeID;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDirectoryCacheLookupResult KDirectoryCache::Lookup(
    ino_t parentInodeID,
    std::string_view name,
    ino_t* resultInodeID) noexcept
{
    if (!IsCacheableName(name)) {
        return KDirectoryCacheLookupResult::Miss;
    }

    const uint32_t hash = CalculateHash(parentInodeID, name);
    CRITICAL_SCOPE(s_Mutex);
    if (m_VolumeID == -1) {
        return KDirectoryCacheLookupResult::Miss;
    }

    Entry* entry = FindEntry_pl(parentInodeID, name, hash);
    if (entry == nullptr) {
        return KDirectoryCacheLookupResult::Miss;
    }

    TouchEntry_pl(entry);
    if (resultInodeID != nullptr) {
        *resultInodeID = entry->ResultInodeID;
    }
    return (entry->ResultInodeID == 0)
        ? KDirectoryCacheLookupResult::Negative
        : KDirectoryCacheLookupResult::Positive;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::InsertPositive(
    ino_t parentInodeID,
    std::string_view name,
    ino_t resultInodeID) noexcept
{
    if (resultInodeID == 0) {
        RemoveEntry(parentInodeID, name);
    } else {
        Insert(parentInodeID, name, resultInodeID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::InsertNegative(ino_t parentInodeID, std::string_view name) noexcept
{
    Insert(parentInodeID, name, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::RemoveEntry(ino_t parentInodeID, std::string_view name) noexcept
{
    if (!IsCacheableName(name)) {
        return;
    }

    const uint32_t hash = CalculateHash(parentInodeID, name);
    CRITICAL_SCOPE(s_Mutex);
    if (m_VolumeID == -1) {
        return;
    }

    Entry* entry = FindEntry_pl(parentInodeID, name, hash);
    if (entry != nullptr) {
        RemoveEntry_pl(entry);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::RemoveDirectory(fs_id volumeID, ino_t parentInodeID) noexcept
{
    CRITICAL_SCOPE(s_Mutex);
    const auto volumeIterator = s_VolumeMap.find(volumeID);
    if (volumeIterator != s_VolumeMap.end()) {
        volumeIterator->second->RemoveDirectory_pl(parentInodeID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::RemoveVolume(fs_id volumeID) noexcept
{
    CRITICAL_SCOPE(s_Mutex);
    const auto volumeIterator = s_VolumeMap.find(volumeID);
    if (volumeIterator != s_VolumeMap.end()) {
        volumeIterator->second->RemoveAllEntries_pl();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KDirectoryCache::IsCacheableName(std::string_view name) noexcept
{
    return !name.empty() && name != "." && name != "..";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t KDirectoryCache::CalculateHash(
    ino_t parentInodeID,
    std::string_view name) noexcept
{
    PHashCalculator<PHashAlgorithm::CRC32> hashCalculator;
    hashCalculator.AddData(&parentInodeID, sizeof(parentInodeID));
    hashCalculator.AddData(name.data(), name.size());
    return hashCalculator.Finalize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KDirectoryCache::GetBucketIndex(uint32_t hash) noexcept
{
    return hash & (HASH_BUCKET_COUNT - 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KDirectoryCache::Entry* KDirectoryCache::FindEntry_pl(
    ino_t parentInodeID,
    std::string_view name,
    uint32_t hash) noexcept
{
    kassert(s_Mutex.IsLocked());
    kassert(m_VolumeID != -1);

    Entry* entry = m_HashBuckets[GetBucketIndex(hash)];
    while (entry != nullptr)
    {
        if (entry->Hash == hash &&
            entry->ParentInodeID == parentInodeID &&
            size_t(entry->NameLength) == name.size() &&
            memcmp(entry->GetName(), name.data(), name.size()) == 0) {
            return entry;
        }
        entry = entry->HashNext;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::UpdateEntry_pl(Entry* entry, ino_t resultInodeID) noexcept
{
    kassert(s_Mutex.IsLocked());
    kassert(entry != nullptr);

    entry->ResultInodeID = resultInodeID;
    TouchEntry_pl(entry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::TouchEntry_pl(Entry* entry) noexcept
{
    kassert(s_Mutex.IsLocked());
    kassert(entry != nullptr);

    if (s_LRULast == entry) {
        return;
    }

    if (entry->LRUPrevious != nullptr) {
        entry->LRUPrevious->LRUNext = entry->LRUNext;
    } else {
        s_LRUFirst = entry->LRUNext;
    }
    if (entry->LRUNext != nullptr) {
        entry->LRUNext->LRUPrevious = entry->LRUPrevious;
    }

    entry->LRUPrevious = s_LRULast;
    entry->LRUNext = nullptr;
    if (s_LRULast != nullptr) {
        s_LRULast->LRUNext = entry;
    }
    s_LRULast = entry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::InsertEntry_pl(Entry* entry) noexcept
{
    kassert(s_Mutex.IsLocked());
    kassert(entry != nullptr);
    kassert(entry->Owner == this);
    kassert(entry->LRUPrevious == nullptr);
    kassert(entry->LRUNext == nullptr);

    const size_t bucketIndex = GetBucketIndex(entry->Hash);
    entry->HashNext = m_HashBuckets[bucketIndex];
    m_HashBuckets[bucketIndex] = entry;

    entry->LRUPrevious = s_LRULast;
    if (s_LRULast != nullptr) {
        s_LRULast->LRUNext = entry;
    } else {
        s_LRUFirst = entry;
    }
    s_LRULast = entry;
    s_MemoryUsage += entry->AllocationSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::RemoveEntry_pl(Entry* entry) noexcept
{
    kassert(s_Mutex.IsLocked());
    kassert(entry != nullptr);
    kassert(entry->Owner == this);

    Entry** hashEntry = &m_HashBuckets[GetBucketIndex(entry->Hash)];
    while (*hashEntry != entry)
    {
        kassert(*hashEntry != nullptr);
        hashEntry = &(*hashEntry)->HashNext;
    }
    RemoveHashEntry_pl(hashEntry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::RemoveHashEntry_pl(Entry** hashEntry) noexcept
{
    kassert(s_Mutex.IsLocked());
    kassert(hashEntry != nullptr);
    kassert(*hashEntry != nullptr);

    Entry* entry = *hashEntry;
    kassert(entry->Owner == this);
    *hashEntry = entry->HashNext;

    if (entry->LRUPrevious != nullptr) {
        entry->LRUPrevious->LRUNext = entry->LRUNext;
    } else {
        s_LRUFirst = entry->LRUNext;
    }
    if (entry->LRUNext != nullptr) {
        entry->LRUNext->LRUPrevious = entry->LRUPrevious;
    } else {
        s_LRULast = entry->LRUPrevious;
    }
    kassert(s_MemoryUsage >= entry->AllocationSize);
    s_MemoryUsage -= entry->AllocationSize;
    entry->~Entry();
    free(entry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::RemoveDirectory_pl(ino_t parentInodeID) noexcept
{
    kassert(s_Mutex.IsLocked());

    for (size_t i = 0; i < HASH_BUCKET_COUNT; ++i)
    {
        Entry** hashEntry = &m_HashBuckets[i];
        while (*hashEntry != nullptr)
        {
            Entry* entry = *hashEntry;
            if (entry->ParentInodeID == parentInodeID) {
                RemoveHashEntry_pl(hashEntry);
            } else {
                hashEntry = &entry->HashNext;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::RemoveAllEntries_pl() noexcept
{
    kassert(s_Mutex.IsLocked());

    for (size_t i = 0; i < HASH_BUCKET_COUNT; ++i) {
        while (m_HashBuckets[i] != nullptr) {
            RemoveHashEntry_pl(&m_HashBuckets[i]);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KDirectoryCache::Insert(
    ino_t parentInodeID,
    std::string_view name,
    ino_t resultInodeID) noexcept
{
    if (!IsCacheableName(name) ||
        name.size() > ENTRY_ALLOCATION_BUDGET ||
        sizeof(Entry) + 1 > ENTRY_ALLOCATION_BUDGET - name.size()) {
        return;
    }

    const size_t allocationSize = sizeof(Entry) + name.size() + 1;
    const uint32_t hash = CalculateHash(parentInodeID, name);
    CRITICAL_SCOPE(s_Mutex);
    if (m_VolumeID == -1) {
        return;
    }

    Entry* entry = FindEntry_pl(parentInodeID, name, hash);
    if (entry != nullptr)
    {
        UpdateEntry_pl(entry, resultInodeID);
        return;
    }

    while (s_MemoryUsage > ENTRY_ALLOCATION_BUDGET - allocationSize)
    {
        Entry* evictedEntry = s_LRUFirst;
        kassert(evictedEntry != nullptr);
        kassert(evictedEntry->Owner != nullptr);
        evictedEntry->Owner->RemoveEntry_pl(evictedEntry);
    }

    void* allocation = malloc(allocationSize);
    if (allocation != nullptr)
    {
        Entry* newEntry = new (allocation) Entry;
        newEntry->Owner = this;
        newEntry->ParentInodeID = parentInodeID;
        newEntry->ResultInodeID = resultInodeID;
        newEntry->Hash = hash;
        newEntry->NameLength = uint16_t(name.size());
        newEntry->AllocationSize = uint16_t(allocationSize);
        memcpy(newEntry->GetName(), name.data(), name.size());
        newEntry->GetName()[name.size()] = '\0';
        InsertEntry_pl(newEntry);
    }
}

} // namespace kernel
