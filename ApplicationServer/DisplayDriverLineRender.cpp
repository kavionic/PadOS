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

#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>

#include <GUI/Bitmap.h>
#include <GUI/Color.h>
#include <GUI/Region.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void draw_line16(SrvBitmap* pcBitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2, uint16_t nColor)
{
    int nODeltaX = abs(pos2.x - pos1.x);
    int nODeltaY = abs(pos2.y - pos1.y);

    uint16_t* pRaster;
    int   nModulo = pcBitmap->m_BytesPerLine;

    IPoint clippedPos1 = pos1;
    IPoint clippedPos2 = pos2;

    if (!Region::ClipLine(clipRect, &clippedPos1, &clippedPos2)) {
        return;
    }
    int nDeltaX = abs(clippedPos2.x - clippedPos1.x);
    int nDeltaY = abs(clippedPos2.y - clippedPos1.y);

    if (nODeltaX > nODeltaY)
    {
        int dinc1 = nODeltaY << 1;
        int dinc2 = (nODeltaY - nODeltaX) << 1;
        int d = dinc1 - nODeltaX;

        int nYStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaY * dinc2) + ((nClipDeltaX - nClipDeltaY) * dinc1));
        }

        if (clippedPos1.y > clippedPos2.y) {
            nYStep = -nModulo;
        } else {
            nYStep = nModulo;
        }
        if (clippedPos1.x > clippedPos2.x) {
            nYStep = -nYStep;
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos2.x * 2 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos1.x * 2 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaX; ++i)
        {
            *pRaster = nColor;
            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint16_t*)(((uint8_t*)pRaster) + nYStep);
            }
            pRaster++;
        }
    }
    else
    {
        int dinc1 = nODeltaX << 1;
        int d = dinc1 - nODeltaY;
        int dinc2 = (nODeltaX - nODeltaY) << 1;
        int nXStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaX * dinc2) + ((nClipDeltaY - nClipDeltaX) * dinc1));
        }


        if (clippedPos1.x > clippedPos2.x) {
            nXStep = -sizeof(uint16_t);
        } else {
            nXStep = sizeof(uint16_t);
        }
        if (clippedPos1.y > clippedPos2.y) {
            nXStep = -nXStep;
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos2.x * 2 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos1.x * 2 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaY; ++i)
        {
            *pRaster = nColor;
            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint16_t*)(((uint8_t*)pRaster) + nXStep);
            }
            pRaster = (uint16_t*)(((uint8_t*)pRaster) + nModulo);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void invert_line16(SrvBitmap* pcBitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2)
{
    int nODeltaX = abs(pos2.x - pos1.x);
    int nODeltaY = abs(pos2.y - pos1.y);
    uint16_t* pRaster;
    int   nModulo = pcBitmap->m_BytesPerLine;

    IPoint clippedPos1 = pos1;
    IPoint clippedPos2 = pos2;

    if (!Region::ClipLine(clipRect, &clippedPos1, &clippedPos2)) {
        return;
    }
    int nDeltaX = abs(clippedPos2.x - clippedPos1.x);
    int nDeltaY = abs(clippedPos2.y - clippedPos1.y);

    if (nODeltaX > nODeltaY)
    {
        int dinc1 = nODeltaY << 1;
        int dinc2 = (nODeltaY - nODeltaX) << 1;
        int d = dinc1 - nODeltaX;

        int nYStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y)
        {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaY * dinc2) + ((nClipDeltaX - nClipDeltaY) * dinc1));
        }

        if (clippedPos1.y > clippedPos2.y) {
            nYStep = -nModulo;
        } else {
            nYStep = nModulo;
        }
        if (clippedPos1.x > clippedPos2.x) {
            nYStep = -nYStep;
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos2.x * 2 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos1.x * 2 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaX; ++i)
        {
            Color color = Color::FromRGB16(*pRaster);
            *pRaster = color.GetInverted().GetColor16();

            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint16_t*)(((uint8_t*)pRaster) + nYStep);
            }
            pRaster++;
        }
    }
    else
    {
        int dinc1 = nODeltaX << 1;
        int d = dinc1 - nODeltaY;
        int dinc2 = (nODeltaX - nODeltaY) << 1;
        int nXStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaX * dinc2) + ((nClipDeltaY - nClipDeltaX) * dinc1));
        }


        if (clippedPos1.x > clippedPos2.x) {
            nXStep = -sizeof(uint16_t);
        } else {
            nXStep = sizeof(uint16_t);
        }
        if (clippedPos1.y > clippedPos2.y) {
            nXStep = -nXStep;
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos2.x * 2 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos1.x * 2 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaY; ++i)
        {
            Color color = Color::FromRGB16(*pRaster);
            *pRaster = color.GetInverted().GetColor16();

            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint16_t*)(((uint8_t*)pRaster) + nXStep);
            }
            pRaster = (uint16_t*)(((uint8_t*)pRaster) + nModulo);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void invert_line15(SrvBitmap* pcBitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2)
{
    int nODeltaX = abs(pos2.x - pos1.x);
    int nODeltaY = abs(pos2.y - pos1.y);
    uint16_t* pRaster;
    int   nModulo = pcBitmap->m_BytesPerLine;

    IPoint clippedPos1 = pos1;
    IPoint clippedPos2 = pos2;

    if (!Region::ClipLine(clipRect, &clippedPos1, &clippedPos2)) {
        return;
    }
    int nDeltaX = abs(clippedPos2.x - clippedPos1.x);
    int nDeltaY = abs(clippedPos2.y - clippedPos1.y);

    if (nODeltaX > nODeltaY)
    {
        int dinc1 = nODeltaY << 1;
        int dinc2 = (nODeltaY - nODeltaX) << 1;
        int d = dinc1 - nODeltaX;

        int nYStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaY * dinc2) + ((nClipDeltaX - nClipDeltaY) * dinc1));
        }

        if (clippedPos1.y > clippedPos2.y) {
            nYStep = -nModulo;
        } else {
            nYStep = nModulo;
        }
        if (clippedPos1.x > clippedPos2.x) {
            nYStep = -nYStep;
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos2.x * 2 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos1.x * 2 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaX; ++i)
        {
            Color color = Color::FromRGB15(*pRaster);
            *pRaster = color.GetInverted().GetColor15();

            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint16_t*)(((uint8_t*)pRaster) + nYStep);
            }
            pRaster++;
        }
    }
    else
    {
        int dinc1 = nODeltaX << 1;
        int d = dinc1 - nODeltaY;
        int dinc2 = (nODeltaX - nODeltaY) << 1;
        int nXStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaX * dinc2) + ((nClipDeltaY - nClipDeltaX) * dinc1));
        }


        if (clippedPos1.x > clippedPos2.x) {
            nXStep = -sizeof(uint16_t);
        } else {
            nXStep = sizeof(uint16_t);
        }
        if (clippedPos1.y > clippedPos2.y) {
            nXStep = -nXStep;
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos2.x * 2 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint16_t*)(pcBitmap->m_Raster + clippedPos1.x * 2 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaY; ++i)
        {
            Color color = Color::FromRGB15(*pRaster);
            *pRaster = color.GetInverted().GetColor15();

            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint16_t*)(((uint8_t*)pRaster) + nXStep);
            }
            pRaster = (uint16_t*)(((uint8_t*)pRaster) + nModulo);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void draw_line32(SrvBitmap* pcBitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2, uint32_t nColor)
{
    int nODeltaX = abs(pos2.x - pos1.x);
    int nODeltaY = abs(pos2.y - pos1.y);
    uint32_t* pRaster;
    int   nModulo = pcBitmap->m_BytesPerLine;

    IPoint clippedPos1 = pos1;
    IPoint clippedPos2 = pos2;

    if (!Region::ClipLine(clipRect, &clippedPos1, &clippedPos2)) {
        return;
    }
    int nDeltaX = abs(clippedPos2.x - clippedPos1.x);
    int nDeltaY = abs(clippedPos2.y - clippedPos1.y);

    if (nODeltaX > nODeltaY)
    {
        int dinc1 = nODeltaY << 1;
        int dinc2 = (nODeltaY - nODeltaX) << 1;
        int d = dinc1 - nODeltaX;

        int nYStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaY * dinc2) + ((nClipDeltaX - nClipDeltaY) * dinc1));
        }

        if (clippedPos1.y > clippedPos2.y) {
            nYStep = -nModulo;
        } else {
            nYStep = nModulo;
        }
        if (clippedPos1.x > clippedPos2.x) {
            nYStep = -nYStep;
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos2.x * 4 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos1.x * 4 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaX; ++i)
        {
            *pRaster = nColor;
            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint32_t*)(((uint8_t*)pRaster) + nYStep);
            }
            pRaster++;
        }
    }
    else
    {
        int dinc1 = nODeltaX << 1;
        int d = dinc1 - nODeltaY;
        int dinc2 = (nODeltaX - nODeltaY) << 1;
        int nXStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaX * dinc2) + ((nClipDeltaY - nClipDeltaX) * dinc1));
        }

        if (clippedPos1.x > clippedPos2.x) {
            nXStep = -sizeof(uint32_t);
        } else {
            nXStep = sizeof(uint32_t);
        }
        if (clippedPos1.y > clippedPos2.y) {
            nXStep = -nXStep;
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos2.x * 4 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos1.x * 4 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaY; ++i)
        {
            *pRaster = nColor;
            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint32_t*)(((uint8_t*)pRaster) + nXStep);
            }
            pRaster = (uint32_t*)(((uint8_t*)pRaster) + nModulo);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void invert_line32(SrvBitmap* pcBitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2)
{
    int nODeltaX = abs(pos2.x - pos1.x);
    int nODeltaY = abs(pos2.y - pos1.y);
    uint32_t* pRaster;
    int   nModulo = pcBitmap->m_BytesPerLine;

    IPoint clippedPos1 = pos1;
    IPoint clippedPos2 = pos2;

    if (!Region::ClipLine(clipRect, &clippedPos1, &clippedPos2)) {
        return;
    }
    int nDeltaX = abs(clippedPos2.x - clippedPos1.x);
    int nDeltaY = abs(clippedPos2.y - clippedPos1.y);

    if (nODeltaX > nODeltaY)
    {
        int dinc1 = nODeltaY << 1;
        int dinc2 = (nODeltaY - nODeltaX) << 1;
        int d = dinc1 - nODeltaX;

        int nYStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaY * dinc2) + ((nClipDeltaX - nClipDeltaY) * dinc1));
        }

        if (clippedPos1.y > clippedPos2.y) {
            nYStep = -nModulo;
        } else {
            nYStep = nModulo;
        }
        if (clippedPos1.x > clippedPos2.x) {
            nYStep = -nYStep;
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos2.x * 4 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos1.x * 4 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaX; ++i)
        {
            Color color = Color::FromRGB32A(*pRaster);
            *pRaster = color.GetInverted().GetColor32();
            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint32_t*)(((uint8_t*)pRaster) + nYStep);
            }
            pRaster++;
        }
    }
    else
    {
        int dinc1 = nODeltaX << 1;
        int d = dinc1 - nODeltaY;
        int dinc2 = (nODeltaX - nODeltaY) << 1;
        int nXStep;

        if (pos1.x != clippedPos1.x || pos1.y != clippedPos1.y) {
            int nClipDeltaX = abs(clippedPos1.x - pos1.x);
            int nClipDeltaY = abs(clippedPos1.y - pos1.y);
            d += ((nClipDeltaX * dinc2) + ((nClipDeltaY - nClipDeltaX) * dinc1));
        }


        if (clippedPos1.x > clippedPos2.x) {
            nXStep = -sizeof(uint32_t);
        } else {
            nXStep = sizeof(uint32_t);
        }
        if (clippedPos1.y > clippedPos2.y) {
            nXStep = -nXStep;
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos2.x * 4 + nModulo * clippedPos2.y);
        } else {
            pRaster = (uint32_t*)(pcBitmap->m_Raster + clippedPos1.x * 4 + nModulo * clippedPos1.y);
        }

        for (int i = 0; i <= nDeltaY; ++i)
        {
            Color color = Color::FromRGB32A(*pRaster);
            *pRaster = color.GetInverted().GetColor32();

            if (d < 0) {
                d += dinc1;
            } else {
                d += dinc2;
                pRaster = (uint32_t*)(((uint8_t*)pRaster) + nXStep);
            }
            pRaster = (uint32_t*)(((uint8_t*)pRaster) + nModulo);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DisplayDriver::DrawLine(SrvBitmap* psBitmap, const IRect& cClipRect, const IPoint& cPnt1, const IPoint& cPnt2, const Color& sColor, drawing_mode nMode)
{
    switch (nMode)
    {
        case DM_COPY:
        case DM_OVER:
        default:
            switch (psBitmap->m_ColorSpace)
            {
                case CS_RGB15:
                    draw_line16(psBitmap, cClipRect, cPnt1, cPnt2, sColor.GetColor15());
                    break;
                case CS_RGB16:
                    draw_line16(psBitmap, cClipRect, cPnt1, cPnt2, sColor.GetColor16());
                    break;
                case CS_RGB32:
                    draw_line32(psBitmap, cClipRect, cPnt1, cPnt2, sColor.GetColor32());
                    break;
                default:
                    printf("DisplayDriver::DrawLine() unknown color space %d\n", psBitmap->m_ColorSpace);
            }
            break;
        case DM_INVERT:
            switch (psBitmap->m_ColorSpace)
            {
                case CS_RGB15:
                    invert_line15(psBitmap, cClipRect, cPnt1, cPnt2);
                    break;
                case CS_RGB16:
                    invert_line16(psBitmap, cClipRect, cPnt1, cPnt2);
                    break;
                case CS_RGB32:
                    invert_line32(psBitmap, cClipRect, cPnt1, cPnt2);
                    break;
                default:
                    printf("DisplayDriver::DrawLine() unknown color space %d can't invert\n", psBitmap->m_ColorSpace);
            }
            break;
    }
}
