// This file is part of PadOS.
//
// Copyright (c) 2014-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 16.01.2014 22:21

#pragma once

#include <Kernel/Drivers/Video/KDisplayDriver.h>
#include <Kernel/VFS/KDriverParametersBase.h>
#include <Kernel/IRQDispatcher.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/Drivers/Video/RA8875Registers.h>

struct KRA8875DriverParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "ra8875";

    KRA8875DriverParameters() = default;
    KRA8875DriverParameters(
        const PString& devicePath,
        PLCDRegisters* registers,
        DigitalPinID pinInterrupt,
        DigitalPinID pinLCDReset,
        DigitalPinID pinBacklightControl,
        DigitalPinID pinLCDWait)
        : KDriverParametersBase(devicePath)
        , Registers(uintptr_t(registers))
        , PinInterrupt(pinInterrupt)
        , PinLCDReset(pinLCDReset)
        , PinBacklightControl(pinBacklightControl)
        , PinLCDWait(pinLCDWait)
    {}

    uintptr_t    Registers    = 0;
    DigitalPinID PinInterrupt = DigitalPinID::None;
    DigitalPinID PinLCDReset = DigitalPinID::None;
    DigitalPinID PinBacklightControl = DigitalPinID::None;
    DigitalPinID PinLCDWait = DigitalPinID::None;

    friend void to_json(Pjson& data, const KRA8875DriverParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
        data.update(Pjson{
            {"registers",    value.Registers},
            {"pin_interrupt", value.PinInterrupt},
            {"pin_lcd_reset", value.PinLCDReset},
            {"pin_backlight_control", value.PinBacklightControl},
            {"pin_lcd_wait", value.PinLCDWait}
        });
    }
    friend void from_json(const Pjson& data, KRA8875DriverParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));
        data.at("registers"    ).get_to(outValue.Registers);
        data.at("pin_interrupt").get_to(outValue.PinInterrupt);
        data.at("pin_lcd_reset").get_to(outValue.PinLCDReset);
        data.at("pin_backlight_control").get_to(outValue.PinBacklightControl);
        data.at("pin_lcd_wait").get_to(outValue.PinLCDWait);
    }
};


namespace kernel
{

class RA8875GfxDriver : public KDisplayDriver
{
public:
    enum Orientation_e
    {
        e_Portrait,
        e_Landscape
    };
    enum FillDirection_e
    {
        e_FillLeftDown,
        e_FillDownLeft
    };

    RA8875GfxDriver(const KRA8875DriverParameters& parameters);

    virtual bool            Open() override;
    virtual void            Close() override;
    virtual void            PowerLost(bool hasPower) override;

    virtual PDisplayBitmap*  GetScreenBitmap() override;

    virtual int             GetScreenModeCount() override;
    virtual bool            GetScreenModeDesc(size_t index, PScreenMode& outMode) override;
    virtual bool            SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate) override;

    virtual PIPoint         GetResolution() override;
    virtual int             GetBytesPerLine() override;
    virtual PEColorSpace    GetColorSpace() override;
    virtual void            SetColor(size_t index, PColor color) override;

    virtual bool            SetMouseCursorBitmap(const PMouseCursorBitmap& cursor) override;
    virtual void            SetMouseCursorVisible(bool visible) override;
    virtual void            SetMousePos(PIPoint position) override;

    virtual void SetFgColor(PColor color) override
    {
        KDisplayDriver::SetFgColor(color);
        SetHardwareFgColor(color.GetColor16());
    }
    virtual void SetBgColor(PColor color) override
    {
        KDisplayDriver::SetBgColor(color);
        SetHardwareBgColor(color.GetColor16());
    }

    virtual void            WritePixel(PDisplayBitmap* bitmap, const PIPoint& pos, PColor color) override;
    virtual void            DrawLine(
        PDisplayBitmap* bitmap,
        const PIRect& clipRect,
        const PIPoint& pos1,
        const PIPoint& pos2,
        const PColor& color,
        PDrawingMode mode) override;
    virtual void            FillRect(PDisplayBitmap* bitmap, const PIRect& rect) override;
    virtual void            CopyRect(
        PDisplayBitmap* dstBitmap,
        PDisplayBitmap* srcBitmap,
        PColor bgColor,
        PColor fgColor,
        const PIRect& srcRect,
        const PIPoint& dstPos,
        PDrawingMode mode) override;
    virtual void            ScaleRect(
        PDisplayBitmap* dstBitmap,
        PDisplayBitmap* srcBitmap,
        PColor bgColor,
        PColor fgColor,
        const PIRect& srcOrigRect,
        const PIRect& dstOrigRect,
        const PRect& srcRect,
        const PIRect& dstRect,
        PDrawingMode mode) override;

