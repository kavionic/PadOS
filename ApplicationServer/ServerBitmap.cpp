// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Bitmap.h>
#include <limits>
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
        const uint64_t minimumStride = (uint64_t(m_Size.x) * BitsPerPixel(colorSpace) + 7) / 8;
        if (minimumStride > std::numeric_limits<size_t>::max() - 3) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        m_BytesPerLine = (size_t(minimumStride) + 3) & ~size_t(0x03);
    }
    if (m_Size.y != 0 && m_BytesPerLine > std::numeric_limits<size_t>::max() / size_t(m_Size.y)) {
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
        m_Raster     = new uint8_t[m_Size.y * m_BytesPerLine];
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
