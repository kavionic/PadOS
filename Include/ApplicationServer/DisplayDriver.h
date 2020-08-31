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

#pragma once

#include <vector>

#include <Ptr/PtrTarget.h>
#include <Math/Point.h>
#include <Math/Rect.h>
#include <GUI/Font.h>
#include <GUI/GUIDefines.h>
#include <ApplicationServer/Font.h>


class   Glyph;
class   MousePtr;
class   SrvSprite;

namespace os
{
class Color;
class SrvBitmap;


struct ScreenMode
{
    ScreenMode() {}
    ScreenMode(const IPoint& resolution, int bytesPerLine, ColorSpace colorSpace) : m_Resolution(resolution), m_BytesPerLine(bytesPerLine), m_ColorSpace(colorSpace) {}
    IPoint      m_Resolution;
    int         m_BytesPerLine = 0;
    ColorSpace m_ColorSpace = ColorSpace::NO_COLOR_SPACE;
};


class DisplayDriver : public PtrTarget
{
public:
    static constexpr int CHARACTER_SPACING = 3;

    DisplayDriver();
    virtual     ~DisplayDriver();

    virtual bool            Open() = 0;
    virtual void            Close() = 0;

    virtual Ptr<SrvBitmap>  GetScreenBitmap() = 0;

    virtual int             GetScreenModeCount() = 0;
    virtual bool            GetScreenModeDesc(size_t index, ScreenMode& outMode) = 0;
    virtual bool            SetScreenMode(const IPoint& resolution, ColorSpace colorSpace, float refreshRate) = 0;

    virtual IPoint          GetResolution() = 0;
    virtual int             GetBytesPerLine() = 0;
    virtual int             GetFramebufferOffset();
    virtual ColorSpace     GetColorSpace() = 0;
    virtual void            SetColor(size_t index, const Color& color) = 0;

    //    virtual void  SetCursorBitmap(mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight );

    virtual void    MouseOn();
    virtual void    MouseOff();
    virtual void    SetMousePos(IPoint cNewPos);
//    virtual bool    IntersectWithMouse(const IRect& cRect) = 0;

    virtual void    WritePixel(SrvBitmap* bitmap, const IPoint& pos, Color color);
    virtual void    DrawLine(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2, const Color& color, DrawingMode mode);
    virtual void    FillRect(SrvBitmap* bitmap, const IRect& rect, const Color& color);
    virtual void    CopyRect(SrvBitmap* dstBitmap, SrvBitmap* srcBitmap, Color bgColor, Color fgColor, const IRect& srcRect, const IPoint& dstPos, DrawingMode mode);
    //    virtual void  BltBitmapMask(SrvBitmap* pcDstBitMap, SrvBitmap* pcSrcBitMap, const Color& sHighColor, const Color& sLowColor, IRect cSrcRect, IPoint cDstPos);

    virtual void    FillCircle(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& center, int32_t radius, const Color& color, DrawingMode mode);

    virtual uint32_t WriteString(os::SrvBitmap* bitmap, const os::IPoint& position, const char* string, size_t strLength, const os::IRect& clipRect, os::Color colorBg, os::Color colorFg, Font_e fontID);

    //    virtual bool  RenderGlyph(SrvBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, uint32_t* anPalette);
    //    virtual bool  RenderGlyph(SrvBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, const Color& sFgColor);

    float   GetFontHeight(Font_e fontID) const;
    float   GetStringWidth(Font_e fontID, const char* string, size_t length) const;
    size_t  GetStringLength(Font_e fontID, const char* string, size_t length, float width, bool includeLast);

    const FONT_INFO* GetFontDesc(Font_e fontID) const;

    static Color   GetPaletteEntry(uint8_t index);
private:
    void    FillBlit8(uint8_t* pDst, int nMod, int W, int H, uint8_t nColor);
    void    FillBlit16(uint16_t* pDst, int nMod, int W, int H, uint16_t nColor);
    void    FillBlit24(uint8_t* pDst, int nMod, int W, int H, uint32_t nColor);
    void    FillBlit32(uint32_t* pDst, int nMod, int W, int H, uint32_t nColor);

    //    SrvBitmap*      m_pcMouseImage;
    //    SrvSprite*          m_pcMouseSprite;
    IPoint        m_cMousePos;
    IPoint        m_cCursorHotSpot;
};

} // namespace os
