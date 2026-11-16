// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <DataTranslation/DataTranslator.h>
#include <GUI/Bitmap.h>

class PPath;
class PStreamableIO;

// Decode through the registered image translators without creating a view.
// Returns nullptr on file/decoder errors and sets errno. Allocation may throw.
// Hold the application's mutex when loading into a running application.
class PBitmapLoader
{
public:
    static Ptr<PBitmap> Load(const PPath& path, PApplication* application = nullptr);
    static Ptr<PBitmap> Load(PStreamableIO& stream, PApplication* application = nullptr);

private:
    bool AddData(const void* data, size_t length, bool isFinal);
    bool BeginBitmap(const void* data, size_t length);
    bool BeginFrame(const void* data, size_t length);
    bool CopyPixels(const void* data, size_t length);

    PBitmapFrameHeader m_Frame = {};
    Ptr<PBitmap> m_Bitmap;
    PApplication* m_Application = nullptr;
    size_t m_FrameOffset = 0;
    bool m_Failed = false;
    bool m_Complete = false;
};
