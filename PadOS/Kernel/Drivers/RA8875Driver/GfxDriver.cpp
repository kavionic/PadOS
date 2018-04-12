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

#include "sam.h"

#include <stdio.h>
#include <algorithm>

#include "GfxDriver.h"
#include "Fonts/MicrosoftSansSerif_14.h"
#include "Fonts/MicrosoftSansSerif_20.h"
#include "Fonts/MicrosoftSansSerif_72.h"

//#include "SDStorage/SDDriver.h"
//#include "SDStorage/FAT.h"
#include "Kernel/SpinTimer.h"

using namespace kernel;

GfxDriver GfxDriver::Instance;

///////////////////////////////////////////////////////////////////////////////
///
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
///
///////////////////////////////////////////////////////////////////////////////

void GfxDriver::InitDisplay()
{
    GfxDriverIO::Setup();

    GfxDriverIO::LCD_RESET_HIGH();
    SpinTimer::SleepMS(1);
    GfxDriverIO::LCD_RESET_LOW();
    SpinTimer::SleepMS(10);
    GfxDriverIO::LCD_RESET_HIGH();
    SpinTimer::SleepMS(100);

    PLL_ini();
    
    LCD_CmdWrite(RA8875_SYSR);  //SYSR   bit[4:3] color  bit[2:1]=  MPU interface
    LCD_DataWrite(0x0f);   //       16 BIT     65K
    
    LCD_CmdWrite(RA8875_PCSR);    //PCLK
    LCD_DataWrite(0x81);   //
    SpinTimer::SleepMS(1);

    //Horizontal set
    LCD_CmdWrite(RA8875_HDWR, 100-1);  //Horizontal display width(pixels) = (HDWR + 1)*8
    LCD_CmdWrite(RA8875_HNDFTR, 0x00); //Horizontal Non-Display Period Fine Tuning(HNDFT) [3:0]
    LCD_CmdWrite(RA8875_HNDR, 0x03);   //Horizontal Non-Display Period (pixels) = (HNDR + 1)*8
    LCD_CmdWrite(RA8875_HSTR, 0x03);   //HSYNC Start Position(PCLK) = (HSTR + 1)*8
    LCD_CmdWrite(RA8875_HPWR, 0x0B);   //HSYNC Width [4:0]   HSYNC Pulse width(PCLK) = (HPWR + 1)*8
    //Vertical set
    LCD_CmdWrite(RA8875_VDHR0, RA8875_VDHR1, 480 - 1); // Vertical pixels = VDHR + 1
    LCD_CmdWrite(RA8875_VNDR0, RA8875_VNDR1, 0x20);    // Vertical Non-Display area = (VNDR + 1)
    LCD_CmdWrite(RA8875_VSTR0, RA8875_VSTR1, 0x16);    // VSYNC Start Position(PCLK) = (VSTR + 1)
    LCD_CmdWrite(RA8875_VPWR, 0x01); // VSYNC Pulse Width(PCLK) = (VPWR + 1)

    

//    Active_Window(0,799,0,479);

    LCD_CmdWrite(RA8875_P1CR, 0x80);  // PWM setting
//    LCD_CmdWrite(RA8875_P2CR, 0x81);  // open PWM
    LCD_CmdWrite(RA8875_P1DCR, 0xff); // Brightness parameter 0xff-0x00    
    LCD_CmdWrite(RA8875_P2DCR, 0xff); // Brightness parameter 0xff-0x00    
    LCD_CmdWrite(RA8875_PWRR, RA8875_PWRR_DISPLAY_ON_bm);
}

void GfxDriver::SetOrientation( Orientation_e orientation )
{
    m_Orientation = orientation;
    UpdateAddressMode();
}

void GfxDriver::SetFgColor( uint16_t color )
{
    m_FgColor = color;
    WaitBlitter();
    LCD_CmdWrite(RA8875_FGCR0);
    LCD_DataWrite(color & 0x1f);
    LCD_CmdWrite(RA8875_FGCR1);
    LCD_DataWrite((color>>5) & 0x3f);
    LCD_CmdWrite(RA8875_FGCR2);
    LCD_DataWrite((color>>11) & 0x1f);        
}

void GfxDriver::SetBgColor( uint16_t color )
{
    m_BgColor = color;
    WaitBlitter();
    LCD_CmdWrite(RA8875_BGCR0);
    LCD_DataWrite(color & 0x1f);
    LCD_CmdWrite(RA8875_BGCR1);
    LCD_DataWrite((color>>5) & 0x3f);
    LCD_CmdWrite(RA8875_BGCR2);
    LCD_DataWrite((color>>11) & 0x1f);        
}

