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
// Created: 16.05.2025 22:00

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <Storage/MemFile.h>
#include <Utils/String.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMemFile::PMemFile(std::vector<uint8_t>&& data) : m_Buffer(std::move(data))
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t PMemFile::Read(void* buffer, ssize_t size)
{
    ssize_t curLength = ReadPos(m_Position, buffer, size);
    if (curLength > 0) {
        m_Position += curLength;
    }
    return curLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t PMemFile::Write(const void* buffer, ssize_t size)
{
    ssize_t curLength = WritePos(m_Position, buffer, size);
    if (curLength > 0) {
        m_Position += curLength;
    }
    return curLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t PMemFile::ReadPos(off64_t position, void* buffer, ssize_t size) const
{
    const size_t bufferPos = size_t(position);

    if (size < 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (bufferPos >= m_Buffer.size()) {
        return 0;
    }
    size_t curLength = std::min(size_t(size), m_Buffer.size() - bufferPos);
    memcpy(buffer, &m_Buffer[bufferPos], curLength);
    return curLength;

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t PMemFile::WritePos(off64_t position, const void* buffer, ssize_t size)
{
    const size_t bufferPos = size_t(position);

    if (bufferPos + size > m_Buffer.size()) {
        m_Buffer.resize(bufferPos + size);
    }
    memcpy(&m_Buffer[bufferPos], buffer, size);

    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

off64_t PMemFile::Seek(off64_t position, int mode)
{
    const ssize_t bufferPos = ssize_t(position);

    switch (mode)
    {
        case SEEK_SET:
            if (bufferPos < 0)
            {
                errno = EINVAL;
                return -1;
            }
            m_Position = bufferPos;
            return m_Position;
        case SEEK_CUR:
            if (m_Position + bufferPos < 0)
            {
                errno = EINVAL;
                return -1;
            }
            m_Position += bufferPos;
            return m_Position;
        case SEEK_END:
        {
            if (ssize_t(m_Buffer.size()) + bufferPos < 0)
            {
                errno = EINVAL;
                return -1;
            }
            m_Position = m_Buffer.size() + bufferPos;
            return m_Position;
        }
        default:
            errno = EINVAL;
            return -1;
    }
}
