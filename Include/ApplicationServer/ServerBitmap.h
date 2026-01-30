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

#pragma once

#include <GUI/GUIDefines.h>
#include <Ptr/PtrTarget.h>


class PDisplayDriver;

class PSrvBitmap : public PtrTarget
{
public:
    PSrvBitmap(const PIPoint& size, PEColorSpace colorSpace, uint8_t* raster = nullptr, size_t bytesPerLine = 0);

    PEColorSpace     m_ColorSpace    = PEColorSpace::NO_COLOR_SPACE;
    PIPoint          m_Size;
    size_t          m_BytesPerLine  = 0;
    uint8_t*        m_Raster        = nullptr;  // Frame buffer address.
    PDisplayDriver*  m_Driver        = nullptr;
    bool            m_FreeRaster    = false;    // true if the raster memory is allocated by the constructor
    bool            m_VideoMem      = false;
protected:
    ~PSrvBitmap();
};
