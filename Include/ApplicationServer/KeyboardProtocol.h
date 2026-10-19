// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/KeyboardLayout.h>
#include <RPC/ArgumentSerializer.h>

template<>
struct PArgumentPacker<PKeyboardLayoutInfo>
{
    using Fields = PArgumentSerializer<void, const PString&, const PString&, const PString&>;

    static size_t GetSize(const PKeyboardLayoutInfo& layout) noexcept
    {
        return Fields::GetSize(layout.Layout, layout.Variant, layout.Description);
    }

    static ssize_t Write(const PKeyboardLayoutInfo& layout, void* buffer, size_t length)
    {
        return Fields::WriteArg(buffer, length, layout.Layout, layout.Variant, layout.Description);
    }

    static ssize_t Read(const void* buffer, size_t length, PKeyboardLayoutInfo* layout)
    {
        const uint8_t* source = static_cast<const uint8_t*>(buffer);
        size_t offset = 0;
        for (PString* field : {&layout->Layout, &layout->Variant, &layout->Description})
        {
            const ssize_t size = PArgumentPacker<PString>::Read(source + offset, length - offset, field);
            if (size < 0 || align_argument_size(static_cast<size_t>(size)) > length - offset) {
                return -1;
            }
            offset += align_argument_size(static_cast<size_t>(size));
        }
        return static_cast<ssize_t>(offset);
    }
};