const FONT_INFO* GfxDriver::GetFontDesc(Font_e fontID) const
{
    switch(fontID)
    {
        case e_FontSmall:
        case e_FontNormal:
            return &microsoftSansSerif_14ptFontInfo;
        case e_FontLarge:
            return &microsoftSansSerif_20ptFontInfo;
        case e_Font7Seg:
            return &microsoftSansSerif_72ptFontInfo;
        case e_FontCount:
            return nullptr;
    }
    return nullptr;
}

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


float GfxDriver::GetFontHeight(Font_e fontID) const
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font != nullptr)
    {
        return font->heightPages;
    }
    return 0.0f;        
}

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

void GfxDriver::FastFill(uint32_t words, uint16_t color)
{
    for ( uint32_t i = 0 ; i < words ; ++i )
    {
        Write16(color);
    }
}

#if 0
void GfxDriver::FastFill16(uint16_t words)
{
    for ( uint8_t i = words & 0xf ; i ; --i )
    {
        WriteBus();
    }
/*    switch(words & 0xf)
    {
        case 15: WriteBus();
        case 14: WriteBus();
        case 13: WriteBus();
        case 12: WriteBus();
        case 11: WriteBus();
        case 10: WriteBus();
        case 9: WriteBus();
        case 8: WriteBus();
        case 7: WriteBus();
        case 6: WriteBus();
        case 5: WriteBus();
        case 4: WriteBus();
        case 3: WriteBus();
        case 2: WriteBus();
        case 1: WriteBus();
    }*/
    words >>= 4;
    while( words-- )
    {
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
        WriteBus();
    }
}

void GfxDriver::FastFill32(uint32_t words)
{
    for(;;)
    {
        if (words > 65535)
        {
            GfxDriverIO::LCD_WR_WAIT();
            GfxDriverIO::LCD_WR_PULSES(65535);
            words -= 65535;
        }
        else
        {
            GfxDriverIO::LCD_WR_WAIT();
            GfxDriverIO::LCD_WR_PULSES(words);
            break;
        }
    }
}
#endif

void GfxDriver::FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
#if 1
    BLT_FillRect(x1, y1, x2, y2);
#else
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

void GfxDriver::WritePixel(int16_t x, int16_t y)
{
    MemoryWrite_Position(x, y);
    WriteData16(m_FgColor);
}

void GfxDriver::DrawLine(int x1, int y1, int x2, int y2)
{
    if (y1==y2)
        DrawHLine(x1, y1, x2-x1);
    else if (x1==x2)
        DrawVLine(x1, y1, y2-y1);
    else
        BLT_DrawLine(x1, y1, x2, y2);
    /*
    if (y1==y2)
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
                WriteData16(m_FgColor);
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
                WriteData16(m_FgColor);
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

void GfxDriver::DrawHLine(int x, int y, int l)
{
    if (l>0)
    {
        FillRect(x, y, x + l, y);        
    }
}

void GfxDriver::DrawVLine(int x, int y, int l)
{
    FillRect(x, y, x, y + l);
}

void GfxDriver::FillCircle(int32_t x, int32_t y, int32_t radius)
{
    BLT_FillCircle(x, y, radius);
/*    for( int y1 =- radius ; y1 <= 0; ++y1 )
    {
        for( int x1 =- radius; x1 <= 0; ++x1 )
        {
            if( x1*x1 + y1*y1 <= radius*radius )
            {
                DrawHLine(m_FgColor, x+x1, y+y1, 2*(-x1));
                DrawHLine(m_FgColor, x+x1, y-y1, 2*(-x1));
                break;
            }
        }
    } */
}

void GfxDriver::BLT_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{   
    WaitBlitter();

    if (x1 != x2 || y1 != y2)
    {
        LCD_CmdWrite(RA8875_DLHSR0, RA8875_DLHSR1, x1);
        LCD_CmdWrite(RA8875_DLVSR0, RA8875_DLVSR1, y1);
        LCD_CmdWrite(RA8875_DLHER0, RA8875_DLHER1, x2);
        LCD_CmdWrite(RA8875_DLVER0, RA8875_DLVER1, y2);

        LCD_CmdWrite(RA8875_DCR, RA8875_DCR_FILL_bm | RA8875_DCR_LINE_SQR_TRI_bm | RA8875_DCR_SQUARE_bm);
    }
    else
    {
        WritePixel(x1, y1);
    }        
}

void GfxDriver::BLT_DrawLine(int x1, int y1, int x2, int y2 )
{
    WaitBlitter();

    LCD_CmdWrite(RA8875_DLHSR0, RA8875_DLHSR1, x1);
    LCD_CmdWrite(RA8875_DLVSR0, RA8875_DLVSR1, y1);
    LCD_CmdWrite(RA8875_DLHER0, RA8875_DLHER1, x2);
    LCD_CmdWrite(RA8875_DLVER0, RA8875_DLVER1, y2);

    LCD_CmdWrite(RA8875_DCR, RA8875_DCR_LINE_SQR_TRI_bm);
}

