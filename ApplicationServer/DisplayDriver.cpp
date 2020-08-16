// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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
#include <stdlib.h>
#include <stdio.h>


#include "ApplicationServer/DisplayDriver.h"
#include <ApplicationServer/ServerBitmap.h>

#include <GUI/Bitmap.h>
#include <GUI/Color.h>
#include <Utils/UTF8Utils.h>

#include "ApplicationServer/Fonts/SansSerif_14.h"
#include "ApplicationServer/Fonts/SansSerif_20.h"
#include "ApplicationServer/Fonts/SansSerif_72.h"

using namespace os;

#define RAS_OFFSET8( ptr, x, y, bpl )  (((uint8_t*)(ptr)) + (x) + (y) * (bpl))
#define RAS_OFFSET16( ptr, x, y, bpl ) ((uint16_t*)(((uint8_t*)(ptr)) + (x*2) + (y) * (bpl)))
#define RAS_OFFSET32( ptr, x, y, bpl ) ((uint32_t*)(((uint8_t*)(ptr)) + (x*4) + (y) * (bpl)))


static const Color g_asDefaultPallette[] =
{
    Color(0x00, 0x00, 0x00, 0x00),  // 0
    Color(0x08, 0x08, 0x08, 0x00),
    Color(0x10, 0x10, 0x10, 0x00),
    Color(0x18, 0x18, 0x18, 0x00),
    Color(0x20, 0x20, 0x20, 0x00),
    Color(0x28, 0x28, 0x28, 0x00),  // 5
    Color(0x30, 0x30, 0x30, 0x00),
    Color(0x38, 0x38, 0x38, 0x00),
    Color(0x40, 0x40, 0x40, 0x00),
    Color(0x48, 0x48, 0x48, 0x00),
    Color(0x50, 0x50, 0x50, 0x00),  // 10
    Color(0x58, 0x58, 0x58, 0x00),
    Color(0x60, 0x60, 0x60, 0x00),
    Color(0x68, 0x68, 0x68, 0x00),
    Color(0x70, 0x70, 0x70, 0x00),
    Color(0x78, 0x78, 0x78, 0x00),  // 15
    Color(0x80, 0x80, 0x80, 0x00),
    Color(0x88, 0x88, 0x88, 0x00),
    Color(0x90, 0x90, 0x90, 0x00),
    Color(0x98, 0x98, 0x98, 0x00),
    Color(0xa0, 0xa0, 0xa0, 0x00),  // 20
    Color(0xa8, 0xa8, 0xa8, 0x00),
    Color(0xb0, 0xb0, 0xb0, 0x00),
    Color(0xb8, 0xb8, 0xb8, 0x00),
    Color(0xc0, 0xc0, 0xc0, 0x00),
    Color(0xc8, 0xc8, 0xc8, 0x00),  // 25
    Color(0xd0, 0xd0, 0xd0, 0x00),
    Color(0xd9, 0xd9, 0xd9, 0x00),
    Color(0xe2, 0xe2, 0xe2, 0x00),
    Color(0xeb, 0xeb, 0xeb, 0x00),
    Color(0xf5, 0xf5, 0xf5, 0x00),  // 30
    Color(0xfe, 0xfe, 0xfe, 0x00),
    Color(0x00, 0x00, 0xff, 0x00),
    Color(0x00, 0x00, 0xe5, 0x00),
    Color(0x00, 0x00, 0xcc, 0x00),
    Color(0x00, 0x00, 0xb3, 0x00),  // 35
    Color(0x00, 0x00, 0x9a, 0x00),
    Color(0x00, 0x00, 0x81, 0x00),
    Color(0x00, 0x00, 0x69, 0x00),
    Color(0x00, 0x00, 0x50, 0x00),
    Color(0x00, 0x00, 0x37, 0x00),  // 40
    Color(0x00, 0x00, 0x1e, 0x00),
    Color(0xff, 0x00, 0x00, 0x00),
    Color(0xe4, 0x00, 0x00, 0x00),
    Color(0xcb, 0x00, 0x00, 0x00),
    Color(0xb2, 0x00, 0x00, 0x00),  // 45
    Color(0x99, 0x00, 0x00, 0x00),
    Color(0x80, 0x00, 0x00, 0x00),
    Color(0x69, 0x00, 0x00, 0x00),
    Color(0x50, 0x00, 0x00, 0x00),
    Color(0x37, 0x00, 0x00, 0x00),  // 50
    Color(0x1e, 0x00, 0x00, 0x00),
    Color(0x00, 0xff, 0x00, 0x00),
    Color(0x00, 0xe4, 0x00, 0x00),
    Color(0x00, 0xcb, 0x00, 0x00),
    Color(0x00, 0xb2, 0x00, 0x00),  // 55
    Color(0x00, 0x99, 0x00, 0x00),
    Color(0x00, 0x80, 0x00, 0x00),
    Color(0x00, 0x69, 0x00, 0x00),
    Color(0x00, 0x50, 0x00, 0x00),
    Color(0x00, 0x37, 0x00, 0x00),  // 60
    Color(0x00, 0x1e, 0x00, 0x00),
    Color(0x00, 0x98, 0x33, 0x00),
    Color(0xff, 0xff, 0xff, 0x00),
    Color(0xcb, 0xff, 0xff, 0x00),
    Color(0xcb, 0xff, 0xcb, 0x00),  // 65
    Color(0xcb, 0xff, 0x98, 0x00),
    Color(0xcb, 0xff, 0x66, 0x00),
    Color(0xcb, 0xff, 0x33, 0x00),
    Color(0xcb, 0xff, 0x00, 0x00),
    Color(0x98, 0xff, 0xff, 0x00),
    Color(0x98, 0xff, 0xcb, 0x00),
    Color(0x98, 0xff, 0x98, 0x00),
    Color(0x98, 0xff, 0x66, 0x00),
    Color(0x98, 0xff, 0x33, 0x00),
    Color(0x98, 0xff, 0x00, 0x00),
    Color(0x66, 0xff, 0xff, 0x00),
    Color(0x66, 0xff, 0xcb, 0x00),
    Color(0x66, 0xff, 0x98, 0x00),
    Color(0x66, 0xff, 0x66, 0x00),
    Color(0x66, 0xff, 0x33, 0x00),
    Color(0x66, 0xff, 0x00, 0x00),
    Color(0x33, 0xff, 0xff, 0x00),
    Color(0x33, 0xff, 0xcb, 0x00),
    Color(0x33, 0xff, 0x98, 0x00),
    Color(0x33, 0xff, 0x66, 0x00),
    Color(0x33, 0xff, 0x33, 0x00),
    Color(0x33, 0xff, 0x00, 0x00),
    Color(0xff, 0x98, 0xff, 0x00),
    Color(0xff, 0x98, 0xcb, 0x00),
    Color(0xff, 0x98, 0x98, 0x00),
    Color(0xff, 0x98, 0x66, 0x00),
    Color(0xff, 0x98, 0x33, 0x00),
    Color(0xff, 0x98, 0x00, 0x00),
    Color(0x00, 0x66, 0xff, 0x00),
    Color(0x00, 0x66, 0xcb, 0x00),
    Color(0xcb, 0xcb, 0xff, 0x00),
    Color(0xcb, 0xcb, 0xcb, 0x00),
    Color(0xcb, 0xcb, 0x98, 0x00),
    Color(0xcb, 0xcb, 0x66, 0x00),
    Color(0xcb, 0xcb, 0x33, 0x00),
    Color(0xcb, 0xcb, 0x00, 0x00),
    Color(0x98, 0xcb, 0xff, 0x00),
    Color(0x98, 0xcb, 0xcb, 0x00),
    Color(0x98, 0xcb, 0x98, 0x00),
    Color(0x98, 0xcb, 0x66, 0x00),
    Color(0x98, 0xcb, 0x33, 0x00),
    Color(0x98, 0xcb, 0x00, 0x00),
    Color(0x66, 0xcb, 0xff, 0x00),
    Color(0x66, 0xcb, 0xcb, 0x00),
    Color(0x66, 0xcb, 0x98, 0x00),
    Color(0x66, 0xcb, 0x66, 0x00),
    Color(0x66, 0xcb, 0x33, 0x00),
    Color(0x66, 0xcb, 0x00, 0x00),
    Color(0x33, 0xcb, 0xff, 0x00),
    Color(0x33, 0xcb, 0xcb, 0x00),
    Color(0x33, 0xcb, 0x98, 0x00),
    Color(0x33, 0xcb, 0x66, 0x00),
    Color(0x33, 0xcb, 0x33, 0x00),
    Color(0x33, 0xcb, 0x00, 0x00),
    Color(0xff, 0x66, 0xff, 0x00),
    Color(0xff, 0x66, 0xcb, 0x00),
    Color(0xff, 0x66, 0x98, 0x00),
    Color(0xff, 0x66, 0x66, 0x00),
    Color(0xff, 0x66, 0x33, 0x00),
    Color(0xff, 0x66, 0x00, 0x00),
    Color(0x00, 0x66, 0x98, 0x00),
    Color(0x00, 0x66, 0x66, 0x00),
    Color(0xcb, 0x98, 0xff, 0x00),
    Color(0xcb, 0x98, 0xcb, 0x00),
    Color(0xcb, 0x98, 0x98, 0x00),
    Color(0xcb, 0x98, 0x66, 0x00),
    Color(0xcb, 0x98, 0x33, 0x00),
    Color(0xcb, 0x98, 0x00, 0x00),
    Color(0x98, 0x98, 0xff, 0x00),
    Color(0x98, 0x98, 0xcb, 0x00),
    Color(0x98, 0x98, 0x98, 0x00),
    Color(0x98, 0x98, 0x66, 0x00),
    Color(0x98, 0x98, 0x33, 0x00),
    Color(0x98, 0x98, 0x00, 0x00),
    Color(0x66, 0x98, 0xff, 0x00),
    Color(0x66, 0x98, 0xcb, 0x00),
    Color(0x66, 0x98, 0x98, 0x00),
    Color(0x66, 0x98, 0x66, 0x00),
    Color(0x66, 0x98, 0x33, 0x00),
    Color(0x66, 0x98, 0x00, 0x00),
    Color(0x33, 0x98, 0xff, 0x00),
    Color(0x33, 0x98, 0xcb, 0x00),
    Color(0x33, 0x98, 0x98, 0x00),
    Color(0x33, 0x98, 0x66, 0x00),
    Color(0x33, 0x98, 0x33, 0x00),
    Color(0x33, 0x98, 0x00, 0x00),
    Color(0xe6, 0x86, 0x00, 0x00),
    Color(0xff, 0x33, 0xcb, 0x00),
    Color(0xff, 0x33, 0x98, 0x00),
    Color(0xff, 0x33, 0x66, 0x00),
    Color(0xff, 0x33, 0x33, 0x00),
    Color(0xff, 0x33, 0x00, 0x00),
    Color(0x00, 0x66, 0x33, 0x00),
    Color(0x00, 0x66, 0x00, 0x00),
    Color(0xcb, 0x66, 0xff, 0x00),
    Color(0xcb, 0x66, 0xcb, 0x00),
    Color(0xcb, 0x66, 0x98, 0x00),
    Color(0xcb, 0x66, 0x66, 0x00),
    Color(0xcb, 0x66, 0x33, 0x00),
    Color(0xcb, 0x66, 0x00, 0x00),
    Color(0x98, 0x66, 0xff, 0x00),
    Color(0x98, 0x66, 0xcb, 0x00),
    Color(0x98, 0x66, 0x98, 0x00),
    Color(0x98, 0x66, 0x66, 0x00),
    Color(0x98, 0x66, 0x33, 0x00),
    Color(0x98, 0x66, 0x00, 0x00),
    Color(0x66, 0x66, 0xff, 0x00),
    Color(0x66, 0x66, 0xcb, 0x00),
    Color(0x66, 0x66, 0x98, 0x00),
    Color(0x66, 0x66, 0x66, 0x00),
    Color(0x66, 0x66, 0x33, 0x00),
    Color(0x66, 0x66, 0x00, 0x00),
    Color(0x33, 0x66, 0xff, 0x00),
    Color(0x33, 0x66, 0xcb, 0x00),
    Color(0x33, 0x66, 0x98, 0x00),
    Color(0x33, 0x66, 0x66, 0x00),
    Color(0x33, 0x66, 0x33, 0x00),
    Color(0x33, 0x66, 0x00, 0x00),
    Color(0xff, 0x00, 0xff, 0x00),
    Color(0xff, 0x00, 0xcb, 0x00),
    Color(0xff, 0x00, 0x98, 0x00),
    Color(0xff, 0x00, 0x66, 0x00),
    Color(0xff, 0x00, 0x33, 0x00),
    Color(0xff, 0xaf, 0x13, 0x00),
    Color(0x00, 0x33, 0xff, 0x00),
    Color(0x00, 0x33, 0xcb, 0x00),
    Color(0xcb, 0x33, 0xff, 0x00),
    Color(0xcb, 0x33, 0xcb, 0x00),
    Color(0xcb, 0x33, 0x98, 0x00),
    Color(0xcb, 0x33, 0x66, 0x00),
    Color(0xcb, 0x33, 0x33, 0x00),
    Color(0xcb, 0x33, 0x00, 0x00),
    Color(0x98, 0x33, 0xff, 0x00),
    Color(0x98, 0x33, 0xcb, 0x00),
    Color(0x98, 0x33, 0x98, 0x00),
    Color(0x98, 0x33, 0x66, 0x00),
    Color(0x98, 0x33, 0x33, 0x00),
    Color(0x98, 0x33, 0x00, 0x00),
    Color(0x66, 0x33, 0xff, 0x00),
    Color(0x66, 0x33, 0xcb, 0x00),
    Color(0x66, 0x33, 0x98, 0x00),
    Color(0x66, 0x33, 0x66, 0x00),
    Color(0x66, 0x33, 0x33, 0x00),
    Color(0x66, 0x33, 0x00, 0x00),
    Color(0x33, 0x33, 0xff, 0x00),
    Color(0x33, 0x33, 0xcb, 0x00),
    Color(0x33, 0x33, 0x98, 0x00),
    Color(0x33, 0x33, 0x66, 0x00),
    Color(0x33, 0x33, 0x33, 0x00),
    Color(0x33, 0x33, 0x00, 0x00),
    Color(0xff, 0xcb, 0x66, 0x00),
    Color(0xff, 0xcb, 0x98, 0x00),
    Color(0xff, 0xcb, 0xcb, 0x00),
    Color(0xff, 0xcb, 0xff, 0x00),
    Color(0x00, 0x33, 0x98, 0x00),
    Color(0x00, 0x33, 0x66, 0x00),
    Color(0x00, 0x33, 0x33, 0x00),
    Color(0x00, 0x33, 0x00, 0x00),
    Color(0xcb, 0x00, 0xff, 0x00),
    Color(0xcb, 0x00, 0xcb, 0x00),
    Color(0xcb, 0x00, 0x98, 0x00),
    Color(0xcb, 0x00, 0x66, 0x00),
    Color(0xcb, 0x00, 0x33, 0x00),
    Color(0xff, 0xe3, 0x46, 0x00),
    Color(0x98, 0x00, 0xff, 0x00),
    Color(0x98, 0x00, 0xcb, 0x00),
    Color(0x98, 0x00, 0x98, 0x00),
    Color(0x98, 0x00, 0x66, 0x00),
    Color(0x98, 0x00, 0x33, 0x00),
    Color(0x98, 0x00, 0x00, 0x00),
    Color(0x66, 0x00, 0xff, 0x00),
    Color(0x66, 0x00, 0xcb, 0x00),
    Color(0x66, 0x00, 0x98, 0x00),
    Color(0x66, 0x00, 0x66, 0x00),
    Color(0x66, 0x00, 0x33, 0x00),
    Color(0x66, 0x00, 0x00, 0x00),
    Color(0x33, 0x00, 0xff, 0x00),
    Color(0x33, 0x00, 0xcb, 0x00),
    Color(0x33, 0x00, 0x98, 0x00),
    Color(0x33, 0x00, 0x66, 0x00),
    Color(0x33, 0x00, 0x33, 0x00),
    Color(0x33, 0x00, 0x00, 0x00),
    Color(0xff, 0xcb, 0x33, 0x00),
    Color(0xff, 0xcb, 0x00, 0x00),
    Color(0xff, 0xff, 0x00, 0x00),
    Color(0xff, 0xff, 0x33, 0x00),
    Color(0xff, 0xff, 0x66, 0x00),
    Color(0xff, 0xff, 0x98, 0x00),
    Color(0xff, 0xff, 0xcb, 0x00),
    Color(0xff, 0xff, 0xff, 0xff)
};


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DisplayDriver::DisplayDriver() : m_cCursorHotSpot(0, 0)
{
//    m_pcMouseImage = nullptr;
//    m_pcMouseSprite = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DisplayDriver::~DisplayDriver()
{
//    delete m_pcMouseSprite;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::Close(void)
{
}

///////////////////////////////////////////////////////////////////////////////
/// Get offset between raster-area start and frame-buffer start
///
/// \par Description:
///     Should return the offset from the frame-buffer area's logical
///     address to the actual start of the frame-buffer. This should
///     normally be 0 but might be between 0-4095 if the frame-buffer
///     is not page-aligned (Like when running under WMVare).
/// \par Note:
///     The driver should try to avoid using this offset since it will give
///     user-space applications access to the memory area between the start
///     of the raster-area and the start of the bitmap.
/// \par Warning:
/// \param
/// \return
/// \sa
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int DisplayDriver::GetFramebufferOffset()
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void DisplayDriver::SetCursorBitmap(mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight)
//{
//    SrvSprite::Hide();
//
//    m_cCursorHotSpot = cHotSpot;
//    delete m_pcMouseSprite;
//
//    if (m_pcMouseImage != NULL) {
//        m_pcMouseImage->Release();
//    }
//    m_pcMouseImage = new SrvBitmap(nWidth, nHeight, CS_CMAP8);
//
//    uint8_t* pDstRaster = m_pcMouseImage->m_Raster;
//    const uint8_t* pSrcRaster = static_cast<const uint8_t*>(pRaster);
//
//    for (int i = 0; i < nWidth * nHeight; ++i) {
//        uint8_t anPalette[] = { 255, 0, 0, 63 };
//        if (pSrcRaster[i] < 4) {
//            pDstRaster[i] = anPalette[pSrcRaster[i]];
//        } else {
//            pDstRaster[i] = 255;
//        }
//    }
//    if (m_pcMouseImage != NULL) {
//        m_pcMouseSprite = new SrvSprite(IRect(0, 0, nWidth, nHeight), m_cMousePos, m_cCursorHotSpot, g_pcScreenBitmap, m_pcMouseImage);
//    }
//    SrvSprite::Unhide();
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::MouseOn()
{
//    if (m_pcMouseSprite == NULL) {
//        m_pcMouseSprite = new SrvSprite(IRect(0, 0, m_pcMouseImage->m_nWidth, m_pcMouseImage->m_nHeight), m_cMousePos, m_cCursorHotSpot, g_pcScreenBitmap, m_pcMouseImage);
//    } else {
//        printf("Warning: DisplayDriver::MouseOn() called while mouse visible\n");
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::MouseOff()
{
//    if (m_pcMouseSprite == NULL) {
//        printf("Warning: VDisplayDriver::MouseOff() called while mouse hidden\n");
//    }
//    delete m_pcMouseSprite;
//    m_pcMouseSprite = NULL;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::SetMousePos(IPoint cNewPos)
{
//    if (m_pcMouseSprite != NULL) {
//        m_pcMouseSprite->MoveTo(cNewPos);
//    }
    m_cMousePos = cNewPos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::FillBlit8(uint8_t* pDst, int nMod, int W, int H, int nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++) {
            *pDst++ = nColor;
        }
        pDst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::FillBlit16(uint16_t* pDst, int nMod, int W, int H, uint32_t nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++) {
            *pDst++ = nColor;
        }
        pDst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::FillBlit24(uint8_t* pDst, int nMod, int W, int H, uint32_t nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++)
        {
            *pDst++ = nColor & 0xff;
            *((uint16_t*)pDst) = nColor >> 8;
            pDst += 2;
        }
        pDst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::FillBlit32(uint32_t* pDst, int nMod, int W, int H, uint32_t nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++) {
            *pDst++ = nColor;
        }
        pDst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::WritePixel(SrvBitmap* bitmap, const IPoint& pos, Color color)
{
    switch (bitmap->m_ColorSpace)
    {
        case CS_RGB15:
            *reinterpret_cast<uint16_t*>(&bitmap->m_Raster[pos.x * 2 + pos.y * bitmap->m_BytesPerLine]) = color.GetColor15();
            break;
        case CS_RGB16:
            *reinterpret_cast<uint16_t*>(&bitmap->m_Raster[pos.x * 2 + pos.y * bitmap->m_BytesPerLine]) = color.GetColor16();
            break;
        case CS_RGB32:
            *reinterpret_cast<uint32_t*>(&bitmap->m_Raster[pos.x * 4 + pos.y * bitmap->m_BytesPerLine]) = color.GetColor32();
            break;
        default:
            printf("DisplayDriver::WritePixel() unknown color space %d\n", bitmap->m_ColorSpace);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::FillRect(SrvBitmap* pcBitmap, const IRect& cRect, const Color& sColor)
{
    int BltX, BltY, BltW, BltH;

    BltX = cRect.left;
    BltY = cRect.top;
    BltW = cRect.Width();
    BltH = cRect.Height();

    switch (pcBitmap->m_ColorSpace)
    {
        //    case CS_CMAP8:
        //      FillBlit8( pcBitmap->m_Raster + ((BltY * pcBitmap->m_BytesPerLine) + BltX),
        //       pcBitmap->m_BytesPerLine - BltW, BltW, BltH, nColor );
        //      break;
        case CS_RGB15:
            FillBlit16((uint16_t*)&pcBitmap->m_Raster[BltY * pcBitmap->m_BytesPerLine + BltX * 2], pcBitmap->m_BytesPerLine / 2 - BltW, BltW, BltH, sColor.GetColor15());
            break;
        case CS_RGB16:
            FillBlit16((uint16_t*)&pcBitmap->m_Raster[BltY * pcBitmap->m_BytesPerLine + BltX * 2], pcBitmap->m_BytesPerLine / 2 - BltW, BltW, BltH, sColor.GetColor16());
            break;
        case CS_RGB24:
            FillBlit24( &pcBitmap->m_Raster[ BltY * pcBitmap->m_BytesPerLine + BltX * 3 ], pcBitmap->m_BytesPerLine - BltW * 3, BltW, BltH, sColor.GetColor32() );
            break;
        case CS_RGB32:
            FillBlit32((uint32_t*)&pcBitmap->m_Raster[BltY * pcBitmap->m_BytesPerLine + BltX * 4], pcBitmap->m_BytesPerLine / 4 - BltW, BltW, BltH, sColor.GetColor32());
            break;
        default:
            printf("DisplayDriver::FillRect() unknown color space %d\n", pcBitmap->m_ColorSpace);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void Blit(uint8_t* Src, uint8_t* Dst, int SMod, int DMod, int W, int H, bool Rev)
{
    int   i;
    int       X, Y;
    uint32_t* LSrc;
    uint32_t* LDst;

    if (Rev)
    {
        for (Y = 0; Y < H; Y++)
        {
            for (X = 0; (X < W) && ((uint32_t(Src - 3)) & 3); X++) {
                *Dst-- = *Src--;
            }

            LSrc = (uint32_t*)(((uint32_t)Src) - 3);
            LDst = (uint32_t*)(((uint32_t)Dst) - 3);

            i = (W - X) / 4;

            X += i * 4;

            for (; i; i--) {
                *LDst-- = *LSrc--;
            }

            Src = (uint8_t*)(((uint32_t)LSrc) + 3);
            Dst = (uint8_t*)(((uint32_t)LDst) + 3);

            for (; X < W; X++) {
                *Dst-- = *Src--;
            }

            Dst -= (int32_t)DMod;
            Src -= (int32_t)SMod;
        }
    }
    else
    {
        for (Y = 0; Y < H; Y++)
        {
            for (X = 0; (X < W) && (((uint32_t)Src) & 3); ++X) {
                *Dst++ = *Src++;
            }

            LSrc = (uint32_t*)Src;
            LDst = (uint32_t*)Dst;

            i = (W - X) / 4;

            X += i * 4;

            for (; i; i--) {
                *LDst++ = *LSrc++;
            }

            Src = (uint8_t*)LSrc;
            Dst = (uint8_t*)LDst;

            for (; X < W; X++) {
                *Dst++ = *Src++;
            }

            Dst += DMod;
            Src += SMod;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void BitBlit(SrvBitmap* sbm, SrvBitmap* dbm, int sx, int sy, int dx, int dy, int w, int h)
{
    int Smod, Dmod;
    int BytesPerPix = 1;

    int InPtr, OutPtr;

    int nBitsPerPix = BitsPerPixel(dbm->m_ColorSpace);

    if (nBitsPerPix == 15) {
        BytesPerPix = 2;
    } else {
        BytesPerPix = nBitsPerPix / 8;
    }

    sx *= BytesPerPix;
    dx *= BytesPerPix;
    w *= BytesPerPix;

    if (sx >= dx)
    {
        if (sy >= dy)
        {
            Smod = sbm->m_BytesPerLine - w;
            Dmod = dbm->m_BytesPerLine - w;
            InPtr = sy * sbm->m_BytesPerLine + sx;
            OutPtr = dy * dbm->m_BytesPerLine + dx;

            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, false);
        }
        else
        {
            Smod = -sbm->m_BytesPerLine - w;
            Dmod = -dbm->m_BytesPerLine - w;
            InPtr = ((sy + h - 1) * sbm->m_BytesPerLine) + sx;
            OutPtr = ((dy + h - 1) * dbm->m_BytesPerLine) + dx;

            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, false);
        }
    }
    else
    {
        if (sy > dy)
        {
            Smod = -(sbm->m_BytesPerLine + w);
            Dmod = -(dbm->m_BytesPerLine + w);
            InPtr = (sy * sbm->m_BytesPerLine) + sx + w - 1;
            OutPtr = (dy * dbm->m_BytesPerLine) + dx + w - 1;
            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, true);
        }
        else
        {
            Smod = sbm->m_BytesPerLine - w;
            Dmod = dbm->m_BytesPerLine - w;
            InPtr = (sy + h - 1) * sbm->m_BytesPerLine + sx + w - 1;
            OutPtr = (dy + h - 1) * dbm->m_BytesPerLine + dx + w - 1;
            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void blit_convert_copy(SrvBitmap* pcDst, SrvBitmap* pcSrc, const IRect& cSrcRect, const IPoint& cDstPos)
{
    switch ((int)pcSrc->m_ColorSpace)
    {
        case CS_CMAP8:
        {
            uint8_t* pSrc = RAS_OFFSET8(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);

            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width();

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB15:
                case CS_RGBA15:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);

                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = g_asDefaultPallette[*pSrc++].GetColor15();
                        }
                        pSrc += nSrcModulo;
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB16:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);

                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = g_asDefaultPallette[*pSrc++].GetColor16();
                        }
                        pSrc += nSrcModulo;
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB32:
                case CS_RGBA32:
                {
                    uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = g_asDefaultPallette[*pSrc++].GetColor32();
                        }
                        pSrc += nSrcModulo;
                        pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
            }
            break;
        }
        case CS_RGB15:
        case CS_RGBA15:
        {
            uint16_t* pSrc = RAS_OFFSET16(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);
            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width() * 2;

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB16:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB15(*pSrc++).GetColor16();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB32:
                case CS_RGBA32:
                {
                    uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB15(*pSrc++).GetColor32();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
            }
            break;
        }
        case CS_RGB16:
        {
            uint16_t* pSrc = RAS_OFFSET16(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);
            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width() * 2;

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB15:
                case CS_RGBA15:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB16(*pSrc++).GetColor15();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB32:
                case CS_RGBA32:
                {
                    uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB16(*pSrc++).GetColor32();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
            }
            break;
        }
        case CS_RGB32:
        case CS_RGBA32:
        {
            uint32_t* pSrc = RAS_OFFSET32(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);
            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width() * 4;

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB16:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB32A(*pSrc++).GetColor16();
                        }
                        pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB15:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB32A(*pSrc++).GetColor15();
                        }
                        pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
            }
            break;
        }
        default:
            printf("blit_convert_copy() unknown src color space %d\n", pcSrc->m_ColorSpace);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void blit_convert_over(SrvBitmap* pcDst, SrvBitmap* pcSrc, const IRect& cSrcRect, const IPoint& cDstPos)
{
    switch ((int)pcSrc->m_ColorSpace)
    {
        case CS_CMAP8:
        {
            uint8_t* pSrc = RAS_OFFSET8(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);

            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width();

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB15:
                case CS_RGBA15:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);

                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                        {
                            int nPix = *pSrc++;
                            if (nPix != TRANSPARENT_CMAP8) {
                                *pDst = g_asDefaultPallette[nPix].GetColor15();
                            }
                            pDst++;
                        }
                        pSrc += nSrcModulo;
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB16:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                        {
                            int nPix = *pSrc++;
                            if (nPix != TRANSPARENT_CMAP8) {
                                *pDst = g_asDefaultPallette[nPix].GetColor16();
                            }
                            pDst++;
                        }
                        pSrc += nSrcModulo;
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB32:
                case CS_RGBA32:
                {
                    uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                        {
                            int nPix = *pSrc++;
                            if (nPix != TRANSPARENT_CMAP8) {
                                *pDst = g_asDefaultPallette[nPix].GetColor32();
                            }
                            pDst++;
                        }
                        pSrc += nSrcModulo;
                        pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                default:
                    printf("blit_convert_over() unknown dst colorspace for 8 bit src %d\n", pcDst->m_ColorSpace);
                    break;
            }
            break;
        }
        case CS_RGB15:
        case CS_RGBA15:
        {
            uint16_t* pSrc = RAS_OFFSET16(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);
            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width() * 2;

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB16:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB15(*pSrc++).GetColor16();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB32:
                case CS_RGBA32:
                {
                    uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);

                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB15(*pSrc++).GetColor32();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
            }
            break;
        }
        case CS_RGB16:
        {
            uint16_t* pSrc = RAS_OFFSET16(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);
            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width() * 2;

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB15:
                case CS_RGBA15:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB16(*pSrc++).GetColor15();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB32:
                case CS_RGBA32:
                {
                    uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x) {
                            *pDst++ = Color::FromRGB16(*pSrc++).GetColor32();
                        }
                        pSrc = (uint16_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
            }
            break;
        }
        case CS_RGB32:
        case CS_RGBA32:
        {
            uint32_t* pSrc = RAS_OFFSET32(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);
            int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width() * 4;

            switch ((int)pcDst->m_ColorSpace)
            {
                case CS_RGB16:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                        {
                            uint32_t nPix = *pSrc++;
                            if (nPix != 0xffffffff) {
                                *pDst = Color::FromRGB32A(nPix).GetColor16();
                            }
                            pDst++;
                        }
                        pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB15:
                {
                    uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                        {
                            uint32_t nPix = *pSrc++;
                            if (nPix != 0xffffffff) {
                                *pDst = Color::FromRGB32A(nPix).GetColor15();
                            }
                            pDst++;
                        }
                        pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
                case CS_RGB32:
                {
                    uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
                    int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

                    for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
                    {
                        for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                        {
                            uint32_t nPix = *pSrc++;
                            if (nPix != 0xffffffff) {
                                *pDst = nPix;
                            }
                            pDst++;
                        }
                        pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                        pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
                    }
                    break;
                }
            }
            break;
        }
        default:
            printf("blit_convert_over() unknown src color space %d\n", pcSrc->m_ColorSpace);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void blit_convert_alpha(SrvBitmap* pcDst, SrvBitmap* pcSrc, const IRect& cSrcRect, const IPoint& cDstPos)
{
    uint32_t* pSrc = RAS_OFFSET32(pcSrc->m_Raster, cSrcRect.left, cSrcRect.top, pcSrc->m_BytesPerLine);
    int nSrcModulo = pcSrc->m_BytesPerLine - cSrcRect.Width() * 4;

    switch ((int)pcDst->m_ColorSpace)
    {
        case CS_RGB16:
        {
            uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
            int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

            for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
            {
                for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                {
                    Color sSrcColor = Color::FromRGB32A(*pSrc++);

                    int nAlpha = sSrcColor.GetAlpha();
                    if (nAlpha == 0xff) {
                        *pDst = sSrcColor.GetColor16();
                    } else if (nAlpha != 0x00) {
                        Color sDstColor = Color::FromRGB16(*pDst);
                        *pDst = Color(sDstColor.GetRed() * (256 - nAlpha) / 256   + sSrcColor.GetRed() * nAlpha / 256,
                                      sDstColor.GetGreen() * (256 - nAlpha) / 256 + sSrcColor.GetGreen() * nAlpha / 256,
                                      sDstColor.GetBlue() * (256 - nAlpha) / 256  + sSrcColor.GetBlue() * nAlpha / 256,
                                      0).GetColor16();
                    }
                    pDst++;
                }
                pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
            }
            break;
        }
        case CS_RGB15:
        {
            uint16_t* pDst = RAS_OFFSET16(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
            int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 2;

            for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
            {
                for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                {
                    Color sSrcColor = Color::FromRGB32A(*pSrc++);

                    int nAlpha = sSrcColor.GetAlpha();
                    if (nAlpha == 0xff) {
                        *pDst = sSrcColor.GetColor15();
                    } else if (nAlpha != 0x00) {
                        Color sDstColor = Color::FromRGB15(*pDst);
                        *pDst = Color(sDstColor.GetRed() * (256 - nAlpha) / 256   + sSrcColor.GetRed() * nAlpha / 256,
                                      sDstColor.GetGreen() * (256 - nAlpha) / 256 + sSrcColor.GetGreen() * nAlpha / 256,
                                      sDstColor.GetBlue() * (256 - nAlpha) / 256  + sSrcColor.GetBlue() * nAlpha / 256,
                                      0).GetColor15();
                    }
                    pDst++;
                }
                pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                pDst = (uint16_t*)(((uint8_t*)pDst) + nDstModulo);
            }
            break;
        }
        case CS_RGB32:
        {
            uint32_t* pDst = RAS_OFFSET32(pcDst->m_Raster, cDstPos.x, cDstPos.y, pcDst->m_BytesPerLine);
            int nDstModulo = pcDst->m_BytesPerLine - cSrcRect.Width() * 4;

            for (int y = cSrcRect.top; y <= cSrcRect.bottom; ++y)
            {
                for (int x = cSrcRect.left; x <= cSrcRect.right; ++x)
                {
                    Color sSrcColor = Color::FromRGB32A(*pSrc++);

                    int nAlpha = sSrcColor.GetAlpha();
                    if (nAlpha == 0xff) {
                        *pDst = sSrcColor.GetColor32();
                    } else if (nAlpha != 0x00) {
                        Color sDstColor = Color::FromRGB32A(*pDst);
                        *pDst = Color(sDstColor.GetRed() * (256 - nAlpha) / 256 + sSrcColor.GetRed() * nAlpha / 256,
                                      sDstColor.GetGreen() * (256 - nAlpha) / 256 + sSrcColor.GetGreen() * nAlpha / 256,
                                      sDstColor.GetBlue() * (256 - nAlpha) / 256 + sSrcColor.GetBlue() * nAlpha / 256,
                                      0).GetColor32();
                    }
                    pDst++;
                }
                pSrc = (uint32_t*)(((uint8_t*)pSrc) + nSrcModulo);
                pDst = (uint32_t*)(((uint8_t*)pDst) + nDstModulo);
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::CopyRect(SrvBitmap* dstBitmap, SrvBitmap* srcBitmap, const IRect& srcRect, const IPoint& dstPos, drawing_mode mode)
{
    switch (mode)
    {
        case DM_COPY:
            if (srcBitmap->m_ColorSpace == dstBitmap->m_ColorSpace) {
                int sx = srcRect.left;
                int sy = srcRect.top;
                int dx = dstPos.x;
                int dy = dstPos.y;
                int w = srcRect.Width();
                int h = srcRect.Height();

                BitBlit(srcBitmap, dstBitmap, sx, sy, dx, dy, w, h);
            } else {
                blit_convert_copy(dstBitmap, srcBitmap, srcRect, dstPos);
            }
            break;
        case DM_OVER:
            blit_convert_over(dstBitmap, srcBitmap, srcRect, dstPos);
            break;
        case DM_BLEND:
            if (srcBitmap->m_ColorSpace == CS_RGB32) {
                blit_convert_alpha(dstBitmap, srcBitmap, srcRect, dstPos);
            } else {
                blit_convert_over(dstBitmap, srcBitmap, srcRect, dstPos);
            }
            break;
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#if 0
void DisplayDriver::BltBitmapMask(SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, const Color& sHighColor, const Color& sLowColor, IRect   cSrcRect, IPoint cDstPos)
{
    int  DX = cDstPos.x;
    int  DY = cDstPos.y;

    int  SX = cSrcRect.left;
    int  SY = cSrcRect.top;

    int  W = cSrcRect.Width() + 1;
    int  H = cSrcRect.Height() + 1;

    uint32_t Fg = ConvertColor32(sHighColor, pcDstBitMap->m_ColorSpace);
    uint32_t Bg = ConvertColor32(sLowColor, pcDstBitMap->m_ColorSpace);

    char    CB;
    int X, Y, SBit;
    int BytesPerPix = 1;

    uint32_t    BPR, SByte, DYoff;

    BPR = pcSrcBitMap->m_BytesPerLine;

    BytesPerPix = BitsPerPixel(pcDstBitMap->m_ColorSpace) / 8;
    //  if ( pcDstBitMap->m_nBitsPerPixel > 8 ) BytesPerPix++;
    //  if ( pcDstBitMap->m_nBitsPerPixel > 16 )    BytesPerPix++;
    //  if ( pcDstBitMap->m_nBitsPerPixel > 24 )    BytesPerPix++;
    /*
      if ( BytesPerPix > 1 )
      {
      Fg = PenToRGB( DBM->ColorMap, Fg );
      Bg = PenToRGB( DBM->ColorMap, Bg );
      }
      */
    switch (BytesPerPix)
    {
        case 1:
        case 2:
            DYoff = DY * pcDstBitMap->m_BytesPerLine / BytesPerPix;
            break;
        case 3:
            DYoff = DY * pcDstBitMap->m_BytesPerLine;
            break;
        default:
            DYoff = DY * pcDstBitMap->m_BytesPerLine;
            __assertw(0);
            break;
    }

    for (Y = 0; Y < H; Y++)
    {
        SByte = (SY * BPR) + (SX / 8);
        CB = pcSrcBitMap->m_Raster[SByte++];
        SBit = 7 - (SX % 8);

        switch (BytesPerPix)
        {
            case 1:
                for (X = 0; X < W; X++)
                {
                    if (CB & (1L << SBit))
                    {
                        pcDstBitMap->m_Raster[DYoff + DX] = Fg;
                    } else
                    {
                        pcDstBitMap->m_Raster[DYoff + DX] = Bg;
                    }
                    SX++;
                    DX++;
                    if (!SBit)
                    {
                        SBit = 8;
                        CB = pcSrcBitMap->m_Raster[SByte++];
                    }
                    SBit--;
                }
                break;
            case 2:
                for (X = 0; X < W; X++)
                {
                    if (CB & (1L << SBit))
                    {
                        ((uint16_t*)pcDstBitMap->m_Raster)[DYoff + DX] = Fg;
                    } else
                    {
                        ((uint16_t*)pcDstBitMap->m_Raster)[DYoff + DX] = Bg;
                    }
                    SX++;
                    DX++;
                    if (!SBit)
                    {
                        SBit = 8;
                        CB = pcSrcBitMap->m_Raster[SByte++];
                    }
                    SBit--;
                }
                break;
            case 3:
                for (X = 0; X < W; X++)
                {
                    if (CB & (1L << SBit))
                    {
                        pcDstBitMap->m_Raster[DYoff + DX * 3] = Fg & 0xff;
                        ((uint16_t*)&pcDstBitMap->m_Raster[DYoff + DX * 3 + 1])[0] = Fg >> 8;
                    } else
                    {
                        pcDstBitMap->m_Raster[DYoff + DX * 3] = Bg & 0xff;
                        ((uint16_t*)&pcDstBitMap->m_Raster[DYoff + DX * 3 + 1])[0] = Bg >> 8;
                    }
                    SX++;
                    DX++;
                    if (!SBit)
                    {
                        SBit = 8;
                        CB = pcSrcBitMap->m_Raster[SByte++];
                    }
                    SBit--;
                }
                break;
        }
        SX -= W;
        DX -= W;

        SY++;
        DY++;

        if (BytesPerPix == 2)
            DYoff += pcDstBitMap->m_BytesPerLine / 2;
        else
            DYoff += pcDstBitMap->m_BytesPerLine;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::FillCircle(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& center, int32_t radius, const Color& color, drawing_mode mode)
{
    Rect frame(-radius, -radius, radius, radius);

    int radiusSqr = radius * radius;
    for (int y1 = frame.top; y1 <= 0; ++y1)
    {
        for (int x1 = frame.left; x1 <= 0; ++x1)
        {
            if (x1 * x1 + y1 * y1 <= radiusSqr)
            {
                IRect topRect(center.x + x1, center.y + y1, center.x - x1, center.y + y1 + 1);
                IRect bottomRect(center.x + x1, center.y - y1, center.x - x1, center.y - y1 + 1);

                topRect &= clipRect;
                bottomRect &= clipRect;

                if (topRect.IsValid()) {
                    FillRect(bitmap, topRect, color);
                }
                if (bottomRect.IsValid()) {
                    FillRect(bitmap, bottomRect, color);
                }
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t DisplayDriver::WriteString(SrvBitmap* bitmap, const IPoint& position, const char* string, size_t strLength, const IRect& clipRect, Color colorBg, Color colorFg, Font_e fontID)
{
    return position.x;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//bool DisplayDriver::RenderGlyph(SrvBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, uint32_t* anPalette)
//{
//    IRect cBounds = pcGlyph->m_cBounds + cPos;
//    IRect cRect = cBounds & cClipRect;
//
//    if (cRect.IsValid())
//    {
//        int   sx = cRect.left - cBounds.left;
//        int   sy = cRect.top - cBounds.top;
//
//        int   nWidth = cRect.Width();
//        int   nHeight = cRect.Height();
//
//        int   nSrcModulo = pcGlyph->m_BytesPerLine - nWidth;
//        int   nDstModulo = pcBitmap->m_BytesPerLine / 2 - nWidth;
//
//        uint8_t* pSrc = pcGlyph->m_Raster + sx + sy * pcGlyph->m_BytesPerLine;
//        uint16_t* pDst = (uint16_t*)pcBitmap->m_Raster + cRect.left + (cRect.top * pcBitmap->m_BytesPerLine / 2);
//
//        for (int y = 0; y < nHeight; ++y)
//        {
//            for (int x = 0; x < nWidth; ++x)
//            {
//                int nPix = *pSrc++;
//                if (nPix > 0) {
//                    *pDst = anPalette[nPix - 1];
//                }
//                pDst++;
//            }
//            pSrc += nSrcModulo;
//            pDst += nDstModulo;
//        }
//    }
//    return true;
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//bool DisplayDriver::RenderGlyph(SrvBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, const Color& sFgColor)
//{
//    IRect cBounds = pcGlyph->m_cBounds + cPos;
//    IRect cRect = cBounds & cClipRect;
//
//    if (cRect.IsValid() == false) {
//        return(false);
//    }
//    int   sx = cRect.left - cBounds.left;
//    int   sy = cRect.top - cBounds.top;
//
//    int   nWidth = cRect.Width();
//    int   nHeight = cRect.Height();
//
//    int   nSrcModulo = pcGlyph->m_BytesPerLine - nWidth;
//
//    uint8_t* pSrc = pcGlyph->m_Raster + sx + sy * pcGlyph->m_BytesPerLine;
//
//    Color sCurCol;
//    Color sBgColor;
//
//    if (pcBitmap->m_ColorSpace == CS_RGB16) {
//        int   nDstModulo = pcBitmap->m_BytesPerLine / 2 - nWidth;
//        uint16_t* pDst = (uint16_t*)pcBitmap->m_Raster + cRect.left + (cRect.top * pcBitmap->m_BytesPerLine / 2);
//
//        int nFgClut = sFgColor.GetColor16();
//
//        for (int y = 0; y < nHeight; ++y) {
//            for (int x = 0; x < nWidth; ++x) {
//                int nAlpha = *pSrc++;
//
//                if (nAlpha > 0) {
//                    if (nAlpha == 4) {
//                        *pDst = nFgClut;
//                    } else {
//                        int   nClut = *pDst;
//
//                        sBgColor = Color::FromRGB16(nClut);
//
//                        sCurCol.GetRed()   = sBgColor.GetRed()   + (sFgColor.GetRed()   - sBgColor.GetRed()) * nAlpha / 4;
//                        sCurCol.GetGreen() = sBgColor.GetGreen() + (sFgColor.GetGreen() - sBgColor.GetGreen()) * nAlpha / 4;
//                        sCurCol.GetBlue()  = sBgColor.GetBlue()  + (sFgColor.GetBlue()  - sBgColor.GetBlue()) * nAlpha / 4;
//                        *pDst = sCurCol.GetColor16();
//                    }
//                }
//                pDst++;
//            }
//            pSrc += nSrcModulo;
//            pDst += nDstModulo;
//        }
//    } else if (pcBitmap->m_ColorSpace == CS_RGB32) {
//        int   nDstModulo = pcBitmap->m_BytesPerLine / 4 - nWidth;
//        uint32_t* pDst = (uint32_t*)pcBitmap->m_Raster + cRect.left + (cRect.top * pcBitmap->m_BytesPerLine / 4);
//
//        int nFgClut = sFgColor.GetColor32();
//
//        for (int y = 0; y < nHeight; ++y) {
//            for (int x = 0; x < nWidth; ++x) {
//                int nAlpha = *pSrc++;
//
//                if (nAlpha > 0) {
//                    if (nAlpha == 4) {
//                        *pDst = nFgClut;
//                    } else {
//                        int   nClut = *pDst;
//
//                        sBgColor = Color::FromRGB32A(nClut);
//
//                        sCurCol.GetRed   = sBgColor.GetRed()   + (sFgColor.GetRed()   - sBgColor.GetRed()) * nAlpha / 4;
//                        sCurCol.GetGreen = sBgColor.GetGreen() + (sFgColor.GetGreen() - sBgColor.GetGreen()) * nAlpha / 4;
//                        sCurCol.GetBlue  = sBgColor.GetBlue()  + (sFgColor.GetBlue()  - sBgColor.GetBlue()) * nAlpha / 4;
//
//                        *pDst = sCurCol.GetColor32();
//                    }
//                }
//                pDst++;
//            }
//            pSrc += nSrcModulo;
//            pDst += nDstModulo;
//        }
//    }
//    return true;
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float DisplayDriver::GetFontHeight(Font_e fontID) const
{
    const FONT_INFO* font = GetFontDesc(fontID);
    return (font != nullptr) ? float(font->heightPages) : 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float DisplayDriver::GetStringWidth(Font_e fontID, const char* string, size_t length) const
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font != nullptr)
    {
        float width = 0.0f;

        while (length--)
        {
            uint8_t character = *(string++);
            if (character < font->startChar) character = font->startChar;
            const FONT_CHAR_INFO* charInfo = font->charInfo + character - font->startChar;
            width += float(charInfo->widthBits);
            if (length != 0) {
                width += float(CHARACTER_SPACING);
            }
        }
        return width;
    }
    return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t DisplayDriver::GetStringLength(Font_e fontID, const char* string, size_t length, float width, bool includeLast)
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font == nullptr) {
        return 0;
    }
    size_t strLen = 0;

    while (length > 0)
    {
        int charLen = utf8_char_length(*string);
        if (charLen > length) {
            break;
        }
        int character = utf8_to_unicode(string);
        if (character < font->startChar || character > font->endChar) character = font->startChar;

        const FONT_CHAR_INFO* charInfo = font->charInfo + character - font->startChar;
        float advance = float(charInfo->widthBits);
        if (width < advance)
        {
            if (includeLast) {
                strLen += charLen;
            }
            break;
        }
        string += charLen;
        length -= charLen;
        strLen += charLen;
        width -= advance;
    }
    return strLen;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const FONT_INFO* DisplayDriver::GetFontDesc(Font_e fontID) const
{
    switch (fontID)
    {
        case Font_e::e_FontSmall:
        case Font_e::e_FontNormal:
            return &sansSerif_14ptFontInfo;
        case Font_e::e_FontLarge:
            return &sansSerif_20ptFontInfo;
        case Font_e::e_Font7Seg:
            return &sansSerif_72ptFontInfo;
        case Font_e::e_FontCount:
            return nullptr;
    }
    return nullptr;
}
