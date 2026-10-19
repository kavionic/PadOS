// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 19.10.2025 22:30

#pragma once

#include <cstring>
#include <string>
#include <type_traits>
#include <vector>
#include <sys/types.h>
#include <Utils/String.h>
#include <Utils/Logging.h>

constexpr size_t align_argument_size(size_t length) noexcept { return (length + 3) & ~3; }

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct PArgumentPacker
{
    static constexpr size_t GetSize(T value) noexcept { return sizeof(T); }
    static constexpr size_t GetFixedSize() noexcept { return sizeof(T); }

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
struct PArgumentPacker<std::string>
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
struct PArgumentPacker<PString> : public PArgumentPacker<std::string>
{
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct PArgumentPacker<std::vector<T>>
{
    static constexpr bool USE_BULK_COPY = std::is_trivially_copyable_v<T> && !std::is_same_v<T, bool>;

    static size_t GetSize(const std::vector<T>& value) noexcept
    {
        if constexpr (USE_BULK_COPY)
        {
            return sizeof(uint32_t) + value.size() * sizeof(T);
        }
        else
        {
            size_t size = sizeof(uint32_t);
            for (const T& element : value) {
                size += align_argument_size(PArgumentPacker<T>::GetSize(element));
            }
            return size;
        }
    }

    static ssize_t Write(const std::vector<T>& value, void* data, size_t length) noexcept(USE_BULK_COPY)
    {
        const size_t totalSize = GetSize(value);
        if (length < totalSize) {
            return -1;
        }
        const uint32_t count = static_cast<uint32_t>(value.size());
        memcpy(data, &count, sizeof(count));
        uint8_t* destination = static_cast<uint8_t*>(data);
        if constexpr (USE_BULK_COPY)
        {
            if (!value.empty()) {
                memcpy(destination + sizeof(count), value.data(), value.size() * sizeof(T));
            }
            return static_cast<ssize_t>(totalSize);
        }
        else
        {
            size_t offset = sizeof(count);
            for (const T& element : value)
            {
                const ssize_t size = PArgumentPacker<T>::Write(element, destination + offset, length - offset);
                if (size <= 0 || align_argument_size(static_cast<size_t>(size)) > length - offset) {
                    return -1;
                }
                offset += align_argument_size(static_cast<size_t>(size));
            }
            return static_cast<ssize_t>(offset);
        }
    }

    static ssize_t Read(const void* data, size_t length, std::vector<T>* value)
    {
        if (length < sizeof(uint32_t)) {
            return -1;
        }
        uint32_t count;
        memcpy(&count, data, sizeof(count));
        const uint8_t* source = static_cast<const uint8_t*>(data);
        if constexpr (USE_BULK_COPY)
        {
            if (count > (length - sizeof(count)) / sizeof(T)) {
                return -1;
            }
            const T* elements = reinterpret_cast<const T*>(source + sizeof(count));
            value->assign(elements, elements + count);
            return static_cast<ssize_t>(sizeof(count) + count * sizeof(T));
        }
        else
        {
            size_t offset = sizeof(count);
            // Each element occupies at least one aligned argument slot.
            if (count > (length - offset) / align_argument_size(1)) {
                return -1;
            }
            value->clear();
            value->reserve(count);
            for (size_t index = 0; index < count; ++index)
            {
                T element;
                const ssize_t size = PArgumentPacker<T>::Read(source + offset, length - offset, &element);
                if (size <= 0 || align_argument_size(static_cast<size_t>(size)) > length - offset) {
                    return -1;
                }
                offset += align_argument_size(static_cast<size_t>(size));
                value->push_back(std::move(element));
            }
            return static_cast<ssize_t>(offset);
        }
    }
};
