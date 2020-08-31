// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Bitmap.h>
#include <ApplicationServer/ServerBitmap.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SrvBitmap::SrvBitmap(const os::IPoint& size, os::ColorSpace colorSpace, uint8_t* raster, size_t bytesPerLine)
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

SrvBitmap::~SrvBitmap()
{
    if (m_FreeRaster) {
        delete[] m_Raster;
    }
}
