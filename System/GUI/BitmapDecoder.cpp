// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include <GUI/Bitmap.h>
#include <GUI/BitmapDecoder.h>
#include <Storage/StreamableIO.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapDecoder::Decode(PStreamableIO& stream)
{
    constexpr size_t BUFFER_SIZE = 4096;
    constexpr size_t MAX_IDENTIFICATION_SIZE = 32768;
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    size_t bufferedSize = 0;
    Ptr<PDataTranslator> translator;

    for (;;)
    {
        const ssize_t bytesRead = stream.Read(buffer.data() + bufferedSize, buffer.size() - bufferedSize);
        if (bytesRead < 0) {
            return false;
        }
        bufferedSize += size_t(bytesRead);
        if (translator == nullptr)
        {
            PDataTranslatorStatus status;
            translator = PTranslatorFactory::Get().FindTranslator(PString::zero, PDataTranslatorType::Image, buffer.data(), bufferedSize, status);
            if (translator == nullptr)
            {
                if (status != PDataTranslatorStatus::NotEnoughData || bytesRead == 0 || bufferedSize == MAX_IDENTIFICATION_SIZE)
                {
                    set_last_error((status == PDataTranslatorStatus::NoMemory) ? ENOMEM : EINVAL);
                    return false;
                }
                if (bufferedSize == buffer.size()) {
                    buffer.resize(std::min(buffer.size() + BUFFER_SIZE, MAX_IDENTIFICATION_SIZE));
                }
                continue;
            }
            translator->VFDataReady.Connect([this](const void* data, size_t length, bool isFinal) { return AddData(data, length, isFinal); });
        }
        if (translator->AddData(buffer.data(), bufferedSize, bytesRead == 0) < 0 || m_Failed)
        {
            set_last_error(EINVAL);
            return false;
        }
        if (m_Complete) {
            return true;
        }
        if (bytesRead == 0)
        {
            set_last_error(EINVAL);
            return false;
        }
        bufferedSize = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapDecoder::AddData(const void* data, size_t length, bool isFinal)
{
    if (m_Failed || m_Complete) {
        return false;
    }
    bool accepted;
    if (isFinal)
    {
        accepted = length == 0 && m_Raster != nullptr && m_Frame.DataSize != 0 && m_FrameOffset == m_Frame.DataSize;
        m_Complete = accepted;
    }
    else if (m_Raster == nullptr)
    {
        accepted = BeginBitmap(data, length);
    }
    else if (m_FrameOffset == m_Frame.DataSize)
    {
        accepted = BeginFrame(data, length);
    }
    else
    {
        accepted = CopyPixels(data, length);
    }
    m_Failed = !accepted;
    return accepted;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapDecoder::BeginBitmap(const void* data, size_t length)
{
    PBitmapHeader header;
    if (data == nullptr || length < sizeof(header)) {
        return false;
    }
    memcpy(&header, data, sizeof(header));
    if (header.HeaderSize != length || header.Bounds.left != 0 || header.Bounds.top != 0 || header.Bounds.Width() <= 0 || header.Bounds.Height() <= 0) {
        return false;
    }
    const size_t bitsPerPixel = BitsPerPixel(header.ColorSpace);
    if (bitsPerPixel < 8 || bitsPerPixel % 8 != 0) {
        return false;
    }
    const size_t bytesPerPixel = bitsPerPixel / 8;
    const size_t width = size_t(header.Bounds.Width());
    const size_t height = size_t(header.Bounds.Height());
    if (width > std::numeric_limits<size_t>::max() / bytesPerPixel) {
        return false;
    }
    m_Raster = CreateRaster(header, m_BytesPerRow);
    if (m_Raster == nullptr || m_BytesPerRow < width * bytesPerPixel || m_BytesPerRow > std::numeric_limits<size_t>::max() / height) {
        return false;
    }
    m_Header = header;
    memset(m_Raster, 0, m_BytesPerRow * height);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapDecoder::BeginFrame(const void* data, size_t length)
{
    if (data == nullptr || length < sizeof(m_Frame)) {
        return false;
    }
    memcpy(&m_Frame, data, sizeof(m_Frame));
    if (m_Frame.HeaderSize != length || m_Frame.ColorSpace != m_Header.ColorSpace || m_Frame.Frame.Width() <= 0 || m_Frame.Frame.Height() <= 0) {
        return false;
    }
    if ((m_Frame.Frame & m_Header.Bounds) != m_Frame.Frame) {
        return false;
    }
    const size_t bytesPerPixel = BitsPerPixel(m_Frame.ColorSpace) / 8;
    const size_t rowBytes = size_t(m_Frame.Frame.Width()) * bytesPerPixel;
    const size_t height = size_t(m_Frame.Frame.Height());
    if (m_Frame.BytesPerRow < rowBytes || m_Frame.BytesPerRow > std::numeric_limits<size_t>::max() / height) {
        return false;
    }
    if (m_Frame.DataSize != m_Frame.BytesPerRow * height) {
        return false;
    }
    m_FrameOffset = 0;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PBitmapDecoder::CopyPixels(const void* data, size_t length)
{
    if (data == nullptr || length > m_Frame.DataSize - m_FrameOffset) {
        return false;
    }
    const size_t bytesPerPixel = BitsPerPixel(m_Frame.ColorSpace) / 8;
    const size_t rowBytes = size_t(m_Frame.Frame.Width()) * bytesPerPixel;
    const uint8_t* source = static_cast<const uint8_t*>(data);

    while (length != 0)
    {
        const size_t row = m_FrameOffset / m_Frame.BytesPerRow;
        const size_t columnBytes = m_FrameOffset % m_Frame.BytesPerRow;
        const size_t count = std::min(length, m_Frame.BytesPerRow - columnBytes);
        if (columnBytes < rowBytes)
        {
            const size_t destinationOffset = (size_t(m_Frame.Frame.top) + row) * m_BytesPerRow;
            uint8_t* destination = m_Raster + destinationOffset + size_t(m_Frame.Frame.left) * bytesPerPixel + columnBytes;
            memcpy(destination, source, std::min(count, rowBytes - columnBytes));
        }
        source += count;
        length -= count;
        m_FrameOffset += count;
    }
    return true;
}
