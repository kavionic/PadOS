// This file is part of PadOS.
//
// Copyright (C) 2014-2020 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include <Kernel/HAL/DigitalPort.h>
#include <ApplicationServer/DisplayDriver.h>
#include <GUI/Color.h>
#include <GUI/Font.h>
#include "RA8875Registers.h"

namespace os
{

struct LCDRegisters
{
    volatile uint16_t DATA;
    volatile uint16_t CMD;
};



class RA8875Driver : public DisplayDriver
{
public:
    enum Orientation_e { e_Portrait, e_Landscape };
    enum FillDirection_e { e_FillLeftDown, e_FillDownLeft };

    RA8875Driver(LCDRegisters* registers, DigitalPinID pinLCDReset, DigitalPinID pinTouchpadReset, DigitalPinID pinBacklightControl);

    virtual bool            Open() override;
    virtual void            Close() override;
    virtual void            PowerLost(bool hasPower) override;

    virtual Ptr<SrvBitmap>  GetScreenBitmap() override;

    virtual int             GetScreenModeCount() override;
    virtual bool            GetScreenModeDesc(size_t index, ScreenMode& outMode) override;
    virtual bool            SetScreenMode(const IPoint& resolution, EColorSpace colorSpace, float refreshRate) override;

    virtual IPoint          GetResolution() override;
    virtual int             GetBytesPerLine() override;
    virtual EColorSpace      GetColorSpace() override;
    virtual void            SetColor(size_t index, const Color& color) override;

    virtual void            WritePixel(SrvBitmap* bitmap, const IPoint& pos, Color color) override;
    virtual void            DrawLine(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& pos1, const IPoint& pos2, const Color& color, DrawingMode mode) override;
    virtual void            FillRect(SrvBitmap* bitmap, const IRect& rect, const Color& color) override;
    virtual void            CopyRect(SrvBitmap* dstBitmap, SrvBitmap* srcBitmap, Color bgColor, Color fgColor, const IRect& srcRect, const IPoint& dstPos, DrawingMode mode) override;
    //    virtual void    FillCircle(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& center, int32_t radius, const Color& color, DrawingMode mode) override;

    IPoint                  RenderGlyph(const IPoint& position, uint32_t character, const IRect& clipRect, const FONT_INFO* font, uint16_t colorBg, uint16_t colorFg);
    virtual uint32_t        WriteString(SrvBitmap* bitmap, const IPoint& position, const char* string, size_t strLength, const IRect& clipRect, Color colorBg, Color colorFg, Font_e fontID) override;
    //    virtual uint8_t     WriteStringTransparent(SrvBitmap* bitmap, const char* string, uint8_t strLength, int16_t maxWidth, Font_e fontID);

private:
    void PLL_ini();

    void SetFgColor(uint16_t color);
    void SetBgColor(uint16_t color);
    void SetTransparantColor(uint16_t color);

    void SetWindow(int x1, int y1, int x2, int y2);
    void SetWindow(const IRect& frame) { SetWindow(frame.left, frame.top, frame.right, frame.bottom); }

    inline void SetFillDirection(FillDirection_e direction) { m_FillDirection = direction; UpdateAddressMode(); }
    inline void UpdateAddressMode()
    {
        if (m_FillDirection == e_FillLeftDown)
        {
            if (m_Orientation == e_Landscape) {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_LR_TD_bg); // Left -> Right then Top -> Down
            } else {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_TD_LR_bg); // Top -> Down then Left -> Right
            }
        } else
        {
            if (m_Orientation == e_Landscape) {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_TD_LR_bg); // Top -> Down then Left -> Right
            } else {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_LR_TD_bg); // Left -> Right then Top -> Down
            }
        }
    }

    void MemoryWrite_Position(int X, int Y)
    {
        WriteCommand(RA8875_CURH0, RA8875_CURH1, uint16_t(X));
        WriteCommand(RA8875_CURV0, RA8875_CURV1, uint16_t(Y));
        BeginWriteData();
    }
    void        BeginWriteData() { WriteCommand(RA8875_MRWC); }
    void        WaitMemory() { while (ReadCommand() & RA8875_STATUS_MEMORY_BUSY_bm); }
    void        WaitBTE() { while (ReadCommand() & RA8875_STATUS_BTE_BUSY_bm); }
    void        WaitROM() { while (ReadCommand() & RA8875_STATUS_ROM_BUSY_bm); }
    void        WaitBlitter() { while (ReadCommand() & (RA8875_STATUS_MEMORY_BUSY_bm | RA8875_STATUS_BTE_BUSY_bm)); }

    uint16_t    ReadCommand() { return m_Registers->CMD; }
    void        WriteCommand(uint8_t cmd) { m_Registers->CMD = cmd; }
    void        WriteCommand(uint8_t cmd, uint8_t data) { m_Registers->CMD = cmd; m_Registers->DATA = data; }
    void        WriteCommand(uint8_t cmdL, uint8_t cmdH, uint16_t data) { WriteCommand(cmdL, uint8_t(data & 0xff)); WriteCommand(cmdH, uint8_t(data >> 8)); }

    uint16_t    ReadData() { return m_Registers->DATA; }
    void        WriteData(uint16_t data) { m_Registers->DATA = data; }

    LCDRegisters*   m_Registers = nullptr;
    DigitalPin      m_PinLCDReset;
    DigitalPin      m_PinTouchpadReset;
    DigitalPin      m_PinBacklightControl;

    Ptr<SrvBitmap>  m_ScreenBitmap;

    Orientation_e   m_Orientation = e_Landscape;
    FillDirection_e m_FillDirection = e_FillLeftDown;
};

} // namespace os
