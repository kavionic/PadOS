// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cstring>
#include <limits>

#include <fcntl.h>
#include <unistd.h>

#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/Fonts/ServerFontInstance.h>
#include <ApplicationServer/ServerBitmap.h>
#include <DeviceControl/DisplayBrush.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

#include "ServerView.h"


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PDisplayCommandAllocator::operator()(size_t size) const
{
    return m_Driver.AllocCommand(m_CommandID, size);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayDriver::PDisplayDriver(const PString& devicePath)
    : m_DevicePath(devicePath)
    , m_CommandBuffer(DISPLAY_COMMAND_BUFFER_SIZE / sizeof(uint64_t))
    , m_Commands(*this)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayDriver::~PDisplayDriver()
{
    Close();
    for (PSrvBitmap* bitmap : m_BitmapResources) {
        bitmap->m_Driver = nullptr;
    }
    for (PServerImage* image : m_ImageResources) {
        image->m_Driver = nullptr;
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
    PScopeFail closeDevice([this] { Close(); });
    m_Info = m_DeviceControl.Initialize();
    const PDisplayBitmap& screen = m_Info.ScreenBitmap;
    // Views retain this object across a driver reset.
    if (m_ScreenBitmap == nullptr) {
        m_ScreenBitmap = ptr_new<PSrvBitmap>(PIPoint(0, 0), screen.ColorSpace);
    }
    m_ScreenBitmap->m_Size = screen.Size;
    m_ScreenBitmap->m_ColorSpace = screen.ColorSpace;
    m_ScreenBitmap->m_BytesPerLine = screen.BytesPerLine;
    m_ScreenBitmap->m_Raster = screen.Raster;
    m_ScreenBitmap->m_VideoMem = screen.VideoMemory;
    m_ScreenBitmap->m_Driver = this;
    m_ScreenBitmap->m_DriverResourceID = m_Info.ScreenBitmapDriverResourceID;
    for (PSrvBitmap* bitmap : m_BitmapResources) {
        bitmap->m_DriverResourceID = CreateBitmapResource(*bitmap);
    }
    for (PServerImage* image : m_ImageResources) {
        image->m_DriverResourceID = CreateImageResource(*image);
    }
    m_FgColor = PColor(0);
    m_BgColor = PColor(0);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::Close()
{
    // Stop kernel access before releasing any resources retained by commands.
    const int deviceFD = m_DeviceControl.GetDeviceFD();
    m_DeviceControl.SetDeviceFD(-1);
    if (deviceFD >= 0) {
        close(deviceFD);
    }
    for (PSrvBitmap* bitmap : m_BitmapResources) {
        bitmap->m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    }
    for (PServerImage* image : m_ImageResources) {
        image->m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    }
    if (m_ScreenBitmap != nullptr) {
        m_ScreenBitmap->m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    }
    m_UsedBufferSize = 0;
    AdvanceSequence();
    m_FontReferences.clear();
    m_BrushLayers.clear();
    m_PendingImageReferences.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::PowerLost(bool hasPower)
{
    m_Commands.PowerLost(hasPower);
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

void PDisplayDriver::RegisterBitmap(PSrvBitmap* bitmap)
{
    m_BitmapResources.push_back(bitmap);
    PScopeFail rollback([this] { m_BitmapResources.pop_back(); });
    bitmap->m_DriverResourceID = CreateBitmapResource(*bitmap);
    bitmap->m_ResourceIndex = m_BitmapResources.size() - 1;
    bitmap->m_Driver = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::UnregisterBitmap(PSrvBitmap* bitmap) noexcept
{
    if (m_ScreenBitmap == bitmap) {
        return;
    }
    m_BitmapResources.back()->m_ResourceIndex = bitmap->m_ResourceIndex;
    m_BitmapResources[bitmap->m_ResourceIndex] = m_BitmapResources.back();
    m_BitmapResources.pop_back();
    try
    {
        FlushBitmap(bitmap);
        if (bitmap->m_DriverResourceID != DRIVER_INVALID_RESOURCE_ID && m_DeviceControl.GetDeviceFD() >= 0) {
            m_DeviceControl.DeleteBitmap(bitmap->m_DriverResourceID);
        }
    }
    PERROR_CATCH([this](PErrorCode error)
    {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "Unregistering display bitmap failed: {}", p_strerror(error));
        Close();
    });
    bitmap->m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    bitmap->m_Driver = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::RegisterImage(PServerImage* image)
{
    m_ImageResources.push_back(image);
    PScopeFail rollback([this] { m_ImageResources.pop_back(); });
    image->m_DriverResourceID = CreateImageResource(*image);
    image->m_ResourceIndex = m_ImageResources.size() - 1;
    image->m_Driver = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::UnregisterImage(PServerImage* image) noexcept
{
    m_ImageResources.back()->m_ResourceIndex = image->m_ResourceIndex;
    m_ImageResources[image->m_ResourceIndex] = m_ImageResources.back();
    m_ImageResources.pop_back();
    try
    {
        if (image->m_DriverResourceID != DRIVER_INVALID_RESOURCE_ID && m_DeviceControl.GetDeviceFD() >= 0) {
            m_DeviceControl.DeleteImage(image->m_DriverResourceID);
        }
    }
    PERROR_CATCH([this](PErrorCode error)
    {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "Unregistering display image failed: {}", p_strerror(error));
        Close();
    });
    image->m_DriverResourceID = DRIVER_INVALID_RESOURCE_ID;
    image->m_Driver = nullptr;
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
        m_ScreenBitmap->m_DriverResourceID = m_Info.ScreenBitmapDriverResourceID;
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
    m_Commands.SetColor(index, color);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDisplayDriver::SetMouseCursorBitmap(const PMouseCursorBitmap& cursor)
{
    return m_DeviceControl.SetMouseCursorBitmap(cursor);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetMouseCursorVisible(bool visible)
{
    m_Commands.SetMouseCursorVisible(visible);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetMousePos(PIPoint position)
{
    m_Commands.SetMousePos(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetFgColor(PColor color)
{
    if (color.GetColor32() != m_FgColor.GetColor32())
    {
        m_Commands.SetFgColor(color);
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
        m_Commands.SetBgColor(color);
        m_BgColor = color;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::WritePixel(PSrvBitmap* bitmap, const PIPoint& pos, PColor color)
{
    void* buffer = m_Commands.WritePixel.Alloc();
    m_Commands.WritePixel(buffer, ReferenceBitmap(bitmap), pos, color);
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
    void* buffer = m_Commands.DrawLine.Alloc();
    m_Commands.DrawLine(buffer, ReferenceBitmap(bitmap), clipRect, pos1, pos2, color, mode);
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
    void* buffer = m_Commands.FillTriangle.Alloc();
    m_Commands.FillTriangle(buffer, ReferenceBitmap(bitmap), clipRect, pos1, pos2, pos3, mode);
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
    void* buffer = m_Commands.FillRect.Alloc();
    m_Commands.FillRect(buffer, ReferenceBitmap(bitmap), rect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetBrush(std::span<const PServerBrushLayer> layers)
{
    if (std::ranges::equal(m_BrushLayers, layers)) {
        return;
    }
    constexpr size_t ARGUMENT_SIZE = decltype(m_Commands.SetBrush)::GetFixedSize();
    constexpr size_t LIMIT = std::numeric_limits<uint32_t>::max() - sizeof(PDisplayCommandHeader) - ARGUMENT_SIZE - DISPLAY_COMMAND_ALIGNMENT;
    if (layers.size() > LIMIT / sizeof(PDisplayBrushBinding)) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    // Reserve before emitting the command. Retention cannot allocate afterwards.
    m_BrushLayers.reserve(std::max(m_BrushLayers.size(), layers.size()));
    m_PendingImageReferences.reserve(m_PendingImageReferences.size() + m_BrushLayers.size());
    void* buffer = m_Commands.SetBrush.Alloc(layers.size() * sizeof(PDisplayBrushBinding));
    RetainBrushLayers(m_BrushLayers);
    m_Commands.SetBrush(buffer, layers.size());
    PDisplayBrushBinding* destination = reinterpret_cast<PDisplayBrushBinding*>(static_cast<uint8_t*>(buffer) + ARGUMENT_SIZE);
    for (const PServerBrushLayer& layer : layers)
    {
        PBrushData data = layer.Brush.GetData();
        const Ptr<PServerImage>& image = layer.Brush.GetImage();
        data.ImageID = (image != nullptr) ? image->m_DriverResourceID : INVALID_IMAGE_ID;
        new (destination++) PDisplayBrushBinding{data, layer.Bounds.TopLeft(), layer.ReferenceSize};
    }
    m_BrushLayers.assign(layers.begin(), layers.end());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillBrushRect(PSrvBitmap* bitmap, const PIRect& rect)
{
    void* buffer = m_Commands.FillBrushRect.Alloc();
    m_Commands.FillBrushRect(buffer, ReferenceBitmap(bitmap), rect);
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
    void* buffer = m_Commands.CopyRect.Alloc();
    m_Commands.CopyRect(buffer, ReferenceBitmap(dstBitmap), ReferenceBitmap(srcBitmap), bgColor, fgColor, srcRect, dstPos, mode);
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
    void* buffer = m_Commands.ScaleRect.Alloc();
    m_Commands.ScaleRect(
        buffer,
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
    void* buffer = m_Commands.FillCircle.Alloc();
    m_Commands.FillCircle(buffer, ReferenceBitmap(bitmap), clipRect, center, radius, color, mode);
    m_FgColor = color;
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

PFontTextMetrics PDisplayDriver::DrawString(
    PSrvBitmap* bitmap,
    const PIPoint& position,
    std::string_view text,
    PServerFontInstance& font,
    std::span<const PIRect> clipRects,
    PColor background,
    PColor foreground,
    PDrawingMode mode,
    float width,
    bool allowPartialCharacter)
{
    if (PMath::is_nan(width)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (width <= 0.0f) {
        return {};
    }
    const bool bounded = PMath::is_finite(width);
    if (!bounded && (text.empty() || clipRects.empty())) {
        return font.MeasureText(text);
    }
    static_assert(alignof(PIRect) <= align_argument_size(1));
    static_assert(alignof(int32_t) <= alignof(PIRect));
    const size_t argumentSize = bounded
        ? decltype(m_Commands.WriteStringWidth)::GetFixedSize() : decltype(m_Commands.WriteString)::GetFixedSize();
    const size_t LIMIT = std::numeric_limits<uint32_t>::max() - argumentSize - sizeof(PDisplayCommandHeader)
        - DISPLAY_COMMAND_ALIGNMENT;
    if (clipRects.size() > LIMIT / sizeof(PIRect) || text.size() > LIMIT - clipRects.size_bytes()) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    const size_t kerningCount = font.GetTextKerningCount(text);
    if (kerningCount > (LIMIT - clipRects.size_bytes() - text.size()) / sizeof(int32_t)) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    const size_t kerningBytes = kerningCount * sizeof(int32_t);
    if (m_FontReferences.size() == m_FontReferences.capacity())
    {
        const size_t capacity = m_FontReferences.capacity();
        const size_t growth = std::min(std::max(size_t(1), capacity), m_FontReferences.max_size() - capacity);
        m_FontReferences.reserve(capacity + growth);
    }
    const size_t payloadSize = clipRects.size_bytes() + kerningBytes + text.size();
    void* buffer = bounded ? m_Commands.WriteStringWidth.Alloc(payloadSize) : m_Commands.WriteString.Alloc(payloadSize);
    const size_t commandOffset = size_t(static_cast<uint8_t*>(buffer) - reinterpret_cast<uint8_t*>(m_CommandBuffer.data()))
        - sizeof(PDisplayCommandHeader);
    PScopeFail rollback([this, commandOffset] { m_UsedBufferSize = commandOffset; });
    uint8_t* payload = static_cast<uint8_t*>(buffer) + argumentSize;
    int32_t* kernings = (kerningCount != 0) ? reinterpret_cast<int32_t*>(payload + clipRects.size_bytes()) : nullptr;
    // Preparation writes directly into the reserved command. It cannot submit or allocate display commands.
    const PServerPreparedText prepared = font.PrepareText(text, kernings, width, allowPartialCharacter, position);
    text = text.substr(0, prepared.Metrics.ByteCount);
    const size_t usedKerningBytes = prepared.KerningCount * sizeof(int32_t);
    if (bounded)
    {
        m_Commands.WriteStringWidth(
            buffer,
            ReferenceBitmap(bitmap),
            position,
            prepared.Cache->GetDisplayCache(),
            text.size(),
            clipRects.size(),
            prepared.KerningCount,
            background,
            foreground,
            prepared.Metrics.Advance,
            mode,
            width);
        // Preparation may stop early. Reclaim the unused suffix before reserving the next command.
        const size_t recordSize = sizeof(PDisplayCommandHeader) + argumentSize + clipRects.size_bytes()
            + usedKerningBytes + text.size();
        const size_t length = (recordSize + DISPLAY_COMMAND_ALIGNMENT - 1) & ~(DISPLAY_COMMAND_ALIGNMENT - 1);
        PDisplayCommandHeader* header = static_cast<PDisplayCommandHeader*>(buffer) - 1;
        header->Length = uint32_t(length);
        m_UsedBufferSize = commandOffset + length;
    }
    else
    {
        m_Commands.WriteString(
            buffer,
            ReferenceBitmap(bitmap),
            position,
            prepared.Cache->GetDisplayCache(),
            text.size(),
            clipRects.size(),
            prepared.KerningCount,
            background,
            foreground,
            prepared.Metrics.Advance,
            mode);
    }
    if (!clipRects.empty()) {
        std::memcpy(payload, clipRects.data(), clipRects.size_bytes());
    }
    if (!text.empty()) {
        std::memcpy(payload + clipRects.size_bytes() + usedKerningBytes, text.data(), text.size());
    }
    if (std::find(m_FontReferences.begin(), m_FontReferences.end(), prepared.Cache) == m_FontReferences.end()) {
        m_FontReferences.push_back(prepared.Cache);
    }
    return prepared.Metrics;
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
    return AllocGeometry(m_Commands.FillPolygon, bitmap, clipCount, pointCount, mode);
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
    return AllocGeometry(m_Commands.FillTriangleFan, bitmap, clipCount, pointCount, mode);
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
    return AllocGeometry(m_Commands.FillTriangleStrip, bitmap, clipCount, pointCount, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::Flush()
{
    // A prior flush may have failed inside another operation, leaving no commands.
    if (!IsOpen()) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (m_UsedBufferSize != 0)
    {
        // Submission completes all hardware accesses; registered resources remain available.
        const size_t size = m_UsedBufferSize;
        m_UsedBufferSize = 0;
        PScopeExit completeBatch([this]
        {
            m_FontReferences.clear();
            // Unregistration can close a faulted device. Detach the completed
            // references before destruction so recovery cannot clear them twice.
            std::vector<Ptr<PServerImage>> completedImages;
            completedImages.swap(m_PendingImageReferences);
            completedImages.clear();
            completedImages.swap(m_PendingImageReferences);
            AdvanceSequence();
        });
        // Close before releasing retained resources if any part of submission fails.
        PScopeFail closeDevice([this] { Close(); });
        m_DeviceControl.Submit(m_CommandBuffer.data(), size);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FlushBitmap(const PSrvBitmap* bitmap)
{
    if (bitmap->m_Driver == this)
    {
        const bool selected = std::ranges::any_of(m_BrushLayers, [bitmap](const PServerBrushLayer& layer) noexcept
        {
            return layer.Brush.GetImage() != nullptr && layer.Brush.GetImage()->GetBitmap() == bitmap;
        });
        if (bitmap->m_RenderSequence == m_Sequence || selected) {
            Flush();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PDisplayDriver::AllocCommand(PDisplayCommandID commandID, size_t size)
{
    constexpr size_t LIMIT = std::numeric_limits<uint32_t>::max() - sizeof(PDisplayCommandHeader) - DISPLAY_COMMAND_ALIGNMENT;
    if (size > LIMIT) {
        PERROR_THROW_CODE(PErrorCode::OVERFLOW);
    }
    const size_t recordSize = sizeof(PDisplayCommandHeader) + size;
    const size_t length = (recordSize + DISPLAY_COMMAND_ALIGNMENT - 1) & ~(DISPLAY_COMMAND_ALIGNMENT - 1);
    PDisplayCommandHeader* header = new (AllocCommandBuffer(length)) PDisplayCommandHeader{commandID, uint32_t(length)};
    return header + 1;
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
/// Batches complete before advancing. Clear stale stamps only once per wrap;
/// zero is never a live batch, including for long-idle resources and the screen.
/// 
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::AdvanceSequence() noexcept
{
    if (++m_Sequence == 0)
    {
        for (PSrvBitmap* bitmap : m_BitmapResources) {
            bitmap->m_RenderSequence = 0;
        }
        for (PServerImage* image : m_ImageResources) {
            image->m_RenderSequence = 0;
        }
        if (m_ScreenBitmap != nullptr) {
            m_ScreenBitmap->m_RenderSequence = 0;
        }
        m_Sequence = 1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDriverResourceID PDisplayDriver::ReferenceBitmap(PSrvBitmap* bitmap)
{
    bitmap->m_RenderSequence = m_Sequence;
    return bitmap->m_DriverResourceID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::RetainBrushLayers(std::span<const PServerBrushLayer> layers)
{
    for (const PServerBrushLayer& layer : layers)
    {
        const Ptr<PServerImage>& image = layer.Brush.GetImage();
        if (image != nullptr && image->m_RenderSequence != m_Sequence)
        {
            m_PendingImageReferences.push_back(image);
            image->m_RenderSequence = m_Sequence;
            image->GetBitmap()->m_RenderSequence = m_Sequence;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDriverResourceID PDisplayDriver::CreateBitmapResource(const PSrvBitmap& bitmap)
{
    const PDisplayBitmap description{bitmap.m_ColorSpace, bitmap.m_Size, bitmap.m_BytesPerLine, bitmap.m_Raster, bitmap.m_VideoMem};
    return m_DeviceControl.CreateBitmap(description, !bitmap.m_ReadOnly);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDriverResourceID PDisplayDriver::CreateImageResource(const PServerImage& image)
{
    return m_DeviceControl.CreateImage(image.GetBitmap()->m_DriverResourceID, image.GetDescription());
}