void GfxDriver::BLT_FillCircle(int32_t x, int32_t y, int32_t radius)
{
    WaitBlitter();
        
    LCD_CmdWrite(RA8875_DCHR0);
    LCD_DataWrite(x & 0xff);

    LCD_CmdWrite(RA8875_DCHR1);
    LCD_DataWrite(x >> 8);

    LCD_CmdWrite(RA8875_DCVR0);
    LCD_DataWrite(y & 0xff);

    LCD_CmdWrite(RA8875_DCVR1);
    LCD_DataWrite(y >> 8);

    LCD_CmdWrite(RA8875_DCRR);
    LCD_DataWrite(radius);
    
    LCD_CmdWrite(RA8875_DCR, RA8875_DCR_FILL_bm | RA8875_DCR_CIRCLE_bm);
}

void GfxDriver::BLT_MoveRect(const IRect& srcRect, const IPoint& dstPosIn)
{
    Chk_BTE_Busy();
    SetWindow(0, 0, 799, 479);
    
    uint8_t ctrl;
    IPoint srcPos;
    IPoint dstPos;

    if ((dstPosIn.y > srcRect.top) || (dstPosIn.y == srcRect.top && dstPosIn.x > srcRect.left))
    {
        ctrl = RA8875_BTE_OP_MOVE_NEG_ROP;
        srcPos = IPoint(srcRect.right, srcRect.bottom);
        dstPos = dstPosIn + IPoint(srcRect.Width() - 1, srcRect.Height() - 1);
    }
    else
    {
        ctrl = RA8875_BTE_OP_MOVE_POS_ROP;
        srcPos = IPoint(srcRect.left, srcRect.top);
        dstPos = dstPosIn;
    }
    LCD_CmdWrite(RA8875_HSBE0, RA8875_HSBE1, srcPos.x);
    LCD_CmdWrite(RA8875_VSBE0, RA8875_VSBE1, srcPos.y);
    LCD_CmdWrite(RA8875_HDBE0, RA8875_HDBE1, dstPos.x);
    LCD_CmdWrite(RA8875_VDBE0, RA8875_VDBE1, dstPos.y);
        
    LCD_CmdWrite(RA8875_BEWR0, RA8875_BEWR1, srcRect.Width());
    LCD_CmdWrite(RA8875_BEHR0, RA8875_BEHR1, srcRect.Height());
//    uint8_t ctrl = ((dstPos.y > srcRect.top) || (dstPos.y == srcRect.top && dstPos.x > srcRect.left)) ? RA8875_BTE_OP_MOVE_NEG_ROP : RA8875_BTE_OP_MOVE_POS_ROP;
    LCD_CmdWrite(RA8875_BECR1, ctrl | RA8875_BTE_ROP_S);
    LCD_CmdWrite(RA8875_BECR0, RA8875_BECR0_SRC_BLOCK | RA8875_BECR0_DST_BLOCK | RA8875_BECR0_ENABLE_bm);
//    SpinTimer::SleepMS(200);
}

bool GfxDriver::RenderGlyph(char character, const IRect& clipRect)
{
    if ( character < m_FontFirstChar ) character = m_FontFirstChar;
    const FONT_CHAR_INFO* charInfo = m_FontCharInfo + character;
        
    uint8_t        charWidth = charInfo->widthBits;
    const uint8_t* srcAddr = m_FontGlyphData + charInfo->offset;

    IRect bounds(m_Cursor.x, m_Cursor.y, m_Cursor.x + charWidth, m_Cursor.y + m_FontHeight);
    IRect clippedBounds = bounds & clipRect;
        
    if (clippedBounds.Height() <= 0) return false;
    
    if (!clippedBounds.IsValid2())
    {
        m_Cursor.x += charWidth;
        return m_Cursor.x < clipRect.right;
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
                Write16(m_FgColor);
            } else {
                Write16(m_BgColor);
            }
        }
        xOffset += charHeightBytes;
    }
    m_Cursor.x += charWidth;
    
    return m_Cursor.x < clippedBounds.right;
}

