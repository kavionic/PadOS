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
// Created: 19.10.2025 22:30

#pragma once

#include <string>
#include <sys/types.h>
#include <Utils/String.h>
#include <Utils/Logging.h>

constexpr size_t align_argument_size(size_t length) noexcept { return (length + 3) & ~3; }

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct ArgumentPacker
{
    static constexpr size_t GetSize(T value) noexcept { return sizeof(T); }

    static ssize_t  Write(T value, void* data, size_t length)
    {
        if (length >= sizeof(value))
        {
            *reinterpret_cast<T*>(data) = value;
            return sizeof(value);
        }
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "{}: not enough data {}/{}.", __PRETTY_FUNCTION__, length, sizeof(T));
        return -1;
    }
    static ssize_t Read(const void* data, size_t length, T* value)
    {
        if (length >= sizeof(T))
        {
            *value = *reinterpret_cast<const T*>(data);
            return sizeof(T);
        }
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "{}: not enough data {}/{}.", __PRETTY_FUNCTION__, length, sizeof(T));
        return -1;
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<>
struct ArgumentPacker<std::string>
{
    static size_t   GetSize(const std::string& value) noexcept { return sizeof(uint32_t) + value.size(); }
    static ssize_t  Write(const std::string& value, void* data, size_t length) noexcept
    {
        if (length >= (sizeof(uint32_t) + value.size()))
        {
            *reinterpret_cast<uint32_t*>(data) = value.size();
            data = reinterpret_cast<uint32_t*>(data) + 1;
            value.copy(reinterpret_cast<char*>(data), value.size());
            return sizeof(uint32_t) + value.size();
        }
        return -1;
    }
    static ssize_t Read(const void* data, size_t length, std::string* value)
    {
        if (length < sizeof(uint32_t))
        {
            p_system_log<PLogSeverity::ERROR>(LogCat_General, "{}: not enough data {}.", __PRETTY_FUNCTION__, length);
            return -1;
        }
        const uint32_t strLength = *reinterpret_cast<const uint32_t*>(data);

        if (length < (sizeof(uint32_t) + strLength))
        {
            p_system_log<PLogSeverity::ERROR>(LogCat_General, "{}: not enough data {} / {}.", __PRETTY_FUNCTION__, length, strLength);
            return -1;
        }

        data = reinterpret_cast<const uint32_t*>(data) + 1;
        value->assign(reinterpret_cast<const char*>(data), strLength);

        return sizeof(uint32_t) + strLength;
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<>
struct ArgumentPacker<PString> : public ArgumentPacker<std::string>
{
};
