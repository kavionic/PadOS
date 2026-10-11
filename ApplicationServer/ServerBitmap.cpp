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

PSrvBitmap::PSrvBitmap(const PIPoint& size, PEColorSpace colorSpace, uint8_t* raster, size_t bytesPerLine)
    : m_ColorSpace(colorSpace)
    , m_Size(size)
    , m_BytesPerLine(bytesPerLine)
{
    if (m_BytesPerLine == 0)
    {
        int bitsPerPixel = BitsPerPixel(colorSpace);
        m_BytesPerLine = (size.x * bitsPerPixel + 7) / 8;
        m_BytesPerLine = (m_BytesPerLine + 3) & ~3; // Keep each line 32-bit aligned.
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
    try
    {
        PrepareForModification();
    }
    catch (const std::exception& error)
    {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "Completing bitmap rendering failed: {}", error.what());
    }
    if (m_FreeRaster) {
        delete[] m_Raster;
    }
}
