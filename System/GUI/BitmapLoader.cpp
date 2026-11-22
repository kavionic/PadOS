// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/BitmapLoader.h>
#include <Storage/File.h>
#include <Storage/Path.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PBitmap> PBitmapLoader::Load(const PPath& path, PApplication* application)
{
    PFile file(path.GetPath());
    return file.IsValid() ? Load(file, application) : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PBitmap> PBitmapLoader::Load(PStreamableIO& stream, PApplication* application)
{
    PBitmapLoader loader;
    loader.m_Application = application;
    return loader.Decode(stream) ? loader.m_Bitmap : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t* PBitmapLoader::CreateRaster(const PBitmapHeader& header, size_t& bytesPerRow)
{
    m_Bitmap = ptr_new<PBitmap>(header.Bounds.Width(), header.Bounds.Height(), header.ColorSpace, PBitmap::SHARE_FRAMEBUFFER, m_Application);
    if (!m_Bitmap->IsValid()) {
        return nullptr;
    }
    bytesPerRow = size_t(m_Bitmap->GetBytesPerRow());
    m_Raster = m_Bitmap->LockRaster();
    return m_Raster.Get();
}
