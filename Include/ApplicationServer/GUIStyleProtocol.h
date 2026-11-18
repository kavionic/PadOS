// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/GUIStyleInfo.h>
#include <RPC/ArgumentSerializer.h>

template<>
struct PArgumentPacker<PGUIStyleInfo>
{
    using Fields = PArgumentSerializer<void, const PString&, const std::vector<PString>&>;

    static size_t GetSize(const PGUIStyleInfo& style) noexcept
    {
        return Fields::GetSize(style.Name, style.Variants);
    }

    static ssize_t Write(const PGUIStyleInfo& style, void* buffer, size_t length)
    {
        return Fields::WriteArg(buffer, length, style.Name, style.Variants);
    }

    static ssize_t Read(const void* buffer, size_t length, PGUIStyleInfo* style)
    {
        const uint8_t* source = static_cast<const uint8_t*>(buffer);
        const ssize_t nameSize = PArgumentPacker<PString>::Read(source, length, &style->Name);
        if (nameSize < 0 || align_argument_size(size_t(nameSize)) > length) {
            return -1;
        }
        const size_t offset = align_argument_size(size_t(nameSize));
        const ssize_t variantsSize = PArgumentPacker<std::vector<PString>>::Read(source + offset, length - offset, &style->Variants);
        if (variantsSize < 0 || align_argument_size(size_t(variantsSize)) > length - offset) {
            return -1;
        }
        return ssize_t(offset + align_argument_size(size_t(variantsSize)));
    }
};
