// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <System/ExceptionHandling.h>
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayDriver::PDisplayDriver(const PString& devicePath)
    : m_DevicePath(devicePath)
    , m_CommandBuffer(DISPLAY_COMMAND_BUFFER_SIZE / sizeof(uint64_t))
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayDriver::~PDisplayDriver()
{
    try
    {
        Close();
    }
    catch (const std::exception& error)
    {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "Closing display driver failed: {}", error.what());
    }
    if (m_ScreenBitmap != nullptr) {
        m_ScreenBitmap->m_Driver = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDisplayDriver::Open()
{
    if (m_DeviceControl.GetDeviceFD() >= 0) {
        return true;
    }
    const int deviceFD = open(m_DevicePath.c_str(), O_RDWR);
    if (deviceFD < 0) {
        return false;
    }
    m_DeviceControl.SetDeviceFD(deviceFD);
    try
    {
        m_Info = m_DeviceControl.Initialize();
        const PDisplayBitmap& screen = m_Info.ScreenBitmap;
        m_ScreenBitmap = ptr_new<PSrvBitmap>(PIPoint(0, 0), screen.ColorSpace);
        m_ScreenBitmap->m_Size = screen.Size;
        m_ScreenBitmap->m_BytesPerLine = screen.BytesPerLine;
        m_ScreenBitmap->m_Raster = screen.Raster;
        m_ScreenBitmap->m_VideoMem = screen.VideoMemory;
        m_ScreenBitmap->m_Driver = this;
        m_FgColor = PColor(0);
        m_BgColor = PColor(0);
    }
    catch (...)
    {
        close(deviceFD);
        m_DeviceControl.SetDeviceFD(-1);
        throw;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::Close()
{
    if (m_DeviceControl.GetDeviceFD() >= 0)
    {
        PScopeExit closeDevice([this]
        {
            m_UsedBufferSize = 0;
            close(m_DeviceControl.GetDeviceFD());
            m_DeviceControl.SetDeviceFD(-1);
        });
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::PowerLost(bool hasPower)
{
    PDisplayPowerLostCommand* storage = AllocCommand<PDisplayPowerLostCommand>();
    ConstructCommand(storage, 0, hasPower);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PSrvBitmap> PDisplayDriver::GetScreenBitmap()
{
    return m_ScreenBitmap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PDisplayDriver::GetScreenModeCount()
{
    return m_Info.ModeCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDisplayDriver::GetScreenModeDesc(size_t index, PScreenMode& outMode)
{
    Flush();
    return m_DeviceControl.GetScreenMode(index, &outMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDisplayDriver::SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate)
{
    Flush();
    const bool accepted = m_DeviceControl.SetScreenMode(resolution, colorSpace, refreshRate);
    if (accepted)
    {
        m_Info = m_DeviceControl.Initialize();
        const PDisplayBitmap& screen = m_Info.ScreenBitmap;
        m_ScreenBitmap->m_Size = screen.Size;
        m_ScreenBitmap->m_ColorSpace = screen.ColorSpace;
        m_ScreenBitmap->m_BytesPerLine = screen.BytesPerLine;
        m_ScreenBitmap->m_Raster = screen.Raster;
        m_ScreenBitmap->m_VideoMem = screen.VideoMemory;
    }
    return accepted;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIPoint PDisplayDriver::GetResolution()
{
    return m_Info.ScreenBitmap.Size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PDisplayDriver::GetBytesPerLine()
{
    return int(m_Info.ScreenBitmap.BytesPerLine);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PEColorSpace PDisplayDriver::GetColorSpace()
{
    return m_Info.ScreenBitmap.ColorSpace;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetColor(size_t index, PColor color)
{
    PDisplaySetColorCommand* storage = AllocCommand<PDisplaySetColorCommand>();
    ConstructCommand(storage, 0, index, color);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDisplayDriver::SetMouseCursorBitmap(const PMouseCursorBitmap& cursor)
{
    if (cursor.Width <= 0 || cursor.Height <= 0
        || size_t(cursor.Width) > std::numeric_limits<size_t>::max() / size_t(cursor.Height)) {
        return false;
    }
    const size_t count = size_t(cursor.Width) * size_t(cursor.Height);
    if (cursor.Raster.size() < count) {
        return false;
    }
    const size_t payloadSize = count * sizeof(PMouseCursorPixel);
    PDisplaySetMouseCursorBitmapCommand* storage = AllocCommand<PDisplaySetMouseCursorBitmapCommand>(payloadSize);
    PDisplaySetMouseCursorBitmapCommand* command = ConstructCommand(
        storage,
        payloadSize,
        cursor.Width,
        cursor.Height,
        cursor.HotSpot,
        cursor.Color1,
        cursor.Color2,
        count);
    std::uninitialized_copy_n(cursor.Raster.data(), count, reinterpret_cast<PMouseCursorPixel*>(command + 1));
    Flush();
    return m_LastResult.CursorAccepted;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetMouseCursorVisible(bool visible)
{
    PDisplaySetMouseCursorVisibleCommand* storage = AllocCommand<PDisplaySetMouseCursorVisibleCommand>();
    ConstructCommand(storage, 0, visible);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetMousePos(PIPoint position)
{
    PDisplaySetMousePosCommand* storage = AllocCommand<PDisplaySetMousePosCommand>();
    ConstructCommand(storage, 0, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetFgColor(PColor color)
{
    if (color.GetColor32() != m_FgColor.GetColor32())
    {
        PDisplaySetFgColorCommand* storage = AllocCommand<PDisplaySetFgColorCommand>();
        ConstructCommand(storage, 0, color);
        m_FgColor = color;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetBgColor(PColor color)
{
    if (color.GetColor32() != m_BgColor.GetColor32())
    {
        PDisplaySetBgColorCommand* storage = AllocCommand<PDisplaySetBgColorCommand>();
        ConstructCommand(storage, 0, color);
        m_BgColor = color;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::WritePixel(PSrvBitmap* bitmap, const PIPoint& pos, PColor color)
{
    PDisplayWritePixelCommand* storage = AllocCommand<PDisplayWritePixelCommand>();
    ConstructCommand(storage, 0, ReferenceBitmap(bitmap), pos, color);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::DrawLine(
    PSrvBitmap* bitmap,
    const PIRect& clipRect,
    const PIPoint& pos1,
    const PIPoint& pos2,
    const PColor& color,
    PDrawingMode mode)
{
    PDisplayDrawLineCommand* storage = AllocCommand<PDisplayDrawLineCommand>();
    ConstructCommand(storage, 0, ReferenceBitmap(bitmap), clipRect, pos1, pos2, color, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillPolygon(
    PSrvBitmap* bitmap,
    std::span<const PIRect> clipRects,
    std::span<const PPoint> points,
    PDrawingMode mode)
{
    PDisplayPolygonBuffer payload = AllocPolygon(bitmap, clipRects.size(), points.size(), mode);
    std::uninitialized_copy(clipRects.begin(), clipRects.end(), payload.ClipRects.data());
    std::uninitialized_copy(points.begin(), points.end(), payload.Points.data());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillTriangle(
    PSrvBitmap* bitmap,
    const PIRect& clipRect,
    const PIPoint& pos1,
    const PIPoint& pos2,
    const PIPoint& pos3,
    PDrawingMode mode)
{
    PDisplayFillTriangleCommand* storage = AllocCommand<PDisplayFillTriangleCommand>();
    ConstructCommand(storage, 0, ReferenceBitmap(bitmap), clipRect, pos1, pos2, pos3, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillTriangleFan(
    PSrvBitmap* bitmap,
    const PIRect& clipRect,
    std::span<const PPoint> points,
    PDrawingMode mode)
{
    PDisplayPolygonBuffer payload = AllocTriangleFan(bitmap, 1, points.size(), mode);
    std::construct_at(payload.ClipRects.data(), clipRect);
    std::uninitialized_copy(points.begin(), points.end(), payload.Points.data());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillTriangleStrip(
    PSrvBitmap* bitmap,
    const PIRect& clipRect,
    std::span<const PPoint> points,
    PDrawingMode mode)
{
    PDisplayPolygonBuffer payload = AllocTriangleStrip(bitmap, 1, points.size(), mode);
    std::construct_at(payload.ClipRects.data(), clipRect);
    std::uninitialized_copy(points.begin(), points.end(), payload.Points.data());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillRect(PSrvBitmap* bitmap, const PIRect& rect)
{
    PDisplayFillRectCommand* storage = AllocCommand<PDisplayFillRectCommand>();
    ConstructCommand(storage, 0, ReferenceBitmap(bitmap), rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::CopyRect(
    PSrvBitmap* dstBitmap,
    PSrvBitmap* srcBitmap,
    PColor bgColor,
    PColor fgColor,
    const PIRect& srcRect,
    const PIPoint& dstPos,
    PDrawingMode mode)
{
    PDisplayCopyRectCommand* storage = AllocCommand<PDisplayCopyRectCommand>();
    ConstructCommand(storage, 0, ReferenceBitmap(dstBitmap), ReferenceBitmap(srcBitmap), bgColor, fgColor, srcRect, dstPos, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::ScaleRect(
    PSrvBitmap* dstBitmap,
    PSrvBitmap* srcBitmap,
    PColor bgColor,
    PColor fgColor,
    const PIRect& srcOrigRect,
    const PIRect& dstOrigRect,
    const PRect& srcRect,
    const PIRect& dstRect,
    PDrawingMode mode)
{
    PDisplayScaleRectCommand* storage = AllocCommand<PDisplayScaleRectCommand>();
    ConstructCommand(
        storage,
        0,
        ReferenceBitmap(dstBitmap),
        ReferenceBitmap(srcBitmap),
        bgColor,
        fgColor,
        srcOrigRect,
        dstOrigRect,
        srcRect,
        dstRect,
        mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillCircle(
    PSrvBitmap* bitmap,
    const PIRect& clipRect,
    const PIPoint& center,
    int32_t radius,
    const PColor& color,
    PDrawingMode mode)
{
    PDisplayFillCircleCommand* storage = AllocCommand<PDisplayFillCircleCommand>();
    ConstructCommand(storage, 0, ReferenceBitmap(bitmap), clipRect, center, radius, color, mode);
    m_FgColor = color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t PDisplayDriver::WriteString(
    PSrvBitmap* bitmap,
    const PIPoint& position,
    const char* string,
    size_t strLength,
    const PIRect& clipRect,
    PColor colorBg,
    PColor colorFg,
    PFontID fontID)
{
    DrawString(bitmap, position, string, strLength, clipRect, colorBg, colorFg, fontID);
    Flush();
    return m_LastResult.TextPosition;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PDisplayDriver::GetPaletteEntry(uint8_t index)
{
    return PColor::FromCMAP8(index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::DrawString(
    PSrvBitmap* bitmap,
    const PIPoint& position,
    const char* string,
    size_t length,
    const PIRect& clipRect,
    PColor background,
    PColor foreground,
    PFontID fontID)
{
    PDisplayWriteStringCommand* storage = AllocCommand<PDisplayWriteStringCommand>(length);
    PDisplayWriteStringCommand* command = ConstructCommand(
        storage,
        length,
        ReferenceBitmap(bitmap),
        position,
        length,
        clipRect,
        background,
        foreground,
        fontID);
    if (length != 0) {
        std::memcpy(static_cast<void*>(command + 1), string, length);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayPolygonBuffer PDisplayDriver::AllocPolygon(
    PSrvBitmap* bitmap,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    return AllocGeometry<PDisplayFillPolygonCommand>(bitmap, clipCount, pointCount, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayPolygonBuffer PDisplayDriver::AllocTriangleFan(
    PSrvBitmap* bitmap,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    return AllocGeometry<PDisplayFillTriangleFanCommand>(bitmap, clipCount, pointCount, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayPolygonBuffer PDisplayDriver::AllocTriangleStrip(
    PSrvBitmap* bitmap,
    size_t clipCount,
    size_t pointCount,
    PDrawingMode mode)
{
    return AllocGeometry<PDisplayFillTriangleStripCommand>(bitmap, clipCount, pointCount, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::Flush()
{
    if (m_UsedBufferSize != 0)
    {
        // The kernel drains started hardware operations before returning, including on errors.
        const size_t size = m_UsedBufferSize;
        m_UsedBufferSize = 0;
        PScopeExit advanceSequence([this] { ++m_Sequence; });
        try
        {
            m_LastResult = m_DeviceControl.Submit(m_CommandBuffer.data(), size);
        }
        catch (...)
        {
            // Restore cached colors before subsequent drawing after a partially executed batch.
            PDisplaySetFgColorCommand* foreground = AllocCommand<PDisplaySetFgColorCommand>();
            ConstructCommand(foreground, 0, m_FgColor);
            PDisplaySetBgColorCommand* background = AllocCommand<PDisplaySetBgColorCommand>();
            ConstructCommand(background, 0, m_BgColor);
            throw;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FlushBitmap(const PSrvBitmap* bitmap)
{
    if (bitmap->m_Driver == this && bitmap->m_RenderSequence == m_Sequence) {
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PDisplayDriver::AllocCommandBuffer(size_t size)
{
    if (m_DeviceControl.GetDeviceFD() < 0) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (size > std::numeric_limits<uint32_t>::max() - DISPLAY_COMMAND_ALIGNMENT) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    size = (size + DISPLAY_COMMAND_ALIGNMENT - 1) & ~(DISPLAY_COMMAND_ALIGNMENT - 1);
    const size_t capacity = m_CommandBuffer.size() * sizeof(uint64_t);
    if (size > capacity - m_UsedBufferSize) {
        Flush();
    }
    if (size > capacity) {
        m_CommandBuffer.resize(size / sizeof(uint64_t));
    }
    uint8_t* result = reinterpret_cast<uint8_t*>(m_CommandBuffer.data()) + m_UsedBufferSize;
    m_UsedBufferSize += size;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayBitmap PDisplayDriver::ReferenceBitmap(PSrvBitmap* bitmap)
{
    bitmap->m_Driver = this;
    bitmap->m_RenderSequence = m_Sequence;
    return {bitmap->m_ColorSpace, bitmap->m_Size, bitmap->m_BytesPerLine, bitmap->m_Raster, bitmap->m_VideoMem};
}
