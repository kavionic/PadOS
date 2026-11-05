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
#include <GUI/BrushTypes.h>
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


// Identifies a resource registered with one open kernel driver session.
// These IDs are separate from appserver bitmap and brush handles.
using PDriverResourceID = uint32_t;
static constexpr PDriverResourceID DRIVER_INVALID_RESOURCE_ID = 0;

// The owner preserves raster storage and validated access grants while any
// registered bitmap, brush, selected composition, or pending command uses it.
// Deleting an ID removes that handle; dependent resources retain their objects.
// Device close releases all references after completing hardware accesses.
// Future address-span revocation must retire these uses before removing grants.
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
    SetMouseCursorBitmap,
    CreateBitmap,
    DeleteBitmap,
    CreateBrush,
    DeleteBrush
};

struct PDisplayInfo
{
    PDisplayBitmap ScreenBitmap;
    PDriverResourceID ScreenBitmapDriverResourceID = DRIVER_INVALID_RESOURCE_ID;
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
        , CreateBitmap(*this)
        , DeleteBitmap(*this)
        , CreateBrush(*this)
        , DeleteBrush(*this)
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
    PDeviceControlInvoker<std::to_underlying(PDisplayRequest::CreateBitmap), PDriverResourceID(PDisplayBitmap bitmap, bool writable)> CreateBitmap;
    PDeviceControlInvoker<std::to_underlying(PDisplayRequest::DeleteBitmap), void(PDriverResourceID bitmap)> DeleteBitmap;
    PDeviceControlInvoker<
        std::to_underlying(PDisplayRequest::CreateBrush),
        PDriverResourceID(PDriverResourceID bitmap, const PBrushDescription& description)
    > CreateBrush;
    PDeviceControlInvoker<std::to_underlying(PDisplayRequest::DeleteBrush), void(PDriverResourceID brush)> DeleteBrush;
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
    SetBrush,
    FillBrushRect,
    UpdateBrush,
    SetBrushColor,
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
        , SetBrush(driver)
        , FillBrushRect(driver)
        , UpdateBrush(driver)
        , SetBrushColor(driver)
    {
    }

    PDisplayDriverInvoker<PDisplayCommandID::SetFgColor, void(PColor color)> SetFgColor;
    PDisplayDriverInvoker<PDisplayCommandID::SetBgColor, void(PColor color)> SetBgColor;
    PDisplayDriverInvoker<PDisplayCommandID::SetColor, void(size_t index, PColor color)> SetColor;
    PDisplayDriverInvoker<PDisplayCommandID::PowerLost, void(bool hasPower)> PowerLost;
    PDisplayDriverInvoker<PDisplayCommandID::SetMouseCursorVisible, void(bool visible)> SetMouseCursorVisible;
    PDisplayDriverInvoker<PDisplayCommandID::SetMousePos, void(PIPoint position)> SetMousePos;
    PDisplayDriverInvoker<PDisplayCommandID::WritePixel, void(PDriverResourceID bitmap, PIPoint position, PColor color)> WritePixel;
    PDisplayDriverInvoker<
        PDisplayCommandID::DrawLine,
        void(PDriverResourceID bitmap, PIRect clipRect, PIPoint position1, PIPoint position2, PColor color, PDrawingMode mode)
    > DrawLine;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillPolygon,
        void(PDriverResourceID bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    > FillPolygon;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillTriangle,
        void(PDriverResourceID bitmap, PIRect clipRect, PIPoint position1, PIPoint position2, PIPoint position3, PDrawingMode mode)
    > FillTriangle;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillTriangleFan,
        void(PDriverResourceID bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    > FillTriangleFan;
    PDisplayDriverInvoker<
        PDisplayCommandID::FillTriangleStrip,
        void(PDriverResourceID bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    > FillTriangleStrip;
    PDisplayDriverInvoker<PDisplayCommandID::FillRect, void(PDriverResourceID bitmap, PIRect rect)> FillRect;
    PDisplayDriverInvoker<
        PDisplayCommandID::CopyRect,
        void(
            PDriverResourceID destination,
            PDriverResourceID source,
            PColor background,
            PColor foreground,
            PIRect sourceRect,
            PIPoint position,
            PDrawingMode mode)
    > CopyRect;
    PDisplayDriverInvoker<
        PDisplayCommandID::ScaleRect,
        void(
            PDriverResourceID destination,
            PDriverResourceID source,
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
        void(PDriverResourceID bitmap, PIRect clipRect, PIPoint center, int32_t radius, PColor color, PDrawingMode mode)
    > FillCircle;
    // Payload: clips, optional int32_t kerning adjustments, UTF-8.
    // One signed 26.6 adjustment per decoded character, applied before its glyph.
    // Fonts without kerning omit the adjustment array and use a zero count.
    PDisplayDriverInvoker<
        PDisplayCommandID::WriteString,
        void(
            PDriverResourceID bitmap,
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
            PDriverResourceID bitmap,
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

    // SetBrush payload contains layerCount PDisplayBrushBinding records. State persists across submissions.
    PDisplayDriverInvoker<PDisplayCommandID::SetBrush, void(size_t layerCount)> SetBrush;
    PDisplayDriverInvoker<PDisplayCommandID::FillBrushRect, void(PDriverResourceID bitmap, PIRect bounds)> FillBrushRect;
    PDisplayDriverInvoker<
        PDisplayCommandID::UpdateBrush,
        void(PDriverResourceID brush, PDriverResourceID bitmap, const PBrushDescription& description)
    > UpdateBrush;
    // Sets a solid color, releasing any previous bitmap at execution time.
    PDisplayDriverInvoker<PDisplayCommandID::SetBrushColor, void(PDriverResourceID brush, PColor color)> SetBrushColor;
};