    PIPoint                  RenderGlyph(
        const PIPoint& position,
        uint32_t character,
        const PIRect& clipRect,
        const FONT_INFO* font,
        uint16_t colorBg,
        uint16_t colorFg);
    virtual uint32_t        WriteString(
        PDisplayBitmap* bitmap,
        const PIPoint& position,
        const char* string,
        size_t strLength,
        const PIRect& clipRect,
        PColor colorBg,
        PColor colorFg,
        PFontID fontID) override;

    virtual void WaitIdle() override;

private:
    static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IRQResult HandleIRQ();
    void WaitBTE();

    void Reset();
    void PLL_ini();

    void SetHardwareFgColor(uint16_t color);
    void SetHardwareBgColor(uint16_t color);
    void SetTransparantColor(uint16_t color);

    static uint8_t ConvertMouseCursorPixelToRA8875(PMouseCursorPixel pixel);
    void SetMouseCursorColors(PColor color1, PColor color2);
    void UploadMouseCursorRaster(PIPoint rasterOffset);
    void SetGraphicCursorPosition(PIPoint position);
    void SetGraphicCursorEnabled(bool enabled);
    uint8_t ReadRegister(uint8_t command);

    void SetWindow(int x1, int y1, int x2, int y2);
    void SetWindow(const PIRect& frame) { SetWindow(frame.left, frame.top, frame.right, frame.bottom); }
    void UnsetWindow();

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
    void WaitMemory()
    {
        while ((ReadCommand() & RA8875_STATUS_MEMORY_BUSY_bm) != 0) {
        }
    }
    void        WaitBlitter()
    {
        uint16_t status = ReadCommand();
        if (status & (RA8875_STATUS_MEMORY_BUSY_bm | RA8875_STATUS_BTE_BUSY_bm))
        {
            if (status & RA8875_STATUS_BTE_BUSY_bm)
            {
                WaitBTE();
                status = ReadCommand();
            }
            while ((status & RA8875_STATUS_MEMORY_BUSY_bm) != 0) {
                status = ReadCommand();
            }
        }

        WriteCommand(RA8875_DCR);
        while ((ReadData() & (RA8875_DCR_LINE_SQR_TRI_bm | RA8875_DCR_CIRCLE_bm)) != 0)
        {
            WriteCommand(RA8875_DCR);
        }
    }

    uint16_t    ReadCommand() { return m_Registers->CMD; }
    void WriteCommand(uint8_t command)
    {
        m_Registers->CMD = command;
    }
    void WriteCommand(uint8_t command, uint8_t data)
    {
        WriteCommand(command);
        m_Registers->DATA = data;
    }
    void        WriteCommand(uint8_t cmdL, uint8_t cmdH, uint16_t data) { WriteCommand(cmdL, uint8_t(data & 0xff)); WriteCommand(cmdH, uint8_t(data >> 8)); }

    uint16_t    ReadData() { return m_Registers->DATA; }
    void        WriteData(uint16_t data) { m_Registers->DATA = data; }

    PLCDRegisters*  m_Registers = nullptr;
    DigitalPin m_PinLCDReset;
    DigitalPin m_PinBacklightControl;
    DigitalPin m_PinLCDWait;
    DigitalPin m_PinInterrupt;
    KConditionVariable m_CondVar;
    uint16_t m_HardwareFgColor = 0;
    uint16_t m_HardwareBgColor = 0;
    bool m_HardwareFgColorValid = false;
    bool m_HardwareBgColorValid = false;

    PDisplayBitmap m_ScreenBitmap;
    std::vector<uint8_t> m_MouseCursorRaster;
    PIPoint         m_MouseCursorSize;
    PIPoint         m_UploadedMouseCursorRasterOffset;

    PIPoint         m_CursorHotSpot;
    PIPoint         m_MousePos;

    Orientation_e   m_Orientation = e_Landscape;
    FillDirection_e m_FillDirection = e_FillLeftDown;
    bool            m_IsWindowSet = false;
    bool            m_IsMouseCursorVisible = false;
    bool            m_IsMouseCursorRasterUploaded = false;
};

} // namespace kernel
