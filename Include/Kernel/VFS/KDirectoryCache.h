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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <map>
#include <string_view>

#include <System/Types.h>

#include <Kernel/KMutex.h>

namespace kernel
{

enum class KDirectoryCacheLookupResult
{
    Miss,
    Negative,
    Positive
};

class KDirectoryCache
{
public:
    KDirectoryCache();
    ~KDirectoryCache();

    bool SetVolume(fs_id volumeID);

    // Filesystem callers hold their namespace lock while using this cache. Cache
    // maintenance must never unwind through a filesystem transaction. Cached
    // inodes belong to this cache's volume, and inode ID zero marks a negative
    // entry.
    KDirectoryCacheLookupResult Lookup(
        ino_t parentInodeID,
        std::string_view name,
        ino_t* resultInodeID) noexcept;

    void InsertPositive(
        ino_t parentInodeID,
        std::string_view name,
        ino_t resultInodeID) noexcept;
    void InsertNegative(ino_t parentInodeID, std::string_view name) noexcept;

    void RemoveEntry(ino_t parentInodeID, std::string_view name) noexcept;
    static void RemoveDirectory(fs_id volumeID, ino_t parentInodeID) noexcept;
    static void RemoveVolume(fs_id volumeID) noexcept;

private:
    struct Entry;

    // Per-volume hash-table storage and allocator metadata are outside this budget.
    static constexpr size_t ENTRY_ALLOCATION_BUDGET = 32 * 1024;
    static constexpr size_t HASH_BUCKET_COUNT = 64;
    static_assert((HASH_BUCKET_COUNT & (HASH_BUCKET_COUNT - 1)) == 0);
    static_assert(ENTRY_ALLOCATION_BUDGET <= UINT16_MAX);

    static bool IsCacheableName(std::string_view name) noexcept;
    static uint32_t CalculateHash(ino_t parentInodeID, std::string_view name) noexcept;
    static size_t GetBucketIndex(uint32_t hash) noexcept;
    Entry* FindEntry_pl(
        ino_t parentInodeID,
        std::string_view name,
        uint32_t hash) noexcept;
    static void UpdateEntry_pl(Entry* entry, ino_t resultInodeID) noexcept;
    static void TouchEntry_pl(Entry* entry) noexcept;
    void InsertEntry_pl(Entry* entry) noexcept;
    void RemoveEntry_pl(Entry* entry) noexcept;
    void RemoveHashEntry_pl(Entry** hashEntry) noexcept;
    void RemoveDirectory_pl(ino_t parentInodeID) noexcept;
    void RemoveAllEntries_pl() noexcept;
    void Insert(
        ino_t parentInodeID,
        std::string_view name,
        ino_t resultInodeID) noexcept;

    static std::map<fs_id, KDirectoryCache*> s_VolumeMap;
    static KMutex s_Mutex;
    static Entry* s_LRUFirst;
    static Entry* s_LRULast;
    static size_t s_MemoryUsage;

    fs_id m_VolumeID = -1;
    Entry* m_HashBuckets[HASH_BUCKET_COUNT] = {};

    KDirectoryCache(const KDirectoryCache&) = delete;
    KDirectoryCache& operator=(const KDirectoryCache&) = delete;
};

} // namespace kernel
