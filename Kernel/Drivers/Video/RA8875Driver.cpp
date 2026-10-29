// This file is part of PadOS.
//
// Copyright (c) 2014-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 16.01.2014 22:21

#include <Kernel/Drivers/Video/RA8875Driver.h>
#include <Kernel/Drivers/Video/BlitterUtils.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/HAL/STM32/Peripherals_STM32H7.h>
#include <Kernel/KThread.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>
#include <algorithm>
#include <array>
#include <utility>

namespace kernel
{

PREGISTER_KERNEL_DRIVER(RA8875GfxDriver, KRA8875DriverParameters);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

RA8875GfxDriver::RA8875GfxDriver(const KRA8875DriverParameters& parameters)
    : m_Registers(reinterpret_cast<PLCDRegisters*>(parameters.Registers))
    , m_PinLCDReset(parameters.PinLCDReset)
    , m_PinBacklightControl(parameters.PinBacklightControl)
    , m_PinLCDWait(parameters.PinLCDWait)
    , m_PinInterrupt(parameters.PinInterrupt)
    , m_CondVar("ra8875irq")
{
    m_ScreenBitmap.ColorSpace = PEColorSpace::RGB16;
    m_ScreenBitmap.VideoMemory = true;
    m_PinInterrupt.SetDirection(DigitalPinDirection_e::In);
    m_PinInterrupt.DisableInterrupts();
    m_PinInterrupt.SetInterruptMode(PinInterruptMode_e::FallingEdge);
    m_PinInterrupt.GetAndClearInterruptStatus();
    register_irq_handler(get_peripheral_irq(parameters.PinInterrupt), IRQCallback, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875GfxDriver::Open()
{
    if (m_PinLCDReset.GetID() != DigitalPinID::None)
    {
        m_PinLCDReset.Write(true);
        m_PinLCDReset.SetDirection(DigitalPinDirection_e::Out);
    }
    m_PinBacklightControl.Write(true);
    m_PinBacklightControl.SetDirection(DigitalPinDirection_e::Out);
    m_PinLCDWait.SetDirection(DigitalPinDirection_e::In);

    m_ScreenBitmap.Size = GetResolution();
    m_ScreenBitmap.BytesPerLine = size_t(GetBytesPerLine());
    return Reset();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::Close()
{
    WaitIdle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::PowerLost(bool hasPower)
{
    kassert(m_Mutex.IsLocked());
    m_PinBacklightControl.Write(hasPower);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayBitmap* RA8875GfxDriver::GetScreenBitmap()
{
    return &m_ScreenBitmap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int RA8875GfxDriver::GetScreenModeCount()
{
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875GfxDriver::GetScreenModeDesc(size_t index, PScreenMode& outMode)
{
    if (index == 0)
    {
        outMode = PScreenMode(PIPoint(800, 480), 800 * 2, PEColorSpace::RGB16);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875GfxDriver::SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate)
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIPoint RA8875GfxDriver::GetResolution()
{
    return PIPoint(800, 480);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int RA8875GfxDriver::GetBytesPerLine()
{
    return GetResolution().x * 2;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PEColorSpace RA8875GfxDriver::GetColorSpace()
{
    return PEColorSpace::RGB16;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetColor(size_t index, PColor color)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875GfxDriver::SetMouseCursorBitmap(const PMouseCursorBitmap& cursor)
{
    if (cursor.Width <= 0 || cursor.Height <= 0 || cursor.Width > RA8875_GRAPHIC_CURSOR_SIZE || cursor.Height > RA8875_GRAPHIC_CURSOR_SIZE) {
        return false;
    }
    if (cursor.HotSpot.x < 0 || cursor.HotSpot.y < 0 || cursor.HotSpot.x >= cursor.Width || cursor.HotSpot.y >= cursor.Height) {
        return false;
    }

    const size_t pixelCount = size_t(cursor.Width) * size_t(cursor.Height);
    if (cursor.Raster.size() < pixelCount) {
        return false;
    }

    m_CursorHotSpot = cursor.HotSpot;
    m_MouseCursorSize = PIPoint(cursor.Width, cursor.Height);
    m_MouseCursorRaster.resize(pixelCount);
    for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
        m_MouseCursorRaster[pixelIndex] = ConvertMouseCursorPixelToRA8875(cursor.Raster[pixelIndex]);
    }
    m_IsMouseCursorRasterUploaded = false;

    const bool wasVisible = m_IsMouseCursorVisible;
    SetGraphicCursorEnabled(false);
    SetMouseCursorColors(cursor.Color1, cursor.Color2);
    SetGraphicCursorPosition(m_MousePos);
    SetGraphicCursorEnabled(wasVisible);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetMouseCursorVisible(bool visible)
{
    kassert(m_Mutex.IsLocked());
    m_IsMouseCursorVisible = visible;
    SetGraphicCursorEnabled(visible);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetMousePos(PIPoint position)
{
    kassert(m_Mutex.IsLocked());
    m_MousePos = position;
    SetGraphicCursorPosition(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::WritePixel(PDisplayBitmap* bitmap, const PIPoint& pos, PColor color)
{
    if (bitmap->VideoMemory)
    {
        WaitBlitter();
        MemoryWrite_Position(pos.x, pos.y);
        WriteData(color.GetColor16());
    }
    else
    {
        KDisplayDriver::WritePixel(bitmap, pos, color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::DrawLine(PDisplayBitmap* bitmap, const PIRect& clipRect, const PIPoint& pos1, const PIPoint& pos2, const PColor& color, PDrawingMode mode)
{
    if (bitmap->VideoMemory)
    {
        if (pos1 == pos2)
        {
            WritePixel(bitmap, pos1, color);
            return;
        }
        SetWindow(clipRect);
        WaitBlitter();

        SetHardwareFgColor(color.GetColor16());

        WriteCommand(RA8875_DLHSR0, RA8875_DLHSR1, uint16_t(pos1.x));
        WriteCommand(RA8875_DLVSR0, RA8875_DLVSR1, uint16_t(pos1.y));
        WriteCommand(RA8875_DLHER0, RA8875_DLHER1, uint16_t(pos2.x));
        WriteCommand(RA8875_DLVER0, RA8875_DLVER1, uint16_t(pos2.y));

        WriteCommand(RA8875_DCR, RA8875_DCR_LINE_SQR_TRI_bm);
    }
    else
    {
        KDisplayDriver::DrawLine(bitmap, clipRect, pos1, pos2, color, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::FillRect(PDisplayBitmap* bitmap, const PIRect& rect)
{
    if (bitmap->VideoMemory)
    {
        SetHardwareFgColor(GetFgColor().GetColor16());
        UnsetWindow();

        if (rect.left != (rect.right - 1) || rect.top != (rect.bottom - 1))
        {
            WaitBlitter();

            WriteCommand(RA8875_DLHSR0, RA8875_DLHSR1, uint16_t(rect.left));
            WriteCommand(RA8875_DLVSR0, RA8875_DLVSR1, uint16_t(rect.top));
            WriteCommand(RA8875_DLHER0, RA8875_DLHER1, uint16_t(rect.right - 1));
            WriteCommand(RA8875_DLVER0, RA8875_DLVER1, uint16_t(rect.bottom - 1));

            WriteCommand(RA8875_DCR, RA8875_DCR_FILL_bm | RA8875_DCR_LINE_SQR_TRI_bm | RA8875_DCR_SQUARE_bm);
        }
        else
        {
            WritePixel(bitmap, rect.TopLeft(), GetFgColor());
        }
    }
    else
    {
        KDisplayDriver::FillRect(bitmap, rect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::CopyRect(
    PDisplayBitmap* dstBitmap,
    PDisplayBitmap* srcBitmap,
    PColor bgColor,
    PColor fgColor,
    const PIRect& srcRect,
    const PIPoint& dstPosIn,
    PDrawingMode mode)
{
    if (dstBitmap->VideoMemory && srcBitmap->VideoMemory)
    {
        WaitBlitter();

        uint8_t ctrl;
        PIPoint srcPos;
        PIPoint dstPos;

        if ((dstPosIn.y > srcRect.top) || (dstPosIn.y == srcRect.top && dstPosIn.x > srcRect.left))
        {
            ctrl = RA8875_BTE_OP_MOVE_NEG_ROP;
            srcPos = PIPoint(srcRect.right - 1, srcRect.bottom - 1);
            dstPos = dstPosIn + PIPoint(srcRect.Width() - 1, srcRect.Height() - 1);
        }
        else
        {
            ctrl = RA8875_BTE_OP_MOVE_POS_ROP;
            srcPos = PIPoint(srcRect.left, srcRect.top);
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
    else if (dstBitmap->VideoMemory)
    {
        const bool colorKeyed = mode == PDrawingMode::Overlay;
        if (colorKeyed)
        {
            if (srcBitmap->ColorSpace != PEColorSpace::MONO1) {
                SetHardwareFgColor(PTransparentColors::RGB16);
            } else {
                SetHardwareFgColor(uint16_t(~fgColor.GetColor16()));
            }
        }
        const PIRect bounds(dstPosIn, dstPosIn + PIPoint(srcRect.Width(), srcRect.Height()));
        auto producePixels = [this, srcBitmap, bgColor, fgColor, &srcRect, colorKeyed](auto writePixel)
        {
            CopyBitmapPixels(srcBitmap, bgColor, fgColor, srcRect, colorKeyed, writePixel);
        };
        WritePixelBlock(bounds, colorKeyed, producePixels);
    }
    else if (!srcBitmap->VideoMemory && !dstBitmap->VideoMemory)
    {
        KDisplayDriver::CopyRect(dstBitmap, srcBitmap, bgColor, fgColor, srcRect, dstPosIn, mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::ScaleRect(
    PDisplayBitmap* dstBitmap,
    PDisplayBitmap* srcBitmap,
    PColor bgColor,
    PColor fgColor,
    const PIRect& srcOrigRect,
    const PIRect& dstOrigRect,
    const PRect& srcRect,
    const PIRect& dstRect,
    PDrawingMode mode)
{
    if (dstBitmap->VideoMemory && srcBitmap->VideoMemory)
    {
        WaitBlitter();
    }
    else if (dstBitmap->VideoMemory)
    {
        const bool colorKeyed = mode == PDrawingMode::Overlay;
        if (colorKeyed)
        {
            if (srcBitmap->ColorSpace != PEColorSpace::MONO1) {
                SetHardwareFgColor(PTransparentColors::RGB16);
            } else {
                SetHardwareFgColor(uint16_t(~fgColor.GetColor16()));
            }
        }
        auto producePixels = [this, srcBitmap, bgColor, &srcOrigRect, &dstOrigRect, &srcRect, &dstRect](auto writePixel)
        {
            ScaleBitmapPixels(srcBitmap, bgColor, srcOrigRect, dstOrigRect, srcRect, dstRect, writePixel);
        };
        WritePixelBlock(dstRect, colorKeyed, producePixels);
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
//    SetHardwareFgColor(color.GetColor16());
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

void RA8875GfxDriver::WriteString(
    PDisplayBitmap* bitmap,
    const PIPoint& position,
    std::string_view text,
    const PDisplayFontCache& font,
    std::span<const int32_t> kernings,
    std::span<const PIRect> clipRects,
    PColor background,
    PColor foreground,
    PPoint advance,
    PDrawingMode mode,
    float width)
{
    kassert(m_Mutex.IsLocked());
    if (mode != PDrawingMode::Copy && mode != PDrawingMode::Overlay) {
        PERROR_THROW_CODE(PErrorCode::NOSYS);
    }
    if (mode == PDrawingMode::Overlay && foreground.GetAlpha() == 0) {
        return;
    }
    if (!bitmap->VideoMemory)
    {
        KDisplayDriver::WriteString(
            bitmap,
            position,
            text,
            font,
            kernings,
            clipRects,
            background,
            foreground,
            advance,
            mode,
            width);
    }
    else if ((!text.empty() || PMath::is_finite(width)) && !clipRects.empty())
    {
        std::optional<uint32_t> colorKey;
        if (mode == PDrawingMode::Overlay)
        {
            // Pick red opposite the background's upper bit and green opposite the foreground's.
            // No interpolation can approach both extremes closely enough to quantize to this RGB565 corner.
            const uint32_t transparentRed = (background.GetRed() < 128) ? 0xf800 : 0;
            const uint32_t transparentGreen = (foreground.GetGreen() < 128) ? 0x07e0 : 0;
            colorKey = transparentRed | transparentGreen;
        }
        const std::array<uint32_t, 256>& palette = PrepareFontPalette(bitmap->ColorSpace, background, foreground, colorKey);
        const PIRect bitmapBounds = PIRect(PIPoint(0, 0), bitmap->Size) & PIRect(PIPoint(0, 0), GetResolution());
        m_FontText.Begin(font, text, position, bitmapBounds, kernings, advance, mode, width);
        PScopeExit releaseText([this] { m_FontText.End(); });
        const PColor previousColor = GetFgColor();
        // Only spans outside every glyph raster use fills. Their pixels will not be written again.
        KDisplayDriver::SetFgColor(PColor(background.GetRed(), background.GetGreen(), background.GetBlue()));
        PScopeExit restoreColor([this, previousColor] { KDisplayDriver::SetFgColor(previousColor); });
        if (mode == PDrawingMode::Overlay) {
            SetHardwareFgColor(uint16_t(palette[0]));
        }
        WaitBlitter();
        PScopeExit finishWrite([this]
        {
            // A BTE timeout faults the controller, so skip the final memory wait.
            if (!m_IsFaulted) {
                WaitMemory();
            }
        });
        while (m_FontText.NextSpan())
        {
            for (const PIRect& clip : clipRects)
            {
                const PIRect bounds = m_FontText.GetSpan().Bounds & clip & bitmapBounds;
                if (bounds.IsValid())
                {
                    if (m_FontText.GetSpan().HasGlyphs) {
                        WriteFontSpan(bounds, palette, mode);
                    } else {
                        FillRect(bitmap, bounds);
                    }
                }
            }
        }
        WaitBlitter();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::WaitIdle()
{
    // All client raster access is synchronous CPU I/O. A faulted BTE cannot retain references to client memory.
    // Allow batch unwinding and close/reopen to reach Reset() without waiting on the stalled controller again.
    if (!m_IsFaulted)
    {
        WaitBlitter();
        WaitMemory();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::BeginTransparentWrite(const PIRect& bounds)
{
    WaitBlitter();
    WriteCommand(RA8875_HDBE0, RA8875_HDBE1, uint16_t(bounds.left));
    WriteCommand(RA8875_VDBE0, RA8875_VDBE1, uint16_t(bounds.top));
    WriteCommand(RA8875_BEWR0, RA8875_BEWR1, uint16_t(bounds.Width()));
    WriteCommand(RA8875_BEHR0, RA8875_BEHR1, uint16_t(bounds.Height()));
    WriteCommand(RA8875_BECR1, RA8875_BTE_OP_WRITE_TRAN | RA8875_BTE_ROP_S);
    WriteCommand(RA8875_BECR0, RA8875_BECR0_SRC_BLOCK | RA8875_BECR0_DST_BLOCK | RA8875_BECR0_ENABLE_bm);
    BeginWriteData();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename PixelProducer>
void RA8875GfxDriver::WritePixelBlock(const PIRect& bounds, bool useBTE, PixelProducer producePixels)
{
    PScopeExit finishWrite([this]
    {
        // A BTE timeout faults the controller, so skip the final memory wait.
        if (!m_IsFaulted) {
            WaitMemory();
        }
    });
    if (useBTE)
    {
        BeginTransparentWrite(bounds);
        auto writePixel = [this](uint16_t pixel) PALWAYS_INLINE { WaitMemory(); WriteData(pixel); };
        producePixels(writePixel);
    }
    else
    {
        SetWindow(bounds);
        MemoryWrite_Position(bounds.left, bounds.top);
        auto writePixel = [this](uint16_t pixel) PALWAYS_INLINE { WriteData(pixel); __DSB(); };
        producePixels(writePixel);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename PixelWriter>
void RA8875GfxDriver::CopyBitmapPixels(
    PDisplayBitmap* srcBitmap,
    PColor bgColor,
    PColor fgColor,
    const PIRect& srcRect,
    bool colorKeyed,
    PixelWriter writePixel16)
{
    switch (srcBitmap->ColorSpace)
    {
        case PEColorSpace::MONO1:
        {
            const uint32_t  wordsPerLine = srcBitmap->BytesPerLine / sizeof(uint32_t);
            const uint32_t* src = reinterpret_cast<const uint32_t*>(srcBitmap->Raster + srcRect.top * srcBitmap->BytesPerLine);

            const uint16_t bgColor16 = (colorKeyed) ? uint16_t(~fgColor.GetColor16()) : bgColor.GetColor16();
            const uint16_t fgColor16 = fgColor.GetColor16();

            int32_t x = srcRect.left;
            auto readPixel = [&x, &src]() PALWAYS_INLINE
            {
                const int32_t column = x++;
                return (src[column / 32] & (1 << (31 - (column & 31)))) != 0;
            };
            auto writePixel = [&writePixel16, bgColor16, fgColor16](bool pixel) PALWAYS_INLINE
            {
                writePixel16(pixel ? fgColor16 : bgColor16);
            };
            auto nextLine = [&x, &src, left = srcRect.left, wordsPerLine]() PALWAYS_INLINE { src += wordsPerLine; x = left; };
            PBlitterUtils::CopyBitmap(readPixel, writePixel, nextLine, srcRect);
            break;
        }
        case PEColorSpace::CMAP8:
        {
            const int32_t srcModulo = srcBitmap->BytesPerLine - srcRect.Width();
            const uint8_t* src = srcBitmap->Raster + srcRect.top * srcBitmap->BytesPerLine;

            auto readPixel = [this, &src]() PALWAYS_INLINE { return GetPaletteEntry(*src++).GetColor16(); };
            auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

            PBlitterUtils::CopyBitmap(readPixel, writePixel16, nextLine, srcRect);
            break;
        }
        case PEColorSpace::RGB15:
        {
            const int32_t srcModulo = srcBitmap->BytesPerLine / 2 - srcRect.Width();
            const uint16_t* src = RAS_OFFSET16(srcBitmap->Raster, srcRect.left, srcRect.top, srcBitmap->BytesPerLine);

            auto readPixel = [&src]() PALWAYS_INLINE { return PColor::FromRGB15(*src++).GetColor16(); };
            auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

            PBlitterUtils::CopyBitmap(readPixel, writePixel16, nextLine, srcRect);
            break;
        }
        case PEColorSpace::RGB16:
        {
            const int32_t srcModulo = srcBitmap->BytesPerLine / 2 - srcRect.Width();
            const uint16_t* src = RAS_OFFSET16(srcBitmap->Raster, srcRect.left, srcRect.top, srcBitmap->BytesPerLine);

            auto readPixel = [&src]() PALWAYS_INLINE { return *src++; };
            auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

            PBlitterUtils::CopyBitmap(readPixel, writePixel16, nextLine, srcRect);
            break;
        }
        case PEColorSpace::RGB32:
        case PEColorSpace::RGBA32:
        {
            const int32_t srcModulo = srcBitmap->BytesPerLine / 4 - srcRect.Width();
            const uint32_t* src = RAS_OFFSET32(srcBitmap->Raster, srcRect.left, srcRect.top, srcBitmap->BytesPerLine);

            auto readPixelRGB = [&src]() PALWAYS_INLINE { return PColor::FromRGB32(*src++).GetColor16(); };
            auto readPixelRGBA = [&src, bgColor]() PALWAYS_INLINE
            {
                const PColor pixel32 = PColor::FromRGB32A(*src++);
                return PColor::Blend16(pixel32, bgColor);
            };
            auto nextLine = [&src, srcModulo]() PALWAYS_INLINE { src += srcModulo; };

            if (srcBitmap->ColorSpace == PEColorSpace::RGB32) {
                PBlitterUtils::CopyBitmap(readPixelRGB, writePixel16, nextLine, srcRect);
            } else {
                PBlitterUtils::CopyBitmap(readPixelRGBA, writePixel16, nextLine, srcRect);
            }
            break;
        }
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename PixelWriter>
void RA8875GfxDriver::ScaleBitmapPixels(
    PDisplayBitmap* srcBitmap,
    PColor bgColor,
    const PIRect& srcOrigRect,
    const PIRect& dstOrigRect,
    const PRect& srcRect,
    const PIRect& dstRect,
    PixelWriter writePixel16)
{
    switch (srcBitmap->ColorSpace)
    {
        case PEColorSpace::RGB16:
        {
            const uint16_t* const sourceRaster = reinterpret_cast<const uint16_t*>(srcBitmap->Raster);
            const size_t pixelsPerLine = srcBitmap->BytesPerLine / sizeof(uint16_t);

            auto readPixel = [sourceRaster, pixelsPerLine](int32_t x, int32_t y) PALWAYS_INLINE
            {
                return PColor::FromRGB16(sourceRaster[y * pixelsPerLine + x]);
            };
            auto writePixel = [&writePixel16](int32_t x, int32_t y, const PColor& pixel) PALWAYS_INLINE
            {
                writePixel16(pixel.GetColor16());
            };

            const PIRect sourceBounds = srcOrigRect & PIRect(PIPoint(0, 0), srcBitmap->Size);
            PBlitterUtils::ScaleBitmapBilinear(readPixel, writePixel, srcOrigRect, dstOrigRect, srcRect, dstRect, sourceBounds);
            break;
        }
        case PEColorSpace::RGB32:
        case PEColorSpace::RGBA32:
        {
            const uint32_t* const src = reinterpret_cast<const uint32_t*>(srcBitmap->Raster);
            const uint32_t wordsPerLine = srcBitmap->BytesPerLine / 4;

            auto readPixel = [src, wordsPerLine](int32_t x, int32_t y) PALWAYS_INLINE
            {
                return PColor::FromRGB32A(src[y * wordsPerLine + x]);
            };
            auto writePixel = [&writePixel16, bgColor](int32_t x, int32_t y, const PColor& pixel) PALWAYS_INLINE
            {
                writePixel16(PColor::Blend16(pixel, bgColor));
            };

            const PIRect srcBounds = srcOrigRect & PIRect(PIPoint(0, 0), srcBitmap->Size);
            PBlitterUtils::ScaleBitmapBilinear(readPixel, writePixel, srcOrigRect, dstOrigRect, srcRect, dstRect, srcBounds);
            break;
        }
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::WriteFontSpan(const PIRect& bounds, const std::array<uint32_t, 256>& palette, PDrawingMode mode)
{
    auto producePixels = [this, &bounds, &palette](auto writePixel) { WriteFontSpanPixels(bounds, palette, writePixel); };
    WritePixelBlock(bounds, mode == PDrawingMode::Overlay, producePixels);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename PixelWriter>
void RA8875GfxDriver::WriteFontSpanPixels(const PIRect& bounds, const std::array<uint32_t, 256>& palette, PixelWriter writePixel)
{
    const KDisplayTextGlyph* singleGlyph = m_FontText.GetSpan().SingleGlyph;
    if (singleGlyph != nullptr)
    {
        const size_t stride = size_t(singleGlyph->Bounds.Width());
        const size_t column = size_t(bounds.left - singleGlyph->Bounds.left);
        const uint8_t* sourceRow = singleGlyph->Glyph->Raster + size_t(bounds.top - singleGlyph->Bounds.top) * stride;
        for (int y = bounds.top; y < bounds.bottom; ++y)
        {
            const uint8_t* source = sourceRow + column;
            WriteFontRow(bounds.Width(), palette, writePixel, [source]() mutable { return *source++; });
            sourceRow += stride;
        }
    }
    else
    {
        for (int y = bounds.top; y < bounds.bottom; ++y)
        {
            const std::span<const uint8_t*> sources = m_FontText.GetRowSources(bounds.left, y);
            if (sources.size() == 1)
            {
                const uint8_t* source = sources[0];
                WriteFontRow(bounds.Width(), palette, writePixel, [source]() mutable { return *source++; });
            }
            else
            {
                WriteFontRow(bounds.Width(), palette, writePixel, [sources] { return KDisplayText::ReadCoverage(sources); });
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename PixelWriter, typename CoverageReader>
void RA8875GfxDriver::WriteFontRow(
    int width,
    const std::array<uint32_t, 256>& palette,
    PixelWriter writePixel,
    CoverageReader readCoverage)
{
    for (int x = 0; x < width; ++x)
    {
        const uint16_t color = uint16_t(palette[readCoverage()]);
        writePixel(color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult RA8875GfxDriver::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<RA8875GfxDriver*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult RA8875GfxDriver::HandleIRQ()
{
    if (m_PinInterrupt.GetAndClearInterruptStatus())
    {
        m_PinInterrupt.DisableInterrupts();
        m_CondVar.Wakeup(1);
        return IRQResult::HANDLED;
    }
    return IRQResult::UNHANDLED;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::WaitBTE()
{
    static constexpr TimeValNanos BTE_WAIT_TIMEOUT = TimeValNanos::FromMilliseconds(1000);
    static constexpr TimeValNanos BTE_IRQ_RECHECK_INTERVAL = TimeValNanos::FromMilliseconds(100);
    const TimeValNanos deadline = kget_monotonic_time() + BTE_WAIT_TIMEOUT;
    uint16_t status = 0;

    for (;;)
    {
        // Restore interrupts between iterations so SysTick can advance even if INT# stays asserted.
        CRITICAL_SCOPE(CRITICAL_IRQ);

        // Clear stale EXTI state before checking the level of the latched LCD interrupt.
        m_PinInterrupt.GetAndClearInterruptStatus();
        if (!m_PinInterrupt.Read()) {
            WriteCommand(RA8875_INTC2, RA8875_INTC2_BTE_bm);
        }
        status = ReadCommand();
        if ((status & RA8875_STATUS_BTE_BUSY_bm) == 0) {
            return;
        }

        const TimeValNanos currentTime = kget_monotonic_time();
        if (currentTime >= deadline) {
            break;
        }

        m_PinInterrupt.EnableInterrupts();
        // An already asserted pin will not generate another falling edge.
        // IRQWaitDeadline queues the waiter before enabling CPU interrupts.
        if (m_PinInterrupt.Read()) {
            m_CondVar.IRQWaitDeadline(std::min(deadline, currentTime + BTE_IRQ_RECHECK_INTERVAL));
        }
        m_PinInterrupt.DisableInterrupts();
    }
    HandleBTETimeout(status);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::HandleBTETimeout(uint16_t status)
{
    // Stable function-breakpoint location, reached with CPU interrupts enabled.
    m_PinInterrupt.DisableInterrupts();
    m_IsFaulted = true;
    ++m_BTETimeoutCount;
    m_LastBTETimeoutStatus = status;
    kernel_log<PLogSeverity::ERROR>(
        LogCategoryDisplay,
        "RA8875 BTE timeout #{}: status={:#04x}; close/reopen required",
        m_BTETimeoutCount,
        status);
    PERROR_THROW_CODE(PErrorCode::IO);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool RA8875GfxDriver::Reset()
{
    m_PinInterrupt.DisableInterrupts();
    m_IsFaulted = false;
    m_HardwareFgColorValid = false;
    m_HardwareBgColorValid = false;
    m_MouseCursorRaster.clear();
    m_MouseCursorSize = PIPoint(0);
    m_UploadedMouseCursorRasterOffset = PIPoint(0);
    m_CursorHotSpot = PIPoint(0);
    m_MousePos = PIPoint(0);
    m_Orientation = e_Landscape;
    m_FillDirection = e_FillLeftDown;
    m_IsWindowSet = false;
    m_IsMouseCursorVisible = false;
    m_IsMouseCursorRasterUploaded = false;

    if (m_PinLCDReset.GetID() != DigitalPinID::None)
    {
        ksnooze_ms(2);
        m_PinLCDReset.Write(false);
        ksnooze_ms(10);
        m_PinLCDReset.Write(true);
        ksnooze_ms(100);
    }
    else
    {
        WriteCommand(RA8875_PWRR, RA8875_PWRR_SW_RESET_bm);
        ksnooze_ms(10);
        WriteCommand(RA8875_PWRR, 0);
        ksnooze_ms(100);
    }

    PLL_ini();

    WriteCommand(RA8875_SYSR); // SYSR   bit[4:3] color  bit[2:1]=  MPU interface
    WriteData(0x0f);           // 16 BIT     65K

    WriteCommand(RA8875_PCSR); // PCLK
    WriteData(0x81);
    ksnooze_ms(2);

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

    m_LastResetStatus = ReadCommand();
    const uint8_t drawingStatus = ReadRegister(RA8875_DCR);
    if ((m_LastResetStatus & (RA8875_STATUS_BTE_BUSY_bm | RA8875_STATUS_MEMORY_BUSY_bm)) != 0
        || (drawingStatus & (RA8875_DCR_LINE_SQR_TRI_bm | RA8875_DCR_CIRCLE_bm)) != 0)
    {
        m_IsFaulted = true;
        kernel_log<PLogSeverity::ERROR>(
            LogCategoryDisplay,
            "RA8875 reset failed: status={:#04x}, drawing status={:#04x}",
            m_LastResetStatus,
            drawingStatus);
        return false;
    }

    UpdateAddressMode();
    const PIRect screenFrame(PIPoint(0), GetResolution());
    SetWindow(screenFrame);
    m_IsWindowSet = false;
    SetFgColor(PColor(0));
    SetBgColor(PColor(0));
    FillRect(&m_ScreenBitmap, screenFrame);
    WaitIdle();
    SetGraphicCursorEnabled(false);

    WriteCommand(RA8875_INTC2, RA8875_INTC2_BTE_bm);
    m_PinInterrupt.GetAndClearInterruptStatus();
    WriteCommand(RA8875_INTC1, RA8875_INTC1_BTE_bm);
    WriteCommand(RA8875_PWRR, RA8875_PWRR_DISPLAY_ON_bm);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::PLL_ini()
{
    WriteCommand(RA8875_PLLC1);
    ksnooze_ms(2);
    WriteData(0x0b);
    ksnooze_ms(2);
    WriteCommand(RA8875_PLLC2);
    ksnooze_ms(2);
    WriteData(0x02);
    ksnooze_ms(2);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetHardwareFgColor(uint16_t color)
{
    if (!m_HardwareFgColorValid || m_HardwareFgColor != color)
    {
        WaitBlitter();
        WriteCommand(RA8875_FGCR0);
        WriteData((color >> 11) & 0x1f);
        WriteCommand(RA8875_FGCR1);
        WriteData((color >> 5) & 0x3f);
        WriteCommand(RA8875_FGCR2);
        WriteData(color & 0x1f);
        m_HardwareFgColor = color;
        m_HardwareFgColorValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetHardwareBgColor(uint16_t color)
{
    if (!m_HardwareBgColorValid || m_HardwareBgColor != color)
    {
        WaitBlitter();
        WriteCommand(RA8875_BGCR0);
        WriteData((color >> 11) & 0x1f);
        WriteCommand(RA8875_BGCR1);
        WriteData((color >> 5) & 0x3f);
        WriteCommand(RA8875_BGCR2);
        WriteData(color & 0x1f);
        m_HardwareBgColor = color;
        m_HardwareBgColorValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetTransparantColor(uint16_t color)
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

uint8_t RA8875GfxDriver::ConvertMouseCursorPixelToRA8875(PMouseCursorPixel pixel)
{
    switch (pixel)
    {
        case PMouseCursorPixel::Color1:
            return RA8875_MOUSE_CURSOR_PIXEL_COLOR1;
        case PMouseCursorPixel::Color2:
            return RA8875_MOUSE_CURSOR_PIXEL_COLOR2;
        case PMouseCursorPixel::Transparent:
            return RA8875_MOUSE_CURSOR_PIXEL_TRANSPARENT;
        case PMouseCursorPixel::Invert:
            return RA8875_MOUSE_CURSOR_PIXEL_INVERT;
    }
    return RA8875_MOUSE_CURSOR_PIXEL_TRANSPARENT;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetMouseCursorColors(PColor color1, PColor color2)
{
    WriteCommand(RA8875_GCC0, color1.GetColor332());
    WriteCommand(RA8875_GCC1, color2.GetColor332());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::UploadMouseCursorRaster(PIPoint rasterOffset)
{
    std::array<uint8_t, RA8875_GRAPHIC_CURSOR_BYTES> mouseCursorBuffer = {};
    mouseCursorBuffer.fill(RA8875_MOUSE_CURSOR_TRANSPARENT_BYTE);

    for (int32_t sourceY = 0; sourceY < m_MouseCursorSize.y; ++sourceY)
    {
        const int32_t destY = sourceY + rasterOffset.y;
        if (destY < 0 || destY >= RA8875_GRAPHIC_CURSOR_SIZE) {
            continue;
        }

        for (int32_t sourceX = 0; sourceX < m_MouseCursorSize.x; ++sourceX)
        {
            const int32_t destX = sourceX + rasterOffset.x;
            if (destX < 0 || destX >= RA8875_GRAPHIC_CURSOR_SIZE) {
                continue;
            }

            const size_t srcIndex = size_t(sourceY) * size_t(m_MouseCursorSize.x) + size_t(sourceX);
            const size_t dstIndex = size_t(destY) * size_t(RA8875_GRAPHIC_CURSOR_SIZE / 4) + size_t(destX / 4);
            const uint8_t shift = uint8_t((3 - (destX & 3)) * 2);
            const uint8_t mask = uint8_t(0x03 << shift);
            const uint8_t pixel = m_MouseCursorRaster[srcIndex];
            mouseCursorBuffer[dstIndex] = uint8_t((mouseCursorBuffer[dstIndex] & ~mask) | (pixel << shift));
        }
    }

    WaitBlitter();

    const uint8_t previousMWCR0 = ReadRegister(RA8875_MWCR0);
    const uint8_t previousMWCR1 = ReadRegister(RA8875_MWCR1);
    const uint8_t cursorWriteControl = uint8_t((previousMWCR1 & ~(RA8875_MWCR1_WRITE_DEST_bm | RA8875_MWCR1_GCURSOR_SELECT_bm)) | RA8875_MWCR1_WRITE_DEST_GCURSOR_bg);
    const uint8_t memoryWriteControl = uint8_t((previousMWCR0 & ~RA8875_MWCR0_DIRECTON_bm) | RA8875_MWCR0_LR_TD_bg);

    WriteCommand(RA8875_MWCR1, cursorWriteControl);
    WriteCommand(RA8875_MWCR0, memoryWriteControl);
    WriteCommand(RA8875_CURH0, RA8875_CURH1, uint16_t(0));
    WriteCommand(RA8875_CURV0, RA8875_CURV1, uint16_t(0));
    BeginWriteData();

    for (size_t i = 0; i < RA8875_GRAPHIC_CURSOR_BYTES; ++i)
    {
        WaitMemory();
        WriteData(mouseCursorBuffer[i]);
    }
    WaitMemory();

    WriteCommand(RA8875_MWCR1, previousMWCR1);
    WriteCommand(RA8875_MWCR0, previousMWCR0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetGraphicCursorPosition(PIPoint position)
{
    const PIPoint desiredOrigin = position - m_CursorHotSpot;

    const PIPoint resolution = GetResolution();
    PIPoint hardwareOrigin;
    hardwareOrigin.x = std::clamp(desiredOrigin.x, 0, std::max(0, resolution.x - 1));
    hardwareOrigin.y = std::clamp(desiredOrigin.y, 0, std::max(0, resolution.y - 1));

    const PIPoint rasterOffset = desiredOrigin - hardwareOrigin;
    const bool needsRasterUpload = !m_MouseCursorRaster.empty() && (!m_IsMouseCursorRasterUploaded || rasterOffset != m_UploadedMouseCursorRasterOffset);
    if (needsRasterUpload)
    {
        UploadMouseCursorRaster(rasterOffset);
        m_UploadedMouseCursorRasterOffset = rasterOffset;
        m_IsMouseCursorRasterUploaded = true;
    }

    WaitBlitter();

    WriteCommand(RA8875_GCHP0, RA8875_GCHP1, uint16_t(hardwareOrigin.x));
    WriteCommand(RA8875_GCVP0, RA8875_GCVP1, uint16_t(hardwareOrigin.y));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetGraphicCursorEnabled(bool enabled)
{
    uint8_t control = ReadRegister(RA8875_MWCR1);
    if (enabled) {
        control |= RA8875_MWCR1_GCURSOR_ENABLE_bm;
    } else {
        control &= ~RA8875_MWCR1_GCURSOR_ENABLE_bm;
    }
    WriteCommand(RA8875_MWCR1, control);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t RA8875GfxDriver::ReadRegister(uint8_t command)
{
    WriteCommand(command);
    return uint8_t(ReadData());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::SetWindow(int x1, int y1, int x2, int y2)
{
    if (m_Orientation == e_Portrait)
    {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }
    m_IsWindowSet = true;

    WaitBlitter();

    WriteCommand(RA8875_HSAW0, RA8875_HSAW1, uint16_t(x1));
    WriteCommand(RA8875_HEAW0, RA8875_HEAW1, uint16_t(x2 - 1));
    WriteCommand(RA8875_VSAW0, RA8875_VSAW1, uint16_t(y1));
    WriteCommand(RA8875_VEAW0, RA8875_VEAW1, uint16_t(y2 - 1));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void RA8875GfxDriver::UnsetWindow()
{
    if (m_IsWindowSet)
    {
        SetWindow(PIRect(PIPoint(0), GetResolution()));
        m_IsWindowSet = false;
    }
}

} // namespace kernel
