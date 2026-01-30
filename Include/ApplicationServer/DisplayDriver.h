// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include <vector>

#include <Ptr/PtrTarget.h>
#include <Math/Point.h>
#include <Math/Rect.h>
#include <GUI/Font.h>
#include <GUI/GUIDefines.h>
#include <ApplicationServer/Font.h>


class   Glyph;
class   SrvSprite;

struct PColor;

#define RAS_OFFSET8( ptr, x, y, bpl) (((uint8_t*)(ptr)) + (x) + (y) * (bpl))
#define RAS_OFFSET16(ptr, x, y, bpl) ((uint16_t*)(((uint8_t*)(ptr)) + (x*2) + (y) * (bpl)))
#define RAS_OFFSET32(ptr, x, y, bpl) ((uint32_t*)(((uint8_t*)(ptr)) + (x*4) + (y) * (bpl)))


class PSrvBitmap;


struct PScreenMode
{
    PScreenMode() {}
    PScreenMode(const PIPoint& resolution, int bytesPerLine, PEColorSpace colorSpace) : m_Resolution(resolution), m_BytesPerLine(bytesPerLine), m_ColorSpace(colorSpace) {}
    PIPoint      m_Resolution;
    size_t      m_BytesPerLine = 0;
    PEColorSpace m_ColorSpace = PEColorSpace::NO_COLOR_SPACE;
};


class PDisplayDriver : public PtrTarget
{
public:
    static constexpr int CHARACTER_SPACING = 3;

    PDisplayDriver();
    virtual     ~PDisplayDriver();

    virtual bool            Open() = 0;
    virtual void            Close() = 0;
    virtual void            PowerLost(bool hasPower) = 0;
    virtual Ptr<PSrvBitmap>  GetScreenBitmap() = 0;

    virtual int             GetScreenModeCount() = 0;
    virtual bool            GetScreenModeDesc(size_t index, PScreenMode& outMode) = 0;
    virtual bool            SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate) = 0;

    virtual PIPoint          GetResolution() = 0;
    virtual int             GetBytesPerLine() = 0;
    virtual int             GetFramebufferOffset();
    virtual PEColorSpace     GetColorSpace() = 0;
    virtual void            SetColor(size_t index, PColor color) = 0;

    //    virtual void  SetCursorBitmap(mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight );

    virtual void    MouseOn();
    virtual void    MouseOff();
    virtual void    SetMousePos(PIPoint cNewPos);
//    virtual bool    IntersectWithMouse(const IRect& cRect) = 0;

    virtual void    WritePixel(PSrvBitmap* bitmap, const PIPoint& pos, PColor color);
    virtual void    DrawLine(PSrvBitmap* bitmap, const PIRect& clipRect, const PIPoint& pos1, const PIPoint& pos2, const PColor& color, PDrawingMode mode);
    virtual void    FillRect(PSrvBitmap* bitmap, const PIRect& rect, const PColor& color);
    virtual void    CopyRect(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, PColor bgColor, PColor fgColor, const PIRect& srcRect, const PIPoint& dstPos, PDrawingMode mode);
    virtual void    ScaleRect(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, PColor bgColor, PColor fgColor, const PIRect& srcOrigRect, const PIRect& dstOrigRect, const PRect& srcRect, const PIRect& dstRect, PDrawingMode mode);
    //    virtual void  BltBitmapMask(SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, const Color& sHighColor, const Color& sLowColor, IRect cSrcRect, IPoint cDstPos);

    virtual void    FillCircle(PSrvBitmap* bitmap, const PIRect& clipRect, const PIPoint& center, int32_t radius, const PColor& color, PDrawingMode mode);

    virtual uint32_t WriteString(PSrvBitmap* bitmap, const PIPoint& position, const char* string, size_t strLength, const PIRect& clipRect, PColor colorBg, PColor colorFg, PFontID fontID);

    //    virtual bool  RenderGlyph(SrvBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, uint32_t* anPalette);
    //    virtual bool  RenderGlyph(SrvBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, const Color& sFgColor);

    float   GetFontHeight(PFontID fontID) const;
    float   GetStringWidth(PFontID fontID, const char* string, size_t length) const;
    size_t  GetStringLength(PFontID fontID, const char* string, size_t length, float width, bool includeLast);

    const FONT_INFO* GetFontDesc(PFontID fontID) const;

    static PColor   GetPaletteEntry(uint8_t index);
private:
    void    FillBlit8(uint8_t* pDst, int nMod, int W, int H, uint8_t nColor);
    void    FillBlit16(uint16_t* pDst, int nMod, int W, int H, uint16_t nColor);
    void    FillBlit24(uint8_t* pDst, int nMod, int W, int H, uint32_t nColor);
    void    FillBlit32(uint32_t* pDst, int nMod, int W, int H, uint32_t nColor);

    //    SrvBitmap*      m_pcMouseImage;
    //    SrvSprite*          m_pcMouseSprite;
    PIPoint        m_cMousePos;
    PIPoint        m_cCursorHotSpot;
};
