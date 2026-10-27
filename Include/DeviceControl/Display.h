// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <DeviceControl/DeviceControlInvoker.h>
#include <DeviceControl/DisplayFont.h>
#include <ApplicationServer/DisplayDriverInvoker.h>
#include <GUI/Color.h>
#include <GUI/GUIDefines.h>
#include <GUI/MouseCursor.h>
#include <Math/Rect.h>

struct PScreenMode
{
    PScreenMode() {}
    PScreenMode(const PIPoint& resolution, int bytesPerLine, PEColorSpace colorSpace)
        : m_Resolution(resolution)
        , m_BytesPerLine(bytesPerLine)
        , m_ColorSpace(colorSpace)
    {}
    PIPoint      m_Resolution;
    size_t      m_BytesPerLine = 0;
    PEColorSpace m_ColorSpace = PEColorSpace::NO_COLOR_SPACE;
};


// Raster storage remains owned by the appserver/client until submission completes.
struct PDisplayBitmap
{
    PEColorSpace ColorSpace = PEColorSpace::NO_COLOR_SPACE;
    PIPoint Size;
    size_t BytesPerLine = 0;
    uint8_t* Raster = nullptr;
    bool VideoMemory = false;
};

enum class PDisplayRequest : int
{
    Initialize,
    Submit,
    GetScreenMode,
    SetScreenMode,
    SetMouseCursorBitmap
};

struct PDisplayInfo
{
    PDisplayBitmap ScreenBitmap;
    int ModeCount = 0;
};

class PDisplayDeviceControl : public PDeviceControlInterface
{
public:
    PDisplayDeviceControl()
        : Initialize(*this)
        , Submit(*this)
        , GetScreenMode(*this)
        , SetScreenMode(*this)
        , SetMouseCursorBitmap(*this)
    {
    }

    explicit PDisplayDeviceControl(int fileHandle) : PDisplayDeviceControl() { SetDeviceFD(fileHandle); }

    PDeviceControlInvoker<std::to_underlying(PDisplayRequest::Initialize), PDisplayInfo()> Initialize;
    // EIO requires closing and reopening the device, restoring all driver state, and repainting all views.
    // A failed batch can be partially executed and must not be replayed.
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::Submit),
        void(const void* buffer, size_t length)
    > Submit;
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::GetScreenMode),
        bool(size_t index, PScreenMode* outMode)
    > GetScreenMode;
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::SetScreenMode),
        bool(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate)
    > SetScreenMode;
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::SetMouseCursorBitmap),
        bool(const PMouseCursorBitmap& cursor)
    > SetMouseCursorBitmap;
};

static constexpr size_t DISPLAY_COMMAND_ALIGNMENT = 8;
static constexpr size_t DISPLAY_COMMAND_BUFFER_SIZE = 16 * 1024;

enum class PDisplayCommandID : uint32_t
{
    SetFgColor,
    SetBgColor,
    SetColor,
    PowerLost,
    SetMouseCursorVisible,
    SetMousePos,
    WritePixel,
    DrawLine,
    FillPolygon,
    FillTriangle,
    FillTriangleFan,
    FillTriangleStrip,
    FillRect,
    CopyRect,
    ScaleRect,
    FillCircle,
    WriteString,
    WriteStringWidth,
    Count
};

struct PDisplayCommandHeader
{
    PDisplayCommandID Code;
    uint32_t Length;
};

class PDisplayCommands
{
public:
    explicit PDisplayCommands(PDisplayDriver& driver)
        : SetFgColor(driver)
        , SetBgColor(driver)
        , SetColor(driver)
        , PowerLost(driver)
        , SetMouseCursorVisible(driver)
        , SetMousePos(driver)
        , WritePixel(driver)
        , DrawLine(driver)
        , FillPolygon(driver)
        , FillTriangle(driver)
        , FillTriangleFan(driver)
        , FillTriangleStrip(driver)
        , FillRect(driver)
        , CopyRect(driver)
        , ScaleRect(driver)
        , FillCircle(driver)
        , WriteString(driver)
        , WriteStringWidth(driver)
    {
    }

