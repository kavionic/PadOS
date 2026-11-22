// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Bitmap.h>
#include <GUI/BitmapDecoder.h>

class PPath;
class PStreamableIO;

// Decode through the registered image translators without creating a view.
// Returns nullptr on file/decoder errors and sets errno. Allocation may throw.
// Hold the application's mutex when loading into a running application.
class PBitmapLoader : public PBitmapDecoder
{
public:
    static Ptr<PBitmap> Load(const PPath& path, PApplication* application = nullptr);
    static Ptr<PBitmap> Load(PStreamableIO& stream, PApplication* application = nullptr);

private:
    virtual uint8_t* CreateRaster(const PBitmapHeader& header, size_t& bytesPerRow) override;

    Ptr<PBitmap> m_Bitmap;
    PApplication* m_Application = nullptr;
    PBitmapRasterRW m_Raster;
};
