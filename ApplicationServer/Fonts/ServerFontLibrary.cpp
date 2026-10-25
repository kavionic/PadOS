// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/ServerFontLibrary.h>

#include <System/ExceptionHandling.h>
#include <Utils/String.h>

PServerFontLibrary::PServerFontLibrary()
{
    CheckError(FT_Init_FreeType(&m_Library));
}

PServerFontLibrary::~PServerFontLibrary()
{
    FT_Done_FreeType(m_Library);
}

void PServerFontLibrary::CheckError(FT_Error error)
{
    if (error != FT_Err_Ok)
    {
        PErrorCode result = PErrorCode::INVAL;
        switch (FT_ERROR_BASE(error))
        {
            case FT_Err_Out_Of_Memory:
                result = PErrorCode::NOMEM;
                break;
            case FT_Err_Cannot_Open_Resource:
                result = PErrorCode::NOENT;
                break;
            case FT_Err_Invalid_Stream_Operation:
                result = PErrorCode::IO;
                break;
            default:
                break;
        }
        PERROR_THROW_CODE_WHAT(result, PString::format_string("FreeType error {}", error));
    }
}
