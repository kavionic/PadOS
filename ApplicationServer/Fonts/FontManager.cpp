// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/FontManager.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <sys/stat.h>

#include <Storage/Directory.h>
#include <Storage/Path.h>
#include <System/ExceptionHandling.h>

#include <ApplicationServer/Fonts/FontGlyphCache.h>

PFontManager::PFontManager()
    : m_Library(ptr_new<PServerFontLibrary>())
    , m_Cache(ptr_new<PFontGlyphCache>())
{
}

PFontManager::~PFontManager() = default;

void PFontManager::ScanFonts(const PString& directory)
{
    DirectorySet directories;
    std::vector<FaceRecord> faces;
    ScanDirectory(directory, directories, faces);
    std::sort(faces.begin(), faces.end(), [](const FaceRecord& lhs, const FaceRecord& rhs)
    {
        return std::tie(lhs.Info.Family, lhs.Info.Style, lhs.Path, lhs.FaceIndex)
            < std::tie(rhs.Info.Family, rhs.Info.Style, rhs.Path, rhs.FaceIndex);
    });
    faces.erase(std::unique(faces.begin(), faces.end(), [](const FaceRecord& lhs, const FaceRecord& rhs)
    {
        return lhs.Info.Family == rhs.Info.Family && lhs.Info.Style == rhs.Info.Style;
    }), faces.end());
    std::vector<PFontFaceInfo> faceInfo;
    faceInfo.reserve(faces.size());
    for (const FaceRecord& face : faces) {
        faceInfo.push_back(face.Info);
    }
    m_Faces.swap(faces);
    m_FaceInfo.swap(faceInfo);
    m_Instances.clear();
}

Ptr<PServerFontInstance> PFontManager::GetFont(const PFontDescription& description)
{
    const PFontDescription normalized = NormalizeDescription(description);
    const auto record = std::lower_bound(
        m_Faces.begin(),
        m_Faces.end(),
        normalized,
        [](const FaceRecord& face, const PFontDescription& font)
        {
            return std::tie(face.Info.Family, face.Info.Style) < std::tie(font.Family, font.Style);
        });
    if (record == m_Faces.end() || record->Info.Family != normalized.Family || record->Info.Style != normalized.Style) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    const InstanceKey key(
        size_t(record - m_Faces.begin()),
        normalized.PointSize,
        normalized.Rotation,
        normalized.Shear,
        normalized.RenderMode,
        normalized.Hinting);
    const auto cached = m_Instances.find(key);
    if (cached != m_Instances.end())
    {
        Ptr<PServerFontInstance> instance = cached->second.Lock();
        if (instance != nullptr) {
            return instance;
        }
    }
    std::erase_if(m_Instances, [](const auto& entry) { return entry.second.Lock() == nullptr; });
    Ptr<PServerFontFace> face = record->Face.Lock();
    if (face == nullptr)
    {
        face = ptr_new<PServerFontFace>(m_Library, record->Path, record->FaceIndex);
        record->Face = face;
    }
    Ptr<PServerFontInstance> instance = ptr_new<PServerFontInstance>(face, m_Cache, normalized);
    m_Instances[key] = instance;
    return instance;
}

void PFontManager::ScanDirectory(const PString& path, DirectorySet& directories, std::vector<FaceRecord>& faces)
{
    PDirectory directory;
    if (!directory.Open(path)) {
        PERROR_THROW_CODE(PErrorCode(errno));
    }
    struct stat directoryStat{};
    if (!directory.GetStat(&directoryStat)) {
        PERROR_THROW_CODE(PErrorCode(errno));
    }
    if (!directories.emplace(directoryStat.st_dev, directoryStat.st_ino).second) {
        return;
    }
    for (;;)
    {
        PString name;
        errno = 0;
        if (!directory.GetNextEntry(name))
        {
            if (errno != 0) {
                PERROR_THROW_CODE(PErrorCode(errno));
            }
            break;
        }
        if (name.is_dot_or_dot_dot()) {
            continue;
        }
        PPath child(path);
        child.Append(name);
        struct stat childStat{};
        if (stat(child.c_str(), &childStat) != 0) {
            PERROR_THROW_CODE(PErrorCode(errno));
        }
        if (S_ISDIR(childStat.st_mode)) {
            ScanDirectory(child.GetPath(), directories, faces);
        } else if (S_ISREG(childStat.st_mode)) {
            ScanFile(child.GetPath(), faces);
        }
    }
}

void PFontManager::ScanFile(const PString& path, std::vector<FaceRecord>& faces)
{
    FT_Long faceCount = 1;
    for (FT_Long faceIndex = 0; faceIndex < faceCount; ++faceIndex)
    {
        FT_Face face = nullptr;
        const FT_Error error = FT_New_Face(m_Library->GetHandle(), path.c_str(), faceIndex, &face);
        if (error != FT_Err_Ok)
        {
            if (FT_ERROR_BASE(error) == FT_Err_Out_Of_Memory || FT_ERROR_BASE(error) == FT_Err_Invalid_Stream_Operation) {
                PServerFontLibrary::CheckError(error);
            }
            continue;
        }
        PScopeExit releaseFace([face] { FT_Done_Face(face); });
        faceCount = face->num_faces;
        if (face->family_name == nullptr || FT_Select_Charmap(face, FT_ENCODING_UNICODE) != FT_Err_Ok) {
            continue;
        }
        FaceRecord record;
        record.Path = path;
        record.FaceIndex = faceIndex;
        record.Info.Family = face->family_name;
        record.Info.Style = (face->style_name != nullptr) ? face->style_name : "Regular";
        record.Info.Scalable = FT_IS_SCALABLE(face) != 0;
        record.Info.FixedWidth = FT_IS_FIXED_WIDTH(face) != 0;
        faces.push_back(std::move(record));
    }
}

PFontDescription PFontManager::NormalizeDescription(const PFontDescription& description)
{
    if (description.Family.empty() || description.Style.empty()
        || !std::isfinite(description.PointSize) || description.PointSize < FONT_F26DOT6_TO_FLOAT
        || description.PointSize > FONT_MAX_POINT_SIZE || !std::isfinite(description.Rotation)
        || !std::isfinite(description.Shear) || description.Shear < 1.0f || description.Shear > 179.0f
        || (description.RenderMode != PFontRenderMode::Antialiased && description.RenderMode != PFontRenderMode::Monochrome)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    PFontDescription normalized = description;
    normalized.PointSize = std::round(normalized.PointSize * FONT_FLOAT_TO_F26DOT6) * FONT_F26DOT6_TO_FLOAT;
    normalized.Rotation = std::fmod(normalized.Rotation, 360.0f);
    if (normalized.Rotation < 0.0f) {
        normalized.Rotation += 360.0f;
    }
    normalized.Rotation = std::fmod(std::round(normalized.Rotation * FONT_FLOAT_TO_F26DOT6) * FONT_F26DOT6_TO_FLOAT, 360.0f);
    normalized.Shear = std::round(normalized.Shear * FONT_FLOAT_TO_F26DOT6) * FONT_F26DOT6_TO_FLOAT;
    return normalized;
}
