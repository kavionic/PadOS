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
struct PArgumentPacker<PGUIStyleVariantInfo>
{
    using Fields = PArgumentSerializer<void, const PString&, const PString&, const PString&>;

    static size_t GetSize(const PGUIStyleVariantInfo& variant) noexcept
    {
        return Fields::GetSize(variant.Name, variant.DisplayName, variant.PreviewPath);
    }

    static ssize_t Write(const PGUIStyleVariantInfo& variant, void* buffer, size_t length)
    {
        return Fields::WriteArg(buffer, length, variant.Name, variant.DisplayName, variant.PreviewPath);
    }

    static ssize_t Read(const void* buffer, size_t length, PGUIStyleVariantInfo* variant)
    {
        const uint8_t* source = static_cast<const uint8_t*>(buffer);
        size_t offset = 0;
        for (PString* field : {&variant->Name, &variant->DisplayName, &variant->PreviewPath})
        {
            const ssize_t fieldSize = PArgumentPacker<PString>::Read(source + offset, length - offset, field);
            if (fieldSize < 0 || align_argument_size(size_t(fieldSize)) > length - offset) {
                return -1;
            }
            offset += align_argument_size(size_t(fieldSize));
        }
        return ssize_t(offset);
    }
};

template<>
struct PArgumentPacker<PGUIStyleInfo>
{
    using Fields = PArgumentSerializer<void, const PString&, const std::vector<PGUIStyleVariantInfo>&>;

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
        const ssize_t variantsSize = PArgumentPacker<std::vector<PGUIStyleVariantInfo>>::Read(source + offset, length - offset, &style->Variants);
        if (variantsSize < 0 || align_argument_size(size_t(variantsSize)) > length - offset) {
            return -1;
        }
        return ssize_t(offset + align_argument_size(size_t(variantsSize)));
    }
};
