// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Bitmap.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSrvBitmap::PSrvBitmap(const PIPoint& size, PEColorSpace colorSpace, uint8_t* raster, size_t bytesPerLine, bool readOnly)
    : m_ColorSpace(colorSpace)
    , m_Size(size)
    , m_BytesPerLine(bytesPerLine)
    , m_ReadOnly(readOnly)
{
    if (m_Size.x < 0 || m_Size.y < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (m_BytesPerLine == 0)
    {
        const size_t width = m_Size.x;
        const size_t bitsPerPixel = BitsPerPixel(colorSpace);
        size_t minimumStride;
        if (bitsPerPixel == 1) {
            minimumStride = (width + 7) / 8;
        } else if (__builtin_mul_overflow(width, bitsPerPixel / 8, &minimumStride)) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        if (__builtin_add_overflow(minimumStride, 3, &m_BytesPerLine)) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        m_BytesPerLine &= ~0x03;
    }
    size_t rasterSize;
    if (__builtin_mul_overflow(m_BytesPerLine, m_Size.y, &rasterSize)) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    if (m_Size.x == 0 || raster != nullptr)
    {
        m_FreeRaster = false;
        m_Raster     = raster;
    }
    else
    {
        m_FreeRaster = true;
        m_Raster     = new uint8_t[rasterSize];
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSrvBitmap::PrepareForModification()
{
    if (m_Driver != nullptr) {
        m_Driver->FlushBitmap(this);
    }
}

PSrvBitmap::~PSrvBitmap()
{
    if (m_Driver != nullptr) {
        m_Driver->UnregisterBitmap(this);
    }
    if (m_FreeRaster) {
        delete[] m_Raster;
    }
}
