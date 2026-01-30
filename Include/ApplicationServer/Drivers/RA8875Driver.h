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

#pragma once

#include <Kernel/HAL/DigitalPort.h>
#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>
#include <GUI/Color.h>
#include <GUI/Font.h>
#include "RA8875Registers.h"


struct PLCDRegisters
{
    volatile uint16_t DATA;
    volatile uint16_t CMD;
};


struct RA8875DriverParameters
{
    RA8875DriverParameters() = default;
    RA8875DriverParameters(void* registers, DigitalPinID pinLCDReset, DigitalPinID pinTouchpadReset, DigitalPinID pinBacklightControl)
        : Registers(uintptr_t(registers))
        , PinLCDReset(pinLCDReset)
        , PinTouchpadReset(pinTouchpadReset)
        , PinBacklightControl(pinBacklightControl)
    {}

    uintptr_t       Registers;
    DigitalPinID    PinLCDReset;
    DigitalPinID    PinTouchpadReset;
    DigitalPinID    PinBacklightControl;

    friend void to_json(Pjson& data, const RA8875DriverParameters& value)
    {
        data = Pjson{
            {"registers",               value.Registers},
            {"pin_lcd_reset",           value.PinLCDReset},
            {"pin_touchpad_reset",      value.PinTouchpadReset},
            {"pin_backlight_control",   value.PinBacklightControl }
        };
    }
    friend void from_json(const Pjson& data, RA8875DriverParameters& outValue)
    {
        data.at("registers"             ).get_to(outValue.Registers);
        data.at("pin_lcd_reset"         ).get_to(outValue.PinLCDReset);
        data.at("pin_touchpad_reset"    ).get_to(outValue.PinTouchpadReset);
        data.at("pin_backlight_control" ).get_to(outValue.PinBacklightControl);
    }
};

class RA8875Driver : public PDisplayDriver
{
public:
    enum Orientation_e { e_Portrait, e_Landscape };
    enum FillDirection_e { e_FillLeftDown, e_FillDownLeft };

    RA8875Driver(const RA8875DriverParameters& config);

    virtual bool            Open() override;
    virtual void            Close() override;
    virtual void            PowerLost(bool hasPower) override;

    virtual Ptr<PSrvBitmap>  GetScreenBitmap() override;

    virtual int             GetScreenModeCount() override;
    virtual bool            GetScreenModeDesc(size_t index, PScreenMode& outMode) override;
    virtual bool            SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate) override;

    virtual PIPoint          GetResolution() override;
    virtual int             GetBytesPerLine() override;
    virtual PEColorSpace     GetColorSpace() override;
    virtual void            SetColor(size_t index, PColor color) override;

    virtual void            WritePixel(PSrvBitmap* bitmap, const PIPoint& pos, PColor color) override;
    virtual void            DrawLine(PSrvBitmap* bitmap, const PIRect& clipRect, const PIPoint& pos1, const PIPoint& pos2, const PColor& color, PDrawingMode mode) override;
    virtual void            FillRect(PSrvBitmap* bitmap, const PIRect& rect, const PColor& color) override;
    virtual void            CopyRect(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, PColor bgColor, PColor fgColor, const PIRect& srcRect, const PIPoint& dstPos, PDrawingMode mode) override;
    virtual void            ScaleRect(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, PColor bgColor, PColor fgColor, const PIRect& srcOrigRect, const PIRect& dstOrigRect, const PRect& srcRect, const PIRect& dstRect, PDrawingMode mode) override;
    //    virtual void    FillCircle(SrvBitmap* bitmap, const IRect& clipRect, const IPoint& center, int32_t radius, const Color& color, DrawingMode mode) override;

    PIPoint                  RenderGlyph(const PIPoint& position, uint32_t character, const PIRect& clipRect, const FONT_INFO* font, uint16_t colorBg, uint16_t colorFg);
    virtual uint32_t        WriteString(PSrvBitmap* bitmap, const PIPoint& position, const char* string, size_t strLength, const PIRect& clipRect, PColor colorBg, PColor colorFg, PFontID fontID) override;
    //    virtual uint8_t     WriteStringTransparent(SrvBitmap* bitmap, const char* string, uint8_t strLength, int16_t maxWidth, Font_e fontID);

private:
    void Reset();
    void PLL_ini();

    void SetFgColor(uint16_t color);
    void SetBgColor(uint16_t color);
    void SetTransparantColor(uint16_t color);

    void SetWindow(int x1, int y1, int x2, int y2);
    void SetWindow(const PIRect& frame) { SetWindow(frame.left, frame.top, frame.right, frame.bottom); }

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
        }
        else
        {
            if (m_Orientation == e_Landscape) {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_TD_LR_bg); // Top -> Down then Left -> Right
            } else {
                WriteCommand(RA8875_MWCR0, RA8875_MWCR0_LR_TD_bg); // Left -> Right then Top -> Down
            }
        }
    }

    void MemoryWrite_Position(int32_t X, int32_t Y)
    {
        WriteCommand(RA8875_CURH0, RA8875_CURH1, uint16_t(X));
        WriteCommand(RA8875_CURV0, RA8875_CURV1, uint16_t(Y));
        BeginWriteData();
    }
    void        BeginWriteData() { WriteCommand(RA8875_MRWC); }
    void        WaitMemory() { while (ReadCommand() & RA8875_STATUS_MEMORY_BUSY_bm); }
    void        WaitBTE() { while (ReadCommand() & RA8875_STATUS_BTE_BUSY_bm); }
    void        WaitROM() { while (ReadCommand() & RA8875_STATUS_ROM_BUSY_bm); }
    void        WaitBlitter()
    {
        size_t spinCount = 0;
        while (ReadCommand() & (RA8875_STATUS_MEMORY_BUSY_bm | RA8875_STATUS_BTE_BUSY_bm))
        {
            if (spinCount++ > 10000000)
            {
                Reset();
                spinCount = 0;

                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "Resetting video chip.");
            }
        }

        static size_t maxSpinCount = 0;
        if (spinCount > maxSpinCount)
        {
            maxSpinCount = spinCount;
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "SC: {}", maxSpinCount);
        }
    }

    uint16_t    ReadCommand() { return m_Registers->CMD; }
    void        WriteCommand(uint8_t cmd) { m_Registers->CMD = cmd; }
    void        WriteCommand(uint8_t cmd, uint8_t data) { m_Registers->CMD = cmd; m_Registers->DATA = data; }
    void        WriteCommand(uint8_t cmdL, uint8_t cmdH, uint16_t data) { WriteCommand(cmdL, uint8_t(data & 0xff)); WriteCommand(cmdH, uint8_t(data >> 8)); }

    uint16_t    ReadData() { return m_Registers->DATA; }
    void        WriteData(uint16_t data) { m_Registers->DATA = data; }

    PLCDRegisters*   m_Registers = nullptr;
    DigitalPinID    m_PinLCDResetID;
    DigitalPinID    m_PinTouchpadResetID;
    DigitalPinID    m_PinBacklightControlID;

    Ptr<PSrvBitmap>  m_ScreenBitmap;

    Orientation_e   m_Orientation = e_Landscape;
    FillDirection_e m_FillDirection = e_FillLeftDown;
};
