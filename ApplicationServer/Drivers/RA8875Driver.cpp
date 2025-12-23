// This file is part of PadOS.
//
// Copyright (C) 2014-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 16.01.2014 22:21

#include <ApplicationServer/Drivers/RA8875Driver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <ApplicationServer/BlitterUtils.h>

#include <GUI/Color.h>
#include <Utils/UTF8Utils.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

RA8875Driver::RA8875Driver(const RA8875DriverParameters& config)
    : m_Registers((LCDRegisters*)config.Registers)
    , m_PinLCDResetID(config.PinLCDReset)
    , m_PinTouchpadResetID(config.PinTouchpadReset)
    , m_PinBacklightControlID(config.PinBacklightControl)
{
    m_ScreenBitmap = ptr_new<SrvBitmap>(IPoint(0, 0), EColorSpace::RGB16);
    m_ScreenBitmap->m_VideoMem = true;
    m_ScreenBitmap->m_Driver = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875Driver::Open()
{
    if (m_PinLCDResetID != DigitalPinID::None)
    {
        digital_pin_write(m_PinLCDResetID, true);
        digital_pin_set_direction(m_PinLCDResetID, DigitalPinDirection_e::Out);
    }
    digital_pin_write(m_PinBacklightControlID, true);
    digital_pin_set_direction(m_PinBacklightControlID, DigitalPinDirection_e::Out);

    Reset();
    m_ScreenBitmap->m_Size = GetResolution();

    IRect screenFrame(IPoint(0, 0), GetResolution());

    SetWindow(screenFrame);
    FillRect(ptr_raw_pointer_cast(m_ScreenBitmap), screenFrame, Color::FromRGB32A(0));

    //    WriteCommand(RA8875_P1CR, 0x00);  // PWM setting
    //    WriteCommand(RA8875_P2CR, 0x00);  // open PWM
    /*
        WriteCommand(RA8875_P1CR, 0x80);  // PWM setting
    //    WriteCommand(RA8875_P2CR, 0x81);  // open PWM
        WriteCommand(RA8875_P1DCR, 0xff); // Brightness parameter 0xff-0x00
        WriteCommand(RA8875_P2DCR, 0xff); // Brightness parameter 0xff-0x00
        */
    WriteCommand(RA8875_PWRR, RA8875_PWRR_DISPLAY_ON_bm);
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::Close()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::PowerLost(bool hasPower)
{
    digital_pin_write(m_PinBacklightControlID, hasPower);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<os::SrvBitmap> RA8875Driver::GetScreenBitmap()
{
    return m_ScreenBitmap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int RA8875Driver::GetScreenModeCount()
{
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875Driver::GetScreenModeDesc(size_t index, ScreenMode& outMode)
{
    if (index == 0)
    {
        outMode = ScreenMode(IPoint(800, 480), 800 * 2, EColorSpace::RGB16);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875Driver::SetScreenMode(const IPoint& resolution, EColorSpace colorSpace, float refreshRate)
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IPoint RA8875Driver::GetResolution()
{
    return IPoint(800, 480);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int RA8875Driver::GetBytesPerLine()
{
    return GetResolution().x * 2;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

EColorSpace RA8875Driver::GetColorSpace()
{
    return EColorSpace::RGB16;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::SetColor(size_t index, Color color)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::WritePixel(SrvBitmap* bitmap, const IPoint& pos, Color color)
{
    if (bitmap->m_VideoMem)
    {
        MemoryWrite_Position(pos.x, pos.y);
        WriteData(color.GetColor16());
    }
    else
    {
        DisplayDriver::WritePixel(bitmap, pos, color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::DrawLine(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2, const Color& color, DrawingMode mode)
{
    if (bitmap->m_VideoMem)
    {
        if (pos1 == pos2)
        {
            WritePixel(bitmap, pos1, color);
            return;
        }
        SetWindow(clipRect);
        WaitBlitter();

        SetFgColor(color.GetColor16());

        WriteCommand(RA8875_DLHSR0, RA8875_DLHSR1, uint16_t(pos1.x));
        WriteCommand(RA8875_DLVSR0, RA8875_DLVSR1, uint16_t(pos1.y));
        WriteCommand(RA8875_DLHER0, RA8875_DLHER1, uint16_t(pos2.x));
        WriteCommand(RA8875_DLVER0, RA8875_DLVER1, uint16_t(pos2.y));

        WriteCommand(RA8875_DCR, RA8875_DCR_LINE_SQR_TRI_bm);
    }
    else
    {
        DisplayDriver::DrawLine(bitmap, clipRect, pos1, pos2, color, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::FillRect(SrvBitmap* bitmap, const IRect& rect, const Color& color)
{
    if (bitmap->m_VideoMem)
    {
        WaitBlitter();
        SetWindow(IRect(IPoint(0), GetResolution()));

        if (rect.left != (rect.right - 1) || rect.top != (rect.bottom - 1))
        {
            SetFgColor(color.GetColor16());
            WriteCommand(RA8875_DLHSR0, RA8875_DLHSR1, uint16_t(rect.left));
            WriteCommand(RA8875_DLVSR0, RA8875_DLVSR1, uint16_t(rect.top));
            WriteCommand(RA8875_DLHER0, RA8875_DLHER1, uint16_t(rect.right - 1));
            WriteCommand(RA8875_DLVER0, RA8875_DLVER1, uint16_t(rect.bottom - 1));

            WriteCommand(RA8875_DCR, RA8875_DCR_FILL_bm | RA8875_DCR_LINE_SQR_TRI_bm | RA8875_DCR_SQUARE_bm);
        }
        else
        {
            WritePixel(bitmap, rect.TopLeft(), color);
        }
    }
    else
    {
        DisplayDriver::FillRect(bitmap, rect, color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::CopyRect(SrvBitmap* dstBitmap, SrvBitmap* srcBitmap, Color bgColor, Color fgColor, const IRect& srcRect, const IPoint& dstPosIn, DrawingMode mode)
{
    if (dstBitmap->m_VideoMem && srcBitmap->m_VideoMem)
    {
        WaitBlitter();

        uint8_t ctrl;
        IPoint srcPos;
        IPoint dstPos;

        if ((dstPosIn.y > srcRect.top) || (dstPosIn.y == srcRect.top && dstPosIn.x > srcRect.left))
        {
            ctrl = RA8875_BTE_OP_MOVE_NEG_ROP;
            srcPos = IPoint(srcRect.right - 1, srcRect.bottom - 1);
            dstPos = dstPosIn + IPoint(srcRect.Width() - 1, srcRect.Height() - 1);
        }
        else
        {
            ctrl = RA8875_BTE_OP_MOVE_POS_ROP;
            srcPos = IPoint(srcRect.left, srcRect.top);
            dstPos = dstPosIn;
        }
        WriteCommand(RA8875_HSBE0, RA8875_HSBE1, uint16_t(srcPos.x));
        WriteCommand(RA8875_VSBE0, RA8875_VSBE1, uint16_t(srcPos.y));
        WriteCommand(RA8875_HDBE0, RA8875_HDBE1, uint16_t(dstPos.x));
        WriteCommand(RA8875_VDBE0, RA8875_VDBE1, uint16_t(dstPos.y));

        WriteCommand(RA8875_BEWR0, RA8875_BEWR1, uint16_t(srcRect.Width()));
        WriteCommand(RA8875_BEHR0, RA8875_BEHR1, uint16_t(srcRect.Height()));

        WriteCommand(RA8875_BECR1, ctrl | RA8875_BTE_ROP_S);
        WriteCommand(RA8875_BECR0, RA8875_BECR0_SRC_BLOCK | RA8875_BECR0_DST_BLOCK | RA8875_BECR0_ENABLE_bm);
    }
    else if (dstBitmap->m_VideoMem)
    {
        WaitBlitter();

        WriteCommand(RA8875_HDBE0, RA8875_HDBE1, uint16_t(dstPosIn.x));
        WriteCommand(RA8875_VDBE0, RA8875_VDBE1, uint16_t(dstPosIn.y));
        WriteCommand(RA8875_BEWR0, RA8875_BEWR1, uint16_t(srcRect.Width()));
        WriteCommand(RA8875_BEHR0, RA8875_BEHR1, uint16_t(srcRect.Height()));

        bool colorKeyed = false;
        switch (mode)
        {
            case DrawingMode::Copy:
                WriteCommand(RA8875_BECR1, RA8875_BTE_OP_WRITE_ROP | RA8875_BTE_ROP_S);
                break;
            case DrawingMode::Overlay:
                WriteCommand(RA8875_BECR1, RA8875_BTE_OP_WRITE_TRAN | RA8875_BTE_ROP_S);
                colorKeyed = true;
                break;
            case DrawingMode::Invert:
                break;
            case DrawingMode::Erase:
                break;
            case DrawingMode::Blend:
                break;
            case DrawingMode::Add:
                break;
            case DrawingMode::Subtract:
                break;
            case DrawingMode::Min:
                break;
            case DrawingMode::Max:
                break;
            case DrawingMode::Select:
                break;
            default:
                break;
        }
        if (colorKeyed)
        {
            if (srcBitmap->m_ColorSpace != EColorSpace::MONO1) {
                SetFgColor(TransparentColors::RGB16);
            } else {
                SetFgColor(uint16_t(~fgColor.GetColor16()));
            }
        }
        WriteCommand(RA8875_BECR0, RA8875_BECR0_SRC_BLOCK | RA8875_BECR0_DST_BLOCK | RA8875_BECR0_ENABLE_bm);

        auto writePixel16 = [this](uint16_t pixel16) PALWAYS_INLINE
        {
            WaitMemory();
            WriteData(pixel16);
        };

        switch (srcBitmap->m_ColorSpace)
        {
            case EColorSpace::MONO1:
            {
                const uint32_t  wordsPerLine = srcBitmap->m_BytesPerLine / sizeof(uint32_t);
                const uint32_t* src = reinterpret_cast<const uint32_t*>(srcBitmap->m_Raster + srcRect.top * srcBitmap->m_BytesPerLine);

                const uint16_t bgColor16 = (colorKeyed) ? uint16_t(~fgColor.GetColor16()) : bgColor.GetColor16();
                const uint16_t fgColor16 = fgColor.GetColor16();

                int32_t x = srcRect.left;
                auto readPixel = [&x, &src]() PALWAYS_INLINE{ int32_t curX = x++; return (src[curX / 32] & (1 << (31 - (curX & 31)))) != 0; };
                auto writePixel = [this, bgColor16, fgColor16](bool pixel) PALWAYS_INLINE
                    {
                        WaitMemory();
                        if (pixel) {
                            WriteData(fgColor16);
                        } else {
                            WriteData(bgColor16);
                        }
                    };
                auto nextLine = [&x, &src, left = srcRect.left, wordsPerLine]() PALWAYS_INLINE { src += wordsPerLine; x = left; };
                BeginWriteData();

                BlitterUtils::CopyBitmap(readPixel, writePixel, nextLine, srcRect);
                break;
            }
            case EColorSpace::CMAP8:
            {
                const int32_t srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width();
                const uint8_t* src = srcBitmap->m_Raster + srcRect.top * srcBitmap->m_BytesPerLine;

                auto readPixel = [this, &src]() PALWAYS_INLINE{ return GetPaletteEntry(*src++).GetColor16(); };
                auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

                BeginWriteData();
                BlitterUtils::CopyBitmap(readPixel, writePixel16, nextLine, srcRect);
                break;
            }
            case EColorSpace::RGB15:
            {
                const int32_t srcModulo = srcBitmap->m_BytesPerLine / 2 - srcRect.Width();
                const uint16_t* src = RAS_OFFSET16(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);

                auto readPixel = [&src]() PALWAYS_INLINE { return Color::FromRGB15(*src++).GetColor16(); };
                auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

                BeginWriteData();
                BlitterUtils::CopyBitmap(readPixel, writePixel16, nextLine, srcRect);
                break;
            }
            case EColorSpace::RGB16:
            {
                const int32_t srcModulo = srcBitmap->m_BytesPerLine / 2 - srcRect.Width();
                const uint16_t* src = RAS_OFFSET16(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);

                auto readPixel = [&src]() PALWAYS_INLINE { return *src++; };
                auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

                BeginWriteData();
                BlitterUtils::CopyBitmap(readPixel, writePixel16, nextLine, srcRect);
                break;
            }
            case EColorSpace::RGB32:
            case EColorSpace::RGBA32:
            {
                const int32_t srcModulo = srcBitmap->m_BytesPerLine / 4 - srcRect.Width();
                const uint32_t* src = RAS_OFFSET32(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);

                auto readPixelRGB = [&src]() PALWAYS_INLINE { return Color::FromRGB32(*src++).GetColor16(); };
                auto readPixelRGBA = [&src, bgColor]() PALWAYS_INLINE
                    {
                        const Color pixel32 = Color::FromRGB32A(*src++);
                        return Color::Blend16(pixel32, bgColor);
                    };
                auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

                BeginWriteData();
                if (srcBitmap->m_ColorSpace == EColorSpace::RGB32) {
                    BlitterUtils::CopyBitmap(readPixelRGB, writePixel16, nextLine, srcRect);
                } else {
                    BlitterUtils::CopyBitmap(readPixelRGBA, writePixel16, nextLine, srcRect);
                }
                break;
            }
            default:
                break;
        }
        WriteCommand(RA8875_BECR0, 0);
    }
    else if (!srcBitmap->m_VideoMem && !dstBitmap->m_VideoMem)
    {
        DisplayDriver::CopyRect(dstBitmap, srcBitmap, bgColor, fgColor, srcRect, dstPosIn, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::ScaleRect(SrvBitmap* dstBitmap, SrvBitmap* srcBitmap, Color bgColor, Color fgColor, const IRect& srcOrigRect, const IRect& dstOrigRect, const Rect& srcRect, const IRect& dstRect, DrawingMode mode)
{
    if (dstBitmap->m_VideoMem && srcBitmap->m_VideoMem)
    {
        WaitBlitter();
    }
    else if (dstBitmap->m_VideoMem)
    {
        WaitBlitter();

        WriteCommand(RA8875_HDBE0, RA8875_HDBE1, uint16_t(dstRect.left));
        WriteCommand(RA8875_VDBE0, RA8875_VDBE1, uint16_t(dstRect.top));
        WriteCommand(RA8875_BEWR0, RA8875_BEWR1, uint16_t(dstRect.Width()));
        WriteCommand(RA8875_BEHR0, RA8875_BEHR1, uint16_t(dstRect.Height()));

        bool colorKeyed = false;
        switch (mode)
        {
            case DrawingMode::Copy:
                WriteCommand(RA8875_BECR1, RA8875_BTE_OP_WRITE_ROP | RA8875_BTE_ROP_S);
                break;
            case DrawingMode::Overlay:
                WriteCommand(RA8875_BECR1, RA8875_BTE_OP_WRITE_TRAN | RA8875_BTE_ROP_S);
                colorKeyed = true;
                break;
            case DrawingMode::Invert:
                break;
            case DrawingMode::Erase:
                break;
            case DrawingMode::Blend:
                break;
            case DrawingMode::Add:
                break;
            case DrawingMode::Subtract:
                break;
            case DrawingMode::Min:
                break;
            case DrawingMode::Max:
                break;
            case DrawingMode::Select:
                break;
            default:
                break;
        }
        if (colorKeyed)
        {
            if (srcBitmap->m_ColorSpace != EColorSpace::MONO1) {
                SetFgColor(TransparentColors::RGB16);
            }
            else {
                SetFgColor(uint16_t(~fgColor.GetColor16()));
            }
        }
        WriteCommand(RA8875_BECR0, RA8875_BECR0_SRC_BLOCK | RA8875_BECR0_DST_BLOCK | RA8875_BECR0_ENABLE_bm);
        switch (srcBitmap->m_ColorSpace)
        {
            case EColorSpace::RGB32:
            case EColorSpace::RGBA32:
            {
                const uint32_t* const src = reinterpret_cast<const uint32_t*>(srcBitmap->m_Raster);
                const uint32_t wordsPerLine = srcBitmap->m_BytesPerLine / 4;

                BeginWriteData();

                auto readPixel = [src, wordsPerLine](int32_t x, int32_t y) PALWAYS_INLINE { return Color::FromRGB32A(src[y * wordsPerLine + x]); };
                auto writePixel = [this, bgColor, src, wordsPerLine](int32_t x, int32_t y, const Color& pixel) PALWAYS_INLINE
                    {
                        const uint16_t pixel16 = Color::Blend16(pixel, bgColor);

                        WaitMemory();
                        WriteData(pixel16);
                    };

                BlitterUtils::ScaleBitmapBilinear(readPixel, writePixel, srcOrigRect, dstOrigRect, srcRect, dstRect);
                break;
            }
            default:
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void RA8875Driver::FillCircle(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& center, int32_t radius, const Color& color, DrawingMode mode)
//{
//    SetWindow(clipRect);
//
//    WaitBlitter();
//
//    SetFgColor(color.GetColor16());
//
//    WriteCommand(RA8875_DCHR0);
//    WriteData(center.x & 0xff);
//
//    WriteCommand(RA8875_DCHR1);
//    WriteData(center.x >> 8);
//
//    WriteCommand(RA8875_DCVR0);
//    WriteData(center.y & 0xff);
//
//    WriteCommand(RA8875_DCVR1);
//    WriteData(center.y >> 8);
//
//    WriteCommand(RA8875_DCRR);
//    WriteData(radius);
//
//    WriteCommand(RA8875_DCR, RA8875_DCR_FILL_bm | RA8875_DCR_CIRCLE_bm);
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IPoint RA8875Driver::RenderGlyph(const IPoint& position, uint32_t character, const IRect& clipRect, const FONT_INFO* font, uint16_t colorBg, uint16_t colorFg)
{  
    if (font == nullptr || character < font->startChar || character > font->endChar) {
        return position;
    }

    IPoint cursor = position;

    const FONT_CHAR_INFO& charInfo = font->charInfo[character - font->startChar];

    uint8_t        charWidth = charInfo.widthBits;
    const uint8_t* srcAddr = font->data + charInfo.offset;

    IRect bounds(cursor.x, cursor.y, cursor.x + charWidth, cursor.y + font->heightPages);
    IRect clippedBounds = bounds & clipRect;

    if (clippedBounds.Height() <= 0) {
        cursor.x += charWidth;
        return cursor;
    }

    if (!clippedBounds.IsValid())
    {
        cursor.x += charWidth;
        return cursor;
    }

    int m_FontHeightFullBytes = font->heightPages >> 3;
    int m_FontHeightRemainingBits = font->heightPages & 7;

    int charHeightBytes = m_FontHeightFullBytes;
    if (m_FontHeightRemainingBits != 0) charHeightBytes++;


    int xOffset = (clippedBounds.left - cursor.x) * charHeightBytes;
    for (int x = clippedBounds.left; x < clippedBounds.right; ++x)
    {
        for (int y = clippedBounds.top; y < clippedBounds.bottom; ++y)
        {
            int col = y - cursor.y;
            if (srcAddr[xOffset + (col >> 3)] & (0x80 >> (col & 0x7)))
            {
                WriteData(colorFg);
            } else {
                WriteData(colorBg);
            }
        }
        xOffset += charHeightBytes;
    }
    cursor.x += charWidth;

    return cursor;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t RA8875Driver::WriteString(SrvBitmap* bitmap, const IPoint& position, const char* string, size_t strLength, const IRect& clipRect, Color colorBg, Color colorFg, Font_e fontID)
{
    if (bitmap->m_VideoMem)
    {
        const FONT_INFO* font = GetFontDesc(fontID);

        if (font == nullptr) {
            return position.x;
        }

        IPoint cursor = position;

        IRect bounds(cursor.x, cursor.y, GetResolution().x, cursor.y + font->heightPages);
        bounds &= clipRect;

        if (!bounds.IsValid()) return cursor.x;

        WaitBlitter();

        FillDirection_e prevFillDir = m_FillDirection;
        SetFillDirection(e_FillDownLeft);

        SetWindow(bounds);
        MemoryWrite_Position(bounds.left, bounds.top);

        uint16_t colorBg16 = colorBg.GetColor16();
        uint16_t colorFg16 = colorFg.GetColor16();

        while (strLength > 0 && cursor.x < bounds.right)
        {
            int charLen = utf8_char_length(*string);
            if (charLen > strLength) {
                break;
            }
            uint32_t character = utf8_to_unicode(string);
            string    += charLen;
            strLength -= charLen;
            cursor = RenderGlyph(cursor, character, clipRect, font, colorBg16, colorFg16);
            int spaceStart = std::max(cursor.x, clipRect.left);
            int spaceEnd = std::min(cursor.x + CHARACTER_SPACING, clipRect.right);
            int spaceWidth = spaceEnd - spaceStart;
            if (spaceWidth > 0)
            {
                for (int i = 0; i < spaceWidth * bounds.Height(); ++i) {
                    WriteData(colorBg16);
                }
            }
            cursor.x += CHARACTER_SPACING;
        }
        SetFillDirection(prevFillDir);
        return cursor.x;
    }
    else
    {
        return DisplayDriver::WriteString(bitmap, position, string, strLength, clipRect, colorBg, colorFg, fontID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//uint8_t RA8875Driver::WriteStringTransparent(SrvBitmap* bitmap, const char* string, uint8_t strLength, int16_t maxWidth, Font_e fontID)
//{
//    maxWidth += cursor.x;
//    if (maxWidth > GetResolution().x) maxWidth = GetResolution().x;
//
//    uint8_t spacing = m_FontCharSpacing;
//
//    uint8_t wholeBytes = font->heightPages >> 3;
//    uint8_t remainingBits = font->heightPages & 7;
//
//    while (strLength--)
//    {
//        uint8_t character = *(string++);
//        if (character < m_FontFirstChar) character = m_FontFirstChar;
//        const FONT_CHAR_INFO* charInfo = m_FontCharInfo + character;
//
//        uint8_t        charWidth = charInfo->widthBits;
//        const uint8_t* srcAddr = m_FontGlyphData + charInfo->offset;
//        //    printf_P(PSTR("Width of '%c' is %d (%d/%d)\n"), origChar, charWidth, font->heightPages, m_FontHeightBytes);
//
//        if (cursor.x + charWidth > maxWidth) {
//            charWidth = maxWidth - cursor.x;
//        }
//        for (uint8_t x = charWidth; x; --x)
//        {
//            int16_t curY = cursor.y;
//            for (uint8_t y = wholeBytes; y; --y)
//            {
//                uint8_t line = *(srcAddr++);
//                for (int8_t i = 8; i; --i)
//                {
//                    if (line & 0x80) {
//                        WritePixel(cursor.x, curY);
//                    }
//                    curY++;
//                    line <<= 1;
//                }
//            }
//            uint8_t line = *(srcAddr++);
//
//            for (uint8_t i = remainingBits; i; --i)
//            {
//                if (line & 0x80) {
//                    WritePixel(cursor.x, curY);
//                }
//                curY++;
//                line <<= 1;
//            }
//            cursor.x++;
//        }
//
//        if (cursor.x + spacing < maxWidth)
//        {
//            cursor.x += spacing;
//        } else
//        {
//            spacing = maxWidth - cursor.x;
//            cursor.x += spacing;
//            break;
//        }
//    }
//    return 0;
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::Reset()
{
    if (m_PinLCDResetID != DigitalPinID::None)
    {
        snooze_ms(2);
        digital_pin_write(m_PinLCDResetID, false);
        snooze_ms(10);
        digital_pin_write(m_PinLCDResetID, true);
        snooze_ms(100);
    }

    PLL_ini();

    WriteCommand(RA8875_SYSR); // SYSR   bit[4:3] color  bit[2:1]=  MPU interface
    WriteData(0x0f);           // 16 BIT     65K

    WriteCommand(RA8875_PCSR); // PCLK
    WriteData(0x81);
    snooze_ms(2);

    //Horizontal set
    WriteCommand(RA8875_HDWR, 100 - 1); //Horizontal display width(pixels) = (HDWR + 1)*8
    WriteCommand(RA8875_HNDFTR, 0x00);  //Horizontal Non-Display Period Fine Tuning(HNDFT) [3:0]
    WriteCommand(RA8875_HNDR, 0x03);    //Horizontal Non-Display Period (pixels) = (HNDR + 1)*8
    WriteCommand(RA8875_HSTR, 0x03);    //HSYNC Start Position(PCLK) = (HSTR + 1)*8
    WriteCommand(RA8875_HPWR, 0x0B);    //HSYNC Width [4:0]   HSYNC Pulse width(PCLK) = (HPWR + 1)*8
    //Vertical set
    WriteCommand(RA8875_VDHR0, RA8875_VDHR1, 480 - 1); // Vertical pixels = VDHR + 1
    WriteCommand(RA8875_VNDR0, RA8875_VNDR1, 0x20);    // Vertical Non-Display area = (VNDR + 1)
    WriteCommand(RA8875_VSTR0, RA8875_VSTR1, 0x16);    // VSYNC Start Position(PCLK) = (VSTR + 1)
    WriteCommand(RA8875_VPWR, 0x01); // VSYNC Pulse Width(PCLK) = (VPWR + 1)
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::PLL_ini()
{
    WriteCommand(RA8875_PLLC1);
    snooze_ms(2);
    WriteData(0x0b);
    snooze_ms(2);
    WriteCommand(RA8875_PLLC2);
    snooze_ms(2);
    WriteData(0x02);
    snooze_ms(2);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::SetFgColor(uint16_t color)
{
    WaitBlitter();
    WriteCommand(RA8875_FGCR0);
    WriteData((color >> 11) & 0x1f);
    WriteCommand(RA8875_FGCR1);
    WriteData((color >> 5) & 0x3f);
    WriteCommand(RA8875_FGCR2);
    WriteData(color & 0x1f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::SetBgColor(uint16_t color)
{
    WaitBlitter();
    WriteCommand(RA8875_BGCR0);
    WriteData((color >> 11) & 0x1f);
    WriteCommand(RA8875_BGCR1);
    WriteData((color >> 5) & 0x3f);
    WriteCommand(RA8875_BGCR2);
    WriteData(color & 0x1f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::SetTransparantColor(uint16_t color)
{
    WaitBlitter();
    WriteCommand(RA8875_BGTR0);
    WriteData((color >> 11) & 0x1f);
    WriteCommand(RA8875_BGTR1);
    WriteData((color >> 5) & 0x3f);
    WriteCommand(RA8875_BGTR2);
    WriteData(color & 0x1f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875Driver::SetWindow(int x1, int y1, int x2, int y2)
{
    if (m_Orientation == e_Portrait)
    {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }

    WaitBlitter();

    WriteCommand(RA8875_HSAW0, RA8875_HSAW1, uint16_t(x1));
    WriteCommand(RA8875_HEAW0, RA8875_HEAW1, uint16_t(x2 - 1));
    WriteCommand(RA8875_VSAW0, RA8875_VSAW1, uint16_t(y1));
    WriteCommand(RA8875_VEAW0, RA8875_VEAW1, uint16_t(y2 - 1));
}
