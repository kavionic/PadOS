// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <optional>
#include <span>
#include <vector>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/KMutex.h>
#include <Kernel/Drivers/Video/KDisplayText.h>
#include <RPC/RPCDispatcher.h>
#include <DeviceControl/Display.h>
#include <Kernel/Drivers/Video/KDisplayBrush.h>
#include <Kernel/Drivers/Video/KDisplayResources.h>

#define RAS_OFFSET8( ptr, x, y, bpl) (((uint8_t*)(ptr)) + (x) + (y) * (bpl))
#define RAS_OFFSET16(ptr, x, y, bpl) ((uint16_t*)(((uint8_t*)(ptr)) + (x*2) + (y) * (bpl)))
#define RAS_OFFSET32(ptr, x, y, bpl) ((uint32_t*)(((uint8_t*)(ptr)) + (x*4) + (y) * (bpl)))



PDEFINE_LOG_CATEGORY(LogCategoryDisplay, "DISPLAY", PLogSeverity::WARNING);

namespace kernel
{

class KDisplayDriver : public KInode, public KFilesystemFileOps
{
public:
    KDisplayDriver();
    virtual     ~KDisplayDriver();

    virtual Ptr<KFileNode> OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags) override;
    virtual void CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;
    virtual void DeviceControl(
        Ptr<KFileNode> file,
        int request,
        const void* inData,
        size_t inDataLength,
        void* outData,
        size_t outDataLength) override;
    virtual void ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

    virtual void WaitIdle() = 0;
    virtual bool            Open() = 0;
    virtual void            Close() = 0;
    virtual void            PowerLost(bool hasPower) = 0;
    virtual PDisplayBitmap*  GetScreenBitmap() = 0;

    virtual int             GetScreenModeCount() = 0;
    virtual bool            GetScreenModeDesc(size_t index, PScreenMode& outMode) = 0;
    virtual bool            SetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate) = 0;

    virtual PIPoint         GetResolution() = 0;
    virtual int             GetBytesPerLine() = 0;
    virtual PEColorSpace    GetColorSpace() = 0;
    virtual void            SetColor(size_t index, PColor color) = 0;

    virtual bool    SetMouseCursorBitmap(const PMouseCursorBitmap& cursor) = 0;
    virtual void    SetMouseCursorVisible(bool visible) = 0;

    virtual void    SetMousePos(PIPoint cNewPos) = 0;

    virtual void    SetFgColor(PColor color)
    {
        kassert(m_Mutex.IsLocked());
        m_FgColor = color;
    }
    PColor          GetFgColor() const noexcept { return m_FgColor; }

    virtual void    SetBgColor(PColor color)
    {
        kassert(m_Mutex.IsLocked());
        m_BgColor = color;
    }
    PColor          GetBgColor() const noexcept { return m_BgColor; }

    virtual void    WritePixel(PDisplayBitmap* bitmap, const PIPoint& pos, PColor color);
    virtual void    DrawLine(
        PDisplayBitmap* bitmap,
        const PIRect& clipRect,
        const PIPoint& pos1,
        const PIPoint& pos2,
        const PColor& color,
        PDrawingMode mode);
    virtual void    FillPolygon(
        PDisplayBitmap* bitmap,
        std::span<const PIRect> clipRects,
        std::span<const PPoint> points,
        PDrawingMode mode);
    virtual void    FillTriangle(
        PDisplayBitmap* bitmap,
        const PIRect& clipRect,
        const PIPoint& pos1,
        const PIPoint& pos2,
        const PIPoint& pos3,
        PDrawingMode mode);
    virtual void    FillTriangleFan(
        PDisplayBitmap* bitmap,
        const PIRect& clipRect,
        std::span<const PPoint> points,
        PDrawingMode mode);
    virtual void    FillTriangleStrip(
        PDisplayBitmap* bitmap,
        const PIRect& clipRect,
        std::span<const PPoint> points,
        PDrawingMode mode);
    virtual void    FillRect(PDisplayBitmap* bitmap, const PIRect& rect);
    virtual void    CopyRect(
        PDisplayBitmap* dstBitmap,
        PDisplayBitmap* srcBitmap,
        PColor bgColor,
        PColor fgColor,
        const PIRect& srcRect,
        const PIPoint& dstPos,
        PDrawingMode mode);
    virtual void    ScaleRect(
        PDisplayBitmap* dstBitmap,
        PDisplayBitmap* srcBitmap,
        PColor bgColor,
        PColor fgColor,
        const PIRect& srcOrigRect,
        const PIRect& dstOrigRect,
        const PRect& srcRect,
        const PIRect& dstRect,
        PDrawingMode mode);

    virtual void    FillCircle(
        PDisplayBitmap* bitmap,
        const PIRect& clipRect,
        const PIPoint& center,
        int32_t radius,
        const PColor& color,
        PDrawingMode mode);

    virtual void WriteString(
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
        float width = std::numeric_limits<float>::infinity());

    void FillBrushRect(PDisplayBitmap* bitmap, const PIRect& bounds);
    virtual void WriteBrushBlock(PDisplayBitmap* bitmap, const PIRect& bounds, std::span<const PColor> pixels);
    static PColor   GetPaletteEntry(uint8_t index);

