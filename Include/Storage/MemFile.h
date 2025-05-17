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

#pragma once

#include <vector>
#include <Storage/SeekableIO.h>

namespace os
{

class MemFile : public SeekableIO
{
public:
    MemFile() = default;
    MemFile(std::vector<uint8_t>&& data);


    virtual ssize_t Read(void* buffer, ssize_t size) override;
    virtual ssize_t Write(const void* buffer, ssize_t size) override;

    virtual ssize_t ReadPos(off64_t position, void* buffer, ssize_t size) const override;
    virtual ssize_t WritePos(off64_t position, const void* buffer, ssize_t size) override;

    virtual off64_t Seek(off64_t position, int mode) override;
private:
    std::vector<uint8_t> m_Buffer;
    ssize_t              m_Position = 0;
};


} // namespace os
