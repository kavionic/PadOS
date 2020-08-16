// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "System/Platform.h"

#include "GUI/Font.h"
#include "ApplicationServer/ApplicationServer.h"
#include "ApplicationServer/DisplayDriver.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FontHeight Font::GetHeight() const
{
    DisplayDriver* driver = ApplicationServer::GetDisplayDriver();
    FontHeight height;
    height.ascender = 0.0f;
    height.descender = driver->GetFontHeight(m_Font);
    height.line_gap = ceil((height.descender - height.ascender) / 10.0f);
    return height;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Font::GetStringLength(const char* pzString, float vWidth, bool bIncludeLast) const
{
    return GetStringLength(pzString, strlen(pzString), vWidth, bIncludeLast);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Font::GetStringLength(const char* pzString, int nLength, float vWidth, bool bIncludeLast) const
{
    const char* apzStrPtr[] = { pzString };
    int		nMaxLength;

    GetStringLengths(apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast);

    return nMaxLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Font::GetStringLength(const std::string& cString, float vWidth, bool bIncludeLast) const
{
    const char* apzStrPtr[] = { cString.c_str() };
    int		nMaxLength;
    int		nLength = cString.size();

    GetStringLengths(apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast);

    return(nMaxLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Font::GetStringLengths(const char** stringArray, const int* lengthArray, int stringCount, float width, int* maxLengthArray, bool includeLast) const
{
    DisplayDriver* driver = ApplicationServer::GetDisplayDriver();
    for (int i = 0; i < stringCount; ++i)
    {
        maxLengthArray[i] = driver->GetStringLength(m_Font, stringArray[i], lengthArray[i], width, includeLast);
    }
//    int	i;
//    // The first string size, and one byte of the first string is already included
//    int	nBufSize = sizeof(AR_GetStringLengths_s) - sizeof(int) - 1;
//
//    for (i = 0; i < stringCount; ++i) {
//        nBufSize += lengthArray[i] + sizeof(int);
//    }
//
//    AR_GetStringLengths_s* psReq;
//
//    char* pBuffer = new char[nBufSize];
//
//    psReq = (AR_GetStringLengths_s*)pBuffer;
//
//    psReq->hReply = m_hReplyPort;
//    psReq->hFontToken = m_hFontHandle;
//    psReq->stringCount = stringCount;
//    psReq->nWidth = width;
//    psReq->includeLast = includeLast;
//
//    int* pnLen = &psReq->sFirstHeader.nLength;
//
//    for (i = 0; i < stringCount; ++i) {
//        int	nLen = lengthArray[i];
//
//        *pnLen = nLen;
//        pnLen++;
//
//        memcpy(pnLen, stringArray[i], nLen);
//        pnLen = (int*)(((uint8*)pnLen) + nLen);
//    }
//
//    if (send_msg(Application::GetInstance()->GetAppPort(), AR_GET_STRING_LENGTHS, psReq, nBufSize) == 0) {
//        AR_GetStringLengthsReply_s* psReply = (AR_GetStringLengthsReply_s*)pBuffer;
//
//        if (get_msg(m_hReplyPort, NULL, psReply, nBufSize) == 0) {
//            if (0 == psReply->nError) {
//                for (i = 0; i < stringCount; ++i) {
//                    maxLengthArray[i] = psReply->anLengths[i];
//                }
//            }
//        } else {
//            dbprintf("Error: Font::GetStringLengths() failed to get reply\n");
//        }
//    } else {
//        dbprintf("Error: Font::GetStringLengths() failed to send AR_GET_STRING_LENGTHS request to server\n");
//    }
//    delete[] pBuffer;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Font::GetStringWidth(const char* string, size_t length) const
{
    DisplayDriver* driver = ApplicationServer::GetScreenBitmap()->m_Driver;
    return driver->GetStringWidth(m_Font, string, length);
}
