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
#include <GUI/Color.h>
#include <GUI/Font.h>
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
    SetScreenMode
};

struct PDisplayInfo
{
    PDisplayBitmap ScreenBitmap;
    int ModeCount = 0;
};

struct PDisplayBatchResult
{
    uint32_t TextPosition = 0;
    bool CursorAccepted = false;
};

class PDisplayDeviceControl : public PDeviceControlInterface
{
public:
    PDisplayDeviceControl()
        : Initialize(*this)
        , Submit(*this)
        , GetScreenMode(*this)
        , SetScreenMode(*this)
    {
    }

    explicit PDisplayDeviceControl(int fileHandle) : PDisplayDeviceControl() { SetDeviceFD(fileHandle); }

    PDeviceControlInvoker<std::to_underlying(PDisplayRequest::Initialize), PDisplayInfo()> Initialize;
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::Submit),
        PDisplayBatchResult(const void* buffer, size_t length)
    > Submit;
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::GetScreenMode),
        bool(size_t index, PScreenMode* outMode)
    > GetScreenMode;
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::SetScreenMode),
        bool(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate)
    > SetScreenMode;
};

static constexpr size_t DISPLAY_COMMAND_ALIGNMENT = 8;
static constexpr size_t DISPLAY_COMMAND_BUFFER_SIZE = 16 * 1024;

enum class PDisplayCommandID : uint32_t
{
    SetFgColor,
    SetBgColor,
    SetColor,
    PowerLost,
    SetMouseCursorBitmap,
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
    WriteString
};

struct PDisplayCommandHeader
{
    PDisplayCommandID Code;
    uint32_t Length;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplaySetFgColorCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::SetFgColor;
    PDisplayCommandHeader Header;
    PColor Color;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplaySetBgColorCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::SetBgColor;
    PDisplayCommandHeader Header;
    PColor Color;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplaySetColorCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::SetColor;
    PDisplayCommandHeader Header;
    size_t Index;
    PColor Color;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayPowerLostCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::PowerLost;
    PDisplayCommandHeader Header;
    bool HasPower;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplaySetMouseCursorBitmapCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::SetMouseCursorBitmap;
    PDisplayCommandHeader Header;
    int32_t Width;
    int32_t Height;
    PIPoint HotSpot;
    PColor Color1;
    PColor Color2;
    size_t PixelCount;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplaySetMouseCursorVisibleCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::SetMouseCursorVisible;
    PDisplayCommandHeader Header;
    bool Visible;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplaySetMousePosCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::SetMousePos;
    PDisplayCommandHeader Header;
    PIPoint Position;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayWritePixelCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::WritePixel;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    PIPoint Position;
    PColor Color;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayDrawLineCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::DrawLine;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    PIRect ClipRect;
    PIPoint Position1;
    PIPoint Position2;
    PColor Color;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayFillPolygonCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::FillPolygon;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    size_t ClipCount;
    size_t PointCount;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayFillTriangleCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::FillTriangle;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    PIRect ClipRect;
    PIPoint Position1;
    PIPoint Position2;
    PIPoint Position3;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayFillTriangleFanCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::FillTriangleFan;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    size_t ClipCount;
    size_t PointCount;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayFillTriangleStripCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::FillTriangleStrip;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    size_t ClipCount;
    size_t PointCount;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayFillRectCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::FillRect;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    PIRect Rect;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayCopyRectCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::CopyRect;
    PDisplayCommandHeader Header;
    PDisplayBitmap Destination;
    PDisplayBitmap Source;
    PColor Background;
    PColor Foreground;
    PIRect SourceRect;
    PIPoint Position;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayScaleRectCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::ScaleRect;
    PDisplayCommandHeader Header;
    PDisplayBitmap Destination;
    PDisplayBitmap Source;
    PColor Background;
    PColor Foreground;
    PIRect SourceOriginal;
    PIRect DestinationOriginal;
    PRect SourceRect;
    PIRect DestinationRect;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayFillCircleCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::FillCircle;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    PIRect ClipRect;
    PIPoint Center;
    int32_t Radius;
    PColor Color;
    PDrawingMode Mode;
};

struct alignas(DISPLAY_COMMAND_ALIGNMENT) PDisplayWriteStringCommand
{
    static constexpr PDisplayCommandID CODE = PDisplayCommandID::WriteString;
    PDisplayCommandHeader Header;
    PDisplayBitmap Bitmap;
    PIPoint Position;
    size_t StringLength;
    PIRect ClipRect;
    PColor Background;
    PColor Foreground;
    PFontID FontID;
};

