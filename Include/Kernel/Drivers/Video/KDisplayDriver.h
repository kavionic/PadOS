// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <span>
#include <vector>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/KMutex.h>
#include <RPC/RPCDispatcher.h>
#include <DeviceControl/Display.h>
#include <GUI/FontMetrics.h>

#define RAS_OFFSET8( ptr, x, y, bpl) (((uint8_t*)(ptr)) + (x) + (y) * (bpl))
#define RAS_OFFSET16(ptr, x, y, bpl) ((uint16_t*)(((uint8_t*)(ptr)) + (x*2) + (y) * (bpl)))
#define RAS_OFFSET32(ptr, x, y, bpl) ((uint32_t*)(((uint8_t*)(ptr)) + (x*4) + (y) * (bpl)))



PDEFINE_LOG_CATEGORY(LogCategoryDisplay, "DISPLAY", PLogSeverity::WARNING);

namespace kernel
{

class KDisplayDriver : public KInode, public KFilesystemFileOps, public PFontMetrics
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

    virtual void    SetFgColor(PColor color) { m_FgColor = color; }
    PColor          GetFgColor() const noexcept { return m_FgColor; }

    virtual void    SetBgColor(PColor color) { m_BgColor = color; }
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

    virtual uint32_t WriteString(
        PDisplayBitmap* bitmap,
        const PIPoint& position,
        const char* string,
        size_t strLength,
        const PIRect& clipRect,
        PColor colorBg,
        PColor colorFg,
        PFontID fontID);


    static PColor   GetPaletteEntry(uint8_t index);

private:
    // Device-control callbacks run with m_Mutex held.
    PDisplayInfo HandleInitialize();
    PDisplayBatchResult HandleSubmit(const void* buffer, size_t length);
    bool HandleGetScreenMode(size_t index, PScreenMode* outMode);
    bool HandleSetScreenMode(const PIPoint& resolution, PEColorSpace colorSpace, float refreshRate);

    PDisplayBatchResult ExecuteCommands_pl(const void* buffer, size_t length);

    void FillTriangleUnion(PDisplayBitmap* bitmap, const PIRect& clipRect,
                           std::span<const std::array<PPoint, 3>> triangles,
                           PDrawingMode mode);
    void FillBlit8(uint8_t* dst, int nMod, int W, int H, uint8_t nColor);
    void FillBlit16(uint16_t* dst, int nMod, int W, int H, uint16_t nColor);
    void FillBlit24(uint8_t* dst, int nMod, int W, int H, uint32_t nColor);
    void FillBlit32(uint32_t* dst, int nMod, int W, int H, uint32_t nColor);

    KMutex          m_Mutex;
    PRPCDispatcher  m_DeviceControlDispatcher;
    bool            m_IsOpen = false;
    bool            m_IsInitialized = false;

    PColor  m_FgColor;
    PColor  m_BgColor;
};

} // namespace kernel
