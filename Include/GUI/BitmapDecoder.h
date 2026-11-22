// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <DataTranslation/DataTranslator.h>

class PStreamableIO;

// Decode one image stream through the registered translators.
// The derived loader owns the destination raster for the entire decode.
class PBitmapDecoder
{
public:
    virtual ~PBitmapDecoder() = default;

protected:
    // Returns false and sets errno on stream/decoder errors. Allocation may throw.
    bool Decode(PStreamableIO& stream);
    virtual uint8_t* CreateRaster(const PBitmapHeader& header, size_t& bytesPerRow) = 0;

private:
    bool AddData(const void* data, size_t length, bool isFinal);
    bool BeginBitmap(const void* data, size_t length);
    bool BeginFrame(const void* data, size_t length);
    bool CopyPixels(const void* data, size_t length);

    PBitmapHeader m_Header = {};
    PBitmapFrameHeader m_Frame = {};
    uint8_t* m_Raster = nullptr;
    size_t m_BytesPerRow = 0;
    size_t m_FrameOffset = 0;
    bool m_Failed = false;
    bool m_Complete = false;
};