protected:
    // The optional key is a pixel value in colorSpace and replaces only zero coverage.
    const std::array<uint32_t, 256>& PrepareFontPalette(
        PEColorSpace colorSpace,
        PColor background,
        PColor foreground,
        std::optional<uint32_t> colorKey = std::nullopt);
    void WriteBrushString(
        PDisplayBitmap* bitmap,
        const PIPoint& position,
        std::string_view text,
        const PDisplayFontCache& font,
        std::span<const int32_t> kernings,
        std::span<const PIRect> clipRects,
        PColor foreground,
        PPoint advance,
        PDrawingMode mode,
        float width);
    void DrawBrushBitmap(PDisplayBitmap* destination, const PDisplayBrushLayer& source, const PIRect& bounds, PDrawingMode mode);

    KDisplayText m_FontText;
    KDisplayBrush m_Brush;
    std::array<PColor, DISPLAY_BRUSH_BLOCK_PIXELS> m_BrushPixels;
    std::array<PColor, DISPLAY_BRUSH_BLOCK_PIXELS> m_SourcePixels;

    KMutex          m_Mutex;
    // A faulted driver must release client memory and allow Close() without waiting for stalled hardware.
    bool            m_IsFaulted = false;

private:
    // Device-control and batch-command callbacks run with m_Mutex held.
    PDisplayInfo HandleInitialize();
    void HandleSubmit(const void* buffer, size_t length);
    bool HandleGetScreenMode(size_t index, PScreenMode* outMode);
    bool HandleSetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate);
    bool HandleSetMouseCursorBitmap(const PMouseCursorBitmap& cursor);

    PDriverResourceID HandleCreateBitmap(PDisplayBitmap bitmap, bool writable);
    void HandleDeleteBitmap(PDriverResourceID bitmap);
    PDriverResourceID HandleCreateBrush(PDriverResourceID bitmap, const PBrushDescription& description);
    void HandleDeleteBrush(PDriverResourceID brush);

    void HandleWritePixel(PDriverResourceID bitmap, PIPoint position, PColor color);
    void HandleDrawLine(
        PDriverResourceID bitmap,
        PIRect clipRect,
        PIPoint position1,
        PIPoint position2,
        PColor color,
        PDrawingMode mode);
    void HandleFillPolygon(
        std::span<const std::byte> payload,
        PDriverResourceID bitmap,
        size_t clipCount,
        size_t pointCount,
        PDrawingMode mode);
    void HandleFillTriangle(
        PDriverResourceID bitmap,
        PIRect clipRect,
        PIPoint position1,
        PIPoint position2,
        PIPoint position3,
        PDrawingMode mode);
    void HandleFillTriangleFan(
        std::span<const std::byte> payload,
        PDriverResourceID bitmap,
        size_t clipCount,
        size_t pointCount,
        PDrawingMode mode);
    void HandleFillTriangleStrip(
        std::span<const std::byte> payload,
        PDriverResourceID bitmap,
        size_t clipCount,
        size_t pointCount,
        PDrawingMode mode);
    void HandleFillRect(PDriverResourceID bitmap, PIRect rect);
    void HandleCopyRect(
        PDriverResourceID destination,
        PDriverResourceID source,
        PColor background,
        PColor foreground,
        PIRect sourceRect,
        PIPoint position,
        PDrawingMode mode);
    void HandleScaleRect(
        PDriverResourceID destination,
        PDriverResourceID source,
        PColor background,
        PColor foreground,
        PIRect sourceOriginal,
        PIRect destinationOriginal,
        PRect sourceRect,
        PIRect destinationRect,
        PDrawingMode mode);
    void HandleFillCircle(
        PDriverResourceID bitmap,
        PIRect clipRect,
        PIPoint center,
        int32_t radius,
        PColor color,
        PDrawingMode mode);
    void HandleWriteString(
        std::span<const std::byte> payload,
        PDriverResourceID bitmap,
        PIPoint position,
        const PDisplayFontCache* font,
        size_t stringLength,
        size_t clipCount,
        size_t kerningCount,
        PColor background,
        PColor foreground,
        PPoint advance,
        PDrawingMode mode);
    void HandleWriteStringWidth(
        std::span<const std::byte> payload,
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
        float width);

    void HandleSetBrush(std::span<const std::byte> payload, size_t layerCount);
    void HandleFillBrushRect(PDriverResourceID bitmap, PIRect bounds);
    void HandleUpdateBrush(PDriverResourceID brush, PDriverResourceID bitmap, const PBrushDescription& description);
    void HandleSetBrushColor(PDriverResourceID brush, PColor color);
    Ptr<KDisplayBitmapResource> ResolveBrushBitmap_pl(PDriverResourceID bitmap, const PBrushDescription& description) const;
    void RefreshSelectedBrushLayers_pl(const Ptr<KDisplayBrushResource>& brush);
    void RefreshBrushLayer_pl(size_t index);

    void ExecuteCommands_pl(const void* buffer, size_t length);

    void FillTriangleUnion(PDisplayBitmap* bitmap, const PIRect& clipRect,
                           std::span<const std::array<PPoint, 3>> triangles,
                           PDrawingMode mode);
    void FillBlit8(uint8_t* dst, int nMod, int W, int H, uint8_t nColor);
    void FillBlit16(uint16_t* dst, int nMod, int W, int H, uint16_t nColor);
    void FillBlit24(uint8_t* dst, int nMod, int W, int H, uint32_t nColor);
    void FillBlit32(uint32_t* dst, int nMod, int W, int H, uint32_t nColor);

    KDisplayResourceTable<KDisplayBitmapResource> m_BitmapResources;
    KDisplayResourceTable<KDisplayBrushResource> m_BrushResources;
    std::vector<Ptr<KDisplayBrushResource>> m_SelectedBrushResources;
    std::vector<PDisplayBrushLayer> m_BrushLayerBuffer;
    PDriverResourceID m_ScreenBitmapDriverResourceID = DRIVER_INVALID_RESOURCE_ID;

    PRPCDispatcher  m_DeviceControlDispatcher;
    PRPCDispatcher  m_CommandDispatcher;
    bool            m_IsOpen = false;
    bool            m_IsInitialized = false;

    PColor  m_FgColor;
    PColor  m_BgColor;
    std::array<uint32_t, 256> m_FontPalette{};
    PColor m_FontBackground;
    PColor m_FontForeground;
    uint32_t m_FontBackgroundPixel = 0;
    PEColorSpace m_FontColorSpace = PEColorSpace::NO_COLOR_SPACE;
    bool m_FontPaletteValid = false;
};

} // namespace kernel
