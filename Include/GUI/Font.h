// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

#include <GUI/FontDescription.h>
#include <Ptr/Ptr.h>
#include <Ptr/WeakPtr.h>
#include <System/Types.h>


class PApplication;

class PFont : public PtrTarget
{
public:
    // Standard font IDs select bundled TrueType presets, also when passed to Set().
    // Defaults to the current application. Hold its mutex while using or releasing the font.
    explicit PFont(PFontID font, PApplication* application = nullptr);
    explicit PFont(const PFontDescription& description, PApplication* application = nullptr);
    ~PFont();

    // After modifying a font, call SetFont again on views using it.
    void Set(PFontID font);
    void Set(const PFontDescription& description);
    PApplication* GetApplication() const;
    handle_id GetHandle() const;
    const PFontDescription& GetDescription() const;
    PFontHeight GetHeight() const;

    int		GetStringLength(const char* pzString, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const char* pzString, int nLength, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const std::string& cString, float vWidth, bool bIncludeLast = false) const;
    void	GetStringLengths(const char** stringArray, const int* lengthArray, int stringCount, float width, int* maxLengthArray, bool includeLast = false) const;

    float GetStringWidth(const char* string, size_t length) const;
    
private:
    static PFontDescription GetDefaultDescription(PFontID font);
    PApplication& RequireApplication() const;

    PFontDescription m_Description;
    WeakPtr<PApplication> m_Application;
    PFontHeight m_Height{};
    handle_id m_Handle = INVALID_HANDLE;
};
