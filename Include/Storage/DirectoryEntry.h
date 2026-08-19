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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <dirent.h>
#include <limits>
#include <vector>


inline constexpr size_t P_DIR_ENTRY_BUFFER_SIZE = 8 * 1024;
inline constexpr size_t P_DIR_ENTRY_HEADER_SIZE = offsetof(dirent_t, d_name);
inline constexpr size_t P_DIR_ENTRY_ALIGNMENT = alignof(dirent_t);
static_assert((P_DIR_ENTRY_ALIGNMENT & (P_DIR_ENTRY_ALIGNMENT - 1)) == 0);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline constexpr size_t PGetDirEntryRecordSize(size_t nameLength) noexcept
{
    constexpr size_t maxRecordSize = std::numeric_limits<uint16_t>::max();
    if (nameLength > maxRecordSize - P_DIR_ENTRY_HEADER_SIZE - 1) {
        return 0;
    }

    const size_t unalignedSize = P_DIR_ENTRY_HEADER_SIZE + nameLength + 1;
    const size_t recordSize = (unalignedSize + P_DIR_ENTRY_ALIGNMENT - 1) & ~(P_DIR_ENTRY_ALIGNMENT - 1);
    return (recordSize <= maxRecordSize) ? recordSize : 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PDirEntryBuffer
{
public:
    void* GetBuffer()
    {
        if (m_Storage.empty()) {
            m_Storage.resize((P_DIR_ENTRY_BUFFER_SIZE + sizeof(dirent_t) - 1) / sizeof(dirent_t));
        }
        return m_Storage.data();
    }

    constexpr size_t GetSize() const noexcept { return P_DIR_ENTRY_BUFFER_SIZE; }

private:
    std::vector<dirent_t> m_Storage;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PDirEntryIterator
{
public:
    constexpr PDirEntryIterator(const void* buffer, size_t bufferSize) noexcept
        : m_Current(static_cast<const uint8_t*>(buffer))
        , m_RemainingSize(bufferSize)
    {
    }

    explicit operator bool() const noexcept
    {
        if (m_Current == nullptr || m_RemainingSize < P_DIR_ENTRY_HEADER_SIZE) {
            return false;
        }
        if ((reinterpret_cast<uintptr_t>(m_Current) & (P_DIR_ENTRY_ALIGNMENT - 1)) != 0) {
            return false;
        }

        const dirent_t* entry = reinterpret_cast<const dirent_t*>(m_Current);
        const size_t recordSize = entry->d_reclen;
        return recordSize >= P_DIR_ENTRY_HEADER_SIZE + 1
            && recordSize <= m_RemainingSize
            && size_t(entry->d_namlen) + 1 <= recordSize - P_DIR_ENTRY_HEADER_SIZE;
    }

    const dirent_t& operator*() const noexcept
    {
        return *reinterpret_cast<const dirent_t*>(m_Current);
    }

    const dirent_t* operator->() const noexcept
    {
        return reinterpret_cast<const dirent_t*>(m_Current);
    }

    PDirEntryIterator& operator++() noexcept
    {
        if (m_Current != nullptr
            && m_RemainingSize >= P_DIR_ENTRY_HEADER_SIZE
            && (reinterpret_cast<uintptr_t>(m_Current) & (P_DIR_ENTRY_ALIGNMENT - 1)) == 0)
        {
            const size_t recordSize = reinterpret_cast<const dirent_t*>(m_Current)->d_reclen;
            if (recordSize >= P_DIR_ENTRY_HEADER_SIZE + 1 && recordSize <= m_RemainingSize)
            {
                m_Current += recordSize;
                m_RemainingSize -= recordSize;
                return *this;
            }
        }

        m_Current = nullptr;
        m_RemainingSize = 0;
        return *this;
    }

private:
    const uint8_t* m_Current;
    size_t m_RemainingSize;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PDirEntryWriter
{
public:
    PDirEntryWriter(void* buffer, size_t bufferSize) noexcept
        : m_Buffer(static_cast<uint8_t*>(buffer))
        , m_BufferSize(bufferSize)
    {
    }

    bool IsValid() const noexcept
    {
        return m_Buffer != nullptr
            && (reinterpret_cast<uintptr_t>(m_Buffer) & (P_DIR_ENTRY_ALIGNMENT - 1)) == 0;
    }

    dirent_t* AddEntry(const char* name, size_t nameLength) noexcept
    {
        const size_t recordSize = PGetDirEntryRecordSize(nameLength);
        if (!IsValid() || name == nullptr || recordSize == 0 || recordSize > GetRemainingSize()) {
            return nullptr;
        }

        uint8_t* recordBuffer = m_Buffer + m_BytesWritten;
        memset(recordBuffer, 0, P_DIR_ENTRY_HEADER_SIZE);

        dirent_t* entry = reinterpret_cast<dirent_t*>(recordBuffer);
        entry->d_reclen = static_cast<decltype(entry->d_reclen)>(recordSize);
        entry->d_namlen = static_cast<decltype(entry->d_namlen)>(nameLength);

        char* nameBuffer = reinterpret_cast<char*>(recordBuffer + P_DIR_ENTRY_HEADER_SIZE);
        memcpy(nameBuffer, name, nameLength);
        memset(nameBuffer + nameLength, 0, recordSize - P_DIR_ENTRY_HEADER_SIZE - nameLength);

        m_BytesWritten += recordSize;
        return entry;
    }

    size_t GetBytesWritten() const noexcept { return m_BytesWritten; }
    size_t GetRemainingSize() const noexcept { return m_BufferSize - m_BytesWritten; }

private:
    uint8_t* m_Buffer;
    size_t m_BufferSize;
    size_t m_BytesWritten = 0;
};
