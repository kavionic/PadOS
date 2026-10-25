// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/ServerFontFace.h>

#include <utility>

#include <ApplicationServer/Fonts/ServerFontLibrary.h>
#include <System/ExceptionHandling.h>

PServerFontFace::PServerFontFace(Ptr<PServerFontLibrary> library, const PString& path, FT_Long faceIndex)
    : m_Library(std::move(library))
{
    PServerFontLibrary::CheckError(FT_New_Face(m_Library->GetHandle(), path.c_str(), faceIndex, &m_Face));
    PScopeFail rollback([this] { FT_Done_Face(m_Face); });
    PServerFontLibrary::CheckError(FT_Select_Charmap(m_Face, FT_ENCODING_UNICODE));
}

PServerFontFace::~PServerFontFace()
{
    FT_Done_Face(m_Face);
}
