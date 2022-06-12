// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 26.05.2022 22:30

#pragma once

#include <stdint.h>
#include <strings.h>
#include <Threads/Threads.h>
#include <Kernel/Scheduler.h>

template<typename T, int QUEUE_SIZE, typename Y = T> class CircularBuffer
{
public:
    static const uint32_t QUEUE_SIZE_MASK = QUEUE_SIZE - 1;
    static_assert((QUEUE_SIZE & QUEUE_SIZE_MASK) == 0, "QUEUE_SIZE must be a power of 2.");

    CircularBuffer()
    {
        m_QueueInPos = 0;
        m_QueueOutPos = 0;
    }
    
    void Clear()
    {
        m_QueueInPos = 0;
        m_QueueOutPos = 0;
    }
    size_t GetLength() const
    {
        return (m_QueueInPos - m_QueueOutPos) & QUEUE_SIZE_MASK;
    }
    size_t GetRemainingSpace() const
    {
        return QUEUE_SIZE - GetLength();
    }

    ssize_t Write(const Y* data, size_t length)
    {
        const T* curSrc = reinterpret_cast<const T*>(data);

        if (length > QUEUE_SIZE)
        {
            curSrc += length - QUEUE_SIZE;
            length = QUEUE_SIZE;
        }

        const size_t curLength = GetLength();
        size_t freeSpace = QUEUE_SIZE - curLength;


        if (length > freeSpace)
        {
            m_QueueOutPos = (m_QueueOutPos + length - freeSpace) & QUEUE_SIZE_MASK;
            freeSpace = length;
        }

        m_QueueInPos &= QUEUE_SIZE_MASK;
        const size_t postSpace = QUEUE_SIZE - m_QueueInPos;
        if (postSpace >= length)
        {
            WriteElements(&m_Queue[m_QueueInPos], curSrc, length);
            m_QueueInPos += length;
        }
        else
        {
            const size_t preSpace = length - postSpace;
            WriteElements(&m_Queue[m_QueueInPos], curSrc, postSpace);
            WriteElements(&m_Queue[0], curSrc + postSpace, preSpace);
            m_QueueInPos = preSpace;
        }
        return length;
    }

    ssize_t Read(Y* data, size_t length)
    {
        const size_t curLength = GetLength();

        if (length > curLength)
        {
            length = curLength;
        }
        T* curDst = reinterpret_cast<T*>(data);
        const size_t postSpace = QUEUE_SIZE - m_QueueOutPos;
        if (postSpace >= length)
        {
            WriteElements(curDst, &m_Queue[m_QueueOutPos], length);
            m_QueueOutPos = (m_QueueOutPos + length) & QUEUE_SIZE_MASK;
        }
        else
        {
            const size_t preSpace = length - postSpace;
            WriteElements(curDst, &m_Queue[m_QueueOutPos], postSpace);
            WriteElements(curDst + postSpace, &m_Queue[0], preSpace);
            m_QueueOutPos = preSpace & QUEUE_SIZE_MASK;
        }
        return length;
    }

    const T*    GetReadPointer() const { return &m_Queue[m_QueueOutPos]; }
    T*          GetWritePointer() { return &m_Queue[m_QueueInPos & QUEUE_SIZE_MASK]; }

private:
    void WriteElements(T* dst, const T* src, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            dst[i] = src[i];
        }
    }

    T m_Queue[QUEUE_SIZE];
    size_t m_QueueInPos;
    size_t m_QueueOutPos;
        
};
