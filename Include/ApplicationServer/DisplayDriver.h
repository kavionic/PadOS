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
#include <string_view>
#include <vector>
#include <utility>
#include <Ptr/PtrTarget.h>
#include <Ptr/Ptr.h>
#include <Utils/String.h>
#include <DeviceControl/Display.h>
#include <GUI/FontDescription.h>

class PSrvBitmap;
class PServerFontInstance;
class PServerFontDrawCache;

// Payload elements are constructed directly by the caller after allocation.
struct PDisplayPolygonBuffer
{
    std::span<PIRect> ClipRects;
    std::span<PPoint> Points;
};

class PDisplayDriver final : public PtrTarget
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

    static PColor   GetPaletteEntry(uint8_t index);

    // Copies UTF-8 bytes and retains a warmed cache until submission completes.
    // Copy clears the logical text area and glyph bounds; Overlay preserves zero coverage.
    // Antialiasing uses background; overlapping glyph coverage is combined.
    PFontTextMetrics DrawString(
        PSrvBitmap* bitmap,
        const PIPoint& position,
        std::string_view text,
        PServerFontInstance& font,
        std::span<const PIRect> clipRects,
        PColor background,
        PColor foreground,
        PDrawingMode mode,
        float width = std::numeric_limits<float>::infinity(),
        bool allowPartialCharacter = true);

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
    void* AllocCommand(PDisplayCommandID commandID, size_t size);

private:
    template<typename TInvoker>
    PDisplayPolygonBuffer AllocGeometry(TInvoker& invoker, PSrvBitmap* bitmap, size_t clipCount, size_t pointCount, PDrawingMode mode)
    {
        static_assert(alignof(PIRect) <= align_argument_size(1));
        static_assert(alignof(PPoint) <= align_argument_size(1));
        constexpr size_t ARGUMENT_SIZE = TInvoker::GetFixedSize();
        constexpr size_t LIMIT =
            std::numeric_limits<uint32_t>::max() - sizeof(PDisplayCommandHeader) - ARGUMENT_SIZE - DISPLAY_COMMAND_ALIGNMENT;
        if (clipCount > LIMIT / sizeof(PIRect) || pointCount > (LIMIT - clipCount * sizeof(PIRect)) / sizeof(PPoint)) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        const size_t payloadSize = clipCount * sizeof(PIRect) + pointCount * sizeof(PPoint);
        void* buffer = invoker.Alloc(payloadSize);
        // Allocation can flush: reference the bitmap only after the command has its final sequence.
        invoker(buffer, ReferenceBitmap(bitmap), clipCount, pointCount, mode);
        PIRect* clips = reinterpret_cast<PIRect*>(static_cast<uint8_t*>(buffer) + ARGUMENT_SIZE);
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
    std::vector<Ptr<PServerFontDrawCache>> m_FontReferences;
    PDisplayInfo m_Info;
    Ptr<PSrvBitmap> m_ScreenBitmap;
    PColor m_FgColor;
    PColor m_BgColor;

    PDisplayCommands m_Commands;
};
