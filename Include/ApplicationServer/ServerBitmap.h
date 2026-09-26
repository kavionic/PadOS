// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
