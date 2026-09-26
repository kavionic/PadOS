// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace kernel
{

// CDC packet/block bookkeeping. The channel owns the aligned payload allocation and serializes all access.
// A queue is used in one direction only: a reserved TX head or a reserved RX tail never shares a live block.
class USBCDCQueue
{
public:
    USBCDCQueue(uint8_t* storage, size_t blockSize, size_t blockCount)
        : m_Storage(storage), m_BlockSize(blockSize), m_Blocks(blockCount)
    {
        assert(storage != nullptr && blockSize != 0 && blockCount != 0);
    }

    size_t GetLength() const { return m_Length; }
    size_t GetBlockSize() const { return m_BlockSize; }
    size_t GetWriteSpace() const
    {
        size_t available = (m_Blocks.size() - m_Count) * m_BlockSize;
        if (m_Count != 0 && !(m_Reserved && m_Count == 1)) {
            available += m_BlockSize - m_Blocks[(m_Head + m_Count - 1) % m_Blocks.size()].Length;
        }
        return available;
    }

    size_t Write(const void* buffer, size_t length)
    {
        const auto* source = static_cast<const uint8_t*>(buffer);
        size_t written = 0;
        while (written < length && GetWriteSpace() != 0)
        {
            size_t tail = (m_Head + m_Count - 1) % m_Blocks.size();
            if (m_Count == 0 || (m_Reserved && m_Count == 1) || m_Blocks[tail].Length == m_BlockSize)
            {
                tail = (m_Head + m_Count) % m_Blocks.size();
                m_Blocks[tail] = {};
                ++m_Count;
            }
            Block& block = m_Blocks[tail];
            const size_t count = std::min(length - written, m_BlockSize - block.Length);
            std::memcpy(m_Storage + tail * m_BlockSize + block.Length, source + written, count);
            block.Length += count;
            written += count;
            m_Length += count;
        }
        return written;
    }

    size_t Read(void* buffer, size_t length)
    {
        auto* destination = static_cast<uint8_t*>(buffer);
        size_t copied = 0;
        while (copied < length && m_Count != 0)
        {
            Block& block = m_Blocks[m_Head];
            const size_t count = std::min(length - copied, block.Length - block.Offset);
            std::memcpy(destination + copied, m_Storage + m_Head * m_BlockSize + block.Offset, count);
            block.Offset += count;
            copied += count;
            m_Length -= count;
            if (block.Offset == block.Length)
            {
                m_Head = (m_Head + 1) % m_Blocks.size();
                --m_Count;
            }
        }
        return copied;
    }

    uint8_t* BeginTransmit(size_t& length)
    {
        if (m_Reserved || m_Count == 0) {
            return nullptr;
        }
        m_Reserved = true;
        length = m_Blocks[m_Head].Length;
        return m_Storage + m_Head * m_BlockSize;
    }

    void CancelTransmit()
    {
        assert(m_Reserved);
        m_Reserved = false;
    }

    void CompleteTransmit()
    {
        assert(m_Reserved && m_Count != 0);
        m_Length -= m_Blocks[m_Head].Length;
        m_Head = (m_Head + 1) % m_Blocks.size();
        --m_Count;
        m_Reserved = false;
    }

    uint8_t* BeginReceive()
    {
        if (m_Reserved || m_Count == m_Blocks.size()) {
            return nullptr;
        }
        m_Reserved = true;
        return m_Storage + ((m_Head + m_Count) % m_Blocks.size()) * m_BlockSize;
    }

    void CompleteReceive(size_t length)
    {
        assert(m_Reserved && length <= m_BlockSize);
        if (length != 0)
        {
            m_Blocks[(m_Head + m_Count) % m_Blocks.size()] = {length, 0};
            ++m_Count;
            m_Length += length;
        }
        m_Reserved = false;
    }

private:
    struct Block
    {
        size_t Length = 0;
        size_t Offset = 0;
    };

    uint8_t* m_Storage;
    size_t m_BlockSize;
    std::vector<Block> m_Blocks;
    size_t m_Head = 0;
    size_t m_Count = 0;
    size_t m_Length = 0;
    bool m_Reserved = false;
};

} // namespace kernel