uint32_t GfxDriver::WriteString(const char* string, size_t strLength, const IRect& clipRect)
{
    WaitBlitter();

    FillDirection_e prevFillDir = m_FillDirection;
    SetFillDirection(e_FillDownLeft);
    
    IRect bounds(m_Cursor.x, m_Cursor.y, GetResolution().x, m_Cursor.y + m_FontHeight);
    bounds &= clipRect;
    
    if (!bounds.IsValid2()) return 0;
    
    SetWindow(bounds.left, bounds.top, bounds.right - 1, bounds.bottom - 1);
    MemoryWrite_Position(bounds.left, bounds.top);

    while(strLength--)
    {
        RenderGlyph(*(string++), clipRect);
        if ( m_Cursor.x + m_FontCharSpacing < bounds.right )
        {
            FastFill(m_FontCharSpacing * m_FontHeight, m_BgColor);
            m_Cursor.x += m_FontCharSpacing;
        }
        else
        {
            int32_t spacing = bounds.right - m_Cursor.x;
            if (spacing > 0) {
                FastFill(spacing * m_FontHeight, m_BgColor);
                m_Cursor.x += spacing;
            }                
            break;
        }
    }
    SetFillDirection(prevFillDir);
    return m_Cursor.x;
}

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
#if 0
uint8_t GfxDriver::WriteGlyph(char character)
{
    if ( character < m_FontFirstChar ) character = m_FontFirstChar;
    const FONT_CHAR_INFO* charInfo = m_FontCharInfo + character;
    
    uint8_t charWidth = charInfo->widthBits;
//    printf_P(PSTR("Width of '%c' is %d (%d/%d)\n"), origChar, charWidth, m_FontHeight, m_FontHeightBytes);

    FillDirection_e prevFillDir = m_FillDirection;
    SetFillDirection(e_FillDownLeft);
//    SetFillDirection(e_FillLeftDown);
    SetWindow(m_Cursor.x, m_Cursor.y, GetResolution().x - 1/* m_Cursor.x + charWidth + m_FontCharSpacing*/, m_Cursor.y + m_FontHeight - 1);
    MemoryWrite_Position(m_Cursor.x, m_Cursor.y);
    
//    GfxDriverIO::LCD_CS_LOW();
    RenderGlyph(character, GetResolution().x, 0);
    FastFill(m_FontCharSpacing * m_FontHeight, 0xffff);
    m_Cursor.x += charWidth + m_FontCharSpacing;
    
//    GfxDriverIO::LCD_WR_WAIT();
//    GfxDriverIO::LCD_CS_HIGH();
    SetFillDirection(prevFillDir);
    return charWidth + m_FontCharSpacing;
}
#endif
/*static bool StreamImageCallback(int16_t length)
{
    LCD_CS_LOW();
    for ( length >>= 1 ; length ; --length )
    {
        LCD_WR_LOW();
        DigitalPort::Set(LCD_DATA_PORT_0, sdcard.ReadNextByte());
        DigitalPort::Set(LCD_DATA_PORT_1, sdcard.ReadNextByte());
        LCD_WR_PULSE();
    }
    LCD_CS_HIGH();
    return true;
}

void GfxDriver::DrawImage(File* file, int16_t width, int16_t height)
{
    SetWindow(m_Cursor.x, m_Cursor.y, m_Cursor.x + width - 1, m_Cursor.y + height - 1);
    MemoryWrite_Position(m_Cursor.x, m_Cursor.y);
    file->Stream(int32_t(width)*height*2, StreamImageCallback);
}*/


void GfxDriver::PLL_ini()
{
    LCD_CmdWrite(RA8875_PLLC1);
    LCD_DataWrite(0x0a);
    SpinTimer::SleepMS(1);
    LCD_CmdWrite(RA8875_PLLC2);
    LCD_DataWrite(0x02);
    SpinTimer::SleepMS(1);
}

void GfxDriver::SetWindow(int x1, int y1, int x2, int y2)
{
    if ( m_FillDirection == e_FillLeftDown )
    {
        if ( m_Orientation == e_Landscape )
        {
        }
        else
        {
            std::swap(x1, y1);
            std::swap(x2, y2);
        }
    }
    else
    {
        if ( m_Orientation == e_Landscape )
        {
        }
        else
        {
            std::swap(x1, y1);
            std::swap(x2, y2);
        }
    }
/*    
    if ( x1 > x2 )
    {
        addressMode |= ILI9481_ADDRESS_MODE_RIGHT_TO_LEFT;
        std::swap(x1, x2);
    }

    if ( y1 > y2 )
    {
        addressMode |= ILI9481_ADDRESS_MODE_BOTTOM_TO_TOP;
        std::swap(y1, y2);
    }
  */  
    Chk_BTE_Busy();

    Active_Window(x1, x2, y1, y2);    
}



uint8_t GfxDriver::LCD_StatusRead()
{
    return GfxDriverIO::LCD_READ_CMD();
}

uint16_t GfxDriver::LCD_DataRead()
{
    return LCD_REG_DATA;
}

void GfxDriver::WriteCommand(uint8_t cmd)
{
    LCD_REG_CMD = cmd;
}

void GfxDriver::WriteData8(uint8_t data)
{
    LCD_REG_DATA = data;
}

void GfxDriver::WriteData16(uint16_t data)
{
    LCD_REG_DATA = data;
}

void GfxDriver::Write16(uint16_t data)
{
    LCD_REG_DATA = data;
}
