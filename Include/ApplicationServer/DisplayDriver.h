// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <limits>
#include <System/ExceptionHandling.h>
#include <span>
#include <vector>
#include <utility>
#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include <Utils/String.h>
#include <DeviceControl/Display.h>
#include <GUI/FontMetrics.h>

class PSrvBitmap;

// Payload elements are constructed directly by the caller after allocation.
struct PDisplayPolygonBuffer
{
    std::span<PIRect> ClipRects;
    std::span<PPoint> Points;
};

class PDisplayDriver final : public PtrTarget, public PFontMetrics
{
public:
    explicit PDisplayDriver(const PString& devicePath = "/dev/ra8875");
    virtual ~PDisplayDriver() override;

    bool            Open();
    void            Close();
    void            PowerLost(bool hasPower);
    Ptr<PSrvBitmap>  GetScreenBitmap();

    int             GetScreenModeCount();
    bool            GetScreenModeDesc(size_t index, PScreenMode& outMode);
    bool            SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate);

    PIPoint         GetResolution();
    int             GetBytesPerLine();
    PEColorSpace    GetColorSpace();
    void            SetColor(size_t index, PColor color);

    bool    SetMouseCursorBitmap(const PMouseCursorBitmap& cursor);
    void    SetMouseCursorVisible(bool visible);

    void    SetMousePos(PIPoint cNewPos);

    void    SetFgColor(PColor color);
    PColor          GetFgColor() const noexcept { return m_FgColor; }

    void    SetBgColor(PColor color);
    PColor          GetBgColor() const noexcept { return m_BgColor; }

    void    WritePixel(PSrvBitmap* bitmap, const PIPoint& pos, PColor color);
    void    DrawLine(
        PSrvBitmap* bitmap,
        const PIRect& clipRect,
        const PIPoint& pos1,
        const PIPoint& pos2,
        const PColor& color,
        PDrawingMode mode);
    void    FillPolygon(
        PSrvBitmap* bitmap,
        std::span<const PIRect> clipRects,
        std::span<const PPoint> points,
        PDrawingMode mode);
    void    FillTriangle(
        PSrvBitmap* bitmap,
        const PIRect& clipRect,
        const PIPoint& pos1,
        const PIPoint& pos2,
        const PIPoint& pos3,
        PDrawingMode mode);
    void    FillTriangleFan(
        PSrvBitmap* bitmap,
        const PIRect& clipRect,
        std::span<const PPoint> points,
        PDrawingMode mode);
    void    FillTriangleStrip(
        PSrvBitmap* bitmap,
        const PIRect& clipRect,
        std::span<const PPoint> points,
        PDrawingMode mode);
    void    FillRect(PSrvBitmap* bitmap, const PIRect& rect);
    void    CopyRect(
        PSrvBitmap* dstBitmap,
        PSrvBitmap* srcBitmap,
        PColor bgColor,
        PColor fgColor,
        const PIRect& srcRect,
        const PIPoint& dstPos,
        PDrawingMode mode);
    void    ScaleRect(
        PSrvBitmap* dstBitmap,
        PSrvBitmap* srcBitmap,
        PColor bgColor,
        PColor fgColor,
        const PIRect& srcOrigRect,
        const PIRect& dstOrigRect,
        const PRect& srcRect,
        const PIRect& dstRect,
        PDrawingMode mode);

    void    FillCircle(
        PSrvBitmap* bitmap,
        const PIRect& clipRect,
        const PIPoint& center,
        int32_t radius,
        const PColor& color,
        PDrawingMode mode);

    uint32_t WriteString(
        PSrvBitmap* bitmap,
        const PIPoint& position,
        const char* string,
        size_t strLength,
        const PIRect& clipRect,
        PColor colorBg,
        PColor colorFg,
        PFontID fontID);


    static PColor   GetPaletteEntry(uint8_t index);

    // This variant queues text when the caller does not need WriteString's immediate result.
    void DrawString(
        PSrvBitmap* bitmap,
        const PIPoint& position,
        const char* string,
        size_t length,
        const PIRect& clipRect,
        PColor background,
        PColor foreground,
        PFontID fontID);

    PDisplayPolygonBuffer AllocPolygon(
        PSrvBitmap* bitmap,
        size_t clipCount,
        size_t pointCount,
        PDrawingMode mode);
    PDisplayPolygonBuffer AllocTriangleFan(
        PSrvBitmap* bitmap,
        size_t clipCount,
        size_t pointCount,
        PDrawingMode mode);
    PDisplayPolygonBuffer AllocTriangleStrip(
        PSrvBitmap* bitmap,
        size_t clipCount,
        size_t pointCount,
        PDrawingMode mode);
    void Flush();
    void FlushBitmap(const PSrvBitmap* bitmap);

private:
    template<typename Command>
    Command* AllocCommand(size_t payloadSize = 0)
    {
        if (payloadSize > std::numeric_limits<uint32_t>::max() - sizeof(Command) - DISPLAY_COMMAND_ALIGNMENT) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        const size_t size = sizeof(Command) + payloadSize;
        // Allocate before evaluating ReferenceBitmap(), since allocation may flush and advance the sequence.
        return static_cast<Command*>(AllocCommandBuffer(size));
    }

    template<typename Command, typename... Args>
    static Command* ConstructCommand(Command* storage, size_t payloadSize, Args&&... args)
    {
        const size_t size = sizeof(Command) + payloadSize;
        const uint32_t length = uint32_t((size + DISPLAY_COMMAND_ALIGNMENT - 1) & ~(DISPLAY_COMMAND_ALIGNMENT - 1));
        return new (storage) Command{{Command::CODE, length}, std::forward<Args>(args)...};
    }

    template<typename Command>
    PDisplayPolygonBuffer AllocGeometry(PSrvBitmap* bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    {
        const size_t limit = std::numeric_limits<uint32_t>::max() - sizeof(Command) - DISPLAY_COMMAND_ALIGNMENT;
        if (clipCount > limit / sizeof(PIRect) || pointCount > (limit - clipCount * sizeof(PIRect)) / sizeof(PPoint)) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        const size_t payloadSize = clipCount * sizeof(PIRect) + pointCount * sizeof(PPoint);
        Command* storage = AllocCommand<Command>(payloadSize);
        Command* command = ConstructCommand(storage, payloadSize, ReferenceBitmap(bitmap), clipCount, pointCount, mode);
        PIRect* clips = reinterpret_cast<PIRect*>(command + 1);
        PPoint* points = reinterpret_cast<PPoint*>(clips + clipCount);
        return {{clips, clipCount}, {points, pointCount}};
    }

    void* AllocCommandBuffer(size_t size);
    PDisplayBitmap ReferenceBitmap(PSrvBitmap* bitmap);

    uint64_t m_Sequence = 1;
    PString m_DevicePath;
    PDisplayDeviceControl m_DeviceControl;
    std::vector<uint64_t> m_CommandBuffer;
    size_t m_UsedBufferSize = 0;
    PDisplayBatchResult m_LastResult;
    PDisplayInfo m_Info;
    Ptr<PSrvBitmap> m_ScreenBitmap;
    PColor m_FgColor;
    PColor m_BgColor;
};
