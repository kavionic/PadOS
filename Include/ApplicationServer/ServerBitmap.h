// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/GUIDefines.h>
#include <DeviceControl/Display.h>
#include <Ptr/PtrTarget.h>


class PDisplayDriver;

class PSrvBitmap : public PtrTarget
{
public:
    PSrvBitmap(const PIPoint& size, PEColorSpace colorSpace, uint8_t* raster = nullptr, size_t bytesPerLine = 0, bool readOnly = false);

    uint64_t        m_RenderSequence = 0;
    size_t          m_ResourceIndex = 0;
    PDriverResourceID m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    PEColorSpace     m_ColorSpace    = PEColorSpace::NO_COLOR_SPACE;
    PIPoint          m_Size;
    size_t          m_BytesPerLine  = 0;
    uint8_t*        m_Raster        = nullptr;  // Frame buffer address.
    PDisplayDriver*  m_Driver        = nullptr;
    bool            m_FreeRaster    = false;    // true if the raster memory is allocated by the constructor
    bool            m_VideoMem      = false;
    bool            m_ReadOnly      = false;

    void PrepareForModification();
protected:
    ~PSrvBitmap();
};
