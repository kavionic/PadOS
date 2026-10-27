// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <limits>
#include <string_view>
#include <utility>

#include <App/Application.h>
#include <GUI/Font.h>
#include <System/ExceptionHandling.h>

PFont::PFont(PFontID font, PApplication* application) : PFont(GetDefaultDescription(font), application)
{
}

PFont::PFont(const PFontDescription& description, PApplication* application)
    : m_Description(description)
    , m_Application(ptr_weak_cast((application != nullptr) ? application : PApplication::GetCurrentApplication()))
{
    m_Handle = RequireApplication().CreateFont(m_Description, m_Height);
}

PFont::~PFont()
{
    PApplication* application = GetApplication();
    if (application != nullptr) {
        application->DeleteFont(m_Handle);
    }
}

void PFont::Set(PFontID font)
{
    Set(GetDefaultDescription(font));
}

void PFont::Set(const PFontDescription& description)
{
    PApplication& application = RequireApplication();
    PFontDescription newDescription(description);
    PFontHeight height;
    const handle_id handle = application.CreateFont(newDescription, height);
    application.DeleteFont(m_Handle);
    m_Description = std::move(newDescription);
    m_Height = height;
    m_Handle = handle;
}

PApplication* PFont::GetApplication() const
{
    return ptr_raw_pointer_cast(m_Application);
}

handle_id PFont::GetHandle() const
{
    return m_Handle;
}

const PFontDescription& PFont::GetDescription() const
{
    return m_Description;
}

PFontHeight PFont::GetHeight() const
{
    return m_Height;
}

int PFont::GetStringLength(const char* string, float width, bool includeLast) const
{
    const size_t length = std::strlen(string);
    if (length > size_t(std::numeric_limits<int>::max())) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    return GetStringLength(string, int(length), width, includeLast);
}

int PFont::GetStringLength(const char* string, int length, float width, bool includeLast) const
{
    if (length < 0) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return int(RequireApplication().MeasureFont(
        m_Handle,
        std::string_view(string, size_t(length)),
        width,
        includeLast).ByteCount);
}

int PFont::GetStringLength(const std::string& string, float width, bool includeLast) const
{
    if (string.size() > size_t(std::numeric_limits<int>::max())) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    return GetStringLength(string.data(), int(string.size()), width, includeLast);
}

void PFont::GetStringLengths(
    const char** stringArray,
    const int* lengthArray,
    int stringCount,
    float width,
    int* maxLengthArray,
    bool includeLast) const
{
    for (int i = 0; i < stringCount; ++i) {
        maxLengthArray[i] = GetStringLength(stringArray[i], lengthArray[i], width, includeLast);
    }
}

float PFont::GetStringWidth(const char* string, size_t length) const
{
    return RequireApplication().MeasureFont(
        m_Handle,
        std::string_view(string, length),
        std::numeric_limits<float>::infinity(),
        false).Width;
}

PFontDescription PFont::GetDefaultDescription(PFontID font)
{
    PFontDescription description;
    // Preset heights are 23, 30, and 96 pixels at FONT_DPI.
    switch (font)
    {
        case PFontID::e_FontSmall:
        case PFontID::e_FontNormal:
            description.PointSize = 14.0f;
            break;
        case PFontID::e_FontLarge:
            description.PointSize = 19.0f;
            break;
        case PFontID::e_Font7Seg:
            description.PointSize = 61.0f;
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    return description;
}

PApplication& PFont::RequireApplication() const
{
    PApplication* application = GetApplication();
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return *application;
}
