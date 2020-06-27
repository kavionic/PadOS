// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 16.01.2014 22:21:20

#include "System/Platform.h"

#include <stdio.h>
#include <algorithm>

#include "GfxDriver.h"
#include "Fonts/SansSerif_14.h"
#include "Fonts/SansSerif_20.h"
#include "Fonts/SansSerif_72.h"

using namespace kernel;
using namespace os;
GfxDriver GfxDriver::Instance;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GfxDriver::GfxDriver()
{
    m_Orientation = e_Landscape;
    m_FillDirection = e_FillLeftDown;
    m_Font = e_FontCount;
    m_BgColor = 0xffff;
    m_FgColor = 0x0000;
    m_Cursor.x = 0;
    m_Cursor.y = 0;
    m_FontCharSpacing = 3;
    SetFont(e_FontLarge);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::InitDisplay(LCDRegisters* registers, const DigitalPin& pinLCDReset, const DigitalPin& pinTouchpadReset, const DigitalPin& pinBacklightControl)
{
    m_Registers = registers;
    m_PinLCDReset = pinLCDReset;
    m_PinTouchpadReset = pinTouchpadReset;
    m_PinBacklightControl = pinBacklightControl;
    
    m_PinLCDReset         = true;
    m_PinBacklightControl = true;

    m_PinLCDReset.SetDirection(DigitalPinDirection_e::Out);
    m_PinBacklightControl.SetDirection(DigitalPinDirection_e::Out);


	snooze_ms(1);
    m_PinLCDReset = false;
	snooze_ms(10);
    m_PinLCDReset = true;
	snooze_ms(100);

    PLL_ini();
    
    WriteCommand(RA8875_SYSR); // SYSR   bit[4:3] color  bit[2:1]=  MPU interface
    WriteData(0x0f);           //       16 BIT     65K
    
    WriteCommand(RA8875_PCSR); // PCLK
    WriteData(0x81);
	snooze_ms(1);

    //Horizontal set
    WriteCommand(RA8875_HDWR, 100-1);  //Horizontal display width(pixels) = (HDWR + 1)*8
    WriteCommand(RA8875_HNDFTR, 0x00); //Horizontal Non-Display Period Fine Tuning(HNDFT) [3:0]
    WriteCommand(RA8875_HNDR, 0x03);   //Horizontal Non-Display Period (pixels) = (HNDR + 1)*8
    WriteCommand(RA8875_HSTR, 0x03);   //HSYNC Start Position(PCLK) = (HSTR + 1)*8
    WriteCommand(RA8875_HPWR, 0x0B);   //HSYNC Width [4:0]   HSYNC Pulse width(PCLK) = (HPWR + 1)*8
    //Vertical set
    WriteCommand(RA8875_VDHR0, RA8875_VDHR1, 480 - 1); // Vertical pixels = VDHR + 1
    WriteCommand(RA8875_VNDR0, RA8875_VNDR1, 0x20);    // Vertical Non-Display area = (VNDR + 1)
    WriteCommand(RA8875_VSTR0, RA8875_VSTR1, 0x16);    // VSYNC Start Position(PCLK) = (VSTR + 1)
    WriteCommand(RA8875_VPWR, 0x01); // VSYNC Pulse Width(PCLK) = (VPWR + 1)

    SetFgColor(0);
    
    IRect screenFrame(IPoint(0, 0), GetResolution());
    
    SetWindow(screenFrame);
    FillRect(screenFrame);


//    WriteCommand(RA8875_P1CR, 0x00);  // PWM setting
//    WriteCommand(RA8875_P2CR, 0x00);  // open PWM
/*
    WriteCommand(RA8875_P1CR, 0x80);  // PWM setting
//    WriteCommand(RA8875_P2CR, 0x81);  // open PWM
    WriteCommand(RA8875_P1DCR, 0xff); // Brightness parameter 0xff-0x00    
    WriteCommand(RA8875_P2DCR, 0xff); // Brightness parameter 0xff-0x00    
    */
    WriteCommand(RA8875_PWRR, RA8875_PWRR_DISPLAY_ON_bm);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::Shutdown()
{
    m_PinBacklightControl = false;
//    WriteCommand(RA8875_P1CR, 0x00);  // PWM setting
//    WaitBlitter();

/*    m_PinLCDReset = false;
    snooze_ms(1);
    m_PinLCDReset = true;
    snooze_ms(100);*/
    
    //Set PLL to default:
    WriteCommand(RA8875_PLLC1, 0x07);
	snooze_ms(1);
    WriteCommand(RA8875_PLLC2, 0x03);
	snooze_ms(1);
    WriteCommand(RA8875_PCSR, 0x02); // Pixel-clock
	snooze_ms(1);

    // Display off:
//    WriteCommand(RA8875_PWRR, 0);
//    snooze_ms(100);
    // Sleep mode:
    WriteCommand(RA8875_PWRR, RA8875_PWRR_SLEEP_MODE_bm);
	snooze_ms(100);
    m_PinTouchpadReset = false;
//    m_PinLCDReset = false;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::SetOrientation( Orientation_e orientation )
{
    m_Orientation = orientation;
    UpdateAddressMode();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::SetFgColor(uint16_t color)
{
    m_FgColor = color;
    WaitBlitter();
    WriteCommand(RA8875_FGCR0);
    WriteData((color >> 11) & 0x1f);
    WriteCommand(RA8875_FGCR1);
    WriteData((color>>5) & 0x3f);
    WriteCommand(RA8875_FGCR2);
    WriteData(color & 0x1f);        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::SetBgColor(uint16_t color)
{
    m_BgColor = color;
    WaitBlitter();
    WriteCommand(RA8875_BGCR0);
    WriteData((color>>11) & 0x1f);        
    WriteCommand(RA8875_BGCR1);
    WriteData((color>>5) & 0x3f);
    WriteCommand(RA8875_BGCR2);
    WriteData(color & 0x1f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const FONT_INFO* GfxDriver::GetFontDesc(Font_e fontID) const
{
    switch(fontID)
    {
        case e_FontSmall:
        case e_FontNormal:
            return &sansSerif_14ptFontInfo;
        case e_FontLarge:
            return &sansSerif_20ptFontInfo;
        case e_Font7Seg:
            return &sansSerif_72ptFontInfo;
        case e_FontCount:
            return nullptr;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::SetFont(Font_e fontID)
{
    m_Font = fontID;
    
    const FONT_INFO* font = GetFontDesc(fontID);
    
    if (font != nullptr)
    {
        m_FontFirstChar           = font->startChar;
        m_FontHeight              = font->heightPages;
        m_FontHeightFullBytes     = m_FontHeight >> 3;
        m_FontHeightRemainingBits = m_FontHeight & 7;        
        m_FontGlyphData           = font->data;
        m_FontCharInfo            = font->charInfo - m_FontFirstChar;
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float GfxDriver::GetFontHeight(Font_e fontID) const
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font != nullptr)
    {
        return font->heightPages;
    }
    return 0.0f;        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float GfxDriver::GetStringWidth(Font_e fontID, const char* string, uint16_t length ) const
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font != nullptr)
    {
        float width = 0.0f;

        while(length--)
        {
            uint8_t character = *(string++);
            if ( character < font->startChar ) character = font->startChar;
            const FONT_CHAR_INFO* charInfo = font->charInfo + character - font->startChar;
            width += float(charInfo->widthBits + m_FontCharSpacing);
        }
        return width;
    }
    return 0.0f;        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::FastFill(uint32_t words, uint16_t color)
{
    for ( uint32_t i = 0 ; i < words ; ++i )
    {
        WriteData(color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::FillRect(const IRect& frame)
{
#if 1
    BLT_FillRect(frame);
#else
	int32_t x1 = frame.left;
	int32_t y1 = frame.top;
	int32_t x2 = frame.right;
	int32_t y2 = frame.bottom;

	if (x1 > 799) x1 = 799;
    if (x2 > 799) x2 = 799;
    
    if (y1 > 479) y1 = 479;
    if (y2 > 479) y2 = 479;
    
    if ( x1 > x2 ) std::swap(x1, x2);
    if ( y1 > y2 ) std::swap(y1, y2);

    WaitBlitter();
    SetWindow(x1, y1, x2, y2);
    MemoryWrite_Position(x1, y1);
    FastFill(uint32_t(x2 - x1 + 1) * (y2 - y1 + 1), m_FgColor);
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::WritePixel(int16_t x, int16_t y)
{
    MemoryWrite_Position(x, y);
    WriteData(m_FgColor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::DrawLine(int x1, int y1, int x2, int y2)
{
    if (x1 == x2 && y1 == y2) return;
/*    if (y1==y2)
        DrawHLine(x1, y1, x2-x1);
    else if (x1==x2)
        DrawVLine(x1, y1, y2-y1);
    else*/
        BLT_DrawLine(x1, y1, x2, y2);

/*    if (y1==y2)
        DrawHLine(x1, y1, x2-x1);
    else if (x1==x2)
        DrawVLine(x1, y1, y2-y1);
    else
    {
        uint16_t deltaX = (x2 > x1) ? (x2 - x1) : (x1 - x2);
        int8_t   xStep =  (x2 > x1) ? 1 : -1;
        uint16_t deltaY = (y2 > y1) ? (y2 - y1) : (y1 - y2);
        int8_t   yStep =  (y2 > y1) ? 1 : -1;
        int      col = x1;
        int      row = y1;

        if (deltaX < deltaY)
        {
            int t = -(deltaY >> 1);
            for(;;)
            {
                MemoryWrite_Position(col, row);
                WriteData(m_FgColor);
                if (row == y2) break;
                row += yStep;
                t += deltaX;
                if (t >= 0)
                {
                    col += xStep;
                    t   -= deltaY;
                }
            }
        }
        else
        {
            int t = -(deltaX >> 1);
            for(;;)
            {
                MemoryWrite_Position(col, row);
                WriteData(m_FgColor);
                if (col == x2) break;
                col += xStep;
                t += deltaY;
                if (t >= 0)
                {
                    row += yStep;
                    t   -= deltaX;
                }
            }
        }
    }
*/
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::DrawHLine(int x, int y, int l)
{
    if (l>0)
    {
        FillRect(IRect(x, y, x + l, y + 1));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::DrawVLine(int x, int y, int l)
{
    FillRect(IRect(x, y, x + 1, y + l));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::FillCircle(int32_t x, int32_t y, int32_t radius)
{
#if 1
    BLT_FillCircle(x, y, radius);
#else
    for( int y1 =- radius ; y1 <= 0; ++y1 )
    {
        for( int x1 =- radius; x1 <= 0; ++x1 )
        {
            if( x1*x1 + y1*y1 <= radius*radius )
            {
                DrawHLine(x+x1, y+y1, 2*(-x1));
                DrawHLine(x+x1, y-y1, 2*(-x1));
                break;
            }
        }
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::BLT_FillRect(const IRect& frame)
{   
    WaitBlitter();

    if (frame.left != (frame.right - 1) || frame.top != (frame.bottom - 1))
    {
        WriteCommand(RA8875_DLHSR0, RA8875_DLHSR1, frame.left);
        WriteCommand(RA8875_DLVSR0, RA8875_DLVSR1, frame.top);
        WriteCommand(RA8875_DLHER0, RA8875_DLHER1, frame.right - 1);
        WriteCommand(RA8875_DLVER0, RA8875_DLVER1, frame.bottom - 1);

        WriteCommand(RA8875_DCR, RA8875_DCR_FILL_bm | RA8875_DCR_LINE_SQR_TRI_bm | RA8875_DCR_SQUARE_bm);
    }
    else
    {
        WritePixel(frame.left, frame.top);
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::BLT_DrawLine(int x1, int y1, int x2, int y2 )
{
    WaitBlitter();

    WriteCommand(RA8875_DLHSR0, RA8875_DLHSR1, x1);
    WriteCommand(RA8875_DLVSR0, RA8875_DLVSR1, y1);
    WriteCommand(RA8875_DLHER0, RA8875_DLHER1, x2);
    WriteCommand(RA8875_DLVER0, RA8875_DLVER1, y2);

    WriteCommand(RA8875_DCR, RA8875_DCR_LINE_SQR_TRI_bm);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::BLT_FillCircle(int32_t x, int32_t y, int32_t radius)
{
    WaitBlitter();
        
    WriteCommand(RA8875_DCHR0);
    WriteData(x & 0xff);

    WriteCommand(RA8875_DCHR1);
    WriteData(x >> 8);

    WriteCommand(RA8875_DCVR0);
    WriteData(y & 0xff);

    WriteCommand(RA8875_DCVR1);
    WriteData(y >> 8);

    WriteCommand(RA8875_DCRR);
    WriteData(radius);
    
    WriteCommand(RA8875_DCR, RA8875_DCR_FILL_bm | RA8875_DCR_CIRCLE_bm);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::BLT_MoveRect(const IRect& srcRect, const IPoint& dstPosIn)
{
    WaitBlitter();

    SetWindow(IRect(IPoint(0), GetResolution()));
    
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
    WriteCommand(RA8875_HSBE0, RA8875_HSBE1, srcPos.x);
    WriteCommand(RA8875_VSBE0, RA8875_VSBE1, srcPos.y);
    WriteCommand(RA8875_HDBE0, RA8875_HDBE1, dstPos.x);
    WriteCommand(RA8875_VDBE0, RA8875_VDBE1, dstPos.y);
        
    WriteCommand(RA8875_BEWR0, RA8875_BEWR1, srcRect.Width());
    WriteCommand(RA8875_BEHR0, RA8875_BEHR1, srcRect.Height());

    WriteCommand(RA8875_BECR1, ctrl | RA8875_BTE_ROP_S);
    WriteCommand(RA8875_BECR0, RA8875_BECR0_SRC_BLOCK | RA8875_BECR0_DST_BLOCK | RA8875_BECR0_ENABLE_bm);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::RenderGlyph(char character, const IRect& clipRect)
{
    if ( character < m_FontFirstChar ) character = m_FontFirstChar;
    const FONT_CHAR_INFO* charInfo = m_FontCharInfo + character;
        
    uint8_t        charWidth = charInfo->widthBits;
    const uint8_t* srcAddr = m_FontGlyphData + charInfo->offset;

    IRect bounds(m_Cursor.x, m_Cursor.y, m_Cursor.x + charWidth, m_Cursor.y + m_FontHeight);
    IRect clippedBounds = bounds & clipRect;
        
    if (clippedBounds.Height() <= 0) {
        m_Cursor.x += charWidth;
        return;
    }        
    
    if (!clippedBounds.IsValid())
    {
        m_Cursor.x += charWidth;
        return;
    }        
    
    int charHeightBytes = m_FontHeightFullBytes;
    if (m_FontHeightRemainingBits != 0) charHeightBytes++;
    
    
    int xOffset = (clippedBounds.left - m_Cursor.x) * charHeightBytes;
    for (int x = clippedBounds.left; x < clippedBounds.right; ++x)
    {
        for (int y = clippedBounds.top; y < clippedBounds.bottom; ++y)
        {
            int col = y - m_Cursor.y;
            if (srcAddr[xOffset + (col >> 3)] & (0x80 >> (col & 0x7)))
            {
                WriteData(m_FgColor);
            } else {
                WriteData(m_BgColor);
            }
        }
        xOffset += charHeightBytes;
    }
    m_Cursor.x += charWidth;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t GfxDriver::WriteString(const char* string, size_t strLength, const IRect& clipRect)
{
    WaitBlitter();
    
    IRect bounds(m_Cursor.x, m_Cursor.y, GetResolution().x, m_Cursor.y + m_FontHeight);
    bounds &= clipRect;
    
    if (!bounds.IsValid()) return 0;

    FillDirection_e prevFillDir = m_FillDirection;
    SetFillDirection(e_FillDownLeft);
    
    SetWindow(bounds);
    MemoryWrite_Position(bounds.left, bounds.top);

    while(strLength-- && m_Cursor.x < bounds.right)
    {
        RenderGlyph(*(string++), clipRect);
        int spaceStart = std::max(m_Cursor.x, clipRect.left);
        int spaceEnd   = std::min(m_Cursor.x + m_FontCharSpacing, clipRect.right);
        int spaceWidth = spaceEnd - spaceStart;
        if (spaceWidth > 0)
        {
            FastFill(spaceWidth * bounds.Height(), m_BgColor);
        }
        m_Cursor.x += m_FontCharSpacing;
    }
    SetFillDirection(prevFillDir);
    return m_Cursor.x;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t GfxDriver::WriteStringTransparent(const char* string, uint8_t strLength, int16_t maxWidth)
{
    maxWidth += m_Cursor.x;
    if ( maxWidth > GetResolution().x ) maxWidth = GetResolution().x;
    
    uint8_t spacing = m_FontCharSpacing;
    
    uint8_t wholeBytes    = m_FontHeight >> 3;
    uint8_t remainingBits = m_FontHeight & 7;

    while(strLength--)
    {
        uint8_t character = *(string++);
        if ( character < m_FontFirstChar ) character = m_FontFirstChar;
        const FONT_CHAR_INFO* charInfo = m_FontCharInfo + character;
        
        uint8_t        charWidth = charInfo->widthBits;
        const uint8_t* srcAddr = m_FontGlyphData + charInfo->offset;
        //    printf_P(PSTR("Width of '%c' is %d (%d/%d)\n"), origChar, charWidth, m_FontHeight, m_FontHeightBytes);
        
        if ( m_Cursor.x + charWidth > maxWidth ) {
            charWidth = maxWidth - m_Cursor.x;
        }
        for (uint8_t x = charWidth ; x ; --x )
        {
            int16_t curY = m_Cursor.y;
            for (uint8_t y = wholeBytes ; y  ; --y )
            {
                uint8_t line = *(srcAddr++);
                for( int8_t i = 8 ; i ; --i )
                {
                    if ( line & 0x80 ) {
                        WritePixel(m_Cursor.x, curY);
                    }
                    curY++;
                    line <<= 1;
                }
            }
            uint8_t line = *(srcAddr++);

            for( uint8_t i = remainingBits ; i ; --i )
            {
                if ( line & 0x80 ) {
                   WritePixel(m_Cursor.x, curY);
                }
                curY++;
                line <<= 1;
            }
            m_Cursor.x++;
        }
        
        if ( m_Cursor.x + spacing < maxWidth )
        {
            m_Cursor.x += spacing;
        }
        else
        {
            spacing = maxWidth - m_Cursor.x;
            m_Cursor.x += spacing;
            break;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::PLL_ini()
{
    WriteCommand(RA8875_PLLC1);
    WriteData(0x0a);
	snooze_ms(1);
    WriteCommand(RA8875_PLLC2);
    WriteData(0x02);
	snooze_ms(1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::SetWindow(int x1, int y1, int x2, int y2)
{
    if ( m_Orientation == e_Portrait )
    {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }

    WaitBlitter();
    
    WriteCommand(RA8875_HSAW0, RA8875_HSAW1, x1);
    WriteCommand(RA8875_HEAW0, RA8875_HEAW1, x2 - 1);
    WriteCommand(RA8875_VSAW0, RA8875_VSAW1, y1);
    WriteCommand(RA8875_VEAW0, RA8875_VEAW1, y2 - 1);    
}
