// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/FontDescription.h>
#include <RPC/ArgumentSerializer.h>
#include <sys/pados_error_codes.h>

template<>
struct PArgumentPacker<PFontDescription>
{
    using Fields = PArgumentSerializer<void, const PString&, const PString&, float, float, float, PFontRenderMode, bool>;

    static size_t GetSize(const PFontDescription& description) noexcept
    {
        return Fields::GetSize(
            description.Family,
            description.Style,
            description.PointSize,
            description.Rotation,
            description.Shear,
            description.RenderMode,
            description.Hinting);
    }

    static ssize_t Write(const PFontDescription& description, void* buffer, size_t length)
    {
        return Fields::WriteArg(
            buffer,
            length,
            description.Family,
            description.Style,
            description.PointSize,
            description.Rotation,
            description.Shear,
            description.RenderMode,
            description.Hinting);
    }

    static ssize_t Read(const void* buffer, size_t length, PFontDescription* description)
    {
        const uint8_t* source = static_cast<const uint8_t*>(buffer);
        size_t offset = 0;
        const auto readField = [source, length, &offset]<typename T>(T& field)
        {
            const ssize_t size = PArgumentPacker<T>::Read(source + offset, length - offset, &field);
            if (size < 0 || align_argument_size(size_t(size)) > length - offset) {
                return false;
            }
            offset += align_argument_size(size_t(size));
            return true;
        };
        if (!readField(description->Family) || !readField(description->Style) || !readField(description->PointSize)
            || !readField(description->Rotation) || !readField(description->Shear)
            || !readField(description->RenderMode) || !readField(description->Hinting)) {
            return -1;
        }
        return ssize_t(offset);
    }
};

inline constexpr size_t FONT_MEASUREMENT_CHUNK_BYTES = 4096;

struct MsgCreateFontReply
{
    PErrorCode Error = PErrorCode::Success;
    uint32_t FontHandle = 0;
    PFontHeight Height{};
};

struct MsgMeasureFontReply
{
    PErrorCode Error = PErrorCode::Success;
    PFontTextMetrics Metrics;
    uint32_t PreviousGlyph = 0;
};
