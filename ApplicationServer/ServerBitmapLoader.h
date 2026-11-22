// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/BitmapDecoder.h>
#include <Ptr/Ptr.h>
#include <System/ExceptionHandling.h>

#include "CachedSrvBitmap.h"

class ServerBitmapLoader : public PBitmapDecoder
{
public:
    static Ptr<CachedSrvBitmap> Load(PStreamableIO& stream, const CachedSrvBitmap::CacheKey& key)
    {
        ServerBitmapLoader loader;
        loader.m_CacheKey = key;
        if (!loader.Decode(stream)) {
            PERROR_THROW_CODE(PErrorCode(get_last_error()));
        }
        return loader.m_Bitmap;
    }

private:
    virtual uint8_t* CreateRaster(const PBitmapHeader& header, size_t& bytesPerRow) override
    {
        m_Bitmap = ptr_new<CachedSrvBitmap>(header.Bounds.Size(), header.ColorSpace, m_CacheKey);
        bytesPerRow = m_Bitmap->m_BytesPerLine;
        return m_Bitmap->m_Raster;
    }

    CachedSrvBitmap::CacheKey m_CacheKey;
    Ptr<CachedSrvBitmap> m_Bitmap;
};