    PDisplayDriverInvoker<PDisplayCommandID::SetFgColor, void(PColor color)> SetFgColor;
    PDisplayDriverInvoker<PDisplayCommandID::SetBgColor, void(PColor color)> SetBgColor;
    PDisplayDriverInvoker<PDisplayCommandID::SetColor, void(size_t index, PColor color)> SetColor;
    PDisplayDriverInvoker<PDisplayCommandID::PowerLost, void(bool hasPower)> PowerLost;
    PDisplayDriverInvoker<PDisplayCommandID::SetMouseCursorVisible, void(bool visible)> SetMouseCursorVisible;
    PDisplayDriverInvoker<PDisplayCommandID::SetMousePos, void(PIPoint position)> SetMousePos;
    PDisplayDriverInvoker<PDisplayCommandID::WritePixel, void(PDisplayBitmap bitmap, PIPoint position, PColor color)> WritePixel;
    PDisplayDriverInvoker<
        PDisplayCommandID::DrawLine,
        void(PDisplayBitmap bitmap, PIRect clipRect, PIPoint position1, PIPoint position2, PColor color, PDrawingMode mode)
    > DrawLine;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillPolygon,
        void(PDisplayBitmap bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    > FillPolygon;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillTriangle,
        void(PDisplayBitmap bitmap, PIRect clipRect, PIPoint position1, PIPoint position2, PIPoint position3, PDrawingMode mode)
    > FillTriangle;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillTriangleFan,
        void(PDisplayBitmap bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    > FillTriangleFan;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillTriangleStrip,
        void(PDisplayBitmap bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    > FillTriangleStrip;
    PDisplayDriverInvoker<PDisplayCommandID::FillRect, void(PDisplayBitmap bitmap, PIRect rect)> FillRect;
    PDisplayDriverInvoker<
        PDisplayCommandID::CopyRect,
        void(
            PDisplayBitmap destination,
            PDisplayBitmap source,
            PColor background,
            PColor foreground,
            PIRect sourceRect,
            PIPoint position,
            PDrawingMode mode)
    > CopyRect;
    PDisplayDriverInvoker<
        PDisplayCommandID::ScaleRect,
        void(
            PDisplayBitmap destination,
            PDisplayBitmap source,
            PColor background,
            PColor foreground,
            PIRect sourceOriginal,
            PIRect destinationOriginal,
            PRect sourceRect,
            PIRect destinationRect,
            PDrawingMode mode)
    > ScaleRect;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillCircle,
        void(PDisplayBitmap bitmap, PIRect clipRect, PIPoint center, int32_t radius, PColor color, PDrawingMode mode)
    > FillCircle;
    // Payload: clips, optional int32_t kerning adjustments, UTF-8.
    // One signed 26.6 adjustment per decoded character, applied before its glyph.
    // Fonts without kerning omit the adjustment array and use a zero count.
    PDisplayDriverInvoker<
        PDisplayCommandID::WriteString,
        void(
            PDisplayBitmap bitmap,
            PIPoint position,
            const PDisplayFontCache* font,
            size_t stringLength,
            size_t clipCount,
            size_t kerningCount,
            PColor background,
            PColor foreground,
            PPoint advance,
            PDrawingMode mode)
    > WriteString;
    PDisplayDriverInvoker<
        PDisplayCommandID::WriteStringWidth,
        void(
            PDisplayBitmap bitmap,
            PIPoint position,
            const PDisplayFontCache* font,
            size_t stringLength,
            size_t clipCount,
            size_t kerningCount,
            PColor background,
            PColor foreground,
            PPoint advance,
            PDrawingMode mode,
            float width)
    > WriteStringWidth;
};
