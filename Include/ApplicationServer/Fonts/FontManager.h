// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>
#include <set>
#include <tuple>
#include <sys/types.h>

#include <ApplicationServer/Fonts/ServerFontFace.h>
#include <ApplicationServer/Fonts/ServerFontInstance.h>
#include <ApplicationServer/Fonts/ServerFontLibrary.h>
#include <Ptr/WeakPtr.h>

class PFontGlyphCache;

class PFontManager : public PtrTarget
{
public:
    PFontManager();
    ~PFontManager();

    // Replaces the catalog atomically. Existing font instances remain valid.
    void ScanFonts(const PString& directory = FONT_DIRECTORY_PATH);
    const std::vector<PFontFaceInfo>& GetFaces() const { return m_FaceInfo; }
    Ptr<PServerFontInstance> GetFont(const PFontDescription& description);

private:
    struct FaceRecord
    {
        PFontFaceInfo Info;
        PString Path;
        FT_Long FaceIndex = 0;
        WeakPtr<PServerFontFace> Face;
    };

    using DirectorySet = std::set<std::pair<dev_t, ino_t>>;
    using InstanceKey = std::tuple<size_t, float, float, float, PFontRenderMode, bool>;

    void ScanDirectory(const PString& path, DirectorySet& directories, std::vector<FaceRecord>& faces);
    void ScanFile(const PString& path, std::vector<FaceRecord>& faces);
    static PFontDescription NormalizeDescription(const PFontDescription& description);

    Ptr<PServerFontLibrary> m_Library;
    Ptr<PFontGlyphCache> m_Cache;
    std::vector<FaceRecord> m_Faces;
    std::vector<PFontFaceInfo> m_FaceInfo;
    std::map<InstanceKey, WeakPtr<PServerFontInstance>> m_Instances;
};
